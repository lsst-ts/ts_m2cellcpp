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
#include "system/ComServer.h"
#include "system/Config.h"
#include "util/Log.h"

using namespace std;
using namespace std::placeholders;

namespace {

bool isErrorCode(boost::system::error_code const& ec, string const& note) {
    if (ec.value() != 0) {
        if (ec == boost::asio::error::eof) {
            LINFO(note, "  ** closed **");
        } else {
            LERROR(note, "  ** failed: ", ec.message(), " ** ");
        }
        return true;
    }
    return false;
}

}  // namespace

namespace LSST {
namespace m2cellcpp {
namespace system {

ComConnection::Ptr ComConnection::create(IoContextPtr const& ioContext, uint64_t connId,
                                         shared_ptr<ComServer> const& server) {
    return ComConnection::Ptr(new ComConnection(ioContext, connId, server));
}

ComConnection::ComConnection(IoContextPtr const& ioContext, uint64_t connId,
                             shared_ptr<ComServer> const& server)
        : _socket(*ioContext), _ioContext(ioContext), _connId(connId), _server(server) {}

ComConnection::~ComConnection() { shutdown(); }

void ComConnection::beginProtocol() { _receiveCommand(); }

void ComConnection::_receiveCommand() {
    LDEBUG("ComConnection::_receiveCommand");
    if (_shutdown) {
        return;
    }
    boost::asio::async_read_until(_socket, _streamBuf, getDelimiter(),
                                  bind(&ComConnection::_readCommand, shared_from_this(), _1, _2));
}

void ComConnection::_readCommand(boost::system::error_code const& ec, size_t xfer) {
    assert(_streamBuf.size() >= xfer);

    if (::isErrorCode(ec, __func__)) return;
    size_t msgSz = xfer - getDelimiter().size();
    std::string msgStr(buffers_begin(_streamBuf.data()), buffers_begin(_streamBuf.data()) + msgSz);
    _streamBuf.consume(xfer);

    LINFO("received msg: ", msgStr, " streamBuf size=", msgStr.size());

    // `interpretCommand()` will add a shared_from_this pointer to command
    // so it can send a response to the correct ComConnection, and
    // that ComConnection will still exist.
    auto [responseStr, command] = interpretCommand(msgStr);
    // This is really annoying, C++ 17 standard doesn't handle lambda
    // capture of a tuple element properly, so copy command to cmd so
    // it can go in the lambda capture.
    auto cmd = command;
    // The ack response must be sent before the command is run, or the final
    // respsonse could be sent before the ack.
    _sendResponse(responseStr);

    // Running the command could take a while, so start a new thread.
    auto cmdFunc = [cmd]() { cmd->runAction(nullptr); };
    thread cThrd(cmdFunc);
    cThrd.detach();
}

std::tuple<std::string, util::Command::Ptr> ComConnection::interpretCommand(std::string const& commandStr) {
    string ackMsg = makeTestAck(commandStr);
    // This lambda function will be run when `cmd->runAction()` is called.
    // It needs a shared_ptr to this to prevent segfaults if ComConnection was closed.
    auto thisPtr = shared_from_this();
    auto testFunc = [thisPtr, commandStr](util::CmdData*) {
        LDEBUG("ComConnection Running Command func");
        string finalMsg = makeTestFinal(commandStr);
        thisPtr->asyncWrite(finalMsg);
    };
    auto cmd = make_shared<util::Command>(testFunc);
    return {ackMsg, cmd};
}

void ComConnection::asyncWrite(string const& msg) {
    LDEBUG("ComConnection::asyncWrite ", msg);
    string cmd = msg + ComConnection::getDelimiter();
    boost::asio::async_write(_socket, boost::asio::buffer(cmd, cmd.size()),
                             bind(&ComConnection::_asyncWriteSent, shared_from_this(), _1, _2));
}

void ComConnection::_asyncWriteSent(boost::system::error_code const& ec, size_t xfer) {
    LDEBUG("ComConnection::_asyncWriteSent xfer=", xfer);
    // Log if there was an error and nothing else to do.
    ::isErrorCode(ec, __func__);
}

void ComConnection::_sendResponse(string const& command) {
    LDEBUG("ComConnection::_sendResponse command:", command);
    string cmd = command + ComConnection::getDelimiter();
    boost::asio::async_write(_socket, boost::asio::buffer(cmd, cmd.size()),
                             bind(&ComConnection::_responseSent, shared_from_this(), _1, _2));
}

void ComConnection::_responseSent(boost::system::error_code const& ec, size_t xfer) {
    LDEBUG("ComConnection::_responseSent xfer=", xfer);
    if (::isErrorCode(ec, __func__)) {
        return;
    }
    _receiveCommand();
}

void ComConnection::shutdown() {
    if (_shutdown.exchange(true) == true) {
        return;
    }
    // Tell the server to stop tracking this connection
    auto serv = _server.lock();
    if (serv != nullptr) {
        serv->eraseConnection(_connId);
    } else {
        LERROR("ComConnection::shutdown server already destroyed");
    }
    boost::system::error_code ec;
    _socket.shutdown(boost::asio::ip::tcp::socket::shutdown_send, ec);
    ::isErrorCode(ec, __func__);
}

string ComConnection::makeTestAck(string const& msg) {
    string ack = string("{Ack:") + msg + "}";
    return ack;
}

string ComConnection::makeTestFinal(string const& msg) {
    string final = string("{Final:") + msg + "}";
    return final;
}

}  // namespace system
}  // namespace m2cellcpp
}  // namespace LSST
