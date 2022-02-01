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
#include "system/Log.h"

using namespace std;
namespace io = boost::asio;

namespace LSST {
namespace m2cellcpp {
namespace system {

ComClient::ComClient(IoContextPtr const& ioContext, string const& servIp, int port)
        : _ioContext(ioContext), _socket(*_ioContext) {
    _setup(servIp, port);
}

void ComClient::_setup(string const& servIp, int port) {
    Log::log(Log::DEBUG, string("ComClient setup " + servIp + " " + to_string(port)));
    io::ip::tcp::resolver resolv(*_ioContext);
    boost::system::error_code ec;
    string strPort = to_string(port);
    io::ip::tcp::resolver::results_type endpoints = resolv.resolve(servIp, strPort, ec);
    for (io::ip::tcp::endpoint const& endpoint : endpoints) {
        stringstream os;
        os << endpoint;
        Log::log(Log::INFO, string("endpoint ") + os.str());
    }
    io::connect(_socket, endpoints);
}

void ComClient::writeCommand(string const& cmd) {
    boost::system::error_code ec;
    io::write(_socket, boost::asio::buffer(cmd + ComConnection::getDelimiter()), ec);
    if (ec) {
        _socket.close();
        Log::log(Log::ERROR, string("writeCommand error ec=") + ec.message());
        throw boost::system::system_error(ec);
    }
    Log::log(Log::DEBUG, "ComClient::writeCommand " + cmd);
}

string ComClient::readCommand() {
    io::streambuf streamB;
    boost::system::error_code ec;
    io::read_until(_socket, streamB, ComConnection::getDelimiter(), ec);
    if (ec) {
        _socket.close();
        Log::log(Log::ERROR, string("readCommand error ec=") + ec.message());
        throw boost::system::system_error(ec);
    }

    // TODO Put this streambuf to string conversion in a utility function.
    using boost::asio::buffers_begin;
    auto buf = streamB.data();
    size_t msgSz = streamB.size() - ComConnection::getDelimiter().size();
    string cmd(buffers_begin(buf), buffers_begin(buf) + msgSz);
    Log::log(Log::DEBUG, "ComClient::readCommand " + cmd);
    return cmd;
}

}  // namespace system
}  // namespace m2cellcpp
}  // namespace LSST
