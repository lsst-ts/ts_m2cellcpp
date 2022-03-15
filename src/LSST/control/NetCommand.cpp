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
#include "control/NetCommand.h"

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

NetCommand::NetCommand(JsonPtr const& inJson_)
        : inJson(inJson_) {
    if (inJson == nullptr) {
        throw NetCommandException("NetCommand constructor inJson=null");
    }
    try {
        _name = inJson->at("id");
        _seqId = inJson->at("seq_id");
        Log::log(Log::DEBUG, string("NetCommand constructor id=") + _name + " seqId=" + to_string(_seqId));
    } catch (json::exception const& ex) {
        string eMsg = string("NetCommand constructor error in ") + inJson->dump() + " what=" + ex.what();
        Log::log(Log::ERROR, eMsg);
        throw NetCommandException(eMsg);
    }
    
    ackJson["seq_id"] = _seqId;
    ackJson["user_info"] = string("invalid:") + _name;
    respJson["seq_id"] = _seqId;
}

NetCommand::JsonPtr NetCommand::parse(string const& inStr) {
    JsonPtr inJson = std::shared_ptr<nlohmann::json>(new nlohmann::json());
    // Not much can be done if parsing fails.
    try {
        *inJson = json::parse(inStr);
    } catch (json::parse_error& ex) {
        string eMsg = string("NetCommand::parse error ") + ex.what() + " " + inStr;
        Log::log(Log::ERROR, eMsg);
        throw NetCommandException(eMsg);
    }
    Log::log(Log::DEBUG, string("NetCommand::parse inStr=") + inStr + "\njson=" + inJson->dump(2));
    // All commands must have at least id and seq_id
    try {
        string id = inJson->at("id");
        uint64_t seqId = inJson->at("seq_id");
        Log::log(Log::DEBUG, string("NetCommand::parse id=") + id + " seq=" + to_string(seqId));
    } catch (json::exception const& ex) {
        string eMsg = string("NetCommand::parse error missing id or seq_id in ") + inJson->dump() +
                      " what=" + ex.what();
        Log::log(Log::ERROR, eMsg);
        throw NetCommandException(eMsg);
    }
    return inJson;
}

bool NetCommand::run() {
    Log::log(Log::DEBUG, string("NetCommand run action for seqId=") + to_string(_seqId) + " " + getName());
    bool result = action();
    if (result) {
        respJson["id"] = "success";
    } else {
        respJson["id"] = "fail";
    }
    return result;
}


string NetCommand::getAckJsonStr() {
    return ackJson.dump();
}


string NetCommand::getRespJsonStr() {
    return respJson.dump();
}


NCmdAck::Ptr NCmdAck::create(JsonPtr const& inJson_) {
    auto cmd = Ptr(new NCmdAck(inJson_));
    return cmd;
}

NCmdAck::NCmdAck(JsonPtr const& inJson) : NetCommand(inJson) {
    ackJson["id"] = "ack";
    ackJson["user_info"] = "ack";
}


NetCommand::Ptr NCmdAck::createNewNetCommand(JsonPtr const& inJson) {
    NCmdAck::Ptr cmd = NCmdAck::create(inJson);
    return cmd;
}


NCmdNoAck::Ptr NCmdNoAck::create(JsonPtr const& inJson_) {
    auto cmd = Ptr(new NCmdNoAck(inJson_));
    return cmd;
}

NCmdNoAck::NCmdNoAck(JsonPtr const& inJson) : NetCommand(inJson) {
    ackJson["id"] = "noack";
    ackJson["user_info"] = "noack";
}


NetCommand::Ptr NCmdNoAck::createNewNetCommand(JsonPtr const& inJson) {
    NCmdNoAck::Ptr cmd = NCmdNoAck::create(inJson);
    return cmd;
}


NCmdEcho::Ptr NCmdEcho::create(JsonPtr const& inJson_) {
    auto cmd = Ptr(new NCmdEcho(inJson_));
    return cmd;
}

NCmdEcho::NCmdEcho(JsonPtr const& inJson) : NetCommand(inJson) {
    try {
        _msg = inJson->at("msg");
        Log::log(Log::DEBUG, string("NCmdEcho seqId=") + to_string(getSeqId()) + " msg=" + _msg);
    } catch (json::exception const& ex) {
        string eMsg = string("NCmdEcho constructor error in ") + inJson->dump() + " what=" + ex.what();
        Log::log(Log::ERROR, eMsg);
        throw NetCommandException(eMsg);
    }
    
    ackJson["id"] = "ack";
    ackJson["user_info"] = "echo";
}


NetCommand::Ptr NCmdEcho::createNewNetCommand(JsonPtr const& inJson) {
    NCmdEcho::Ptr cmd = NCmdEcho::create(inJson);
    return cmd;
}


bool NCmdEcho::action() {
    respJson["msg"] = _msg;
    return true;
}


}  // namespace control
}  // namespace m2cellcpp
}  // namespace LSST
