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
#include "control/NetCommandFactory.h"

// System headers

// Third party headers

// Project headers
#include "system/Log.h"

using namespace std;
using json = nlohmann::json;
using Log = LSST::m2cellcpp::system::Log;

namespace LSST {
namespace m2cellcpp {
namespace control {

NetCommandFactory::Ptr NetCommandFactory::create() {
    NetCommandFactory::Ptr factory = NetCommandFactory::Ptr(new NetCommandFactory());
    return factory;
}

void NetCommandFactory::addNetCommand(NetCommand::Ptr const& cmd) {
    lock_guard<mutex> lg(_mtx);
    string cmdName = cmd->getCommandName();
    auto result = _cmdMap.emplace(cmdName, cmd);
    if (result.second == false) {
        string eMsg = string("addNetCommand failed as this command was already in the map ") + cmdName;
        Log::log(Log::ERROR, eMsg);
        throw NetCommandException(eMsg);
    }
}

NetCommand::Ptr NetCommandFactory::getCommandFor(std::string const& jsonStr) {
    auto inJson = NetCommand::parse(jsonStr);
    lock_guard<mutex> lg(_mtx);
    // If parse didn't throw, there must be a valid id and seq_id.
    string cmdId = inJson->at("id");
    uint64_t seqId = inJson->at("seq_id");
    NetCommand::Ptr cmdOut;
    // Check if seqId is valid (must be larger than the previous one)
    if (seqId <= _prevSeqId) {
        string badSeqId = string("Bad seq_id ") + to_string(seqId) + " " + cmdId + " previous seq_id was " +
                          to_string(_prevSeqId);
        Log::log(Log::WARN, string("getCommandFor seq_id ") + to_string(seqId) + " " + cmdId + badSeqId +
                                    " returning " + _defaultNoAck->getCommandName());
        cmdOut = _defaultNoAck->createNewNetCommand(inJson);
        cmdOut->setAckUserInfo(badSeqId);
        return cmdOut;
    }
    _prevSeqId = seqId;
    auto iter = _cmdMap.find(cmdId);
    if (iter == _cmdMap.end()) {
        Log::log(Log::WARN, string("getCommandFor ") + cmdId + " not found. Returning defaultNoAck" +
                                    _defaultNoAck->getCommandName());
        cmdOut = _defaultNoAck->createNewNetCommand(inJson);
        cmdOut->setAckUserInfo(string("Original command not found " + cmdId));
        return cmdOut;
    }
    auto& cmdFactory = iter->second;
    // If there are errors in 'inJson', this should throw NetCommandException.
    try {
        cmdOut = cmdFactory->createNewNetCommand(inJson);
    } catch (NetCommandException const& ex) {
        Log::log(Log::WARN, string("getCommandFor invalid json ") + ex.what() + " Returning defaultNoAck" +
                                    _defaultNoAck->getCommandName());
        cmdOut = _defaultNoAck->createNewNetCommand(inJson);
        cmdOut->setAckUserInfo(string("Invalid json ") + ex.what());
        return cmdOut;
    }
    return cmdOut;
}

}  // namespace control
}  // namespace m2cellcpp
}  // namespace LSST