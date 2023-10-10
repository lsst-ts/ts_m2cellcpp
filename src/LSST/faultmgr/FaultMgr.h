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
    /// @param subsystemMask - mask of bits for the specific subsystem.
    /// @param note - identifier for calling class and/or function.
    bool checkForPowerSubsystemFaults(FaultStatusBits const& subsystemMask, std::string const& note);

    /// PLACEHOLDER set a fault until the appropriate way to handle it is determined. &&&
    bool setFault(std::string const& faultNote); /// &&& delete

    /// Update faults based on `currentFaults` provided by `subsystem`.
    void updatePowerFaults(FaultStatusBits currentFaults, FaultInfo::CrioSubsystem subsystem);

    /// Get the number of TCP/IP connections `count`, triggering a fault if 0.
    void reportComConnectionCount(size_t count);

    /// Reset the fault bits in `resetMask` and send any required updates.
    void resetFaults(FaultStatusBits resetMask);

private:
    static Ptr _thisPtr; ///< pointer to the global instance of FaultMgr.
    static std::mutex _thisPtrMtx; ///< Protects `_thisPtr`.

    // Private constructor to force use of `setup`;
    FaultMgr();

    /// Update TelemetryCom with `newFsbSummary`, which should be the latest value
    /// of `_summarySystemFaultsStatus`.
    void _updateTelemetryCom(BasicFaultMgr const& newFsbSummary);

    /// Overall system faults status, Model->systemStatus->summaryFaultsStatus
    BasicFaultMgr _summarySystemFaultsStatus;
    std::mutex _summarySystemFaultsMtx; ///< Protects `_summarySystemFaultsStatus`.

    /// Faults, warnings, and info associated with power systems.
    PowerFaultMgr _powerFaultMgr;
    std::mutex _powerFaultMtx; ///< Protects `_powerFaultMgr`.

    /// Faults, warnings, and info associated with ILC's.
    /// FUTURE: complete implementation
    TelemetryFaultMgr _telemetryFaultMgr;
    std::mutex _telemetryFaultMtx; ///< Protects `_telemetryFaultMgr`.

    /// True when there are no ComServer Tcp/Ip connections.
    std::atomic<bool> _commConnectionFault{true};

    FaultStatusBits _healthFaultMask{FaultStatusBits::getMaskHealthFaults()}; ///< "Health Fault Mask"
};

}  // namespace faultmgr
}  // namespace m2cellcpp
}  // namespace LSST

#endif  // LSST_M2CELLCPP_FAULTMGR_H
