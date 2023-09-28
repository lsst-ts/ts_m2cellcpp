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
#include <vector>

// Project headers
#include "control/FpgaIo.h"
#include "control/InputPortBits.h"
#include "control/PowerSubsystem.h"
#include "util/EventThread.h"


namespace LSST {
namespace m2cellcpp {
namespace control {

/// Class is used to contain both the `MOTOR` `PowerSubsystem` and
/// the `COMM` `PowerSubsystem`.
/// unit tests: test_PowerSystem.cpp
class PowerSystem {
public:
    using Ptr = std::shared_ptr<PowerSystem>;

    /// Start the event thread, set power off.
    PowerSystem();

    PowerSystem(PowerSystem const&) = delete;
    PowerSystem& operator=(PowerSystem const&) = delete;

    /// Try to turn off power and stop the event thread.
    virtual ~PowerSystem();

    /// Called when new system information is available so this
    /// class can get a copy and process it.
    void queueDaqInfoRead();

    /// Called at reasonable intervals to make sure the finite state
    /// machine advances and/or to turn off power if no DAQ updates
    /// are being received.
    void queueTimeoutCheck();

    /// If `set` is true, enable the interlock, otherwise disable it.
    void writeCrioInterlockEnable(bool set);

    /// Return a reference to the MOTOR PowerSubSystem.
    PowerSubsystem& getMotor() { return _motor; }

    /// Return a reference to the COMM PowerSubSystem.
    PowerSubsystem& getComm() { return _comm; }

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


    /// To be implemented in DM-40908, handle faults for some hardware signals related to power.
    /// PLACEHOLDER DAQ_to_PS_health_telemetry.vi - copy the data into a placeholder structure,
    ///         but doesn't do any checking.
    void _processDaqHealthTelemetry(SysInfo info);

    PowerSubsystem _motor; ///< Handles MOTOR power control.
    PowerSubsystem _comm; ///< Handles COMM power control.

    SysStatus _motorStatusPrev = SysStatus::WAITING; ///< Previous motor status
    SysStatus _commStatusPrev = SysStatus::WAITING; ///< Previous comm status

    FpgaIo::Ptr _fpgaIo; ///< Pointer to the global instance of FpgaIo.
    util::EventThread _eThrd; ///< Thread running DAQ processing.

    /// Last time the DAQ information was read. Initialized to now to give
    /// the system a chance to read the DAQ instead of instantly timing out.
    util::TIMEPOINT _daqReadTime{util::CLOCK::now()};

    /// Timeout in seconds for fresh DAQ SystemInfo.
    /// DM-40694 set from config file, also needs a real value
    std::atomic<double> _sysInfoTimeoutSecs{1.5};
};


}  // namespace control
}  // namespace m2cellcpp
}  // namespace LSST

#endif  // LSST_M2CELLCPP_CONTROL_POWERSYSTEM_H
