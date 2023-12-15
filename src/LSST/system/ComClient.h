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
#ifndef LSST_M2CELLCPP_SYSTEM_COMCLIENT_H
#define LSST_M2CELLCPP_SYSTEM_COMCLIENT_H

// System headers
#include <deque>
#include <memory>
#include <string>

// Project headers
#include "system/ComConnection.h"

// Third party headers
#include <boost/asio.hpp>
#include <nlohmann/json.hpp>

namespace LSST {
namespace m2cellcpp {
namespace system {

/// This class is used to store json messages from the server
/// that are not associated with the current command the `Client`
/// is running. The `Client` class is only meant for simple
/// testing, and generally only runs one command at a time in
/// a single thread. Messages not required for the current client
/// request are stored here so they can be examined later.
///
/// The key for _msgMap is the "id" from the json message.
/// The json messages for the same "id" are stored in a deque
/// with the oldest being at the front. It is assumed that
/// only the newest will be relevant in most (but not all)
/// cases.
///
/// Most actions involving getting from the map are destructive
/// with the expectation that old entries can confuse tests
/// and the map should be cleared before certain tests.
class JsonMsgMap {
public:
    typedef std::deque<nlohmann::json> JsonDeque;
    typedef std::map<std::string, JsonDeque> JdMap;

    JsonMsgMap() = default;
    JsonMsgMap(JsonMsgMap const&) = default;
    ~JsonMsgMap() = default;

    /// Insert `js` into `_msgMap` where `key` is the "id" from `js`.
    void insert(std::string const& key, nlohmann::json const& js);

    /// Return the JsonDeque for `key` using move.
    JsonDeque getDequeFor(std::string const& key);

    /// Returns the current `_msgMap` and replaces `_msgMap` with a new empty one.
    std::shared_ptr<JdMap> getMsgMap() {
        std::shared_ptr<JdMap> ret = _msgMap;
        _msgMap.reset(new JdMap());
        return ret;
    }

private:
    /// Map of messages read that were not part of a request
    /// made by the client.
    std::shared_ptr<JdMap> _msgMap{new JdMap()};
};

/// A class used for testing ComServer by making a connection to the server
/// and running commands.
///
/// unit test: test_com.cpp
class ComClient {
public:
    typedef std::shared_ptr<ComClient> Ptr;

    ComClient(IoContextPtr const& ioContext, std::string const& servIp, int port);
    ComClient() = delete;
    ComClient(ComClient const&) = delete;
    ComClient& operator=(ComClient const&) = delete;

    ~ComClient();

    /// Send a command to the server.
    /// @throw boost::system::system_error on error.
    void writeCommand(std::string const& cmd);

    /// Read the result of a command to the server.
    /// @return the text response
    /// @throw boost::system::system_error on other errors.
    std::string readCommand();

    /// Return the number of entries read in the welcome message.
    ///  A negative value indicates failure.
    int readWelcomeMsg();

    /// Send a command to the server and receive the "ack", "success" or "fail"
    /// messages.
    /// @param `jStr` a string containing the json formatted command to send to the server.
    /// @param `seqId` the numeric sequence ID associated with this command.
    /// @param `note` a string describing what is responsible for sending the command.
    /// @return ack - the resulting "ack" or "noack" message associated with `seqId`.
    /// @return fin - the resulting "success" or "fail" message associated with `seqId`.
    std::tuple<nlohmann::json, nlohmann::json> cmdSendRecv(std::string const& jStr, uint seqId,
                                                           std::string const& note);

    /// Receive commands from the server until "id" == `targetId` is found.
    /// This function will continue to read commands until `targetId` is
    /// read. Other messages received will be stored in `_jMsgMap`.
    /// @param targetId the "id" of the command that is needed.
    /// @param `note` a string describing what wants the message.
    /// @return the json message where "id" == `targetId`.
    nlohmann::json cmdRecvId(std::string const& targetId, std::string const& note);

    /// Receive commands from the server until "id" == `targetId` is found.
    /// This function will continue to read commands until `targetId` is
    /// read. Other messages received will be stored in `_jMsgMap`.
    /// @param seqId the "sequence_id" of the command that is needed.
    /// @param `note` a string describing what wants the message.
    /// @return the json message where "sequence_id" == `seqId`.
    nlohmann::json cmdRecvSeqId(uint seqId, std::string const& note);

    /// Return the entire deque associated with `key` from `_jMsgMap`,
    /// using a destructive move.
    /// The method first checks `_jMsgMap` for the `key`. If it isn't found,
    /// it then reads commands until it sees "id" == `key` and then returns
    /// what it received inserted into the deque.
    /// @param key the "id" of the message/deque that is needed.
    /// @param `note` a string describing what wants the message/deque.
    /// @return the deque where "id" == `key`.
    JsonMsgMap::JsonDeque recvDequeForId(std::string const& key, std::string const& note);

    /// Close the connection.
    void close();

private:
    IoContextPtr _ioContext;  ///< Maintains the io_context that holds _socket.
    boost::asio::ip::tcp::socket _socket;

    void _setup(std::string const& servIp, int port);

    /// Buffer for asio to store incoming TCP data.
    /// asio may read more into the buffer than needed, so
    /// the same buffer needs to be used repeatedly.
    boost::asio::streambuf _readStream;
    std::mutex _readStreamMtx;  ///< Protects `_readStream`

    /// A map where the key is the "id" of the json message and the value is
    /// a deque of json messages received with that "id".
    JsonMsgMap _jMsgMap;
};

}  // namespace system
}  // namespace m2cellcpp
}  // namespace LSST

#endif  // LSST_M2CELLCPP_SYSTEM_COMCLIENT_H
