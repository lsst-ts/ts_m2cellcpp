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
#include "control/FpgaBooleans.h"

// System headers
#include <stdexcept>
#include <string>

using namespace std;

namespace LSST {
namespace m2cellcpp {
namespace control {

bool FpgaBooleans::getBit(byte byt, int pos) {
    if (pos < BIT_MIN || pos > BIT_MAX) {
        // throw range_error("getBit out of range pos=" + to_string(pos));
        throw range_error("getBit out of range pos=");
    }
    byte bit{1};
    bit <<= pos;
    return (byt & bit) != byte{0};
}

void FpgaBooleans::setBit(byte& byt, int pos, bool val) {
    if (pos < BIT_MIN || pos > BIT_MAX) {
        throw range_error("setBit out of range pos=" + to_string(pos));
    }
    byte bit{1};
    bit <<= pos;
    if (val) {
        byt |= bit;
    } else {
        byt &= ~bit;
    }
}

}  // namespace control
}  // namespace m2cellcpp
}  // namespace LSST
