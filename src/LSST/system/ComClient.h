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

/// &&& doc
class JsonMsgMap {
public:
    typedef std::deque<nlohmann::json> JsonDeque;
    typedef std::map<std::string, JsonDeque> JdMap;

    JsonMsgMap() = default;
    JsonMsgMap(JsonMsgMap const&) = default;
    ~JsonMsgMap() = default;

    /// &&& doc
    void insert(std::string const& key, nlohmann::json const& js);

    /// &&& doc Return the JsonDeque for `key` using move.
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

    /// &&& doc
    std::tuple<nlohmann::json, nlohmann::json> cmdSendRecv(std::string const& jStr, uint seqId, std::string const& note);

    /// &&& doc
    nlohmann::json cmdRecvId(std::string const& targetId, std::string const& note);

    /// &&& doc
    nlohmann::json cmdRecvSeqId(uint seqId, std::string const& note);

    /// &&& doc
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

    /// &&& doc
    JsonMsgMap _jMsgMap;
};

}  // namespace system
}  // namespace m2cellcpp
}  // namespace LSST

#endif  // LSST_M2CELLCPP_SYSTEM_COMCLIENT_H
