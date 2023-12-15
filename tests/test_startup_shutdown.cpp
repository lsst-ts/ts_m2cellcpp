/*
 * This file is part of LSST ts_m2cellcpp test suite.
 *
 * Developed for the Vera C. Rubin Observatory Telescope & Site Software Systems.
 * This product includes software developed by the Vera C.Rubin Observatory Project
 * (https://www.lsst.org). See the COPYRIGHT file at the top-level directory of
 * this distribution for details of code ownership.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation, either version 3 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program. If not, see <https://www.gnu.org/licenses/>.
 */

#define CATCH_CONFIG_MAIN

// System headers
#include <exception>

// 3rd party headers
#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_session.hpp>

// Project headers
#include "control/Context.h"
#include "control/ControlMain.h"
#include "control/FpgaIo.h"
#include "control/MotionEngine.h"
#include "control/PowerSystem.h"
#include "simulator/SimCore.h"
#include "state/State.h"
#include "system/ComClient.h"
#include "system/ComControlServer.h"
#include "system/Config.h"
#include "util/Bug.h"
#include "util/Log.h"

using namespace std;
using namespace LSST::m2cellcpp;

/// Wait up to 5 seconds for `subsystem` to reach `targetState`, return true if that happens within the time
/// limit.
bool waitForPowerState(control::PowerState targetState, control::PowerSubsystem& subsystem,
                       string const& note) {
    auto waitStart = util::CLOCK::now();
    bool success = false;
    while (!success) {
        auto aPowerState = subsystem.getActualPowerState();
        auto volts = subsystem.getVoltage();
        LDEBUG(note, " power=", control::getPowerStateStr(aPowerState), " volt=", volts);
        success = targetState == aPowerState;
        string powMsg(note + " power=" + control::getPowerStateStr(aPowerState) +
                      " volt=" + to_string(volts) + " success=" + to_string(success));
        LDEBUG(powMsg);
        if (!success) {
            auto waitTimePassed = util::CLOCK::now() - waitStart;
            if (waitTimePassed > 5s) {
                LERROR("waitForPowerState timedOut ", powMsg);
                break;
            }
            this_thread::sleep_for(0.1s);
        }
    }
    return success;
}

// Wait up to 1 second for the model state to reach `targetState`, return true if that happens within the time
// limit.
bool waitForModelState(control::Context::Ptr const& context, state::State::StateEnum targetState,
                       string const& note) {
    auto waitStart = util::CLOCK::now();
    bool success = false;
    while (!success) {
        auto currentStateId = context->model.getCurrentState()->getId();
        success = currentStateId == targetState;
        string stateMsg(note + " waitForModelState targ=" + state::State::getStateEnumStr(targetState) +
                        " act=" + state::State::getStateEnumStr(currentStateId));
        LDEBUG(stateMsg);
        if (!success) {
            auto waitTimePassed = util::CLOCK::now() - waitStart;
            if (waitTimePassed > 1s) {
                LERROR("waitForModelState timedOut ", stateMsg);
                break;
            }
            this_thread::sleep_for(0.1s);
        }
    }
    return success;
}

nlohmann::json waitForPowerSystemState(system::ComClient& client, control::PowerSystemType powType,
                                       control::PowerState targetState, string const& note) {
    // Check that the power message was received by the client
    // First check if it was already received, and would be found in the map.
    string const powerSysStateId("powerSystemState");
    string lMsg("waitForPowerSystemState key=" + powerSysStateId +
                " type=" + control::getPowerSystemTypeStr(powType) + " " + to_string(powType) +
                " targ=" + control::getPowerStateStr(targetState) + " " + to_string(targetState));
    LDEBUG(lMsg);
    system::JsonMsgMap::JsonDeque jDeque = client.recvDequeForId(powerSysStateId, note);
    for (auto const& js : jDeque) {
        string jsStr = to_string(js);
        LDEBUG(waitForPowerSystemState, jsStr);
    }

    // The most recent power state with "powerType"==powType is what is important.
    bool found = false;
    nlohmann::json js;
    while (!found && !jDeque.empty()) {
        js = jDeque.back();
        jDeque.pop_back();
        if (js["id"] == powerSysStateId && js["powerType"] == powType && js["state"] == targetState) {
            found = true;
            LDEBUG("found in map ", lMsg);
        }
    }
    auto waitStart = util::CLOCK::now();
    while (!found) {
        js = client.cmdRecvId(powerSysStateId, note);
        if (js["id"] == powerSysStateId && js["powerType"] == powType && js["state"] == targetState) {
            found = true;
            LDEBUG("found by cmdRecvId");
        } else {
            auto waitTimePassed = util::CLOCK::now() - waitStart;
            if (waitTimePassed > 5s) {
                LERROR("waitForPowerSystemState timedOut ", lMsg);
                nlohmann::json jsEmpty;
                return jsEmpty;
            }
        }
    }
    return js;
}

/// This is used where the test needs to wait for another iteration of the system for the
/// status needs to be updated. It may be desirable to have the status update more quickly
/// so using this to make it easy to locate where the tests would need to be updated.
void shortSleep() { this_thread::sleep_for(0.1s); }

TEST_CASE("Test startup shutdown", "[CSV]") {
    LSST::m2cellcpp::util::Log::getLog().useEnvironmentLogLvl();
    string cfgPath = system::Config::getEnvironmentCfgPath("../configs");
    system::Config::setup(cfgPath + "unitTestCfg.yaml");
    auto& config = system::Config::get();

    /// Start the main thread
    control::ControlMain::setup();
    control::ControlMain::Ptr ctMain = control::ControlMain::getPtr();
    int argc = 1;
    string argv0("test_startup_shutdown");
    const char* argv[argc];
    argv[0] = argv0.c_str();
    ctMain->run(argc, argv);

    // Wait a few seconds for ctMain to be running
    for (int j = 0; !ctMain->getRunning() && j < 10; ++j) {
        sleep(1);
    }
    simulator::SimCore::Ptr simCore = ctMain->getSimCore();

    // Start client connection.
    system::IoContextPtr ioContext = make_shared<boost::asio::io_context>();
    int port = config.getControlServerPort();
    system::ComClient client(ioContext, "127.0.0.1", port);
    int welcomeCount = client.readWelcomeMsg();
    LDEBUG("welcomeCount=", welcomeCount);
    REQUIRE(welcomeCount == 16);

    control::Context::Ptr context = control::Context::get();
    REQUIRE(context != nullptr);
    context->model.ctrlSetup();
    control::PowerSystem::Ptr powerSys = context->model.getPowerSystem();

    // FUTURE: TODO: put this item into the configuration (or where does it really belong?)
    powerSys->writeCrioInterlockEnable(true);

    auto currentState = context->model.getCurrentState();
    LINFO("currentState=", currentState->getName());
    REQUIRE(context->model.getCurrentState() == context->model.getState(state::State::STANDBYSTATE));

    int seqId = 0;
    {
        seqId++;
        string note("Correct NCmdPower on comm power ");
        note += to_string(seqId);
        LDEBUG(note);
        string jStr = R"({"id":"cmd_power","powerType": 2, "status": true, "sequence_id":)";
        jStr += to_string(seqId) + " }";
        auto [ackJ, finJ] = client.cmdSendRecv(jStr, seqId, note);
        REQUIRE(ackJ["sequence_id"] == seqId);
        REQUIRE(ackJ["id"] == "ack");
        REQUIRE(finJ["sequence_id"] == seqId);
        REQUIRE(finJ["id"] == "success");
        REQUIRE(finJ["user_info"] == "");

        // verify model state
        bool inStandby = waitForModelState(context, state::State::STANDBYSTATE, "comm on");
        REQUIRE(inStandby);

        // verify comm power bit is on
        shortSleep();
        auto sInfo = simCore->getSysInfo();
        REQUIRE(sInfo.outputPort.getBitAtPos(control::OutputPortBits::MOTOR_POWER_ON) == 0);
        REQUIRE(sInfo.outputPort.getBitAtPos(control::OutputPortBits::ILC_COMM_POWER_ON) != 0);
    }

    {
        // Power bit is on, but comm voltage should still be low,
        // which prevents motor power from turning on.
        string note("Fail NCmdPower on motor power");
        note += to_string(seqId);
        LDEBUG(note);
        string jStr = R"({"id":"cmd_power","powerType": 1, "status": true, "sequence_id":)";
        jStr += to_string(seqId) + " }";
        auto [ackJ, finJ] = client.cmdSendRecv(jStr, seqId, note);
        REQUIRE(ackJ["sequence_id"] == seqId);
        REQUIRE(ackJ["id"] == "noack");
        REQUIRE(finJ["sequence_id"] == seqId);
        REQUIRE(finJ["id"] == "fail");
        REQUIRE(finJ["user_info"] == "");

        // verify power bits haven't changed
        auto sInfo = simCore->getSysInfo();
        REQUIRE(sInfo.outputPort.getBitAtPos(control::OutputPortBits::MOTOR_POWER_ON) == 0);
        REQUIRE(sInfo.outputPort.getBitAtPos(control::OutputPortBits::ILC_COMM_POWER_ON) != 0);

        // wait up to 5 seconds for the comm power to rise to 'ON' level voltage
        bool onSuccess = waitForPowerState(control::PowerState::ON, powerSys->getComm(), "comm");
        REQUIRE(onSuccess);
        shortSleep();
        shortSleep();
        shortSleep();

        // comm power is now on. Verify the "powerSystemState" message.
        nlohmann::json jsPowerSystemState = waitForPowerSystemState(client, control::PowerSystemType::COMM,
                                                                    control::PowerState::ON, "comm on");
        REQUIRE(jsPowerSystemState["id"] == "powerSystemState");
        REQUIRE(jsPowerSystemState["powerType"] == control::PowerSystemType::COMM);  // 2
        REQUIRE(jsPowerSystemState["state"] == control::PowerState::ON);             // 5
        REQUIRE(jsPowerSystemState["status"] == true);
    }

    {
        seqId++;
        string note("Correct NCmdPower on motor power ");
        note += to_string(seqId);
        LDEBUG(note);
        string jStr = R"({"id":"cmd_power","powerType": 1, "status": true, "sequence_id":)";
        jStr += to_string(seqId) + " }";
        auto [ackJ, finJ] = client.cmdSendRecv(jStr, seqId, note);
        REQUIRE(ackJ["sequence_id"] == seqId);
        REQUIRE(ackJ["id"] == "ack");
        REQUIRE(finJ["sequence_id"] == seqId);
        REQUIRE(finJ["id"] == "success");
        REQUIRE(finJ["user_info"] == "");

        // verify power bits are on
        shortSleep();
        auto sInfo = simCore->getSysInfo();
        REQUIRE(sInfo.outputPort.getBitAtPos(control::OutputPortBits::MOTOR_POWER_ON) != 0);
        REQUIRE(sInfo.outputPort.getBitAtPos(control::OutputPortBits::ILC_COMM_POWER_ON) != 0);

        // wait up to 5 seconds for the motor power to rise to 'ON' level voltage
        bool onSuccess = waitForPowerState(control::PowerState::ON, powerSys->getMotor(), "motor");
        REQUIRE(onSuccess);

        // motor power is now on. Verify the "powerSystemState" message.
        nlohmann::json jsPowerSystemState = waitForPowerSystemState(client, control::PowerSystemType::MOTOR,
                                                                    control::PowerState::ON, "motor on");
        REQUIRE(jsPowerSystemState["id"] == "powerSystemState");
        REQUIRE(jsPowerSystemState["powerType"] == control::PowerSystemType::MOTOR);  // 1
        REQUIRE(jsPowerSystemState["state"] == control::PowerState::ON);              // 5
        REQUIRE(jsPowerSystemState["status"] == true);

        // verify model state should switch to IDLESTATE once all power is on
        bool inIdle = waitForModelState(context, state::State::IDLESTATE, "motor on");
        REQUIRE(inIdle);
    }

    {
        seqId++;
        string note("Correct NCmdPower off motor power ");
        note += to_string(seqId);
        LDEBUG(note);
        string jStr = R"({"id":"cmd_power","powerType": 1, "status": false, "sequence_id":)";
        jStr += to_string(seqId) + " }";
        auto [ackJ, finJ] = client.cmdSendRecv(jStr, seqId, note);
        REQUIRE(ackJ["sequence_id"] == seqId);
        REQUIRE(ackJ["id"] == "ack");
        REQUIRE(finJ["sequence_id"] == seqId);
        REQUIRE(finJ["id"] == "success");
        REQUIRE(finJ["user_info"] == "");

        // verify motor power bit is off
        shortSleep();
        auto sInfo = simCore->getSysInfo();
        REQUIRE(sInfo.outputPort.getBitAtPos(control::OutputPortBits::MOTOR_POWER_ON) == 0);
        REQUIRE(sInfo.outputPort.getBitAtPos(control::OutputPortBits::ILC_COMM_POWER_ON) != 0);

        // verify model state should switch to STANDBYSTATE once motor power starts turning off
        bool inStandby = waitForModelState(context, state::State::STANDBYSTATE, "motor off");
        REQUIRE(inStandby);

        // wait up to 5 seconds for the motor power to fall to 'OFF' level voltage
        bool onSuccess = waitForPowerState(control::PowerState::OFF, powerSys->getMotor(), "motor");
        REQUIRE(onSuccess);

        // motor power is now off. Verify the "powerSystemState" message.
        nlohmann::json jsPowerSystemState = waitForPowerSystemState(client, control::PowerSystemType::MOTOR,
                                                                    control::PowerState::OFF, "motor off");
        REQUIRE(jsPowerSystemState["id"] == "powerSystemState");
        REQUIRE(jsPowerSystemState["powerType"] == control::PowerSystemType::MOTOR);  // 1
        REQUIRE(jsPowerSystemState["state"] == control::PowerState::OFF);             // 2
        REQUIRE(jsPowerSystemState["status"] == false);
    }

    {
        seqId++;
        string note("Correct NCmdPower off comm power ");
        note += to_string(seqId);
        LDEBUG(note);
        string jStr = R"({"id":"cmd_power","powerType": 2, "status": false, "sequence_id":)";
        jStr += to_string(seqId) + " }";
        auto [ackJ, finJ] = client.cmdSendRecv(jStr, seqId, note);
        REQUIRE(ackJ["sequence_id"] == seqId);
        REQUIRE(ackJ["id"] == "ack");
        REQUIRE(finJ["sequence_id"] == seqId);
        REQUIRE(finJ["id"] == "success");
        REQUIRE(finJ["user_info"] == "");

        // verify comm power bit is off
        shortSleep();
        auto sInfo = simCore->getSysInfo();
        REQUIRE(sInfo.outputPort.getBitAtPos(control::OutputPortBits::MOTOR_POWER_ON) == 0);
        REQUIRE(sInfo.outputPort.getBitAtPos(control::OutputPortBits::ILC_COMM_POWER_ON) == 0);

        // verify model state
        bool inStandby = waitForModelState(context, state::State::STANDBYSTATE, "comm off");
        REQUIRE(inStandby);

        // comm power is now on. Verify the "powerSystemState" message.
        nlohmann::json jsPowerSystemState = waitForPowerSystemState(client, control::PowerSystemType::COMM,
                                                                    control::PowerState::OFF, "comm off");
        REQUIRE(jsPowerSystemState["id"] == "powerSystemState");
        REQUIRE(jsPowerSystemState["powerType"] == control::PowerSystemType::COMM);  // 2
        REQUIRE(jsPowerSystemState["state"] == control::PowerState::OFF);            // 2
        REQUIRE(jsPowerSystemState["status"] == false);
    }

    {
        seqId++;
        string note("NCmdSystemShutdown");
        string jStr = R"({"id":"cmd_systemShutdown","sequence_id": )";
        jStr += to_string(seqId) + " }";
        LINFO(note, " ", jStr);
        auto [ackJ, finJ] = client.cmdSendRecv(jStr, seqId, note);
    }

    // FUTURE: these should be shutdown properly elsewhere.
    context->model.getPowerSystem()->stopTimeoutLoop();
    control::FpgaIo::getPtr()->stopLoop();
    control::MotionEngine::getPtr()->engineStop();

    LDEBUG("Join main");
    ctMain->join();

    LDEBUG("stopping local io");
    ioContext->stop();

    LINFO("Done");
}
