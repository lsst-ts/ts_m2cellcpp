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
#include "system/ComServer.h"

// System headers
#include <functional>
#include <thread>

// Project headers
#include "faultmgr/FaultMgr.h"
#include "system/ComClient.h"
#include "system/Config.h"
#include "util/Log.h"

// LSST headers

using namespace std;
using namespace std::placeholders;

namespace LSST {
namespace m2cellcpp {
namespace system {

ComServer::Ptr ComServer::create(IoContextPtr const& ioContext, int port) {
    return ComServer::Ptr(new ComServer(ioContext, port));
}

ComServer::ComServer(IoContextPtr const& ioContext, int port)
        : _ioContext(ioContext),
          _port(port),
          _acceptor(*_ioContext, boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), _port)) {
    // Set the socket reuse option to allow recycling ports after catastrophic
    // failures.
    _acceptor.set_option(boost::asio::socket_base::reuse_address(true));
}

ComServer::~ComServer() {
    LDEBUG("ComServer::~ComServer()");
    if (!_shutdown) {
        LERROR("ComServer::~ComServer() shutdown wasn't called, calling now");
        shutdown();
    }
    destroy();
}

void ComServer::run() {
    LDEBUG("ComServer::run()");
    // Begin accepting immediately. Otherwise it will finish when it
    // discovers that there are outstanding operations.
    _beginAccept();

    // Launch all threads in the pool
    int threadCount = Config::get().getControlServerThreads();
    vector<shared_ptr<thread>> threads(threadCount);
    for (auto&& ptr : threads) {
        ptr = shared_ptr<thread>(new thread([&]() { _ioContext->run(); }));
    }
    _state = RUNNING;
    LDEBUG("ComServer::run() RUNNING threads=", threadCount);

    // Wait for all threads in the pool to exit.
    for (auto&& ptr : threads) {
        ptr->join();
    }
    LDEBUG("ComServer::run() finished");
    _state = STOPPED;
}

void ComServer::_beginAccept() {
    if (_state == STOPPED || _shutdown) {
        return;
    }
    auto connId = _connIdSeq++;
    ComConnection::Ptr const connection = newComConnection(_ioContext, connId, shared_from_this());
    _acceptor.async_accept(connection->socket(),
                           bind(&ComServer::_handleAccept, shared_from_this(), connection, _1));
}

void ComServer::_handleAccept(ComConnection::Ptr const& connection, boost::system::error_code const& ec) {
    LINFO("ComServer::_handleAccept");
    if (_state == STOPPED || _shutdown) {
        return;
    }

    size_t connectionSize;
    {
        lock_guard<mutex> lg(_mapMtx);
        _connections.emplace(connection->getConnId(), connection);
        connectionSize = _connections.size();
    }
    // If going from 0 to 1, FaultMgr must clear the appropriate fault so the system can be turned on.
    faultmgr::FaultMgr::get().reportComConnectionCount(connectionSize);

    if (ec.value() == 0) {
        connection->beginProtocol();
    } else {
        LERROR("ComServer::_handleAccept ec:", ec.message());
    }
    _beginAccept();
}

void ComServer::shutdown() {
    LINFO("ComServer::shutdown");
    if (_shutdown.exchange(true) == true) {
        return;
    }
    vector<weak_ptr<ComConnection>> vect;
    {
        lock_guard<mutex> lg(_mapMtx);
        for (auto&& elem : _connections) {
            vect.push_back(elem.second);
        }
    }

    for (auto&& item : vect) {
        std::shared_ptr<ComConnection> conn = item.lock();
        if (conn != nullptr) {
            conn->shutdown();
        }
    }
}

void ComServer::destroy() {
    LINFO("ComServer::destroy");
    if (_destroyCalled.exchange(true) == true) {
        return;
    }

    // Start a client connection to this server to get the
    // accept loop to move. Catch and ignore any problems
    // as the `cmd` is expected to fail.
    try {
        // The server must be on this host.
        ComClient client(_ioContext, "127.0.0.1", _port);
        string cmd("ComServer destructor shutting down");
        client.writeCommand(cmd);
        LDEBUG("wrote cmd=", cmd);
    } catch (std::exception const& ex) {
        LWARN("Ignoring shutdown issue ", ex.what());
    }

    _ioContext->stop();
}

void ComServer::eraseConnection(uint64_t connId) {
    size_t connectionSize;
    {
        lock_guard<mutex> lg(_mapMtx);
        auto iter = _connections.find(connId);
        if (iter == _connections.end()) {
            LWARN("connection not found ", to_string(connId));
            return;
        }
        _connections.erase(iter);
        connectionSize = _connections.size();
    }
    // If going to 0, FaultMgr must set the appropriate fault to force safe mode.
    faultmgr::FaultMgr::get().reportComConnectionCount(connectionSize);
}

int ComServer::connectionCount() const {
    lock_guard<mutex> lg(_mapMtx);
    return _connections.size();
}

string ComServer::prettyState(State state) {
    switch (state) {
        case CREATED:
            return "CREATED";
        case RUNNING:
            return "RUNNING";
        case STOPPED:
            return "STOPPED";
    }
    return "unknown";
}

ComConnection::Ptr ComServer::newComConnection(IoContextPtr const& ioContext, uint64_t connId,
                                               std::shared_ptr<ComServer> const& server) {
    ComConnection::Ptr ptr = ComConnection::create(ioContext, connId, server);
    ptr->setDoSendWelcomeMsg(_doSendWelcomeMsgServ);
    return ptr;
}

void ComServer::asyncWriteToAllComConn(std::string const& msg) {
    lock_guard<mutex> lg(_mapMtx);
    for (auto&& elem : _connections) {
        auto conn = elem.second.lock();
        if (conn != nullptr) {
            conn->asyncWrite(msg);
        }
    }
}

}  // namespace system
}  // namespace m2cellcpp
}  // namespace LSST
