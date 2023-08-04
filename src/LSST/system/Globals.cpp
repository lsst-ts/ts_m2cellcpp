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
#include "system/Globals.h"

// System headers

// Project headers
#include "system/Config.h"
#include "util/Log.h"

using namespace std;

namespace LSST {
namespace m2cellcpp {
namespace system {

// static Globals members
unique_ptr<Globals> Globals::_thisPtr;
mutex Globals::_thisMtx;


void Globals::setup(Config const& config) {
    lock_guard<mutex> lock(_thisMtx);
    if (_thisPtr) {
        LERROR("Globals already setup");
        return;
    }
    _thisPtr = std::unique_ptr<Globals>(new Globals(config));
}


Globals::Globals(Config const& config) {

}

void Globals::reset() {
    LCRITICAL("Config reseting Globals!!!");
    lock_guard<mutex> lock(_thisMtx);
    _thisPtr.reset();
}


Globals& Globals::get() {
    if (_thisPtr == nullptr) {
        throw ConfigException(ERR_LOC, "Globals has not been setup.");
    }
    return *_thisPtr;
}


/// Increase the the number of connections when `connecting` is true, decrease when false;
void Globals::setTcpIpConnected(bool connecting) {
    lock_guard<mutex> lock(_tcpIpConnectedMtx);
    if (connecting) {
        ++_tcpIpConnectedCount;
    } else {
        --_tcpIpConnectedCount;
    }
}

/// Return true if the number of connections is greater than zero.
bool Globals::getTcpIpConnected() const {
    lock_guard<mutex> lock(_tcpIpConnectedMtx);
    return (_tcpIpConnectedCount > 0);
}




}  // namespace system
}  // namespace m2cellcpp
}  // namespace LSST
