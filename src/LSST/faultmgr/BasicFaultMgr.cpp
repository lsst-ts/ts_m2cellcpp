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
#include "faultmgr/BasicFaultMgr.h"

#include <bitset>
#include <sstream>
#include <mutex>
#include <stdexcept>
#include <string>

// Project headers
#include "control/Context.h"
#include "faultmgr/FaultMgr.h"
#include "system/Config.h"
#include "util/Bug.h"
#include "util/Log.h"

using namespace std;

namespace LSST {
namespace m2cellcpp {
namespace faultmgr {

string BasicFaultMgr::getCrioSubsystemStr(CrioSubsystem subSystem) {
    switch (subSystem) {
    case SYSTEM_CONTROLLER: return "SYSTEM_CONTROLLER";
    case FAULT_MANAGER: return "FAULT_MANAGER";
    case POWER_SUBSYSTEM: return "POWER_SUBSYSTEM";
    case CELL_CONTROLLER: return "CELL_CONTROLLER";
    case TELEMETRY_LOGGER: return "TELEMETRY_LOGGER";
    case NETWORK_INTERFACE: return "NETWORK_INTERFACE";
    case MOTION_ENGINE: return "MOTION_ENGINE";
    }
    return string(" unexpected subSytem=") + to_string(subSystem);
}

BasicFaultMgr::BasicFaultMgr() {
    // _summaryFaults, _prevFaults, and _currentFaults should all be zero already.
    _faultEnableMask.setBitmap(FaultStatusBits::getMaskFaults()); ///< "Fault Enable Mask"
    _defaultFaultMask.setBitmap(FaultStatusBits::getMaskFaults());; ///< "Default Fault Mask"
    // _affectedFaultsMask, and _affectedWarnInfoMask should both be zero already.

    _timeStamp = util::CLOCK::now();
}

bool BasicFaultMgr::updateFaults(BasicFaultMgr::CrioSubsystem subsystem) {
    uint64_t diff = (_prevFaults.getBitmap() ^ _currentFaults.getBitmap()) & _faultEnableMask.getBitmap();
    if (diff == 0) {
        return false; // nothing needs to be done.
    }
    _prevFaults = _summaryFaults;

    // from "BasicFaultManager.lvclass:send_faults.vi"
    // remove warning and info bits
    uint64_t summaryAndDefault = _summaryFaults.getBitmap() & _defaultFaultMask.getBitmap();
    // Add _currentFaults to _summaryFaults.
    uint64_t combined = summaryAndDefault | _currentFaults.getBitmap();
    _summaryFaults = combined;

    _timeStamp = util::CLOCK::now();

    return true;
}

void BasicFaultMgr::resetFaults(FaultStatusBits mask) {
    LDEBUG("resetFaults ", mask.getAllSetBitEnums());
    uint64_t notMask = ~(mask.getBitmap());
    _summaryFaults.setBitmap(_summaryFaults.getBitmap() & notMask);
    _currentFaults.setBitmap(_currentFaults.getBitmap() & notMask);
    _prevFaults.setBitmap(_prevFaults.getBitmap() & notMask);

    _timeStamp = util::CLOCK::now();
}


std::tuple<uint16_t, uint16_t> BasicFaultMgr::updateFaultStatus(
        uint64_t summaryFaultStatus, uint64_t faultEnableMask,
        uint64_t newFaultStatus, uint64_t affectedWarnInfo, uint64_t affectedFault) {
    // affectedAll is the same as 1.MASK' from UpdateFaultStatus.vi
    uint64_t affectedAll = affectedFault | affectedWarnInfo;

    // cf is the same as CF from UpdateFaultStatus.vi
    uint64_t cf = summaryFaultStatus;

    // cfPrime is the same as 2.CF' from UpdateFaultStatus.vi
    uint64_t cfPrime = cf & ~(affectedWarnInfo);

    uint64_t newMasked = newFaultStatus & affectedAll;

    // updatedSummaryFaults is the same as "Updated Summary Faults" from UpdateFaultStatus.vi
    uint64_t updatedSummaryFaults = cf ^ (newMasked | cfPrime);

    // changedBits is the same as "Changed Bits" from UpdateFaultStatus.vi
    uint64_t changedBits = faultEnableMask & affectedAll & updatedSummaryFaults;

    return make_tuple(updatedSummaryFaults, changedBits);
}


void BasicFaultMgr::updateSummary(uint64_t newSummary) {
    _prevFaults = _summaryFaults;
    _summaryFaults = newSummary;
    _currentFaults = _summaryFaults;
}


void BasicFaultMgr::setMaskComm(FaultStatusBits newFaultMask) {
    // SendDisconnectFault.vi uses a mask with only the "cRIO COMM error fault" bit set.
    // This mask is used to set New Faults Status, Fault Enable Mask, Affected FaultMask,
    // and Current Faults Status in a message.

    // see FaultManager.lvclass:fault_manager_main.vi
    _faultEnableMask.setBitmap(_faultEnableMask.getBitmap() | newFaultMask.getBitmap());
    _affectedFaultsMask.setBitmap(_affectedFaultsMask.getBitmap() | newFaultMask.getBitmap());
    _prevFaults = _currentFaults; // This is different than what the LabView code does.
    _currentFaults = newFaultMask;

    _summaryFaults.setBitmap((_summaryFaults.getBitmap() & _defaultFaultMask.getBitmap()) | _currentFaults.getBitmap());
    _currentFaults = _summaryFaults;

    _timeStamp = util::CLOCK::now();
}

PowerFaultMgr::PowerFaultMgr() : BasicFaultMgr() {
    // see PowerSubsystem.lvclass:initialize.vi
    FaultStatusBits fsb;
    fsb.setBitmap(FaultStatusBits::getPowerSubsystemFaultManagerAffectedFaultMask());
    setAffectedFaultsMask(fsb);
    fsb.setBitmap(FaultStatusBits::getPowerSubsystemFaultManagerAffectedWarningMask());
    setAffectedFaultsMask(fsb);
}

}  // namespace faultmgr
}  // namespace m2cellcpp
}  // namespace LSST

