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

#include "system/ComClient.h"
#include "system/ComControlServer.h"
#include "system/ComServer.h"
#include "system/Config.h"
#include "system/Globals.h"
#include "util/Log.h"

using namespace std;
using namespace LSST::m2cellcpp::system;
using namespace LSST::m2cellcpp;



tuple<nlohmann::json, nlohmann::json> comTest(string const& jStr, ComClient& client, string const& note) {
    Globals::setup();
    client.writeCommand(jStr);
    LDEBUG(note, ":wrote jStr=", jStr);
    auto ack = client.readCommand();
    LDEBUG(note, ":read ack=", ack);
    auto ackJ = nlohmann::json::parse(ack);
    auto fin = client.readCommand();
    LDEBUG(note, ":read fin=", fin);
    auto finJ = nlohmann::json::parse(fin);
    return {ackJ, finJ};
}

TEST_CASE("Test ComControl", "[ComControl]") {
    util::Log::getLog().useEnvironmentLogLvl();
    string cfgPath = Config::getEnvironmentCfgPath("../configs");
    Config::setup(cfgPath + "unitTestCfg.yaml");



    // Start a ComControlServer
    IoContextPtr ioContext = make_shared<boost::asio::io_context>();
    int port = Config::get().getControlServerPort();
    auto cmdFactory = control::NetCommandFactory::create();
    ComControl::setupNormalFactory(cmdFactory);
    auto serv = ComControlServer::create(ioContext, port, cmdFactory);
    serv->_doSendWelcomeMsgServ = false;

    atomic<bool> done{false};
    REQUIRE(serv->getState() == ComServer::CREATED);
    LDEBUG("server started");

    thread servThrd([&serv, &done]() {
        LINFO("server run ", serv->prettyState(serv->getState()));
        serv->run();
        LINFO("server finish");
        done = true;
    });

    // Make sure the server is running
    for (int j = 0; (serv->getState() != ComServer::RUNNING) && j < 10; ++j) {
        sleep(1);
    }

    {
        ComClient client(ioContext, "127.0.0.1", port);
        {
            string note("Correct NCmdAck");
            LDEBUG(note);
            string jStr = R"({"id":"cmd_ack","seq_id": 1 })";
            int seqId = 1;
            auto [ackJ, finJ] = comTest(jStr, client, note);
            REQUIRE(ackJ["seq_id"] == seqId);
            REQUIRE(ackJ["id"] == "ack");
            REQUIRE(finJ["seq_id"] == seqId);
            REQUIRE(finJ["id"] == "success");
            REQUIRE(finJ["user_info"] == "");
        }
        {
            string note = "Correct NCmdEcho";
            LDEBUG(note);
            string jStr = R"({"id":"cmd_echo","seq_id": 2, "msg":"This is an echomsg" })";
            int seqId = 2;
            auto [ackJ, finJ] = comTest(jStr, client, note);
            REQUIRE(ackJ["seq_id"] == seqId);
            REQUIRE(ackJ["id"] == "ack");
            REQUIRE(finJ["seq_id"] == seqId);
            REQUIRE(finJ["id"] == "success");
            REQUIRE(finJ["msg"] == "This is an echomsg");
        }
        {
            string note = "Incorrect NCmdAck";
            LDEBUG(note);
            string jStr = R"({"id":"cmd_ak","seq_id": 3 })";
            int seqId = 3;
            auto [ackJ, finJ] = comTest(jStr, client, note);
            REQUIRE(ackJ["seq_id"] == seqId);
            REQUIRE(ackJ["id"] == "noack");
            REQUIRE(finJ["seq_id"] == seqId);
            REQUIRE(finJ["id"] == "fail");
        }
    }

    // Shutdown the server
    serv->shutdown();
    REQUIRE(serv->connectionCount() == 0);

    // Server stop
    ioContext->stop();
    for (int j = 0; !done && j < 10; ++j) {
        sleep(1);
        bool d = done;
        LINFO("server wait ", d);
    }
    LDEBUG("server stopped");
    servThrd.join();
}
