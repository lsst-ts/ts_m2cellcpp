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
#include "control/OutputPortBits.h"

// system headers
#include <bitset>
#include <sstream>
#include <mutex>
#include <stdexcept>
#include <string>

// project headers
#include "control/FaultStatusBits.h"


using namespace std;

namespace LSST {
namespace m2cellcpp {
namespace control {


void OutputPortBits::writeBit(int pos, bool set) {
    FaultStatusBits::setBit8(_bitmap, pos, set);
}


bool OutputPortBits::getBit(int pos) {
    if (pos < 0 || pos >= 8 ) {
        return 0;
    }
    uint8_t mask = 1;
    mask <<= pos;
    return getBitsSetInMask(mask);
}


string OutputPortBits::getAllSetBitEnums() {
    string str;
    uint8_t mask = 1;

    for (int j=0; j<8; ++j) {
        if (getBitsSetInMask(mask)) {
            str += getEnumString(j) + ",";
        }
         mask <<= 1;
    }
    return str;
}

string OutputPortBits::getBinaryStr(uint8_t val) {
    stringstream os;
    os << std::bitset<8>(val);
    return os.str();
}

string OutputPortBits::getEnumString(int enumVal) {
    switch(enumVal) {
    case MOTOR_POWER_ON: return "MOTOR_POWER_ON " + to_string(enumVal);
    case ILC_COMM_POWER_ON: return "ILC_COMM_POWER_ON " + to_string(enumVal);
    case CRIO_INTERLOCK_ENABLE: return "CRIO_INTERLOCK_ENABLE " + to_string(enumVal);
    case RESET_MOTOR_BREAKERS: return "RESET_MOTOR_BREAKERS " + to_string(enumVal);
    case RESET_COMM_BREAKERS: return "RESET_COMM_BREAKERS " + to_string(enumVal);
    case SPARE_D05: return "SPARE_D05 " + to_string(enumVal);
    case SPARE_D06: return "SPARE_D06 " + to_string(enumVal);
    case SPARE_D07: return "SPARE_D07 " + to_string(enumVal);
    }
    return "unknown " + to_string(enumVal);
}

}  // namespace control
}  // namespace m2cellcpp
}  // namespace LSST
