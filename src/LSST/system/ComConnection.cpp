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
#include "nlohmann/json.hpp"

// Project headers
#include "faultmgr/FaultMgr.h"
#include "system/ComServer.h"
#include "system/Config.h"
#include "system/Globals.h"
#include "util/Log.h"

using namespace std;
using namespace std::placeholders;

using json = nlohmann::json;

namespace {

bool isErrorCode(boost::system::error_code const& ec, string const& note) {
    if (ec.value() != 0) {
        if (ec == boost::asio::error::eof) {
            LINFO(note, "  ** closed **");
        } else {
            LERROR(note, "  ** failed: ", ec.message(), " **");
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
    auto ptr =  ComConnection::Ptr(new ComConnection(ioContext, connId, server));

    return ptr;
}

ComConnection::ComConnection(IoContextPtr const& ioContext, uint64_t connId,
                             shared_ptr<ComServer> const& server)
        : _socket(*ioContext), _ioContext(ioContext), _connId(connId), _server(server) {}

ComConnection::~ComConnection() { shutdown(); }

void ComConnection::_syncWrite(string const& inMsg) {
    string msg = inMsg + getDelimiter();
    size_t bytesWritten = 0;
    LDEBUG("ComConnection::_syncWrite ", msg);
    while (bytesWritten < msg.length()) {
        size_t bytesToSend = msg.length() - bytesWritten;
        bytesWritten += _socket.write_some(boost::asio::buffer(msg.c_str() + bytesWritten, bytesToSend));
    }

}

void ComConnection::beginProtocol() {
    _connectionActive = true;
    Globals::get().setTcpIpConnected(true); // This seems a bit early to set this, but it's what the gui expects.
    _sendWelcomeMsg();
    _receiveCommand();
}

void ComConnection::_sendWelcomeMsg() {
    if (!_doSendWelcomeMsg) {
        return;
    }

    // Send a bunch of one-time messages that indicate various system states.
    // Seems like this should happen at telemetry startup.

    Globals& globals = Globals::get();

    // send tcp connected message
    {
        json js;
        js["id"] = "tcpIpConnected";
        js["isConnected"] = globals.getTcpIpConnected();
        _syncWrite(to_string(js));
    }

    _syncWrite(to_string(globals.getCommandableByDdsJson()));

    // send hardpoint information mock_server.py:281 await self._message_event.write_hardpoint_list(hardpoints)
    {
        json js;
        js["id"] = "hardpointList";
        js["actuators"] = globals.getHardPointList();
        _syncWrite(to_string(js));
    }

    // send interlock mock_server.py:283 await self._message_event.write_interlock(False)
    {
        json js;
        js["id"] = "interlock";
        js["state"] = globals.getInterlock();
        _syncWrite(to_string(js));
    }

    // elev external source  mock_server.py:290 await self._message_event.write_inclination_telemetry_source(is_external_source)
    {
        json js;
        js["id"] = "inclinationTelemetrySource";
        js["source"] = globals.getTelemetrySource();
        _syncWrite(to_string(js));
    }

    // temp offset mock_server.py:292 await self._message_event.write_temperature_offset(
    {
        json js;
        js["id"] = "temperatureOffset";
        js["ring"] = globals.getTemperatureOffsetsRing();
        js["intake"] = globals.getTemperatureOffsetsIntake();
        js["exhaust"] = globals.getTemperatureOffsetsExhaust();
        _syncWrite(to_string(js));
    }

    // FUTURE: This is only for backward compatibility and will not be needed if the final version
    {
        json js;
        js["id"] = "summaryState";
        globals.setSummaryState(5);
        js["summaryState"] = globals.getSummaryState();
        _syncWrite(to_string(js));
    }

    // # Send the digital input and output
    // digital_input = self.model.get_digital_input()
    // await self._message_event.write_digital_input(digital_input)
    {
        json js;
        js["id"] = "digitalInput";
        js["value"] = globals.getDigitalInput();
        _syncWrite(to_string(js));
    }


    // digital_output = self.model.get_digital_output()
    // await self._message_event.write_digital_output(digital_output)
    {
        json js;
        js["id"] = "digitalOutput";
        js["value"] = globals.getDigitalOutput();
        _syncWrite(to_string(js));
    }

    // await self._message_event.write_config()
    // TODO: It looks like all of these values should come out of the configuration PLACEHOLDER DM-40317
    // FUTURE: Also, can the gui (and future systems) be capable of handling a dump of the entire config in the json msg? Probably useful.
    {
        json js;
        js["id"] = "config";
        js["configuration"] = "Configurable_File_Description_20180831T092556_surrogate_handling.csv";
        js["version"] = "20180831T092556";
        js["controlParameters"] = "CtrlParameterFiles_2018-07-19_104314_surg";
        js["lutParameters"] = "FinalHandlingLUTs";
        js["powerWarningMotor"] = 5.0;
        js["powerFaultMotor"] = 10.0;
        js["powerThresholdMotor"] = 20.0;
        js["powerWarningComm"] = 5.0;
        js["powerFaultComm"] = 10.0;
        js["powerThresholdComm"] = 10.0;
        js["inPositionAxial"] = 0.158;
        js["inPositionTangent"] = 1.1;
        js["inPositionSample"] = 1.0;
        js["timeoutSal"] = 15.0;
        js["timeoutCrio"] = 1.0;
        js["timeoutIlc"] = 3;
        js["inclinometerDelta"] = 2.0;
        js["inclinometerDiffEnabled"] = true;
        js["cellTemperatureDelta"] = 2.0;
        _syncWrite(to_string(js));
    }

    // await self._message_event.write_closed_loop_control_mode( ClosedLoopControlMode.Idle )
    {
        json js;
        js["id"] = "closedLoopControlMode";
        js["mode"] = globals.getClosedLoopControlMode();
        _syncWrite(to_string(js));
    }

    // await self._message_event.write_enabled_faults_mask( self.model.error_handler.enabled_faults_mask )
    {
        json js;
        js["id"] = "enabledFaultsMask";
        js["mask"] = faultmgr::FaultMgr::get().getFaultEnableMask().getBitmap();
        _syncWrite(to_string(js));
    }

    // await self._message_event.write_configuration_files()
    // FUTURE: These files need to be located and added to this project DM-40317
    // PLACEHOLDER
    {
        json js;
        js["id"] = "configurationFiles";
        js["files"] = {"Configurable_File_Description_PLACEHOLDER_M2_optical.csv",
                       "Configurable_File_Description_PLACEHOLDER_M2_handling.csv",
                       "Configurable_File_Description_PLACEHOLDER_surrogate_optical.csv",
                       "Configurable_File_Description_PLACEHOLDER_surrogate_handling.csv"};
        _syncWrite(to_string(js));
    }

    // FUTURE: This is only for backward compatibility and will not be needed if the final version
    // PLACEHOLDER
    {
        json js;
        js["id"] = "summaryState";
        globals.setSummaryState(3);
        js["summaryState"] = globals.getSummaryState();
        _syncWrite(to_string(js));
    }

    // FUTURE: This is only for backward compatibility and will not be needed if the final version
    // PLACEHOLDER
    {
        json js;
        js["id"] = "forceBalanceSystemStatus";
        js["status"] = false;
        _syncWrite(to_string(js));
    }

    {
        json js;
        js["id"] = "summaryFaultsStatus";
        // The value from the ts_m2gui simulator produces this value for
        // 'js["status"] = 144115188075855872;'.
        // FUTURE: Should see how this compares to the simulator as
        // more systems are implemented.
        js["status"] = faultmgr::FaultMgr::get().getSummaryFaults().getBitmap();
        _syncWrite(to_string(js));
    }


}

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
    if (_connectionActive.exchange(false)) {
        Globals::get().setTcpIpConnected(false);
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
