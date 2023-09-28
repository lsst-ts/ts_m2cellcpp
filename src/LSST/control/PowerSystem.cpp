/*
 * This file is part of LSST ts_m2cellcpp test suite.
 *
 * Developed for the Vera C. Rubin Observatory Telescope & Site Software Systems.
 * This product includes software developed by the Vera C.Rubin Observatory Project
 * (https://www.lsst.org). See the COPYRIGHT file at the top-level directory of
 * this distribution for details of code ownership.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation, either version 3 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program. If not, see <https://www.gnu.org/licenses/>.
 */

// Class header
#include "control/PowerSystem.h"

// system headers

// project headers
#include "faultmgr/FaultMgr.h"
#include "util/Bug.h"
#include "util/Log.h"


using namespace std;

namespace LSST {
namespace m2cellcpp {
namespace control {

PowerSystem::PowerSystem() : _motor(MOTOR), _comm(COMM) {
    _fpgaIo = FpgaIo::getPtr(); // If FpgaIo hasn't been setup, now is a good time to find out.
    _eThrd.run();
}

PowerSystem::~PowerSystem() {
    _motor.setPowerOff(__func__);
    _comm.setPowerOff(__func__);
    _eThrd.queEnd();
    _eThrd.join();
}

void PowerSystem::writeCrioInterlockEnable(bool set) {
    _fpgaIo->writeOutputPortBitPos(OutputPortBits::CRIO_INTERLOCK_ENABLE, set);
}

void PowerSystem::_daqInfoRead() {
    _daqReadTime = util::CLOCK::now();

    // Get the latest info, old info is of no use.
    auto sInfo = _fpgaIo->getSysInfo();
    auto diff = (util::timePassedSec(sInfo.timestamp, _daqReadTime));
    _checkTimeout(diff); // shutoff power if timed out.

    _processDaq(sInfo);
}

void PowerSystem::_daqTimeoutCheck() {
    auto diff = (util::timePassedSec( _daqReadTime, util::CLOCK::now()));
    bool timedOut = _checkTimeout(diff); // shutoff power if timed out.
    if (timedOut) {
        // Run the processing, even if the data is old.
        auto sInfo = _fpgaIo->getSysInfo();
        _processDaq(sInfo);
    }
}

bool PowerSystem::_checkTimeout(double diffInSeconds) {
    bool timedOut = diffInSeconds > _sysInfoTimeoutSecs;
    if (timedOut) {
        string eMsg = string(__func__) + " timed out " + to_string(timedOut);
        _motor.setPowerOff(eMsg);
        _comm.setPowerOff(eMsg);
        faultmgr::FaultMgr::get().setFault("PowerSystemTimeOut PLACHOLDER");
    }
    return timedOut;
}

void PowerSystem::_processDaq(SysInfo info) {
    SysStatus motorStat = _motor.processDaq(info);
    SysStatus commStat = _comm.processDaq(info);

    _processDaqHealthTelemetry(info);

    if (_motorStatusPrev != motorStat || _commStatusPrev != commStat) {
        LINFO("Power status change motor:now=", motorStat, " prev=", _motorStatusPrev,
                " comm=", commStat, " prev=", _commStatusPrev);
        _motorStatusPrev = motorStat;
        _commStatusPrev = commStat;
    }
}

void PowerSystem::queueDaqInfoRead() {
    auto cmdDaqInfoRead = std::make_shared<util::Command>([&](util::CmdData*) { _daqInfoRead(); });
    _eThrd.queCmd(cmdDaqInfoRead);
}


void PowerSystem::queueTimeoutCheck() {
    auto cmdDaqTimeoutCheck = std::make_shared<util::Command>([&](util::CmdData*) { _daqTimeoutCheck(); });
    _eThrd.queCmd(cmdDaqTimeoutCheck);
}


void PowerSystem::_processDaqHealthTelemetry(SysInfo sInfo) {
    //  - DAQ_to_PS_health_telemetry.vi - assemble vi output “Power Subsystem Common Telemetry” (shortening to PSCT)
    //  - “Power Control/Status Telemetry.Digital Inputs”

    /// struct to be replaced by real class in its own header file in DM-40908
    struct HealthTelemetryPSCT {
        HealthTelemetryPSCT(SysInfo info) {
            //- AND with “Input Port Bit Masks.RedundancyOK Bit” (active high)
            //   -> convert to bool (true if !=0) -> “PSCT.Redundancy OK”
            redundancyOk = info.inputPort.getBitAtPos(InputPortBits::REDUNDANCY_OK);
            // - AND with “Input Port Bit Masks.Load Distribution OK Bit” (active high)
            //   -> convert to bool (true if !=0) -> “PSCT.Load Distribution OK”
            loadDistributionOk = info.inputPort.getBitAtPos(InputPortBits::LOAD_DISTRIBUTION_OK);
            // - AND with “Input Port Bit Masks.Power Supply #1 DC OK Bit” (active high)
            //   -> convert to bool (true if !=0) -> “PSCT.P/S 1 DC OK”
            powerSupply1DcOk = info.inputPort.getBitAtPos(InputPortBits::POWER_SUPPLY_1_DC_OK);
            // - AND with “Input Port Bit Masks.Power Supply #2 DC OK Bit” (active high)
            //   -> convert to bool (true if !=0) -> “PSCT.P/S 2 DC OK”
            powerSupply2DcOk = info.inputPort.getBitAtPos(InputPortBits::POWER_SUPPLY_2_DC_OK);
            // - AND with “Input Port Bit Masks.Power Supply #1 Current OK Bit” (active low)
            //   -> convert to bool (true if ==0) -> “PSCT.P/S 1 Boost Current ON”
            powerSupply1BoostCurrentOn = !(info.inputPort.getBitAtPos(InputPortBits::POWER_SUPPLY_1_CURRENT_OK));
            // - AND with “Input Port Bit Masks.Power Supply #2 Current OK Bit” (active low)
            //   -> convert to bool (true if ==0) -> “PSCT.P/S 2 Boost Current ON”
            powerSupply2BoostCurrentOn = !(info.inputPort.getBitAtPos(InputPortBits::POWER_SUPPLY_2_CURRENT_OK));
        }

        bool redundancyOk;
        bool loadDistributionOk;
        bool powerSupply1DcOk;
        bool powerSupply2DcOk;
        bool powerSupply1BoostCurrentOn;
        bool powerSupply2BoostCurrentOn;
    };

    HealthTelemetryPSCT psct(sInfo);
}



}  // namespace control
}  // namespace m2cellcpp
}  // namespace LSST

