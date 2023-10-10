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
#include "control/Context.h"
#include "faultmgr/BasicFaultMgr.h"
#include "system/Config.h"
#include "util/Bug.h"
#include "util/Log.h"

using namespace std;

namespace LSST {
namespace m2cellcpp {
namespace faultmgr {

FaultMgr::Ptr FaultMgr::_thisPtr;
std::mutex FaultMgr::_thisPtrMtx;


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


void FaultMgr::resetFaults(FaultStatusBits resetMask) {
    BasicFaultMgr newSummary;
    {
        lock_guard<mutex> lgPower(_powerFaultMtx);
        _powerFaultMgr.resetFaults(resetMask);
    }

    {
        lock_guard<mutex> lgTelemetry(_telemetryFaultMtx);
        _telemetryFaultMgr.resetFaults(resetMask);
    }

    {
        lock_guard<mutex> lgSummary(_summarySystemFaultsMtx);
        // Model.lvclass:resetAndReportSummaryFaultStatus.vi
        _summarySystemFaultsStatus.resetFaults(resetMask);
        newSummary = _summarySystemFaultsStatus;
    }

    // update TelemetryCom value.
    _updateTelemetryCom(newSummary);
}


void FaultMgr::reportComConnectionCount(size_t count) {
    // see FaultManager.lvclass::process_network_fault.vi
    _commConnectionFault = (count < 1);

    FaultStatusBits commMask;
    commMask.setBit(FaultStatusBits::CRIO_COMM_FAULT);
    if (_commConnectionFault) {
        {
            lock_guard<mutex> lgSummary(_summarySystemFaultsMtx);
            _summarySystemFaultsStatus.setMaskComm(commMask);
        }
        control::Context::get()->model.goToSafeMode("no TCP/IP connections");
    } else {
        resetFaults(commMask);
    }
    // checkForPowerSubsystemFaults() checks _commConnectionFault to
    // prevent power on without network connection.
}


void FaultMgr::updatePowerFaults(FaultStatusBits currentFaults, FaultInfo::CrioSubsystem subsystem) {
    BasicFaultMgr bfm;
    {
        lock_guard<mutex> lgPowerFault(_powerFaultMtx);
        _powerFaultMgr.setCurrentFaults(currentFaults);
        // LabView code would like to OR the 'affected' masks together,
        // but that's always the equivalent of ORing something with itself here.
        //  see "FaultManager.lvclass:combine_affected_masks.vi
        // Following merges current faults into _powerFaultMgr._summaryFaults.
        if (!_powerFaultMgr.xmitFaults(subsystem)) {
            // no changes, nothing more to do.
            return;
        }
        bfm = _powerFaultMgr;
    }

    // Check newFaultStatus for faults that require power or system state changes.
    switch (subsystem) {
    case FaultInfo::POWER_SUBSYSTEM:
    {
        // see FaultManager.lvclass:health_fault_occurred.vi"
        FaultStatusBits currentHealthFaults;
        currentHealthFaults.setBitmap(currentFaults.getBitmap() & _healthFaultMask.getBitmap()); // "Health Fault Mask"
        // Instead of only sending the message when something new happens, which is
        // kind of tricky, always have the model go to safe mode. However,
        // safe mode in the model doesn't do anything if it's already trying
        // to go to safe mode.
        if (currentHealthFaults.getBitmap() != 0) {
            control::Context::get()->model.goToSafeMode(string("FaultMgr PowerFault ") + currentHealthFaults.getAllSetBitEnums());
        }
        break;
    }
    case FaultInfo::TELEMETRY_LOGGER: [[fallthrough]];
    case FaultInfo::NETWORK_INTERFACE: [[fallthrough]];
    case FaultInfo::SYSTEM_CONTROLLER: [[fallthrough]];
    case FaultInfo::FAULT_MANAGER: [[fallthrough]];
    case FaultInfo::CELL_CONTROLLER: [[fallthrough]];
    case FaultInfo::MOTION_ENGINE: [[fallthrough]];
    default:
        LCRITICAL(__func__, "unexpected call with subsystem set to ", subsystem);
    }

    BasicFaultMgr newFsbSummary;
    {
        // see "SystemStatus.lvclass:updateSummaryFaultsStatus.vi"
        // Note that `FaultMgr::_summarySystemFaultsStatus` is the global
        //   system status object.
        lock_guard<mutex> lgSummary(_summarySystemFaultsMtx);
        auto [nSummary, changedBits] = BasicFaultMgr::updateFaultStatus(
                _summarySystemFaultsStatus.getSummaryFaults().getBitmap(), bfm.getFaultEnableMask().getBitmap(),
                bfm.getCurrentFaults().getBitmap(),  bfm.getAffectedWarnInfoMask().getBitmap(),
                bfm.getAffectedFaultsMask().getBitmap());

        if (nSummary != _summarySystemFaultsStatus.getSummaryFaults().getBitmap()) {
            _summarySystemFaultsStatus.updateSummary(nSummary);
            FaultStatusBits cBits(changedBits);
            LINFO("FaultMgr::updatePowerFaults changedBits=", cBits.getAllSetBitEnums());
        }
        newFsbSummary = _summarySystemFaultsStatus;
    }
    // TODO: At this point the TelemetryItem for fault status should be updated,
    // but there doesn't seem to be one.
    _updateTelemetryCom(newFsbSummary);
}


bool FaultMgr::checkForPowerSubsystemFaults(FaultStatusBits const& subsystemMask, string const& note) {
    // This will make certain that power cannot be turned on if there are no Tcp/Ip connections.
    bool faultFound = _commConnectionFault;

    FaultStatusBits faultBitmap;
    {
        lock_guard<mutex> lgPowerFault(_powerFaultMtx);
        /// see PowerSubsystem->set_motor_power.vi and BasicFaultManager->read_faults.vi
        PowerFaultMgr& pfm = _powerFaultMgr;
        faultBitmap.setBitmap(subsystemMask.getBitmap() & pfm.getFaultEnableMask().getBitmap()
                & pfm.getCurrentFaults().getBitmap());
    }
    if (!faultFound) {
        faultFound = faultBitmap.getBitmap() != 0;
    }
    if (faultFound) {
        LERROR("checkForPowerSubsystemFaults has faults for ", faultBitmap.getAllSetBitEnums(),
                " _commConnectionFault=", to_string(_commConnectionFault));
    }
    return faultFound;
}

FaultMgr::FaultMgr() {
}

bool FaultMgr::setFault(std::string const& faultNote) {
    LERROR("FaultMgr::setFault PLACEHOLDER ", faultNote);
    return true;
}

void FaultMgr::_updateTelemetryCom(BasicFaultMgr const& newFsbSummary) {
    // TODO: DM-40339 send information to telemetry TCP/IP server
    LCRITICAL("FaultMgr::_updateTelemetry NEEDS CODE");
}

}  // namespace faultmgr
}  // namespace m2cellcpp
}  // namespace LSST

