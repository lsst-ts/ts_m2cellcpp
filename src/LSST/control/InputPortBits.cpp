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
#include "control/InputPortBits.h"

// system headers
#include <bitset>
#include <sstream>
#include <mutex>
#include <stdexcept>
#include <string>

// project headers
#include "faultmgr/FaultStatusBits.h"



using namespace std;

namespace LSST {
namespace m2cellcpp {
namespace control {

void InputPortBits::writeBit(int pos, bool set) {
    faultmgr::FaultStatusBits::setBit32(_bitmap, pos, set);
}


bool InputPortBits::getBitAtPos(int pos) const {
    if (pos < 0 || pos >= 32 ) {
        if (pos == InputPortBits::ALWAYS_HIGH) {
            return 1;
        }
        // No need to check for ALWAYS_LOW
        return 0;
    }
    uint32_t mask = 1;
    mask <<= pos;
    return getBitsSetInMask(mask);
}


string InputPortBits::getAllSetBitEnums() const {
    string str;
    uint32_t mask = 1;

    for (int j=0; j<32; ++j) {
        if (getBitsSetInMask(mask)) {
            str += getEnumString(j) + ",";
        }
         mask <<= 1;
    }
    return str;
}

string InputPortBits::getBinaryStr(uint32_t val) {
    stringstream os;
    os << std::bitset<32>(val);
    return os.str();
}

string InputPortBits::getEnumString(int enumVal) {
    switch(enumVal) {
    case REDUNDANCY_OK: return "REDUNDANCY_OK " + to_string(enumVal);
    case LOAD_DISTRIBUTION_OK: return "LOAD_DISTRIBUTION_OK " + to_string(enumVal);
    case POWER_SUPPLY_2_DC_OK: return "POWER_SUPPLY_2_DC_OK " + to_string(enumVal);
    case POWER_SUPPLY_1_DC_OK: return "POWER_SUPPLY_1_DC_OK " + to_string(enumVal);
    case POWER_SUPPLY_2_CURRENT_OK: return "POWER_SUPPLY_2_CURRENT_OK " + to_string(enumVal);
    case POWER_SUPPLY_1_CURRENT_OK: return "POWER_SUPPLY_1_CURRENT_OK " + to_string(enumVal);
    case J1_W9_1_MTR_PWR_BRKR_OK: return "J1_W9_1_MTR_PWR_BRKR_OK " + to_string(enumVal);
    case J1_W9_2_MTR_PWR_BRKR_OK: return "J1_W9_2_MTR_PWR_BRKR_OK " + to_string(enumVal);
    case J1_W9_3_MTR_PWR_BRKR_OK: return "J1_W9_3_MTR_PWR_BRKR_OK " + to_string(enumVal);
    case J2_W10_1_MTR_PWR_BRKR_OK: return "J2_W10_1_MTR_PWR_BRKR_OK " + to_string(enumVal);
    case J2_W10_2_MTR_PWR_BRKR_OK: return "J2_W10_2_MTR_PWR_BRKR_OK " + to_string(enumVal);
    case J2_W10_3_MTR_PWR_BRKR_OK: return "J2_W10_3_MTR_PWR_BRKR_OK " + to_string(enumVal);
    case J3_W11_1_MTR_PWR_BRKR_OK: return "J3_W11_1_MTR_PWR_BRKR_OK " + to_string(enumVal);
    case J3_W11_2_MTR_PWR_BRKR_OK: return "J3_W11_2_MTR_PWR_BRKR_OK " + to_string(enumVal);
    case J3_W11_3_MTR_PWR_BRKR_OK: return "J3_W11_3_MTR_PWR_BRKR_OK " + to_string(enumVal);
    case J1_W12_1_COMM_PWR_BRKR_OK: return "J1_W12_1_COMM_PWR_BRKR_OK " + to_string(enumVal);
    case SPARE_D16: return "SPARE_D16 " + to_string(enumVal);
    case SPARE_D17: return "SPARE_D17 " + to_string(enumVal);
    case SPARE_D18: return "SPARE_D18 " + to_string(enumVal);
    case SPARE_D19: return "SPARE_D19 " + to_string(enumVal);
    case SPARE_D20: return "SPARE_D20 " + to_string(enumVal);
    case SPARE_D21: return "SPARE_D21 " + to_string(enumVal);
    case SPARE_D22: return "SPARE_D22 " + to_string(enumVal);
    case SPARE_D23: return "SPARE_D23 " + to_string(enumVal);
    case J1_W12_2_COMM_PWR_BRKR_OK: return "J1_W12_2_COMM_PWR_BRKR_OK " + to_string(enumVal);
    case J2_W13_1_COMM_PWR_BRKR_OK: return "J2_W13_1_COMM_PWR_BRKR_OK " + to_string(enumVal);
    case J2_W13_2_COMM_PWR_BRKR_OK: return "J2_W13_2_COMM_PWR_BRKR_OK " + to_string(enumVal);
    case J3_W14_1_COMM_PWR_BRKR_OK: return "J3_W14_1_COMM_PWR_BRKR_OK " + to_string(enumVal);
    case J3_W14_2_COMM_PWR_BRKR_OK: return "J3_W14_2_COMM_PWR_BRKR_OK " + to_string(enumVal);
    case SPARE_D29: return "SPARE_D29 " + to_string(enumVal);
    case SPARE_D30: return "SPARE_D30 " + to_string(enumVal);
    case INTERLOCK_POWER_RELAY: return "INTERLOCK_POWER_RELAY " + to_string(enumVal);
    case ALWAYS_HIGH: return "ALWAYS_HIGH " + to_string(enumVal);
    case ALWAYS_LOW: return "ALWAYS_LOW " + to_string(enumVal);
    }
    return "unknown " + to_string(enumVal);
}


}  // namespace control
}  // namespace m2cellcpp
}  // namespace LSST
