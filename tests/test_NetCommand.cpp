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
#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_session.hpp>

#include "control/NetCommandFactory.h"
#include "util/Log.h"

using namespace std;
using namespace LSST::m2cellcpp::control;

TEST_CASE("Test NetCommand", "[NetCommand]") {
    LSST::m2cellcpp::util::Log::getLog().useEnvironmentLogLvl();
    {
        // Test basic functionality of base class
        string jStr = "{\"id\":\"cmd_ack\",\"seq_id\": 5 }";
        NetCommand::JsonPtr jsn = NetCommand::parse(jStr);
        REQUIRE(jsn->at("id") == "cmd_ack");
        REQUIRE(jsn->at("seq_id") == 5);
        NetCommand::Ptr cmd1 = NCmdAck::create(jsn);
        REQUIRE(cmd1->getName() == "cmd_ack");
        REQUIRE(cmd1->getSeqId() == 5);
    }

    {
        string msg = "simple test msg";
        NetCommandException cExc(ERR_LOC, msg);
        // Only compare the first part of the message, ignore util::Issue add on.
        string exMsg = cExc.what();
        exMsg = exMsg.substr(0, msg.length());
        REQUIRE(exMsg == msg);
    }
    // Test that Command Exception is thrown appropriately.
    string note("Malformed json ");
    LDEBUG(note);
    bool thrown = false;
    try {
        string jStr = "\"id\":\"cmd_ack\",\"seq_id\": 1 }";
        auto jsn = NetCommand::parse(jStr);
    } catch (NetCommandException const& ex) {
        LDEBUG(note, ex.what());
        thrown = true;
    }
    REQUIRE(thrown);

    note = "Missing id ";
    LDEBUG(note);
    try {
        thrown = false;
        string jStr = "{\"seq_id\": 1 }";
        auto jsn = NetCommand::parse(jStr);
    } catch (NetCommandException const& ex) {
        LDEBUG(note, ex.what());
        thrown = true;
    }
    REQUIRE(thrown);

    note = "seq_id not int ";
    LDEBUG(note);
    try {
        thrown = false;
        string jStr = "{\"id\":\"cmd_ack\",\"seq_id\":\"hello\" }";
        auto jsn = NetCommand::parse(jStr);
    } catch (NetCommandException const& ex) {
        LDEBUG(note, ex.what());
        thrown = true;
    }
    REQUIRE(thrown);

    note = "json is nullptr ";
    LDEBUG(note);
    try {
        thrown = false;
        auto cmd = NCmdAck::create(nullptr);
    } catch (NetCommandException const& ex) {
        LDEBUG(note, ex.what());
        thrown = true;
    }
    REQUIRE(thrown);

    note = "create test";
    LDEBUG(note);
    {
        auto js1 = NetCommand::JsonPtr(new nlohmann::json{{"id", "cmd_ack"}, {"seq_id", 7}});
        auto cmd = NCmdAck::create(js1);
        REQUIRE(cmd != nullptr);
    }

    note = "constructor missing seq ";
    LDEBUG(note);
    try {
        thrown = false;
        auto js1 = NetCommand::JsonPtr(new nlohmann::json{{"id", "cmd_ack"}});
        auto cmd = NCmdAck::create(js1);
    } catch (NetCommandException const& ex) {
        LDEBUG(note, ex.what());
        thrown = true;
    }
    REQUIRE(thrown);

    note = "constructor missing id ";
    LDEBUG(note);
    try {
        thrown = false;
        auto js1 = NetCommand::JsonPtr(new nlohmann::json{{"seq_id", 7}});
        auto cmd = NCmdAck::create(js1);
    } catch (NetCommandException const& ex) {
        LDEBUG(note, ex.what());
        thrown = true;
    }
    REQUIRE(thrown);
}

TEST_CASE("Test NetCommandFactory", "[Factory]") {
    std::vector<NetCommand::Ptr> nCmds;
    nCmds.push_back(NCmdAck::createFactoryVersion());
    nCmds.push_back(NCmdEcho::createFactoryVersion());

    auto factory = NetCommandFactory::create();
    for (auto&& cmd : nCmds) {
        factory->addNetCommand(cmd);
    }

    {
        auto noAckCmd = factory->getNoAck();
        auto ackStr = noAckCmd->getAckJsonStr();
        auto ackJ = nlohmann::json::parse(ackStr);
        REQUIRE(ackJ["id"] == "noack");
        REQUIRE(ackJ["seq_id"] == 0);
        REQUIRE(ackJ["user_info"] == "factory default noack");
    }

    string jStr1 = "{\"id\":\"cmd_ack\",\"seq_id\": 1 }";
    {
        string note = "Correct NCmdAck jStr ";
        LDEBUG(note);
        auto inNCmd = factory->getCommandFor(jStr1);
        REQUIRE(inNCmd != nullptr);
        auto inNCmdAck = dynamic_pointer_cast<NCmdAck>(inNCmd);
        REQUIRE(inNCmdAck != nullptr);
        auto ackStr = inNCmd->getAckJsonStr();
        auto ackJ = nlohmann::json::parse(ackStr);
        REQUIRE(ackJ["seq_id"] == 1);
        REQUIRE(ackJ["id"] == "ack");
        REQUIRE(inNCmd->run() == true);
        auto respStr = inNCmd->getRespJsonStr();
        auto respJ = nlohmann::json::parse(respStr);
        REQUIRE(respJ["seq_id"] == 1);
        REQUIRE(respJ["id"] == "success");
    }

    {
        string note = "Correct NCmdEcho jStr ";
        LDEBUG(note);
        string jStr = "{\"id\":\"cmd_echo\",\"seq_id\": 2, \"msg\":\"This is an echomsg\" }";
        auto inNCmd = factory->getCommandFor(jStr);
        REQUIRE(inNCmd != nullptr);
        auto inNCmdEcho = dynamic_pointer_cast<NCmdEcho>(inNCmd);
        REQUIRE(inNCmdEcho != nullptr);
        auto ackStr = inNCmd->getAckJsonStr();
        auto ackJ = nlohmann::json::parse(ackStr);
        REQUIRE(ackJ["seq_id"] == 2);
        REQUIRE(ackJ["id"] == "ack");
        REQUIRE(inNCmd->run() == true);
        auto respStr = inNCmd->getRespJsonStr();
        auto respJ = nlohmann::json::parse(respStr);
        REQUIRE(respJ["seq_id"] == 2);
        REQUIRE(respJ["id"] == "success");
        string respMsg = respJ["msg"];
        REQUIRE(respJ["msg"] == "This is an echomsg");
    }

    {
        string note = "Incorrect NCmdAck jStr ";
        LDEBUG(note);
        string jStr = "{\"id\":\"cmd_ak\",\"seq_id\": 3 }";
        auto inNCmd = factory->getCommandFor(jStr);
        REQUIRE(inNCmd != nullptr);
        auto inNCmdNoAck = dynamic_pointer_cast<NCmdNoAck>(inNCmd);
        REQUIRE(inNCmdNoAck != nullptr);
        auto ackStr = inNCmd->getAckJsonStr();
        auto ackJ = nlohmann::json::parse(ackStr);
        REQUIRE(ackJ["seq_id"] == 3);
        REQUIRE(ackJ["id"] == "noack");
        REQUIRE(inNCmd->run() == false);
        auto respStr = inNCmd->getRespJsonStr();
        auto respJ = nlohmann::json::parse(respStr);
        REQUIRE(respJ["seq_id"] == 3);
        REQUIRE(respJ["id"] == "fail");
    }

    {
        string note = "Incorrect seq_id NCmdAck jStr ";
        LDEBUG(note);
        auto inNCmd = factory->getCommandFor(jStr1);
        REQUIRE(inNCmd != nullptr);
        auto inNCmdNoAck = dynamic_pointer_cast<NCmdNoAck>(inNCmd);
        REQUIRE(inNCmdNoAck != nullptr);
        auto ackStr = inNCmd->getAckJsonStr();
        auto ackJ = nlohmann::json::parse(ackStr);
        REQUIRE(ackJ["seq_id"] == 1);
        REQUIRE(ackJ["id"] == "noack");
        REQUIRE(inNCmd->run() == false);
        auto respStr = inNCmd->getRespJsonStr();
        auto respJ = nlohmann::json::parse(respStr);
        REQUIRE(respJ["seq_id"] == 1);
        REQUIRE(respJ["id"] == "fail");
    }

    {
        string note = "Echo missing message jStr ";
        LDEBUG(note);
        string jStr = "{\"id\":\"cmd_echo\",\"seq_id\": 4, \"msgg\":\"This is an echomsg\" }";
        auto inNCmd = factory->getCommandFor(jStr);
        REQUIRE(inNCmd != nullptr);
        auto inNCmdNoAck = dynamic_pointer_cast<NCmdNoAck>(inNCmd);
        REQUIRE(inNCmdNoAck != nullptr);
        auto ackStr = inNCmd->getAckJsonStr();
        auto ackJ = nlohmann::json::parse(ackStr);
        REQUIRE(ackJ["seq_id"] == 4);
        REQUIRE(ackJ["id"] == "noack");
        REQUIRE(inNCmd->run() == false);
        auto respStr = inNCmd->getRespJsonStr();
        auto respJ = nlohmann::json::parse(respStr);
        REQUIRE(respJ["seq_id"] == 4);
        REQUIRE(respJ["id"] == "fail");
        LDEBUG("ackJson=", ackJ.dump());
        LDEBUG("respJson=", respJ.dump());
    }
}
