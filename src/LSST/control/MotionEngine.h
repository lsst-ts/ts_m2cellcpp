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

#ifndef LSST_M2CELLCPP_CONTROL_MOTIONENGINE_H
#define LSST_M2CELLCPP_CONTROL_MOTIONENGINE_H

// System headers
#include <memory>
#include <thread>

// Project headers
#include "util/clock_defs.h"
#include "util/EventThread.h"

namespace LSST {
namespace m2cellcpp {
namespace control {

/// This class is responsible for the MotionEngine control, mostly
/// following what is found in MotionEngine.lvclass:motionEngineMain.vi.
/// It's primary function is to generate the step vector and pass that
/// to the CellCommunication loop.
/// &&& more doc.
class MotionEngine {
public:
    using Ptr = std::shared_ptr<MotionEngine>;

    /// Create the global MotionEngine instance.
    static void setup();

    /// Return a reference to the global MotionEngine instance.
    /// @throws `ConfigException` if `setup` has not already been called.
    static MotionEngine& get();

    /// Return a shared pointer to the global MotionEngine instance.
    /// @throws `ConfigException` if `setup` has not already been called.
    static Ptr getPtr();

    /// Start the event thread and timeout thread.
    void engineStart();

    /// Wait for `_eThrd` to be running.
    void  waitForEngine() const;

    /// Stop all threads in this object.
    /// @return false if the threads were already stopped.
    bool engineStop();

    /// Join all threads in this object.
    void engineJoin();

    /// Called at reasonable intervals to make sure the finite state
    /// machine advances and/or to turn off power if no DAQ updates
    /// are being received.
    void queueTimeoutCheck();

private:
    static Ptr _thisPtr; ///< pointer to the global instance of MotionEngine.
    static std::mutex _thisPtrMtx; ///< Protects `_thisPtr`.

    // Private constructor to force use of `setup`;
    MotionEngine();

    /// Check that ILC com info has been arriving in a timely fashion.
    void _comTimeoutCheck();

    /// doc &&&
    bool _checkTimeout(double diffInSeconds);

    util::EventThread _eThrd; ///< Thread running ILC processing.
    std::atomic<bool> _eStarted{false}; ///< Flag indicating threads have been started.
    std::atomic<bool> _eStopCalled{false}; ///< Flag indicating threads are stopped or stopping.
    std::atomic<bool> _eJoinCalled{false}; ///< Flag indicating `engineJoin()` has been called.

    /// Last time the ILC information was read. Initialized to now to give
    /// the system a chance to read instead of instantly timing out.
    std::atomic<util::TIMEPOINT> _comReadTime{util::CLOCK::now()};

    /// Timeout in seconds for fresh DAQ SystemInfo.
    /// DM-40694 set from config file, also needs a real value
    std::atomic<double> _comTimeoutSecs{1.5};

    std::thread _timeoutThread; ///< calls _checkTimeout on a regular basis.
    std::atomic<bool> _timeoutLoop{true}; ///< set to false to end _timeoutThread.

};

}  // namespace control
}  // namespace m2cellcpp
}  // namespace LSST

#endif  // LSST_M2CELLCPP_CONTROL_MOTIONENGINE_H
