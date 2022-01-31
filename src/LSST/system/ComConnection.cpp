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
#include "system/ComConnection.h"

// System headers
#include <cerrno>
#include <cstring>
#include <ctime>
#include <functional>
#include <iostream>
#include <stdexcept>

// Third party headers

// Project headers
#include "system/Config.h"
#include "system/Log.h"


using namespace std;
using namespace std::placeholders;


namespace {

bool isErrorCode(boost::system::error_code const& ec, string const& note) {
    using Log = LSST::m2cellcpp::system::Log;
    if (ec.value() != 0) {
        if (ec == boost::asio::error::eof) {
            Log::log(Log::INFO,  note + "  ** closed **");
        } else {
            Log::log(Log::ERROR, note + "  ** failed: " + ec.message() + " **");
        }
        return true;
    }
    return false;
}

}   // namespace

namespace LSST {
namespace m2cellcpp {
namespace system {

ComConnection::Ptr ComConnection::create(IoServicePtr const& ioService) {
    return ComConnection::Ptr(new ComConnection(ioService));
}


ComConnection::ComConnection(IoServicePtr const& ioService)
    : _socket(*ioService), _ioService(ioService) {
}


void ComConnection::beginProtocol() {
    _receiveRequest();
}


void ComConnection::_receiveRequest() {
    Log::log(Log::DEBUG, "ComConnection::_receiveRequest");
    boost::asio::async_read_until(_socket, _streamBuf, getDelimiter(), 
                                  bind(&ComConnection::_readCommand, shared_from_this(), _1, _2));
}


void ComConnection::_readCommand(boost::system::error_code const& ec, size_t xfer) {
    assert(_streamBuf.size() >= xfer);

    if (::isErrorCode(ec, __func__)) return;
    size_t msgSz = xfer - getDelimiter().size();
    std::string command(buffers_begin(_streamBuf.data()), 
                        buffers_begin(_streamBuf.data()) + msgSz);
    _streamBuf.consume(xfer);

    Log::log(Log::INFO, "received command: " + command + " streamBuf size=" + to_string(command.size()));

    // TODO: put command on a queue to be hanlded

    // TODO: instead of echo, send response that the command was received 
    //      (will there be another response when the command is completed?)
    _sendResponse(command);
}


void ComConnection::_sendResponse(string const& command) {
    Log::log(Log::DEBUG, "ComConnection::_sendResponse command:" + command);
    string cmd = command + ComConnection::getDelimiter();
    boost::asio::async_write(_socket, boost::asio::buffer(cmd, cmd.size()),
                             bind(&ComConnection::_responseSent, shared_from_this(), _1, _2));
}


void ComConnection::_responseSent(boost::system::error_code const& ec, size_t xfer) {
    Log::log(Log::DEBUG, "ComConnection::_responseSent xfer=" + to_string(xfer));
    if (::isErrorCode(ec, __func__)) return;
    _receiveRequest();
}

}}} // namespace lsst::qserv::replica
