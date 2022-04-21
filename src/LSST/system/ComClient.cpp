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
    string outStr(buffers_begin(_readStream.data()), buffers_begin(_readStream.data()) + xfer - delimSz);
    _readStream.consume(xfer);
    return outStr;
}

}  // namespace system
}  // namespace m2cellcpp
}  // namespace LSST
