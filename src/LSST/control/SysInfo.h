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

#ifndef LSST_M2CELLCPP_CONTROL_SYSINFO_H
#define LSST_M2CELLCPP_CONTROL_SYSINFO_H

// System headers
#include <memory>

// Project headers
#include "control/InputPortBits.h"
#include "control/OutputPortBits.h"

namespace LSST {
namespace m2cellcpp {
namespace control {


/// Copy of relevant simulation information.
class SysInfo {
public:
    control::OutputPortBits outputPort;
    control::InputPortBits inputPort;
    double motorVoltage; ///< volts
    double motorCurrent; ///< amps
    bool motorBreakerClosed;
    double commVoltage; ///< volts
    double commCurrent; ///< amps
    bool commBreakerClosed;
    uint64_t iterations;

    /// Return a log worthy string containing information about this class.
    std::string dump();
};

}  // namespace control
}  // namespace m2cellcpp
}  // namespace LSST

#endif  // LSST_M2CELLCPP_CONTROL_SYSINFO_H
