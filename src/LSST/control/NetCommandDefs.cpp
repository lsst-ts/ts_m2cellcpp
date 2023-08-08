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
        LDEBUG("NCmdSwitchCommandSource seqId=", getSeqId(), " isRemote=", _isRemote);
    } catch (json::exception const& ex) {
        string eMsg = string("NCmdEcho constructor error in ") + inJson->dump() + " what=" + ex.what();
        LERROR(eMsg);
        throw NetCommandException(ERR_LOC, eMsg);
    }

    ackJson["id"] = "ack";
    ackJson["user_info"] = "switchCommandSource " + to_string(_isRemote);
}

NetCommand::Ptr NCmdSwitchCommandSource::createNewNetCommand(JsonPtr const& inJson) {
    NCmdSwitchCommandSource::Ptr cmd = NCmdSwitchCommandSource::create(inJson);
    return cmd;
}

bool NCmdSwitchCommandSource::action() {
    return system::Globals::get().setCommandSourceIsRemote(_isRemote);
}

}  // namespace control
}  // namespace m2cellcpp
}  // namespace LSST
