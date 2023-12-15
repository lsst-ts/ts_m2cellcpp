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
#include "system/ComClient.h"

// Third party headers
#include "nlohmann/json.hpp"

// Project headers
#include "util/Log.h"

using namespace std;
namespace io = boost::asio;

namespace LSST {
namespace m2cellcpp {
namespace system {

ComClient::ComClient(IoContextPtr const& ioContext, string const& servIp, int port)
        : _ioContext(ioContext), _socket(*_ioContext) {
    _setup(servIp, port);
}

ComClient::~ComClient() {
    LDEBUG("ComClient::~ComClient");
    boost::system::error_code ec;
    _socket.shutdown(boost::asio::ip::tcp::socket::shutdown_send, ec);
    if (ec) {
        LERROR("~ComClient shutdown ec:", ec.message());
    }
    _socket.close();
}

void ComClient::_setup(string const& servIp, int port) {
    LDEBUG("ComClient setup ", servIp, " ", port);
    io::ip::tcp::resolver resolv(*_ioContext);
    boost::system::error_code ec;
    string strPort = to_string(port);
    io::ip::tcp::resolver::results_type endpoints = resolv.resolve(servIp, strPort, ec);
    if (ec) {
        LERROR("ComClient::_setup ec:", ec.message());
        throw boost::system::system_error(ec);
    }
    for (io::ip::tcp::endpoint const& endpoint : endpoints) {
        stringstream os;
        os << endpoint;
        LINFO("endpoint ", os.str());
    }
    io::connect(_socket, endpoints);
}

void ComClient::writeCommand(string const& cmd) {
    boost::system::error_code ec;
    io::write(_socket, boost::asio::buffer(cmd + ComConnection::getDelimiter()), ec);
    if (ec) {
        _socket.close();
        LERROR("writeCommand error ec=", ec.message());
        throw boost::system::system_error(ec);
    }
    LDEBUG("ComClient::writeCommand ", cmd);
}

string ComClient::readCommand() {
    auto delimSz = ComConnection::getDelimiter().size();
    boost::system::error_code ec;
    lock_guard lkg(_readStreamMtx);
    auto xfer = io::read_until(_socket, _readStream, ComConnection::getDelimiter(), ec);
    if (ec) {
        _socket.close();
        LERROR("readCommand error ec=", ec.message());
        throw boost::system::system_error(ec);
    }
    using boost::asio::buffers_begin;
    // Copy the contents of _readStream up to, but not including the delimiter.
    string outStr(buffers_begin(_readStream.data()), buffers_begin(_readStream.data()) + xfer - delimSz);
    // Remove the entire message, including delimiter, from _readStream so the next message is clean.
    _readStream.consume(xfer);
    LTRACE("ComClient::readCommand() ", outStr);
    return outStr;
}

int ComClient::readWelcomeMsg() {
    string lastMsgId("summaryFaultsStatus");
    int count = 0;
    bool done = false;
    while (!done) {
        string inMsg = readCommand();
        try {
            nlohmann::json js = nlohmann::json::parse(inMsg);
            string id = js["id"];
            ++count;
            LDEBUG("readWelcomeMsg count=", count, " ", inMsg);
            if (id == lastMsgId) {
                done = true;
            }
        } catch (nlohmann::json::parse_error const& ex) {
            LERROR("json parse error msg=", ex.what());
            return -1;
        }
    }
    return count;
}

tuple<nlohmann::json, nlohmann::json> ComClient::cmdSendRecv(string const& jStr, uint seqId, string const& note) {
    writeCommand(jStr);
    LDEBUG(note, "cmdSendRecv:wrote jStr=", jStr);
    auto ackJ = cmdRecvSeqId(seqId, "cmdSendRecv");
    auto finJ = cmdRecvSeqId(seqId, "cmdSendRecv");
    return {ackJ, finJ};
}


nlohmann::json ComClient::cmdRecvSeqId(uint seqId, string const& note) {
    bool found = false;
    nlohmann::json js;
    while (!found) {
        LDEBUG(note, "cmdRecvId waiting for ", seqId);
        string inStr = readCommand();
        LDEBUG(note, "cmdRecvId:read ", seqId, " ", inStr);
        js = nlohmann::json::parse(inStr);
        auto iter = js.find("sequence_id");
        if (iter != js.end() && *iter == seqId) {
            found = true;
            LDEBUG(note, "cmdSendRecv:read ", seqId, "=", js);
        } else {
            LDEBUG(note, "cmdSendRecv:read not ", seqId, " storing ", js);
            _jMsgMap.insert(js["id"], js);
        }
    }
    return js;
}

nlohmann::json ComClient::cmdRecvId(string const& targetId, string const& note) {
    bool found = false;
    nlohmann::json js;
    while (!found) {
        LDEBUG(note, "cmdRecvId waiting for ", targetId);
        string inStr = readCommand();
        LDEBUG(note, "cmdRecvId:read ", targetId, " ", inStr);
        js = nlohmann::json::parse(inStr);
        auto& jsId = js["id"];
        if (jsId == targetId) {
            found = true;
            LDEBUG(note, "cmdSendRecv:read ", targetId, "=", js);
        } else {
            LDEBUG(note, "cmdSendRecv:read not ", targetId, " storing ", js);
            _jMsgMap.insert(jsId, js);
        }
    }
    return js;
}


JsonMsgMap::JsonDeque ComClient::recvDequeForId(string const& key, string const& note) {
    // Check if it is in the map
    JsonMsgMap::JsonDeque jDeque = _jMsgMap.getDequeFor(key);
    if (!jDeque.empty()) {
        LTRACE("ComClient::recvDequeForId found ", key, " in map. ", note);
        return jDeque;
    }
    auto js = cmdRecvId(key, note);
    jDeque.push_back(js);
    return jDeque;
}



void JsonMsgMap::insert(string const& key, nlohmann::json const& js) {
    LTRACE("JsonMsgMap::insert js", to_string(js));
    (*_msgMap)[key].push_back(js);

}

JsonMsgMap::JsonDeque JsonMsgMap::getDequeFor(std::string const& key) {
    JsonDeque jDeque;
    auto iter = _msgMap->find(key);
    if (iter == _msgMap->end()) {
        return jDeque;
    }
    JsonDeque& jd = iter->second;
    jDeque = move(jd);
    jd.clear();
    return jDeque;
}


}  // namespace system
}  // namespace m2cellcpp
}  // namespace LSST
