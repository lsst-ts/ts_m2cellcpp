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
#include "control/control_defs.h"

// system headers

// project headers

using namespace std;

namespace LSST {
namespace m2cellcpp {
namespace control {

string getSysStatusStr(SysStatus sta) {
    switch (sta) {
        case GOOD:
            return "GOOD";
        case WAITING:
            return "WATIING";
        case WARN:
            return "WARN";
        case FAULT:
            return "FAULT";
        case CRITICAL:
            return "CRITICAL";
    }
    return "unknown";
}

string getPowerSystemTypeStr(PowerSystemType sysT) {
    switch (sysT) {
        case MOTOR:
            return "MOTOR";
        case COMM:
            return "COMM";
        case UNKNOWNPOWERSYSTEM:
            return "UNKNOWNPOWERSYSTEM";
    }
    return "unknown";
}

PowerSystemType intToPowerSystemType(int val) {
    switch (val) {
        case MOTOR:
            return MOTOR;
        case COMM:
            return COMM;
        default:
            return UNKNOWNPOWERSYSTEM;
    }
}

string getPowerStateOldStr(PowerState sysState) {
    switch (sysState) {
        case UNKNOWN:
            return "Init";
        case OFF:
            return "PoweredOff";
        case TURNING_ON:
            return "PoweringOn";
        case RESET:
            return "ResettingBreakers";
        case ON:
            return "PoweredOn";
        case TURNING_OFF:
            return "PoweringOff";
    }
    return "unknown";
}

string getPowerStateStr(PowerState powerState) {
    int val = powerState;
    string valStr = string(" ") + to_string(val);
    switch (powerState) {
        case ON:
            return "ON" + valStr;
        case TURNING_ON:
            return "TURNING_ON" + valStr;
        case TURNING_OFF:
            return "TURNING_OFF" + valStr;
        case OFF:
            return "OFF" + valStr;
        case RESET:
            return "RESET" + valStr;
        case UNKNOWN:
            return "UNKNOWN" + valStr;
    }
    return "unknown" + valStr;
}

}  // namespace control
}  // namespace m2cellcpp
}  // namespace LSST
