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
#include "faultmgr/FaultMgr.h"

#include <bitset>
#include <sstream>
#include <mutex>
#include <stdexcept>
#include <string>

// Project headers
#include "system/Config.h"
#include "util/Bug.h"
#include "util/Log.h"

using namespace std;

namespace LSST {
namespace m2cellcpp {
namespace faultmgr {

FaultMgr::Ptr FaultMgr::_thisPtr;
std::mutex FaultMgr::_thisPtrMtx;


std::tuple<uint16_t, uint16_t> FaultInfo::updateFaultStatus(uint64_t summaryFaultStatus, uint64_t newFaultStatus) {
    // affectedAll is the same as 1.MASK' from UpdateFaultStatus.vi
    uint64_t affectedAll = _affectedFaultMask.getBitmap() | _affectedWarnInfoMask.getBitmap();

    // cF is the same as CF from UpdateFaultStatus.vi
    uint64_t cF = summaryFaultStatus;

    // cFPrime is the same as 2.CF' from UpdateFaultStatus.vi
    uint64_t cFPrime = cF & ~(_affectedWarnInfoMask.getBitmap());

    uint64_t newMasked = newFaultStatus & affectedAll;

    // updatedSummaryFaults is the same as "Updated Summary Faults" from UpdateFaultStatus.vi
    uint64_t updatedSummaryFaults = cF ^ (newMasked | cFPrime);

    // changedBits is the same as "Changed Bits" from UpdateFaultStatus.vi
    uint64_t changedBits = _faultEnableMask.getBitmap() & affectedAll & updatedSummaryFaults;

    return make_tuple(updatedSummaryFaults, changedBits);
}

BasicFaultMgr::BasicFaultMgr() {
    // _summaryFaults, _prevFaults, and _currentFaults should all be zero already.
    _faultEnableMask.setBitmap(FaultStatusBits::getMaskFaults()); ///< "Fault Enable Mask"
    _defaultFaultMask.setBitmap(FaultStatusBits::getMaskFaults());; ///< "Default Fault Mask"
    // _affectedFaultsMask, and _affectedWarnInfoMask should both be zero already.
}

bool BasicFaultMgr::xmitFaults(FaultInfo::CrioSubsystem subsystem) {
    uint64_t diff = (_prevFaults.getBitmap() ^ _currentFaults.getBitmap()) & _faultEnableMask.getBitmap();
    if (diff == 0) {
        return false; // nothing needs to be done.
    }
    _prevFaults = _currentFaults;

    // from "BasicFaultManager.lvclass:send_faults.vi"
    // remove warning and info bits
    uint64_t summaryAndDefault = _summaryFaults.getBitmap() & _defaultFaultMask.getBitmap();
    // Add _currentFaults to _summaryFaults.
    uint64_t combined = summaryAndDefault | _currentFaults.getBitmap();
    _summaryFaults = combined;

    return true;
}


PowerFaultMgr::PowerFaultMgr() : BasicFaultMgr() {
    // see PowerSubsystem.lvclass:initialize.vi
    FaultStatusBits fsb;
    fsb.setBitmap(FaultStatusBits::getPowerSubsystemFaultManagerAffectedFaultMask());
    setAffectedFaultsMask(fsb);
    fsb.setBitmap(FaultStatusBits::getPowerSubsystemFaultManagerAffectedWarningMask());
    setAffectedFaultsMask(fsb);
}


void FaultMgr::setup() {
    lock_guard<mutex> lock(_thisPtrMtx);
    if (_thisPtr) {
        LERROR("FaultMgr already setup");
        return;
    }
    _thisPtr = Ptr(new FaultMgr());
}


FaultMgr::Ptr FaultMgr::getPtr() {
    if (_thisPtr == nullptr) {
        throw system::ConfigException(ERR_LOC, "FaultMgr has not been setup.");
    }
    return _thisPtr;
}

FaultMgr& FaultMgr::get() {
    if (_thisPtr == nullptr) {
        throw system::ConfigException(ERR_LOC, "FaultMgr has not been setup.");
    }
    return *_thisPtr;
}

std::tuple<uint16_t, uint16_t> FaultMgr::updateFaultStatus(
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


void FaultMgr::updatePowerFaults(FaultStatusBits currentFaults, FaultInfo::CrioSubsystem subsystem) {
    BasicFaultMgr bfm;
    {
        lock_guard<mutex> lgPowerFault(_powerFaultMtx);
        _powerFaultMgr.setCurrentFaults(currentFaults);
        if (!_powerFaultMgr.xmitFaults(subsystem)) {
            // no changes, nothing to do.
            return;
        }
        bfm = _powerFaultMgr;
    }

    {
        // see "SystemStatus.lvclass:updateSummaryFaultsStatus.vi"
        // Note that `FaultMgr::_summarySystemFaultsStatus` is the global
        //   system status object.
        lock_guard<mutex> lgSummary(_summarySystemFaultsMtx);
        auto [newSummary, changedBits] = updateFaultStatus(
                _summarySystemFaultsStatus.getBitmap(), bfm.getFaultEnableMask().getBitmap(),
                bfm.getCurrentFaults().getBitmap(),  bfm.getAffectedWarnInfoMask().getBitmap(),
                bfm.getAffectedFaultsMask().getBitmap());

        if (newSummary != _summarySystemFaultsStatus.getBitmap()) {
            _summarySystemFaultsStatus.setBitmap(newSummary);
            FaultStatusBits cBits(changedBits);
            LINFO("FaultMgr::updatePowerFaults changedBits=", cBits.getAllSetBitEnums());
        }
    }
    // &&& at this point the TelemetryItem for fault status should be updated,
    //     but there doesn't seem to be one.
}

bool FaultMgr::checkForPowerSubsystemFaults(FaultStatusBits const& subsystemMask, string const& note) {
    /* &&&
    /// see PowerSubsystem->set_motor_power.vi and BasicFaultManager->read_faults.vi
    FaultStatusBits faultBitmap(subsystemMask.getBitmap()
            & _faultEnableMask.getBitmap() & _currentFaults.getBitmap());
    if (faultBitmap.getBitmap() != 0) {
        LERROR("checkForPowerSubsystemFaults has faults for ", faultBitmap.getAllSetBitEnums());
    }
    return false;
    */

    FaultStatusBits faultBitmap;
    {
        lock_guard<mutex> lgPowerFault(_powerFaultMtx);
        /// see PowerSubsystem->set_motor_power.vi and BasicFaultManager->read_faults.vi
        PowerFaultMgr& pfm = _powerFaultMgr;
        faultBitmap.setBitmap(subsystemMask.getBitmap() & pfm.getFaultEnableMask().getBitmap()
                & pfm.getCurrentFaults().getBitmap());
    }
    bool result = faultBitmap.getBitmap() != 0;
    if (result) {
        LERROR("checkForPowerSubsystemFaults has faults for ", faultBitmap.getAllSetBitEnums());
    }
    return result;
}

FaultMgr::FaultMgr()   {
}

bool FaultMgr::setFault(std::string const& faultNote) {
    LERROR("FaultMgr::setFault PLACEHOLDER ", faultNote);
    return true;
}

/* &&&
bool FaultMgr::setFaultWithMask(uint64_t bitmask, std::string const& note) {
    //&&& _faultInfo._currentFaults |= bitmask;
    _faultInfo.setCurrentFaults(_currentFaults | bitmask);
}
*/

}  // namespace faultmgr
}  // namespace m2cellcpp
}  // namespace LSST

