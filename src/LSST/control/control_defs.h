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

#ifndef LSST_M2CELLCPP_CONTROL_CONTROLDEFS_H
#define LSST_M2CELLCPP_CONTROL_CONTROLDEFS_H

// System headers
#include <memory>
#include <string>

/// This file contains simple definitions related to the `control` namespace.

namespace LSST {
namespace m2cellcpp {
namespace control {

/// Status enum, 0 indicates everything is GOOD,
/// Values less than 0 indicate problems,
/// Values above 0 indicate a state where nothing is wrong, but GOOD does not apply.
enum SysStatus {
    CRITICAL = -3,
    FAULT = -2,
    WARN = -1,
    GOOD = 0,
    WAITING = 1
};

/// Return a string representation of the enum `sta`.
inline std::string getSysStatusStr(SysStatus sta) {
    switch (sta) {
    case GOOD: return "GOOD";
    case WAITING: return "WATIING";
    case WARN: return "WARN";
    case FAULT: return "FAULT";
    case CRITICAL: return "CRITICAL";
    }
    return "unknown";
}

/// Power system type enum.
enum PowerSystemType {
    MOTOR = 0,
    COMM = 1
};

/// Return a string version of `sysT`.
inline std::string getPowerSystemTypeStr(PowerSystemType sysT) {
    switch(sysT) {
    case MOTOR: return "MOTOR";
    case COMM: return "COMM";
    }
    return "unknown ";
}

}  // namespace control
}  // namespace m2cellcpp
}  // namespace LSST

#endif  // LSST_M2CELLCPP_CONTROL_CONTROLDEFS_H
