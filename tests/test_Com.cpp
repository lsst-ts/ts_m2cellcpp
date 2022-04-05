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
#include <thread>

// 3rd party headers
#include <boost/asio.hpp>
#include <catch2/catch.hpp>

// Project headers
#include "system/Config.h"
#include "system/ComClient.h"
#include "system/ComServer.h"
#include "util/Log.h"

using namespace std;
using namespace LSST::m2cellcpp::system;
using Catch::Matchers::StartsWith;

TEST_CASE("Test Com echo", "[Com]") {
    Config::setup("UNIT_TEST");

    REQUIRE(ComServer::prettyState(ComServer::CREATED) == "CREATED");
    REQUIRE(ComServer::prettyState(ComServer::RUNNING) == "RUNNING");
    REQUIRE(ComServer::prettyState(ComServer::STOPPED) == "STOPPED");

    // Start the server
    IoContextPtr ioContext = make_shared<boost::asio::io_context>();
    string strPort = Config::get().getValue("server", "port");
    int port = stoi(strPort);
    auto serv = ComServer::create(ioContext, port);
    atomic<bool> done{false};
    REQUIRE(serv->getState() == ComServer::CREATED);
    LDEBUG("server started");

    thread servThrd([&serv, &done]() {
        LINFO("server run ", serv->prettyState(serv->getState()));
        serv->run();
        LINFO("server finish");
        done = true;
    });

    for (int j = 0; (serv->getState() != ComServer::RUNNING) && j < 10; ++j) {
        sleep(1);
    }
    REQUIRE(serv->getState() == ComServer::RUNNING);
    LDEBUG("server running");

    // Client echo test 1
    {
        ComClient client(ioContext, "127.0.0.1", port);
        string cmd("This is test 1");
        client.writeCommand(cmd);
        LDEBUG("wrote cmd=", cmd);
        auto str = client.readCommand();
        LDEBUG("read str=", str);
        REQUIRE(cmd == str);
    }

    // Client echo test 2
    {
        ComClient client(ioContext, "127.0.0.1", port);
        string cmd("Something different 42");
        client.writeCommand(cmd);
        LDEBUG("wrote cmd=" + cmd);
        auto str = client.readCommand();
        LDEBUG("read str=", str);
        REQUIRE(cmd == str);
    }

    serv->shutdown();
    REQUIRE(serv->connectionCount() == 0);

    // Client expected failure
    {
        ComClient client(ioContext, "127.0.0.1", port);
        string cmd("expected failure");
        client.writeCommand(cmd);
        LDEBUG("wrote cmd=", cmd);
        REQUIRE_THROWS(client.readCommand());
        REQUIRE_THROWS(client.writeCommand(cmd));
    }

    // Server stop
    ioContext->stop();
    for (int j = 0; !done && j < 10; ++j) {
        sleep(1);
        bool d = done;
        LINFO("server wait ", d);
    }
    LDEBUG("server stopped");
    servThrd.join();
    REQUIRE(done == true);
    REQUIRE(serv->getState() == ComServer::STOPPED);
    serv.reset();
    LDEBUG("server reset");
}
