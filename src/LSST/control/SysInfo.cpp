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
#include "control/SysInfo.h"

// System headers

// Project headers
#include "util/Log.h"

using namespace std;

namespace LSST {
namespace m2cellcpp {
namespace control {

string SysInfo::dump() {
    stringstream os;
    os << "SysInfo"
            << " outputPort=" << outputPort.getAllSetBitEnums()
            << " inputPort=" << inputPort.getAllSetBitEnums()
            << " motorVoltage=" << motorVoltage
            << " motorCurrent=" << motorCurrent
            << " motorBreakerClosed=" << motorBreakerClosed
            << " commVoltage=" << commVoltage
            << " commCurrent=" << commCurrent
            << " commBreakerClosed=" << commBreakerClosed
            << " iterations=" << iterations;
    return os.str();
}

}  // namespace control
}  // namespace m2cellcpp
}  // namespace LSST
