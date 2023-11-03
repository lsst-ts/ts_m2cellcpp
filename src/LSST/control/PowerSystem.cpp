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
#include <ctime>

// project headers
#include "faultmgr/FaultMgr.h"
#include "util/Bug.h"
#include "util/Log.h"


using namespace std;

namespace LSST {
namespace m2cellcpp {
namespace control {

PowerSystem::PowerSystem() : _motor(MOTOR), _comm(COMM) {
    LDEBUG("Creating PowerSystem");
    _fpgaIo = FpgaIo::getPtr(); // If FpgaIo hasn't been setup, now is a good time to find out.
    _eThrd.run();

    // This function will be run in a separate thread until the destructor is called.
    // The destructor sets _timeoutLoop false and then joins the thread.
    auto func = [this]() {
        while (_timeoutLoop) {
            queueTimeoutCheck();
            this_thread::sleep_for(1s); // &&& timeoutSleep
        }
    };
    thread tThrd(func);
    _timeoutThread = move(tThrd);
}

PowerSystem::~PowerSystem() {
    _timeoutLoop = false;
    _motor.setPowerOff(__func__);
    _comm.setPowerOff(__func__);
    _eThrd.queEnd();
    _eThrd.join();
    _timeoutThread.join();
}

void PowerSystem::writeCrioInterlockEnable(bool set) {
    _fpgaIo->writeOutputPortBitPos(OutputPortBits::CRIO_INTERLOCK_ENABLE, set);
}

void PowerSystem::_daqInfoRead() {
    _daqReadTime = util::CLOCK::now();

    // Get the latest info, old info is of no use.
    auto sInfo = _fpgaIo->getSysInfo();
    auto diff = (util::timePassedSec(sInfo.timestamp, _daqReadTime));
    bool timedOut = _checkTimeout(diff); // shutoff power if timed out.
    if (timedOut) {
        stringstream os;
        time_t tm = util::CLOCK::to_time_t(_daqReadTime);
        os << "PowerSystem::_daqInfoRead() timedOut last _daq read=" << ctime(&tm)
                << " seconds since last read=" << diff;
        LERROR(os.str());
    }

    _processDaq(sInfo);
}

void PowerSystem::_daqTimeoutCheck() {
    auto now = util::CLOCK::now();
    auto diff = (util::timePassedSec(_daqReadTime, now));
    bool timedOut = _checkTimeout(diff); // shutoff power if timed out.
    if (timedOut) {
        stringstream os;
        time_t tm = util::CLOCK::to_time_t(_daqReadTime);
        os << "PowerSystem::Timeout timedOut last _daq read=" << ctime(&tm)
           << " seconds since last read=" << diff;
        LERROR(os.str());

        // Run the processing, even if the data is old.
        // This allows power the off logic to try to run.
        auto sInfo = _fpgaIo->getSysInfo();
        _processDaq(sInfo);
    }
}

bool PowerSystem::_checkTimeout(double diffInSeconds) {
    bool timedOut = diffInSeconds > _sysInfoTimeoutSecs;
    if (timedOut) {
        string eMsg = string(__func__) + " _daq timed out " + to_string(timedOut);
        _motor.setPowerOff(eMsg);
        _comm.setPowerOff(eMsg);
        faultmgr::FaultStatusBits cFaults;
        cFaults.setBitAt(faultmgr::FaultStatusBits::POWER_SYSTEM_TIMEOUT);
        faultmgr::FaultMgr::get().updatePowerFaults(cFaults, faultmgr::BasicFaultMgr::POWER_SUBSYSTEM);
    }
    return timedOut;
}

void PowerSystem::_processDaq(SysInfo info) {
    faultmgr::FaultStatusBits currentFaults;

    _processDaqHealthTelemetry(info, currentFaults);
    // If the health check had a fault, update FaultMgr now so motor and comm power
    // can be turned off as soon as possible.
    if (currentFaults.getBitmap() != 0) {
        faultmgr::FaultMgr::get().updatePowerFaults(currentFaults,
                                                    faultmgr::BasicFaultMgr::POWER_SUBSYSTEM);
    }

    SysStatus motorStat = _motor.processDaq(info, currentFaults);
    SysStatus commStat = _comm.processDaq(info, currentFaults);
    faultmgr::FaultMgr::get().updatePowerFaults(currentFaults,
                                                faultmgr::BasicFaultMgr::POWER_SUBSYSTEM);

    if (_motorStatusPrev != motorStat || _commStatusPrev != commStat) {
        LINFO("Power status change motor:now=", motorStat, " prev=", _motorStatusPrev,
                " comm=", commStat, " prev=", _commStatusPrev);
        _motorStatusPrev = motorStat;
        _commStatusPrev = commStat;
        // TODO: DM-40339 send information to telemetry server
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


void PowerSystem::_processDaqHealthTelemetry(SysInfo sInfo, faultmgr::FaultStatusBits& currentFaults) {
    //  - DAQ_to_PS_health_telemetry.vi - assemble vi output “Power Subsystem Common Telemetry”
    //  - “Power Control/Status Telemetry.Digital Inputs”

    bool redundancyOk = sInfo.inputPort.getBitAtPos(InputPortBits::REDUNDANCY_OK);
    bool loadDistributionOk = sInfo.inputPort.getBitAtPos(InputPortBits::LOAD_DISTRIBUTION_OK);
    bool powerSupply1DcOk = sInfo.inputPort.getBitAtPos(InputPortBits::POWER_SUPPLY_1_DC_OK);
    bool powerSupply2DcOk = sInfo.inputPort.getBitAtPos(InputPortBits::POWER_SUPPLY_2_DC_OK);

    // Active low inputs, see PowerSubsystem.lvclass:DAQ_to_PS_health_telemetry.vi
    bool powerSupply1BoostCurrentOn = !(sInfo.inputPort.getBitAtPos(InputPortBits::POWER_SUPPLY_1_CURRENT_OK));
    bool powerSupply2BoostCurrentOn = !(sInfo.inputPort.getBitAtPos(InputPortBits::POWER_SUPPLY_2_CURRENT_OK));

    bool powerLoadOk = redundancyOk && loadDistributionOk;

    if (!powerLoadOk) {
        LERROR("POWER_SUPPLY_LOAD_SHARE_ERR redundancyOk=",
                redundancyOk, " loadDistributionOk=", loadDistributionOk);
        // "power supply load share error"
        currentFaults.setBitAt(faultmgr::FaultStatusBits::POWER_SUPPLY_LOAD_SHARE_ERR);
    }

    bool anyBoostCurrentOnAndFaultEnabled = _boostCurrentFaultEnabled && (powerSupply1BoostCurrentOn || powerSupply2BoostCurrentOn);
    bool allDcOk = powerSupply1DcOk && powerSupply2DcOk;
    bool powerSupplyOk = allDcOk && !anyBoostCurrentOnAndFaultEnabled;

    if (!powerSupplyOk) {
        LERROR("POWER_HEALTH_FAULT _boostCurrentFaultEnabled=", _boostCurrentFaultEnabled,
                " powerSupply1BoostCurrentOn=", powerSupply1BoostCurrentOn,
                " powerSupply2BoostCurrentOn=", powerSupply2BoostCurrentOn,
                " powerSupply1DcOk=", powerSupply1DcOk,
                " powerSupply2DcOk=", powerSupply2DcOk);
        currentFaults.setBitAt(faultmgr::FaultStatusBits::POWER_HEALTH_FAULT); // "power supply health fault"
    }
}

}  // namespace control
}  // namespace m2cellcpp
}  // namespace LSST

