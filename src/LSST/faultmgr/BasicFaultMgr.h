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

/// Stores a copy of fault information for a specific system.
/// All systems use the same layout for a uint64_t with the bits mapped
/// to specific faults (see `FaultStatusBits`).
/// The purpose of most of the member variables is not well defined.
/// `_currentFaults` is a record of the most recently set faults and it
///    can be completely overwritten during a fault update.
/// `_summaryFaults` is a record of all faults that haven't been reset.
///    Values are usually latched to '1' and usually only go back to '0'
///    when `resetFaults()` is called.
/// `_prevFaults` is usually the previous value of `_currentFaults`,
///    which seems to be used to limit message passing in LabView. This
///    isn't that important in the C++ code and should probably be removed.
/// `_faultEnableMask` is used enable/disable faults.
/// `_defaultFaultMask` may be used for the default enable mask.
/// `_affectedFaultsMask` and `_affectedWarnInfoMask` in subsystems
/// appear to indicate which fault bits cause faults or warnings for
/// the subsystem. The central copy has all of the local `_affected`
/// masks added to its `_affected` masks during updates (see
/// `fault_manager_main.vi` "User Event:FaultsTelem").
///
/// They shouldn't need to have separate copies, as faults are system
/// wide, but the LabView code has a central copy and then one copy per
/// subsystem (most of them anyway) that are kept in sync using
/// "FaultsTelem" User Event messages. There are subtle inconsistencies
/// between how they are implemented in the LabView code and it's
/// difficult to tell what effect the inconsistencies have, or
/// are meant to have.
/// The message passing has been eliminated but extra copies
/// still remain.
/// "BasicFaultManager.lvclass:init.vi"
class BasicFaultMgr {
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

    /// from "BasicFaultManager.lvclass:init.vi"
    BasicFaultMgr();

    /// This will be passed as a reference frequently and we want to
    /// avoid accidental copying in function calls, but we do want
    /// to be able to set them equal on occasion.
    BasicFaultMgr(BasicFaultMgr const&) = delete;
    BasicFaultMgr& operator=(BasicFaultMgr const& other) = default;

    virtual ~BasicFaultMgr() = default;

    /// This function is used to combine new fault information with existing
    /// fault information.
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
    /// @return FaultStatusBits with bits that changed set to 1, equivalent to the
    ///         same in UpdateFaultStatus.vi
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

    /// Return true if faults used by this system have changed and need to be shared.
    /// From "BasicFaultManager.lvclass:xmit_faults.vi"
    bool updateFaults(CrioSubsystem subsystem);

    /// Reset the faults in the mask in `_summaryFaults`, `_currentFaults`, and `_prevFaults`.
    /// From "BasicFaultManager.lvclass:resetFaults.vi"
    void resetFaults(FaultStatusBits mask);

    /// Set the faults according to what is needed for a TCP/IP communications fault.
    void setMaskComm(FaultStatusBits mask);

    /// Update `_currentFaults` and `_summaryFaults` to `newSummary` and set `_prevFaults`
    /// to the old `_currentFaults` value;
    void updateSummary(uint64_t newSummary);

    /// Merge FaultStatusBits in `bits` into `_summaryFaults`, but only those
    /// bits that are enabled.
    /// @param - `bits` a mask containing the bits to set.
    /// @return - a mask containing the changed bits.
    ///
    /// Note: Is hoped that once ILC related faults have been added to the
    /// the code base that `mergeFaults` (or some version of it) can replace
    /// `updateFaultStatus`. The individual systems should only need
    /// `_affectedFaultsMask` so they can mask out faults that do not affect them.
    /// Most of the logic in `updateFaultStatus` LabView's version seems to be
    /// focused on avoiding resending messages. The C++ code doesn't send
    /// messages for faults, so that complexity can be removed.
    FaultStatusBits mergeFaults(FaultStatusBits bits);

    /// Enable the fault bits set in `mask` in `_faultEnableMask`.
    /// @param `mask` - bitmap of faults to be enabled.
    /// @return FaultsStatusBits object with the changed bits from
    ///         `_faultEnableMask`.
    FaultStatusBits enableFaultsInMask(FaultStatusBits mask);

    /// Return a log worthy string version of this object.
    std::string dump() const;

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
