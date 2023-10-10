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

#ifndef LSST_M2CELLCPP_BASICFAULTMGR_H
#define LSST_M2CELLCPP_BASICFAULTMGR_H

// System headers
#include <functional>
#include <map>
#include <memory>
#include <mutex>

// Project headers
#include "control/control_defs.h"
#include "faultmgr/FaultStatusBits.h"
#include "util/clock_defs.h"

namespace LSST {
namespace m2cellcpp {
namespace faultmgr {

class FaultMgr;

/// doc &&&
/// BasicFaultManager->send_faults.vi
class FaultInfo {
public:

    /// cRIO subsystem enum values from SystemControlDefs.lvlib:cRIO_Subsystem.ctl
    /// Note: This may not be the best place to put this.
    enum CrioSubsystem {
        SYSTEM_CONTROLLER = 0, ///< "System controller" = 0
        FAULT_MANAGER = 1,     ///< "Fault Manager" = 1
        POWER_SUBSYSTEM = 2,   ///< "Power Subsystem" = 2
        CELL_CONTROLLER = 3,   ///< "Cell Controller" = 3
        TELEMETRY_LOGGER = 4,  ///< "Telemetry Logger" = 4
        NETWORK_INTERFACE = 5, ///< "Network Interface" = 5
        MOTION_ENGINE = 6      ///< "Motion Engine" = 6
    };

    static std::string getCrioSubsystemStr(CrioSubsystem subSystem);
};


/// &&& doc
/// "BasicFaultManager.lvclass:init.vi"
class BasicFaultMgr {
public:
    /// &&& doc
    /// from "BasicFaultManager.lvclass:init.vi"
    BasicFaultMgr();

    /// This will be passed as a reference frequently and we want to
    /// avoid accidental copying in function calls, but we do want
    /// to be able to set them equal on occasion.
    BasicFaultMgr(BasicFaultMgr const&) = delete;
    BasicFaultMgr& operator=(BasicFaultMgr const& other) = default;

    virtual ~BasicFaultMgr() = default;

    /// &&& doc
    /// This is used by "TelemetryFaultManager.lvclass:xmit_faults.vi" and
    ///  "SystemStatus.lvclass:updateSummaryFaultsStatus.vi".
    /// However, "BasicFaultManager.lvclass:send_faults.vi" just ands "Summary Fault Status" with
    ///   "Default Fault Mask" then ors that with "Current Faults Status". Then passes that on
    ///   as a "FaultsTelem" message. There's no explanation given as why different versions are
    ///   required.
    /// There's also no explanation given for the purpose of "Affected" variables. They seem to be
    ///  bit masks that limit the operations to some entries in the bitmap, but that is less clear in
    ///  their use with `_summaryFaultsStatus`.
    /// @param summaryFaultStatus equivalent to Summary Fault Status in UpdateFaultStatus.vi
    /// @param faultEnableMask equivalent to Fault Enable Mask in UpdateFaultStatus.vi
    /// @param newFaultStatus equivalent to New Fault Status in UpdateFaultStatus.vi
    /// @param affectedWarnInfo equivalent to Affected Warnings/Info Mask in UpdateFaultStatus.vi
    /// @param affectedFault equivalent to Affected Fault Mask in UpdateFaultStatus.vi
    /// @return FaultStatusBits with Updated Summary Faults, equivalent to the same in UpdateFaultStatus.vi
    /// @return FaultStatusBits with bits that changed set to 1, , equivalent to the same in UpdateFaultStatus.vi
    /// UpdateFaultStatus.vi
    static std::tuple<uint16_t, uint16_t> updateFaultStatus(
            uint64_t summaryFaultStatus, uint64_t faultEnableMask,
            uint64_t newFaultStatus, uint64_t affectedWarnInfo, uint64_t affectedFault);

    /// Return `_summaryFaults`
    FaultStatusBits getSummaryFaults() const { return _summaryFaults; }

    /// Set `_summaryFaults` to `val`.
    void setSummaryFaults(FaultStatusBits& val) { _summaryFaults = val; }

    /// Return `_prevFaults`
    FaultStatusBits getPrevFaults() const { return _prevFaults; }

    /// Set `_prevFaults` to `val`.
    void setPrevFaults(FaultStatusBits& val) { _prevFaults = val; }

    /// Return `_currentFaults`
    FaultStatusBits getCurrentFaults() const { return _currentFaults; }

    /// Set `_currentFaults` to `val`.
    void setCurrentFaults(FaultStatusBits& val) { _currentFaults = val; }

    /// Return `_faultEnableMask`
    FaultStatusBits getFaultEnableMask() const { return _faultEnableMask; }

    /// Set `_faultEnable` to `val`.
    void setFaultEnableMask(FaultStatusBits& val) { _faultEnableMask = val; }

    /// Return `_defaultFaultMask`
    FaultStatusBits getDefaultFaultMask() const { return _defaultFaultMask; }

    /// Set `_defaultFaultMask` to `val`.
    void setDefaultFaultMask(FaultStatusBits& val) { _defaultFaultMask = val; }

    /// Return `_affectedFaultsMask`
    FaultStatusBits getAffectedFaultsMask() const { return _affectedFaultsMask; }

    /// Set `_affectedFaultsMask` to `val`.
    void setAffectedFaultsMask(FaultStatusBits& val) { _affectedFaultsMask = val; }

    /// Return `_affectedWarnInfoMask`
    FaultStatusBits getAffectedWarnInfoMask() const { return _affectedWarnInfoMask; }

    /// Set `_affectedWarnInfoMask` to `val`.
    void setAffectedWarnInfoMask(FaultStatusBits& val) { _affectedWarnInfoMask = val; }

    /// Return true if faults used by this sytem have changed and need to be shared.
    /// From "BasicFaultManager.lvclass:xmit_faults.vi"
    bool xmitFaults(FaultInfo::CrioSubsystem subsystem); // &&& change name.

    /// Reset the faults in the mask in `_summaryFaults`, `_currentFaults`, and `_prevFaults`.
    /// From "BasicFaultManager.lvclass:resetFaults.vi"
    void resetFaults(FaultStatusBits mask);

    /// Set the faults according to what is needed for a TCP/IP communications fault.
    void setMaskComm(FaultStatusBits mask);

    /// &&& doc
    void updateSummary(uint64_t);

private:
    FaultStatusBits _summaryFaults;        ///< "Summary Faults Status"
    FaultStatusBits _prevFaults;           ///< "Previous Faults Status"
    FaultStatusBits _currentFaults;        ///< "Current Faults Status"
    FaultStatusBits _faultEnableMask;      ///< "Fault Enable Mask"
    FaultStatusBits _defaultFaultMask;     ///< "Default Fault Mask"
    FaultStatusBits _affectedFaultsMask;   ///< "Affected Faults Bit Mask"
    FaultStatusBits _affectedWarnInfoMask; ///< "Affected Warnings/Info Bit Mask"

    util::TIMEPOINT _timeStamp; ///< last time there was a significant change.
};


/// `PowerFaultMgr` is `BasicFaultMgr` with slightly different initialization.
class PowerFaultMgr : public BasicFaultMgr {
public:
    /// Constructor based on PowerSubsystem.lvclass:initialize.vi and
    /// PowerSubsystem.lvclass:create_affected_bit_masks.vi
    PowerFaultMgr();

    ~PowerFaultMgr() override = default;
};


/// `TelemetryFaultMgr` adds arrays for ILC's and such, which will be implemented when ILC
/// handling is added.
class TelemetryFaultMgr : public BasicFaultMgr {
public:
    /// Initialize arrays.
    TelemetryFaultMgr() : BasicFaultMgr() {}
};


}  // namespace faultmgr
}  // namespace m2cellcpp
}  // namespace LSST

#endif  // LSST_M2CELLCPP_BASICFAULTMGR_H
