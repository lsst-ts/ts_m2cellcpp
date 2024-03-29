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
#ifndef LSST_M2CELLCPP_SYSTEM_COMSERVER_H
#define LSST_M2CELLCPP_SYSTEM_COMSERVER_H

// System headers
#include <atomic>
#include <memory>
#include <map>
#include <mutex>

// Third party headers
#include <boost/asio.hpp>

// project headers
#include "system/ComConnection.h"

namespace LSST {
namespace m2cellcpp {
namespace system {

class ComConnection;

/// Class ComServer is used for communicating with TCP/IP clients.
/// This class listens on the port specified in Config. Upon
/// receiving a connection, it creates a ComConnection object
/// to handle the communications with the client. The server
/// tracks open connections in _connections.
///
/// unit test: test_com.cpp
class ComServer : public std::enable_shared_from_this<ComServer> {
public:
    using Ptr = std::shared_ptr<ComServer>;
    enum State { CREATED = 0, RUNNING, STOPPED };

    /// A factory method to prevent issues with enable_shared_from_this.
    /// @return A pointer to the created ComServer object.
    static Ptr create(IoContextPtr const& ioContext, int port);

    ComServer() = delete;
    ComServer(ComServer const&) = delete;
    ComServer& operator=(ComServer const&) = delete;
    virtual ~ComServer();

    /// Run the server, this is a blocking opperation.
    void run();

    /// Remove 'connId' from the set of ComConnections.
    void eraseConnection(uint64_t connId);

    /// Return the number of active ComConnections.
    int connectionCount() const;

    State getState() const { return _state; }

    /// Return a new ComConnection object.
    virtual ComConnection::Ptr newComConnection(IoContextPtr const& ioContext, uint64_t connId,
                                                std::shared_ptr<ComServer> const& server);

    /// Return Human readable string for State
    static std::string prettyState(State state);

    /// Return the value of `_doSendWelcomeMsgServ`, which causes the welcome
    /// message to be sent on new connections.
    bool getDoSendWelcomeMsgServ() { return _doSendWelcomeMsgServ; }

    /// Set the value of `_doSendWelcomeMsgServ`, where true means the welcome message will
    /// be sent, this should be called before the server thread is started.
    /// Setting `doSend` to false is only meant for unit tests.
    void setDoSendWelcomeMsgServ(bool doSend) { _doSendWelcomeMsgServ = doSend; }

    /// Async write the `msg` to all existing ComConnections for this ComServer.
    void asyncWriteToAllComConn(std::string const& msg);

    /// Cause the server to stop responding to client requests.
    /// This also shutsdown all connections using this server.
    void shutdown();

    /// Destroy all communications channels associated with this server, including
    /// `_ioContext`. This will essentially stop anything using `_ioContext`.
    void destroy();

protected:
    /// Protected constructor to force use of create().
    ComServer(IoContextPtr const& ioContext, int port);

private:
    /// Begin (asynchronously) accepting connection requests.
    void _beginAccept();

    /// Handle a connection request.
    void _handleAccept(ComConnection::Ptr const& connection, boost::system::error_code const& ec);

    IoContextPtr _ioContext;             ///< Pointer to the asio io_context
    std::atomic<State> _state{CREATED};  ///< Current state of the machine.
    int _port;
    boost::asio::ip::tcp::acceptor _acceptor;

    std::atomic<bool> _shutdown{false};  ///< set to true the first time shutdown is called.

    std::atomic<bool> _destroyCalled{false};  ///< set to true the first time destroy is called.

    /// A map of all current connections made by this server.
    std::map<uint64_t, std::weak_ptr<ComConnection>> _connections;
    mutable std::mutex _mapMtx;  ///< protects _connections

    /// The source for connections Ids for the _connectionsMap.
    /// It should take hundreds or thousands of years for it
    /// uint64_t to wrap around.
    uint64_t _connIdSeq{0};

    /// When true, new connections will start by sending `_sendWelcomeMsg()`.
    std::atomic<bool> _doSendWelcomeMsgServ{true};
};

}  // namespace system
}  // namespace m2cellcpp
}  // namespace LSST

#endif  // LSST_M2CELLCPP_SYSTEM_COMSERVER_H
