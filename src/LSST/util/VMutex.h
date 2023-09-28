
/*
 * LSST Data Management System
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
#ifndef LSST_M2CELLCPP_UTIL_VMUTEX_H
#define LSST_M2CELLCPP_UTIL_VMUTEX_H

// System headers
#include <atomic>
#include <memory>
#include <mutex>
#include <set>
#include <string>
#include <thread>

// Project headers
#include "util/Bug.h"

/// Used to verify a mutex is locked before accessing a protected variable.
#define VMUTEX_HELD(vmtx) \
    if (!vmtx.lockedByCaller()) throw LSST::m2cellcpp::util::Bug(ERR_LOC, "mutex not locked!");

/// Used to verify a mutex is not locked by this thread before locking a related mutex.
#define VMUTEX_NOT_HELD(vmtx) \
    if (vmtx.lockedByCaller()) throw LSST::m2cellcpp::util::Bug(ERR_LOC, "mutex not free!");

namespace LSST {
namespace m2cellcpp {
namespace util {

/// This class implements a verifiable mutex based on std::mutex. It can be used with the
/// VMUTEX_HELD and VMUTEX_NOT_HELD macros.
/// For it to work properly, all of the lock_guard calls must specify util::VMutex
/// (or a child thereof) and not std::mutex.
/// Making VMutex a wrapper around std::mutex instead of a child causes lines
/// like `std::lock_guard<std::mutex> lck(_vmutex);` to be flagged as errors,
/// which is desirable.
class VMutex {
public:
    explicit VMutex() {}

    /// Lock the mutex (replaces the corresponding method of the base class)
    void lock() {
        _mutex.lock();
        _holder = std::this_thread::get_id();
    }

    /// Release the mutex (replaces the corresponding method of the base class)
    void unlock() {
        _holder = std::thread::id();
        _mutex.unlock();
    }

    bool try_lock() {
        bool res = _mutex.try_lock();
        if (res) {
            _holder = std::this_thread::get_id();
        }
        return res;
    }

    /// @return true if the mutex is locked by this thread.
    /// TODO: Rename lockedByThread()
    bool lockedByCaller() const { return _holder == std::this_thread::get_id(); }

protected:
    std::atomic<std::thread::id> _holder;

private:
    std::mutex _mutex;
};


}  // namespace util
}  // namespace m2cellcpp
}  // namespace LSST

#endif  // LSST_M2CELLCPP_UTIL_VMUTEX_H
