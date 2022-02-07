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
#include <memory>
#include <string>

// Project headers
#include "system/ComConnection.h"

// Third party headers
#include <boost/asio.hpp>

namespace LSST {
namespace m2cellcpp {
namespace system {

/// A class used for testing ComServer by making a connection to the server
/// and running commands.
class ComClient {
public:
    typedef std::shared_ptr<ComClient> Ptr;

    ComClient(IoContextPtr const& ioContext, std::string const& servIp, int port);
    ComClient() = delete;
    ComClient(ComClient const&) = delete;
    ComClient& operator=(ComClient const&) = delete;

    ~ComClient() = default;

    /// Send a command to the server.
    /// @throw boost::system::system_error on error.
    void writeCommand(std::string const& cmd);

    /// Read the result of a command to the server.
    /// @return the text response
    /// @throw boost::system::system_error on other errors.
    std::string readCommand();

private:
    IoContextPtr _ioContext;  ///< Maintains the io_context that holds _socket.
    boost::asio::ip::tcp::socket _socket;

    void _setup(std::string const& servIp, int port);
};

}  // namespace system
}  // namespace m2cellcpp
}  // namespace LSST

#endif  // LSST_M2CELLCPP_SYSTEM_COMCLIENT_H
