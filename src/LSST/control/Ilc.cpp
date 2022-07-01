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

// Class header
#include "control/Ilc.h"

// System headers

// Project headers
#include "system/Config.h"
#include "util/Bug.h"
#include "util/Log.h"

using namespace std;

namespace LSST {
namespace m2cellcpp {
namespace control {

AllIlcs::AllIlcs(bool useMocks) {
    // FUTURE: Once C API is available, create instances capable of communicating
    //         with the FPGA.
    if (!useMocks) {
        throw util::Bug(ERR_LOC, "Only Mock instances available.");
    }

    // The first 72 ILC's are Axial
    // while 73-78 are Tangential.
    for (int j = 1; j <= 78; ++j) {
        string name;
        if (j <= 72) {
            name = "Axial_";
        } else {
            name = "Tangent_";
        }
        name += to_string(j);
        _ilcs.emplace_back(make_shared<Ilc>(name, j));
    }
}

Ilc::Ilc(string const& name, int idNum) : _name(name), _idNum(idNum) {}

Ilc::Ptr& AllIlcs::_getIlcPtr(unsigned int idNum) {
    if (idNum < 1 || idNum > _ilcs.size()) {
        LERROR("AllIlcs::_getIlcPtr ", idNum, " throwing out of range");
        throw std::out_of_range("_getIlcPtr invalid val " + to_string(idNum));
    }
    return _ilcs[idNum - 1];
}

bool Ilc::getBit(int bit, uint8_t byt) {
    uint8_t j = 1;
    j = j << bit;
    return (j & byt) != 0;
}

bool Ilc::getFault() { return getBit(0, _rawStatus); }

bool Ilc::getCWLimit() { return getBit(2, _rawStatus); }

bool Ilc::getCCWLimit() { return getBit(3, _rawStatus); }

void Ilc::setStatus(uint8_t val) { _rawStatus = val; }

uint16_t Ilc::getBroadcastCommCount() {
    /// bit4..7: Broadcast communication counter (0..15)
    uint8_t mask = 0xf0;
    uint8_t out = (mask & _rawStatus) >> 4;
    return out;
}

}  // namespace control
}  // namespace m2cellcpp
}  // namespace LSST
