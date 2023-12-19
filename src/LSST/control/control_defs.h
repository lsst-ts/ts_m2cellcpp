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
enum SysStatus { CRITICAL = -3, FAULT = -2, WARN = -1, GOOD = 0, WAITING = 1 };

/// Return a string representation of the SysStatus enum `sta`.
std::string getSysStatusStr(SysStatus sta);

/// Power system type enum, the values should match those used by "cmd_power".
enum PowerSystemType { MOTOR = 1, COMM = 2, UNKNOWNPOWERSYSTEM = 3 };

/// Return a string version of `sysT`.
std::string getPowerSystemTypeStr(PowerSystemType sysT);

/// Convert an integer to PowerSystemType, any value not `MOTOR` or `COMM`
/// will be returned as `UNKNOWNPOWERSYSTEM`
PowerSystemType intToPowerSystemType(int val);

/// PowerSystemState, values must match lsst-ts/ts_xml/python/lsst/ts/xml/enums/MTM2.py
// PowerSystemState { INIT = 1, POWEREDOFF = 2, POWERINGON = 3, RESETTINGBREAKERS = 4, POWEREDON = 5,
// POWERINGOFF = 6 };
enum PowerState { UNKNOWN = 1, OFF = 2, TURNING_ON = 3, RESET = 4, ON = 5, TURNING_OFF = 6 };

/// Return a string version of `sysState` to be used in TCP/IP communications.
std::string getPowerStateOldStr(PowerState sysState);

/// Return a string version of PowerSystemState that matches the enum value and
/// is similar to those found LabView code.
std::string getPowerStateStr(PowerState powerState);

}  // namespace control
}  // namespace m2cellcpp
}  // namespace LSST

#endif  // LSST_M2CELLCPP_CONTROL_CONTROLDEFS_H
