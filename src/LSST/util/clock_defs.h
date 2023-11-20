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

#ifndef LSST_M2CELLCPP_UTIL_CLOCKDEFS_H
#define LSST_M2CELLCPP_UTIL_CLOCKDEFS_H

// System headers
#include <atomic>
#include <chrono>
#include <functional>
#include <memory>
#include <mutex>
#include <ostream>
#include <sys/time.h>
#include <time.h>

namespace LSST {
namespace m2cellcpp {
namespace util {


/// It's important that the clock used is consistent when calculating time.
/// steady_clock should be monotonic.
using CLOCK = std::chrono::steady_clock;
using TIMEPOINT = CLOCK::time_point;

using SYSCLOCK = std::chrono::system_clock;

inline time_t steadyToTimeT(TIMEPOINT timeP){
    auto sysTime = SYSCLOCK::now() + std::chrono::duration_cast<SYSCLOCK::duration>(timeP - CLOCK::now());
    return SYSCLOCK::to_time_t(sysTime);
}


/// Return the time passed between `start` and `end` in seconds.
inline double timePassedSec(TIMEPOINT const start, TIMEPOINT const end) {
    // ratio<1,1> is redundant, as the default is seconds. If units other than
    // seconds are desired, chrono::duration_cast may be the better option.
    const double timeDiff = std::chrono::duration<double, std::ratio<1,1>>(end - start).count();
    return timeDiff;
}

/// RAII class to help track a changing sum through a begin and end time.
template <typename TType>
class TimeCountTracker {
public:
    using Ptr = std::shared_ptr<TimeCountTracker>;

    using CALLBACKFUNC = std::function<void(TIMEPOINT start, TIMEPOINT end, TType sum, bool success)>;
    TimeCountTracker() = delete;
    TimeCountTracker(TimeCountTracker const&) = delete;
    TimeCountTracker& operator=(TimeCountTracker const&) = delete;

    /// Constructor that includes the callback function that the destructor will call.
    TimeCountTracker(CALLBACKFUNC callback) : _callback(callback) {
        auto now = CLOCK::now();
        _startTime = now;
        _endTime = now;
    }

    /// Call the callback function as the dying act.
    ~TimeCountTracker() {
        TType sum;
        {
            std::lock_guard lg(_mtx);
            _endTime = CLOCK::now();
            sum = _sum;
        }
        _callback(_startTime, _endTime, sum, _success);
    }

    /// Add val to _sum
    void addToValue(TType val) {
        std::lock_guard lg(_mtx);
        _sum += val;
    }

    /// Call if the related action completed.
    void setSuccess() { _success = true; }

private:
    TIMEPOINT _startTime;
    TIMEPOINT _endTime;
    TType _sum = 0;  ///< atomic double doesn't support +=
    std::atomic<bool> _success{false};
    CALLBACKFUNC _callback;
    std::mutex _mtx;
};

}  // namespace util
}  // namespace m2cellcpp
}  // namespace LSST

#endif  // LSST_M2CELLCPP_UTIL_CLOCKDEFS_H
