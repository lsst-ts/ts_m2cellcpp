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

#ifndef LSST_M2CELLCPP_FAULTMGR_FAULTMGR_H
#define LSST_M2CELLCPP_FAULTMGR_FAULTMGR_H

// System headers
#include <functional>
#include <map>
#include <memory>
#include <mutex>

// Project headers
#include "control/control_defs.h"
#include "faultmgr/FaultStatusBits.h"

namespace LSST {
namespace m2cellcpp {
namespace faultmgr {

/// This class will track and reset faults encountered in the system.
/// FUTURE: DM-40909 add functionality to FaultMgr, currently there's just enough here to turn on the power.
/// Unit tests in test_FaultMgr.cpp.
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

    /// PLACEHOLDER set a fault until the appropriate way to handle it is determined.
    void setFault(std::string const& faultNote);

private:
    static Ptr _thisPtr; ///< pointer to the global instance of FaultMgr.
    static std::mutex _thisPtrMtx; ///< Protects `_thisPtr`.

    // Private constructor to force use of `setup`;
    FaultMgr();

    FaultStatusBits _currentFaults;    ///< Current faults.

    /// Mask with '1's at enabled fault positions.
    FaultStatusBits _faultEnableMask{FaultStatusBits::getMaskFaults()};

};

}  // namespace faultmgr
}  // namespace m2cellcpp
}  // namespace LSST

#endif  // LSST_M2CELLCPP_FAULTMGR_FAULTMGR_H
