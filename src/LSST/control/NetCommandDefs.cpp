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
#include "control/NetCommandDefs.h"

// System headers

// Third party headers

// Project headers
#include "control/Context.h"
#include "control/PowerSystem.h"
#include "state/Model.h"
#include "system/ComControlServer.h"
#include "system/Globals.h"
#include "util/Log.h"

using namespace std;
using json = nlohmann::json;

namespace LSST {
namespace m2cellcpp {
namespace control {

NCmdSwitchCommandSource::Ptr NCmdSwitchCommandSource::create(JsonPtr const& inJson_) {
    auto cmd = Ptr(new NCmdSwitchCommandSource(inJson_));
    return cmd;
}

NCmdSwitchCommandSource::NCmdSwitchCommandSource(JsonPtr const& inJson) : NetCommand(inJson) {
    try {
        _isRemote = inJson->at("isRemote");
        LDEBUG(__func__, " ", getCommandName(), " seqId=", getSeqId(), " isRemote=", _isRemote);
    } catch (json::exception const& ex) {
        throwNetCommandException(ERR_LOC, __func__, inJson, ex);
    }

    ackJson["id"] = "ack";
    ackJson["user_info"] = getCommandName() + " " + to_string(_isRemote);
}

NetCommand::Ptr NCmdSwitchCommandSource::createNewNetCommand(JsonPtr const& inJson) {
    return NCmdSwitchCommandSource::create(inJson);
}

bool NCmdSwitchCommandSource::action() {
    bool result = system::Globals::get().setCommandSourceIsRemote(_isRemote);
    // This sets the CommandableByDds global, which needs to be broadcast.
    // This could be a duplicate broadcast, which is expected to be harmless.
    string msg = to_string(system::Globals::get().getCommandableByDdsJson());
    auto comServ = system::ComControlServer::get().lock();
    if (comServ != nullptr) {
        comServ->asyncWriteToAllComConn(msg);
    }
    return result;
}

NCmdPower::Ptr NCmdPower::create(JsonPtr const& inJson_) {
    auto cmd = Ptr(new NCmdPower(inJson_));
    return cmd;
}

NCmdPower::NCmdPower(JsonPtr const& inJson) : NetCommand(inJson) {
    LTRACE("NCmdPower::NCmdPower ", to_string(*inJson));
    try {
        int powerVal = inJson->at("powerType");
        _powerType = intToPowerSystemType(powerVal);
        _status = inJson->at("status");
    } catch (json::exception const& ex) {
        throwNetCommandException(ERR_LOC, __func__, inJson, ex);
    }
    if (_powerType == UNKNOWNPOWERSYSTEM) {
        throw NetCommandException(ERR_LOC, "unknown powerType in " + inJson->dump());
    }
    LDEBUG(__func__, " ", getCommandName(), " seqId=", getSeqId(),
           " powerType=", getPowerSystemTypeStr(_powerType), " status=", _status);
    ackJson["id"] = "ack";
    ackJson["user_info"] = getCommandName() + " " + getPowerSystemTypeStr(_powerType) + to_string(_status);
}

NetCommand::Ptr NCmdPower::createNewNetCommand(JsonPtr const& inJson) { return NCmdPower::create(inJson); }

bool NCmdPower::action() {
    auto context = Context::get();
    bool result = context->model.getCurrentState()->cmdPower(_powerType, _status);

    // Message that looks something like this needs to be broadcast
    // {'powerType': 2, 'status': True, 'id': 'cmd_power', 'sequence_id': 123}
    // {'id': 'powerSystemState', 'powerType': 2, 'status': true, 'state': 3 }
    // {'id': 'powerSystemState', 'powerType': 2, 'status': true, 'state': 5}
    // This is a duplicate broadcast and can probably be removed, but it may
    // be useful as without this motor state is only broadcast on change.
    json js = context->model.getPowerSystem()->getPowerSystemStateJson(_powerType);
    string msg = js.dump();
    auto comServ = system::ComControlServer::get().lock();
    if (comServ != nullptr) {
        comServ->asyncWriteToAllComConn(msg);
    }
    return result;
}

NCmdSystemShutdown::Ptr NCmdSystemShutdown::create(JsonPtr const& inJson_) {
    return Ptr(new NCmdSystemShutdown(inJson_));
}

NCmdSystemShutdown::NCmdSystemShutdown(JsonPtr const& inJson) : NetCommand(inJson) {
    ackJson["id"] = "ack";
    ackJson["user_info"] = getCommandName();
}

NetCommand::Ptr NCmdSystemShutdown::createNewNetCommand(JsonPtr const& inJson) {
    return NCmdSystemShutdown::create(inJson);
}

bool NCmdSystemShutdown::action() {
    LINFO("NCmdSystemShutdown");
    thread thrd([] {
        sleep(1);
        LINFO("NCmdSystemShutdown shutting down");
        auto context = Context::get();
        context->model.systemShutdown();
    });
    thrd.detach();
    return true;
}

}  // namespace control
}  // namespace m2cellcpp
}  // namespace LSST
