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

#ifndef LSST_M2CELLCPP_CONTROL_POWERSYTEM_H
#define LSST_M2CELLCPP_CONTROL_POWERSYTEM_H

// System headers
#include <memory>
#include <stdint.h>
#include <string>
#include <thread>
#include <vector>

// Third party headers
#include <nlohmann/json.hpp>

// Project headers
#include "control/FpgaIo.h"
#include "control/InputPortBits.h"
#include "control/PowerSubsystem.h"
#include "faultmgr/FaultMgr.h"
#include "util/EventThread.h"

namespace LSST {
namespace m2cellcpp {
namespace control {

/// Class is used to contain both the `MOTOR` `PowerSubsystem` and
/// the `COMM` `PowerSubsystem`.
/// Synchronization is largely provided by this being an event driven thread,
/// where the primary event is `queueDaqInfoRead()`.
/// `_comm` and `_motor` have their own mutexes for synchronization
/// and contain most of the data.
/// unit tests: test_PowerSystem.cpp
//
// DM-40694 set from config file,
//  Controller->InitializeFunctionalGlobals.vi - The individual items should be done in a separate
//  class/function
//    - System Config FG:SystemConfigurationFG.vi  ****** power up configuration values ********
//      There are a lot of configuration values in here having to do with power supply very specific,
//      probably best to open up the vi and look at the set values. Elements set listed below
//      - "Power Subsystem Configuration Parameters.Power Subsystem Common Configuration Parameters"
//      - "Power Subsystem Configuration Parameters.Comm Power Bus Configuration Parameters"
//      - "Power Subsystem Configuration Parameters.Motor Power Bus Configuration Parameters"
//      - "Power Subsystem Configuration Parameters.Boost Current Fault Enabled" - this is set to false.
//    - There's an interesting comment in Context.lvclass:loadConfiguration.vi related to the loading of
//      these values in LabView.
class PowerSystem {
public:
    using Ptr = std::shared_ptr<PowerSystem>;

    /// Start the event thread, set power off.
    PowerSystem();

    PowerSystem(PowerSystem const&) = delete;
    PowerSystem& operator=(PowerSystem const&) = delete;

    /// Try to turn off power and stop the event thread.
    virtual ~PowerSystem();

    /// Use this to set this object's pointer to context to the global `context` instance.
    /// In some unit tests, there may be no `Context` instance.
    void setContext(std::shared_ptr<Context> const& context);

    /// Called when new system information is available so this
    /// class can get a copy and process it.
    void queueDaqInfoRead();

    /// Called at reasonable intervals to make sure the finite state
    /// machine advances and/or to turn off power if no DAQ updates
    /// are being received.
    void queueTimeoutCheck();

    /// If `set` is true, enable the interlock, otherwise disable it.
    void writeCrioInterlockEnable(bool set);

    /// Turn motor power on or off.
    /// Motor power can only be turned on if Comm power is already ON.
    /// @param on - Turn power on if true, or off if it is false.
    /// @return true if there wasn't anything preventing power from being changed.
    ///   This only indicates that the power bit could be set. There may be
    ///   other issue that prevent power from reaching the `ON` state.
    bool powerMotor(bool on);

    /// Turn comm power on or off.
    /// @param on - Turn power on if true, or off if it is false.
    /// @return true if there wasn't anything preventing power from being changed.
    ///   This only indicates that the power bit could be set. There may be
    ///   other issue that prevent power from reaching the `ON` state.
    /// If comm power is being turned off, then motor power should already be
    /// turned off. In any case, if the comm power bit is turned off,
    /// `PowerSystem` will try to turn off motor power.
    bool powerComm(bool on);

    /// Return a reference to the MOTOR PowerSubSystem.
    PowerSubsystem& getMotor() { return _motor; }

    /// Return a reference to the COMM PowerSubSystem.
    PowerSubsystem& getComm() { return _comm; }

    /// Provide a json message containting the state of a `PowerSubsystem`.
    /// @param powerType - indicates which subsystem to generate a json message for.
    /// @return a json message describing the state of the `PowerSubsystem` indicated by `powerType`.
    nlohmann::json getPowerSystemStateJson(PowerSystemType powerType) const;

    /// Calling this function will stop the timeout thread.
    void stopTimeoutLoop() { _timeoutLoop = false; }

private:
    /// Read SysInfo from the FPGA and call `_processDaq`
    void _daqInfoRead();

    /// Check that DAQ info has been arriving in a timely fashion.
    void _daqTimeoutCheck();

    /// Turn off all power and return true if `diffInSeconds` is greater
    /// than `_sysInfoTimeoutSecs`, everything in seconds.
    bool _checkTimeout(double diffInSeconds);

    /// Pass the lastest `info` from `FpgaIo` to both PowerSubsytems.
    /// Based on PowerSubsystem->process_DAQ_telemetry.vi
    void _processDaq(SysInfo info);

    /// Handle faults for some hardware signals related to power.
    /// @param sInfo - System hardware information from FpgaIo.
    /// @param currentFaults - Faults detected in this function will be reported here.
    void _processDaqHealthTelemetry(SysInfo sInfo, faultmgr::FaultStatusBits& currentFaults);

    PowerSubsystem _motor;  ///< Handles MOTOR power control.
    PowerSubsystem _comm;   ///< Handles COMM power control.

    SysStatus _motorStatusPrev = SysStatus::WAITING;  ///< Previous motor status
    SysStatus _commStatusPrev = SysStatus::WAITING;   ///< Previous comm status

    FpgaIo::Ptr _fpgaIo;       ///< Pointer to the global instance of FpgaIo.
    util::EventThread _eThrd;  ///< Thread running DAQ processing.

    /// Last time the DAQ information was read. Initialized to now to give
    /// the system a chance to read the DAQ instead of instantly timing out.
    std::atomic<util::TIMEPOINT> _daqReadTime{util::CLOCK::now()};

    /// Timeout in seconds for fresh DAQ SystemInfo.
    /// DM-40694 set from config file, also needs a real value
    std::atomic<double> _sysInfoTimeoutSecs{1.5};

    /// If true, boost current indicators from the power supplies will cause faults.
    /// DM-40694 set from config file
    std::atomic<bool> _boostCurrentFaultEnabled{true};

    std::thread _timeoutThread;            ///< calls _checkTimeout on a regular basis.
    std::atomic<bool> _timeoutLoop{true};  ///< set to false to end _timeoutThread.

    /// Sleep time between timeout checks in milliseconds.
    /// DM-40694 set from config file, also needs a real value
    std::chrono::milliseconds _timeoutSleep{1000};
};

}  // namespace control
}  // namespace m2cellcpp
}  // namespace LSST

#endif  // LSST_M2CELLCPP_CONTROL_POWERSYSTEM_H
