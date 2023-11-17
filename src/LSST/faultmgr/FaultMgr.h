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

#ifndef LSST_M2CELLCPP_FAULTMGR_H
#define LSST_M2CELLCPP_FAULTMGR_H

// System headers
#include <functional>
#include <map>
#include <memory>
#include <mutex>

// Project headers
#include "control/control_defs.h"
#include "faultmgr/BasicFaultMgr.h"
#include "faultmgr/FaultStatusBits.h"
#include "util/clock_defs.h"

namespace LSST {
namespace m2cellcpp {
namespace faultmgr {


/// This class will track and reset faults encountered in the system.
/// Instead of sending messages around to all possible systems when faults are triggered, as
/// the LabView code does, all faults will be stored in a global instance of FaultMgr.
/// When systems check for faults, they will do so through this central instance.
/// Unit tests in test_PowerSystem.cpp.
class FaultMgr {
public:
    using Ptr = std::shared_ptr<FaultMgr>;

    /// Create the global FaultMgr object.
    static void setup();

    /// Return a reference to the global FaultMgr instance.
    /// @throws `ConfigException` if `setup` has not already been called.
    static FaultMgr& get();

    /// Return a shared pointer to the global FaultMgr instance.
    /// @throws `ConfigException` if `setup` has not already been called.
    static Ptr getPtr();

    /// Return true if there are any faults that the subsystem cares about.
    /// @param `subsystemMask` - mask of bits for the specific subsystem.
    /// @param `note` - identifier for calling class and/or function.
    bool checkForPowerSubsystemFaults(FaultStatusBits const& subsystemMask, std::string const& note);

    /// Update faults based on `currentFaults` provided by `subsystem`.
    void updatePowerFaults(FaultStatusBits currentFaults, BasicFaultMgr::CrioSubsystem subsystem);

    /// Get the number of TCP/IP connections `count`, triggering a fault if 0.
    void reportComConnectionCount(size_t count);

    /// Reset the fault bits in `resetMask` and send any required updates.
    void resetFaults(FaultStatusBits resetMask);

    /// Currently, this just logs an error. Eventually, this should result in a message
    /// that can be displayed by the GUI.
    void faultMsg(int errId, std::string const& eMsg);

    /// Enable the fault bits set in `mask` in all `_faultEnableMask` found in
    /// this instance.
    /// @param `mask` - bitmap of faults to be enabled.
    /// @return FaultsStatusBits object with the changed bits from
    ///         `_summarySystemFaultsStatus._faultEnableMask`.
    FaultStatusBits enableFaultsInMask(FaultStatusBits mask);

    /// Report a motion engine data timeout and flag the appropriate fault.
    /// @param `errorLvl` - `true` indicates an error while `false` indicates a warning.
    /// @param `msg` - a test message with more information about the error.
    void reportMotionEngineTimeout(bool errorLvl, std::string const& msg);

    /// Returns the system summary faults from `_summarySystemFaultsStatus`,
    /// this is the primary copy of the system summary faults.
    FaultStatusBits getSummaryFaults() const;

    /// Returns the system enabled faults mask, which contains a mask
    /// of the system wide enabled faults.
    FaultStatusBits getFaultEnableMask() const;

    /// Return a log worthy string version of this object.
    std::string dump() const;

private:
    static Ptr _thisPtr; ///< pointer to the global instance of FaultMgr.
    static std::mutex _thisPtrMtx; ///< Protects `_thisPtr`.

    // Private constructor to force use of `setup`;
    FaultMgr();

    /// Update TelemetryCom with `newFsbSummary`, which should be the latest value
    /// of `_summarySystemFaultsStatus`.
    void _updateTelemetryCom(BasicFaultMgr const& newFsbSummary);

    /// Overall system faults status, Model->systemStatus->summaryFaultsStatus
    /// FUTURE: It is hoped that this will be the only BasicFaultMgr needed after
    /// including ILC faults. Individual systems will probably still need `affect` members.
    BasicFaultMgr _summarySystemFaultsStatus;
    mutable std::mutex _summarySystemFaultsMtx; ///< Protects `_summarySystemFaultsStatus`.

    /// Faults, warnings, and info associated with power systems.
    /// FUTURE: Hopefully, all elements except `affected` can be removed.
    PowerFaultMgr _powerFaultMgr;
    mutable std::mutex _powerFaultMtx; ///< Protects `_powerFaultMgr`.

    /// Faults, warnings, and info associated with ILC's.
    /// FUTURE: complete implementation
    /// FUTURE: Hopefully, all elements except `affected` can be removed.
    TelemetryFaultMgr _telemetryFaultMgr;
    mutable std::mutex _telemetryFaultMtx; ///< Protects `_telemetryFaultMgr`.

    /// True when there are no ComServer Tcp/Ip connections.
    std::atomic<bool> _commConnectionFault{true};

    FaultStatusBits _healthFaultMask{FaultStatusBits::getMaskHealthFaults()}; ///< "Health Fault Mask"
};

}  // namespace faultmgr
}  // namespace m2cellcpp
}  // namespace LSST

#endif  // LSST_M2CELLCPP_FAULTMGR_H
