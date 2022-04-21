// -*- LSST-C++ -*-
/*
 *  This file is part of LSST M2 support system package.
 *
 * This product includes software developed by the
 * LSST Project (http://www.lsst.org/).
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the LSST License Statement and
 * the GNU General Public License along with this program.  If not,
 * see <http://www.lsstcorp.org/LegalNotices/>.
 */

#ifndef LSST_M2CELLCPP_UTIL_THREADPOOL_H
#define LSST_M2CELLCPP_UTIL_THREADPOOL_H

// System headers
#include <atomic>
#include <cassert>
#include <deque>
#include <queue>
#include <thread>
#include <vector>

// Qserv headers
#include "util/EventThread.h"

namespace LSST {
namespace m2cellcpp {
namespace util {

class ThreadPool;

/// An EventThread to be used by the ThreadPool class.
/// `finishup()` is used to tell the ThreadPool that this thread is finished.
class PoolEventThread : public EventThread, public std::enable_shared_from_this<PoolEventThread> {
public:
    using Ptr = std::shared_ptr<PoolEventThread>;

    /// PoolEventThread factory to ensure proper shared__from_this.
    static PoolEventThread::Ptr newPoolEventThread(std::shared_ptr<ThreadPool> const& threadPool,
                                                   CommandQueue::Ptr const& q);
    virtual ~PoolEventThread();

    /// Cause this thread to leave the thread pool, this MUST only called from within
    /// the thread that will be removed (most likely from within a CommandThreadPool action).
    void leavePool();

    /// Cause the thread running `cmd` to leave the thread pool.
    /// This can be called from outside the thread that will be removed.
    /// - If within the thread that will leave the pool, leavePool()
    ///   should be called.
    /// - atMaxThreadCount() should be called first to avoid seriously
    ///   breaking the limit of threads.
    /// - This version of leavePool never waits as that could cause a deadlock.
    ///
    /// This would most likely be done by
    /// a CommandQueue which was having some trouble due to 'cmd', such as 'cmd' taking
    /// too long to complete. This allows the CommandQueue to continue but will have other
    /// consequences.
    /// @return false if a different command is running than cmd.
    bool leavePool(Command::Ptr const& cmd);

    /// Return true if at or above the maximum number of threads that can
    /// exist concurrently.
    bool atMaxThreadCount();

protected:
    /// If `cmd` is a CommandThreadPool object, give it a copy of our this pointer.
    void specialActions(Command::Ptr const& cmd) override;

    /// Claed to cleanup when a `PoolEventThread` is finished.
    void finishup() override;

    std::shared_ptr<ThreadPool> _threadPool;  ///< The threadpool.
    std::atomic<bool> _finishupOnce{false};   ///< Ensure finishup() only called once.

private:
    PoolEventThread(std::shared_ptr<ThreadPool> const& threadPool, CommandQueue::Ptr const& q);
};

/// A Command that is aware that it is running as part of a PoolEventThread,
/// which allows it to tell the event thread and pool to take special actions.
class CommandForThreadPool : public CommandTracked {
public:
    using Ptr = std::shared_ptr<CommandForThreadPool>;

    CommandForThreadPool() = default;
    explicit CommandForThreadPool(std::function<void(CmdData*)> func) : CommandTracked{func} {}

    /// @return true if the number of threads created and still existing is
    /// greater than the max.
    bool atMaxThreadCount();

    /// @return a pointer to the `PoolEventThread` running this command and reset the
    /// the pointer in the pool.
    /// Invalidate _poolEventThread so it can't be used again.
    /// At this point, the reason to get _poolEventThread is to
    /// have the thread leave the pool. This prevents that from
    /// happening more than once.
    PoolEventThread::Ptr getAndNullPoolEventThread();

    friend class PoolEventThread;

private:
    /// Set _poolEventThread pointer to the thread running this command.
    /// Called by the `PoolEventThread` just before it starts running this `Command`.
    void _setPoolEventThread(PoolEventThread::Ptr const& poolEventThread);

    /// Weak pointer to the `PoolEventThread` running this command.
    std::weak_ptr<PoolEventThread> _poolEventThread;
    std::mutex _poolMtx;  ///< Protects `_poolEventThread`
};

/// ThreadPool is a variable size pool of threads all fed by the same CommandQueue.
/// Growing the pool is simple, shrinking the pool is complex. Both operations should
/// have no effect on running commands or commands on the queue.
/// NOTE: shutdownPool() MUST be called before a ThreadPool is destroyed or there will
///  likely be a segmentation fault. Every command sent to the pool before shutdown is
///  called should complete. Once shutdown has been called, the size of the pool
///  cannot be increased (target size permanently set to 0).
/// Note: It is possible for threads to leave the pool and be replaced using leavePool()
///  This is usually done when a thread no longer requires significant CPU but has
///  to wait for something to happen, like transferring data.
///  _poolThreadCount is a total of all threads in the pool and all threads that have
///  left the pool and this total should not exceed _maxThreadCount.
class ThreadPool : public std::enable_shared_from_this<ThreadPool> {
public:
    using Ptr = std::shared_ptr<ThreadPool>;

    /// Used to create thread pool where there is not expected to be be a need
    /// to remove time consuming, low CPU usage threads from the pool.
    static ThreadPool::Ptr newThreadPool(unsigned int thrdCount, CommandQueue::Ptr const& q,
                                         EventThreadJoiner::Ptr const& joiner = nullptr);

    /// Used to create a pool where threads are expected to be removed for processing
    /// and replaced by calling PoolEventThread::leavePool().
    /// Useful for scheduling queries against mysql. The queries use significant CPU until
    /// mysql finishes, and then the results sit around (using little CPU) until the czar
    /// collects them.
    static ThreadPool::Ptr newThreadPool(unsigned int thrdCount, unsigned int maxThreadCount,
                                         CommandQueue::Ptr const& q,
                                         EventThreadJoiner::Ptr const& joiner = nullptr);

    virtual ~ThreadPool();

    /// Call to shutdown the thread pool, and wait for all threads to complete.
    /// The ThreadPool should not be used after this function is called.
    /// This includes threads that were removed from the pool.
    void shutdownPool();

    /// @return a pointer to the internal `CommandQueue`.
    CommandQueue::Ptr getQueue() { return _q; }

    /// @return the desired number of threads in the pool.
    unsigned int getTargetThrdCount() {
        std::lock_guard<std::mutex> lock(_countMutex);
        return _targetThrdCount;
    }

    /// @return the actual number of threads in the pool.
    unsigned int size() {
        std::lock_guard<std::mutex> lock(_poolMutex);
        return _pool.size();
    }

    /// Wait for the pool to reach the _targetThrdCount number of threads.
    /// It will wait forever if `millisecs` is zero, otherwise it will timeout
    /// after that number of milliseconds.
    /// Note that this wont detect changes to `_targetThrdCount`.
    void waitForResize(int millisecs);

    /// Remove all threads from the pool.
    void endAll() { resize(0); }

    /// This method changes the size of the thread pool to `targetThrdCount`
    void resize(unsigned int targetThrdCount);

    /// Release the thread from the thread pool.
    /// @return true if the thread was found and added to the joiner thread.
    bool release(PoolEventThread* thread);

    /// @return true if existing threads are at or above _maxThreadCount.
    bool atMaxThreadPoolCount();

    friend PoolEventThread;

private:
    ThreadPool(unsigned int thrdCount, unsigned int maxPoolThreads, CommandQueue::Ptr const& q,
               EventThreadJoiner::Ptr const& joiner);

    /// Do the work of changing the size of the thread pool.
    /// Making the pool larger is just a matter of adding threads.
    /// Shrinking the pool requires ending one thread at a time.
    /// @see resize.
    void _resize();

    std::mutex _poolMutex;                                ///< Protects _pool
    std::vector<std::shared_ptr<PoolEventThread>> _pool;  ///< All the threads in our pool.

    std::mutex _countMutex;            ///< protects _targetThrdCount
    unsigned int _targetThrdCount{0};  ///< How many threads wanted in the pool.
    std::condition_variable _countCV;  ///< Notifies about changes to _pool size, uses _countMutex.
    CommandQueue::Ptr _q;              ///< The queue used by all threads in the _pool.

    EventThreadJoiner::Ptr _joinerThread;  ///< Tracks and joins threads removed from the pool.
    std::atomic<bool> _shutdown{false};    ///< True after shutdownPool has been called.

    // Functions to track the number of threads created by the pool.
    /// Wait until the number of existing threads is <= max existing threads.
    /// The pool kicks out threads that require significant time to process and
    /// replaces them on a regular basis, see PoolEventThread::leavePool().
    /// Under some circumstances, the number of threads that have been kicked out but
    /// have not finished can become extremely large (tens of thousands). There
    /// isn't much choice but to wait for some to finish before kicking more out.
    void _waitIfAtMaxThreadPoolCount();

    /// Increase the count of existing threads.
    void _incrPoolThreadCount();

    /// Decrease the count of existing threads.
    void _decrPoolThreadCount();

    unsigned int _poolThreadCount = 0;    ///< Number of threads that exist.
    unsigned int _maxThreadCount = 5000;  ///< Max number of thread allowed, set from config.
    std::condition_variable _cvPool;      ///< Signal when threads are deleted.
    mutable std::mutex _mxPool;           ///< Protects _poolThreadCount, _cvPool, _mxPool
};

}  // namespace util
}  // namespace m2cellcpp
}  // namespace LSST

#endif  // LSST_M2CELLCPP_UTIL_THREADPOOL_H
