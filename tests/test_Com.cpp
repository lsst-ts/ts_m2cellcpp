/*
 * This file is part of LSST cRIOcpp test suite. Tests ILC generic functions.
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
#include "system/Log.h"


using namespace std;
using namespace LSST::m2cellcpp::system;
using Catch::Matchers::StartsWith;

TEST_CASE("Test Com echo", "[Com]") {
    Config::setup("UNIT_TEST");

    // Start the server
    IoServicePtr ioContext = make_shared<boost::asio::io_service>(); 
    string strPort = Config::get().getValue("server", "port");
    int port = stoi(strPort);
    auto serv = ComServer::create(ioContext, port);
    atomic<bool> done{false};
    REQUIRE(serv->getState() == ComServer::CREATED);
    Log::log(Log::DEBUG, "server started");

    thread servThrd([serv, &done](){
        Log::log(Log::INFO, "server run " + serv->prettyState(serv->getState()));
        serv->run();
        Log::log(Log::INFO, "server finish"); 
        done = true; 
    });

    for (int j=0; (serv->getState() != ComServer::RUNNING) && j<10; ++j) {
        sleep(1);
    }    
    REQUIRE(serv->getState() == ComServer::RUNNING);
    Log::log(Log::DEBUG, "server running");

    // Client echo test
    ComClient client(ioContext, "127.0.0.1", port);
    string cmd("This is test 1");
    client.writeCommand(cmd);
    Log::log(Log::DEBUG, "wrote cmd=" + cmd);
    auto str = client.readCommand();
    Log::log(Log::DEBUG, "read str=" + str);
    REQUIRE(cmd == str);

    // Server stop
    ioContext->stop();
    for (int j=0; !done && j<10; ++j) {
        sleep(1);
        Log::log(Log::INFO, "server wait " + to_string(done)); 
    }
    Log::log(Log::DEBUG, "server stopped");
    servThrd.join();
    REQUIRE(done == true);
    REQUIRE(serv->getState() == ComServer::STOPPED);
}
