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

// Third party headers
#include "boost/asio.hpp"

// project headers
#include "system/ComConnection.h"

namespace LSST {
namespace m2cellcpp {
namespace system {

/// Class ComServer is used communicating with TCP/IP clients.
class ComServer : public std::enable_shared_from_this<ComServer>  {
public:
    typedef std::shared_ptr<ComServer> Ptr;
    enum State {
        CREATED = 0,
        RUNNING,
        STOPPED
    };

    /// A factory method to prevent issues with enable_shared_from_this.
    /// @return A pointer to the created ComServer object.
    static Ptr create(IoServicePtr const& ioService, int port);

    ComServer() = delete;
    ComServer(ComServer const&) = delete;
    ComServer& operator=(ComServer const&) = delete;
    ~ComServer() = default;

    /// Run the server, this is a blocking opperation.
    void run();

    State getState() const { return _state; }

    /// @return Human readable string for State
    static std::string prettyState(State state);

private:
    /// Private constructor to force use of create().
    ComServer(IoServicePtr const& ioService, int port);

    /// Begin (asynchronously) accepting connection requests.
    void _beginAccept();
    
    /// Handle a connection request.
    void _handleAccept(ComConnection::Ptr const& connection, boost::system::error_code const& ec);

    std::atomic<State> _state{CREATED};
    IoServicePtr _ioService;
    int _port;
    boost::asio::ip::tcp::acceptor _acceptor;
};

}}} // namespace LSST::m2cellcpp::system

#endif // LSST_M2CELLCPP_SYSTEM_COMSERVER_H
