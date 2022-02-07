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
#ifndef LSST_M2CELLCPP_SYSTEM_COMCONNECTION_H
#define LSST_M2CELLCPP_SYSTEM_COMCONNECTION_H

// System headers
#include <cstdint>
#include <cstdio>
#include <memory>
#include <vector>

// Third party headers
#include <boost/asio.hpp>

// Project headers

// This header declarations
namespace LSST {
namespace m2cellcpp {
namespace system {

typedef std::shared_ptr<boost::asio::io_context> IoContextPtr;

/// This class is used to handle commands and responses over a connection
/// until that connection is terminated.
class ComConnection : public std::enable_shared_from_this<ComConnection> {
public:
    typedef std::shared_ptr<ComConnection> Ptr;

    /// Factory method used to prevent issues with enable_shared_from_this.
    /// @param ioContext asio object for the network I/O operations
    static Ptr create(IoContextPtr const& ioContext);

    ComConnection() = delete;
    ComConnection(ComConnection const&) = delete;
    ComConnection& operator=(ComConnection const&) = delete;

    ~ComConnection() = default;

    /// @return network socket associated with the connection.
    boost::asio::ip::tcp::socket& socket() { return _socket; }

    /// Answer incoming communication.
    void beginProtocol();

    // Delimiter for all messages
    static std::string getDelimiter() { return "\r\n"; }

private:
    /// @see ComConnection::create()
    ComConnection(IoContextPtr const& ioContext);

    /// &&& maybe change to receiveCommand
    void _receiveRequest();

    /// Read the command sent.
    /// @param ec An error code to be evaluated.
    /// @param xfer The number of bytes received from a client.
    void _readCommand(boost::system::error_code const& ec, size_t xfer);

    /// The callback on finishing (either successfully or not) of asynchronous
    /// reads. The request will be parsed, analyzed and if everything is right
    /// the file transfer will begin.
    /// @param ec An error code to be evaluated.
    /// @param xfer The number of bytes received from a client.
    void _requestReceived(boost::system::error_code const& ec, size_t xfer);

    /// Begin sending a result back to a client
    void _sendResponse(std::string const& command);

    /// The callback on finishing (either successfully or not) of asynchronous writes.
    /// @param ec An error code to be evaluated.
    /// @param xfer The number of bytes sent to a client in a response.
    void _responseSent(boost::system::error_code const& ec, size_t xfer);

    /// A socket for communication with clients
    boost::asio::ip::tcp::socket _socket;
    IoContextPtr _ioContext;

    boost::asio::streambuf _streamBuf;
    std::string _buffer;
};

}  // namespace system
}  // namespace m2cellcpp
}  // namespace LSST

#endif  // LSST_M2CELLCPP_SYSTEM_COMCONNECTION_H
