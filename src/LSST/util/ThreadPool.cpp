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

// Class header
#include "util/ThreadPool.h"

// System headers
#include <algorithm>

// Project headers
#include "util/Log.h"

using namespace std;

namespace LSST {
namespace m2cellcpp {
namespace util {

PoolEventThread::Ptr PoolEventThread::newPoolEventThread(shared_ptr<ThreadPool> const& threadPool,
                                                         CommandQueue::Ptr const& q) {
    PoolEventThread::Ptr pet(new PoolEventThread(threadPool, q));
    return pet;
}

PoolEventThread::PoolEventThread(shared_ptr<ThreadPool> const& threadPool, CommandQueue::Ptr const& q)
        : EventThread(q), _threadPool(threadPool) {
    LTRACE("PoolEventThread::PoolEventThread() ", this);
    _threadPool->_incrPoolThreadCount();
}

PoolEventThread::~PoolEventThread() {
    LTRACE("PoolEventThread::~PoolEventThread() ", this, " use_count=", _threadPool.use_count());
    _threadPool->_decrPoolThreadCount();
}

void PoolEventThread::specialActions(Command::Ptr const& cmd) {
    CommandForThreadPool::Ptr cmdPool = dynamic_pointer_cast<CommandForThreadPool>(cmd);
    if (cmdPool != nullptr) {
        cmdPool->_setPoolEventThread(shared_from_this());
    }
}

bool PoolEventThread::leavePool(Command::Ptr const& cmd) {
    // This thread will stop accepting commands
    eLoop = false;
    LDEBUG("PoolEventThread::leavePool ", this);

    if (cmd.get() != getCurrentCommand()) {
        LDEBUG("PoolEventThread::leavePool different command ", this);
        // cmd must have finished before the event loop stopped.
        // The current command will complete normally, and the pool
        // should replace this thread with a new one when finishup()
        // is called in handleCmds(). No harm aside from some wasted CPU cycles.
        return false;
    }

    // Have the CommandQueue deal with any accounting that needs to be done.
    callCommandFinish(cmd);

    // Have the thread pool release this thread, which will cause a replacement thread
    // to be created.
    finishup();
    return true;
}

void PoolEventThread::leavePool() {
    // This thread will stop accepting commands
    eLoop = false;
    _threadPool->_waitIfAtMaxThreadPoolCount();
    leavePool(getCurrentCommandPtr());
}

bool PoolEventThread::atMaxThreadCount() { return _threadPool->atMaxThreadPoolCount(); }

void PoolEventThread::finishup() {
    if (_finishupOnce.exchange(true) == false) {
        LDEBUG("Releasing this PoolEventThread");
        if (!_threadPool->release(this)) {
            LWARN("The pool failed to find this PoolEventThread.");
        }
    }
}

void CommandForThreadPool::_setPoolEventThread(PoolEventThread::Ptr const& poolEventThread) {
    _poolEventThread = poolEventThread;
}

PoolEventThread::Ptr CommandForThreadPool::getAndNullPoolEventThread() {
    lock_guard<mutex> lg(_poolMtx);
    auto pet = _poolEventThread.lock();
    _poolEventThread.reset();
    return pet;
}

bool CommandForThreadPool::atMaxThreadCount() {
    lock_guard<mutex> lg(_poolMtx);
    auto pet = _poolEventThread.lock();
    return (pet == nullptr || pet->atMaxThreadCount());
}

ThreadPool::Ptr ThreadPool::newThreadPool(unsigned int thrdCount, unsigned int maxThreadCount,
                                          CommandQueue::Ptr const& q, EventThreadJoiner::Ptr const& joiner) {
    Ptr thp(new ThreadPool(thrdCount, maxThreadCount, q, joiner));  // private constructor
    thp->_resize();
    return thp;
}

ThreadPool::Ptr ThreadPool::newThreadPool(unsigned int thrdCount, CommandQueue::Ptr const& q,
                                          EventThreadJoiner::Ptr const& joiner) {
    Ptr thp(new ThreadPool(thrdCount, thrdCount + 1, q, joiner));  // private constructor
    thp->_resize();
    return thp;
}

ThreadPool::ThreadPool(unsigned int thrdCount, unsigned int maxPoolThreads, CommandQueue::Ptr const& q,
                       EventThreadJoiner::Ptr const& joiner)
        : _targetThrdCount(thrdCount), _q(q), _joinerThread(joiner), _maxThreadCount(maxPoolThreads) {
    if (_q == nullptr) {
        _q = make_shared<CommandQueue>();
    }
    if (_joinerThread == nullptr) {
        _joinerThread = make_shared<EventThreadJoiner>();
    }
}

ThreadPool::~ThreadPool() {
    if (!_shutdown) {
        LCRITICAL("~ThreadPool called without shutdownPool being called first.");
    }
    LDEBUG("~ThreadPool ", this);
}

void ThreadPool::shutdownPool() {
    LDEBUG("shutdownPool begin", this);
    _shutdown = true;
    endAll();
    waitForResize(0);
    _joinerThread->shutdownJoin();
}

bool ThreadPool::release(PoolEventThread* thrd) {
    // Search for the the thread to free
    auto func = [thrd](PoolEventThread::Ptr const& pt) -> bool { return pt.get() == thrd; };

    PoolEventThread::Ptr thrdPtr;
    {
        lock_guard<mutex> lock(_poolMutex);
        auto iter = find_if(_pool.begin(), _pool.end(), func);
        if (iter == _pool.end()) {
            LWARN("ThreadPool::release thread not found ", thrd);
            return false;
        } else {
            thrdPtr = *iter;
            LDEBUG("ThreadPool::release erasing ", thrd);
            _pool.erase(iter);
        }
        _joinerThread->addThread(thrdPtr);  // Add to list of threads to join.
    }
    _resize();  // Check if more threads need to be released.
    return true;
}

/// Change the size of the thread pool.
void ThreadPool::resize(unsigned int targetThrdCount) {
    {
        LINFO("ThreadPool::resize ", targetThrdCount);
        {
            lock_guard<mutex> lockPool(_mxPool);
            /// Normally, targetThrdCount should be much smaller than _maxThreadCount.
            if (targetThrdCount >= _maxThreadCount) {
                LWARN("ThreadPool::resize target count ", targetThrdCount, " is >= than max ",
                      _maxThreadCount);
                // Need at least one thread available to deal with released threads.
                _maxThreadCount = targetThrdCount + 1;
                _cvPool.notify_all();
            }
        }

        lock_guard<mutex> lock(_countMutex);
        if (_shutdown) {
            targetThrdCount = 0;
        }
        _targetThrdCount = targetThrdCount;
    }
    _resize();
}

void ThreadPool::_resize() {
    lock_guard<mutex> lock(_poolMutex);
    auto target = getTargetThrdCount();
    while (target > _pool.size()) {
        LTRACE("ThreadPool::_resize creating new PoolEventThread");
        auto t = PoolEventThread::newPoolEventThread(shared_from_this(), _q);
        _pool.push_back(t);
        t->run();
    }
    // Shrinking the thread pool is much harder. Adding a message to end one thread
    // is sent. When that thread ends, it calls release(), which will then call
    // this function again to check if more threads need to be ended.
    if (target < _pool.size()) {
        auto thrd = _pool.front();
        if (thrd != nullptr) {
            LDEBUG("ThreadPool::_resize sending thrd->queEnd()");
            thrd->queEnd();  // Since all threads share the same queue, this could be answered by any thread.
        } else {
            LWARN("ThreadPool::_resize thrd == nullptr");
        }
    }
    LTRACE("_resize target=", target, " size=", _pool.size());
    {
        unique_lock<mutex> countlock(_countMutex);
        _countCV.notify_all();
    }
}

void ThreadPool::waitForResize(int millisecs) {
    auto eqTest = [this]() { return _targetThrdCount == _pool.size(); };
    unique_lock<mutex> lock(_countMutex);
    if (millisecs > 0) {
        _countCV.wait_for(lock, chrono::milliseconds(millisecs), eqTest);
    } else {
        _countCV.wait(lock, eqTest);
    }
}

void ThreadPool::_incrPoolThreadCount() {
    lock_guard<mutex> lockPool(_mxPool);
    ++_poolThreadCount;
    LDEBUG("incr _poolThreadCount=", _poolThreadCount);
}

void ThreadPool::_decrPoolThreadCount() {
    {
        lock_guard<mutex> lockPool(_mxPool);
        --_poolThreadCount;
    }
    LDEBUG("decr _poolThreadCount=", _poolThreadCount);
    _cvPool.notify_one();
}

void ThreadPool::_waitIfAtMaxThreadPoolCount() {
    unique_lock<mutex> lockPool(_mxPool);
    auto logLvl = spdlog::level::debug;
    if (_poolThreadCount >= _maxThreadCount) {
        logLvl = spdlog::level::warn;
    }
    Log::logW(logLvl, __FILE__, __LINE__, "wait before _poolThreadCount=", _poolThreadCount);
    _cvPool.wait(lockPool, [this]() { return (_poolThreadCount <= _maxThreadCount); });
}

bool ThreadPool::atMaxThreadPoolCount() {
    unique_lock<mutex> lockPool(_mxPool);
    bool atMax = _poolThreadCount > _maxThreadCount;
    if (atMax) {
        LWARN("atMaxThreadPoolCount current=", _poolThreadCount, " max=", _maxThreadCount);
    }
    return atMax;
}

}  // namespace util
}  // namespace m2cellcpp
}  // namespace LSST
