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
#include "control/Context.h"
#include "system/Config.h"
#include "util/Log.h"

using namespace std;
using json = nlohmann::json;

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

Globals::Globals(Config const& config) {}

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
    auto originalCount = _tcpIpConnectedCount;
    if (connecting) {
        ++_tcpIpConnectedCount;
    } else {
        --_tcpIpConnectedCount;
    }
    if (_tcpIpConnectedCount <= 0) {
        // FUTURE: change state to SAFE MODE - power off ILC communication and actuator motor
        LWARN("No TCP/IP connections, going to OFFLINESTATE");
        auto& model = control::Context::get()->model;
        model.changeState(model.getState(state::State::StateEnum::OFFLINESTATE));
    } else if (originalCount <= 0 && _tcpIpConnectedCount > 0) {
        LWARN("Went from 0 to at least 1 TCP/IP connections, going to STANDBYSTATE");
        auto& model = control::Context::get()->model;
        model.changeState(model.getState(state::State::StateEnum::STANDBYSTATE));
    }
}

/// Return true if the number of connections is greater than zero.
bool Globals::getTcpIpConnected() const {
    lock_guard<mutex> lock(_tcpIpConnectedMtx);
    return (_tcpIpConnectedCount > 0);
}

json Globals::getCommandableByDdsJson() const {
    json js;
    js["id"] = "commandableByDDS";
    bool state = _commandableByDds;  // json cannot convert atomic<bool> to bool directly.
    js["state"] = state;
    return js;
}

}  // namespace system
}  // namespace m2cellcpp
}  // namespace LSST
