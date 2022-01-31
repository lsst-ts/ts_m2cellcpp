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
#include "system/Config.h"
#include "system/Log.h"

// LSST headers

using namespace std;
using namespace std::placeholders;

namespace LSST {
namespace m2cellcpp {
namespace system {

ComServer::Ptr ComServer::create(IoServicePtr const& ioService, int port) {
    return ComServer::Ptr(new ComServer(ioService, port));
}


ComServer::ComServer(IoServicePtr const& ioService, int port)
    :   _ioService(ioService), _port(port),
        _acceptor(*_ioService, boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), _port)) {
    // Set the socket reuse option to allow recycling ports after catastrophic
    // failures.
    _acceptor.set_option(boost::asio::socket_base::reuse_address(true));
}


void ComServer::run() {
    Log::log(Log::DEBUG, "ComServer::run()");
    // Begin accepting immediately. Otherwise it will finish when it 
    // discovers that there are outstanding operations.
    _beginAccept();

    // Launch all threads in the pool
    int threadCount = stoi(Config::get().getValue("server", "threads"));
    vector<shared_ptr<thread>> threads(threadCount);
    for (auto&& ptr: threads) {
        ptr = shared_ptr<thread>(new thread([&]() {
            _ioService->run();
        }));
    }
    _state = RUNNING;
    Log::log(Log::DEBUG, "ComServer::run() RUNNING"); 

    // Wait for all threads in the pool to exit.
    for (auto&& ptr: threads) {
        ptr->join();
    }
    Log::log(Log::DEBUG, "ComServer::run() finished");
    _state = STOPPED;
}


void ComServer::_beginAccept() {
    ComConnection::Ptr const connection = ComConnection::create(_ioService);
    _acceptor.async_accept(connection->socket(),
        bind(&ComServer::_handleAccept, shared_from_this(), connection, _1)
    );
}


void ComServer::_handleAccept(ComConnection::Ptr const& connection, boost::system::error_code const& ec) {
    if (ec.value() == 0) {
        connection->beginProtocol();
    } else {
        Log::log(Log::ERROR, string(__func__) + "  ec:" + ec.message());
    }
    _beginAccept();
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

}}} // namespace LSST::m2cellcpp::system
