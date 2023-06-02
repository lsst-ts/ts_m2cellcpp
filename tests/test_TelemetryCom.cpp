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
#include "system/TelemetryCom.h"
#include "system/TelemetryItem.h"
#include "system/TelemetryMap.h"
#include "util/Log.h"

using namespace std;
using namespace LSST::m2cellcpp::system;

TEST_CASE("Test TelemetryItem", "[TelemetryItem]") {
    LSST::m2cellcpp::util::Log::getLog().useEnvironmentLogLvl();

    LDEBUG("TelemetryItem::test() start");
    auto powerStatus1 = TItemPowerStatus::Ptr(new TItemPowerStatus());

    double mv = -1.0;
    double mc = -2.0;
    double cv = 3.0;
    double cc = 4.0;
    powerStatus1->setMotorVoltage(mv);
    powerStatus1->setMotorCurrent(mc);
    powerStatus1->setCommVoltage(cv);
    powerStatus1->setCommCurrent(cc);

    auto powerStatusRaw1 = TItemPowerStatusRaw::Ptr(new TItemPowerStatusRaw());
    double rmv = 7.0;
    double rmc = 8.0;
    double rcv = -9.0;
    double rcc = -12.0;
    powerStatusRaw1->setMotorVoltage(rmv);
    powerStatusRaw1->setMotorCurrent(rmc);
    powerStatusRaw1->setCommVoltage(rcv);
    powerStatusRaw1->setCommCurrent(rcc);

    REQUIRE(powerStatus1->getMotorVoltage() == mv);
    REQUIRE(powerStatus1->getMotorCurrent() == mc);
    REQUIRE(powerStatus1->getCommVoltage() == cv);
    REQUIRE(powerStatus1->getCommCurrent() == cc);
    auto ps1Js = powerStatus1->getJson();
    LDEBUG("powerStatus1=", ps1Js);

    REQUIRE(powerStatusRaw1->getMotorVoltage() == rmv);
    REQUIRE(powerStatusRaw1->getMotorCurrent() == rmc);
    REQUIRE(powerStatusRaw1->getCommVoltage() == rcv);
    REQUIRE(powerStatusRaw1->getCommCurrent() == rcc);
    auto psRaw1Js = powerStatusRaw1->getJson();
    LDEBUG("powerStatusRaw1=", psRaw1Js);

    // Set values of powerStatus2 from powerStatus1 and test that they match.
    auto powerStatus2 = TItemPowerStatus::Ptr(new TItemPowerStatus());
    string ps1Str = to_string(ps1Js);
    bool result = powerStatus2->parse(ps1Str);
    LDEBUG("powerStatus2=", powerStatus2->getJson());
    REQUIRE(result == true);
    REQUIRE(powerStatus2->getMotorVoltage() == mv);
    REQUIRE(powerStatus2->getMotorCurrent() == mc);
    REQUIRE(powerStatus2->getCommVoltage() == cv);
    REQUIRE(powerStatus2->getCommCurrent() == cc);

    // Set values of powerStatusRaw2 from powerStatusRaw1 and test that they match.
    auto powerStatusRaw2 = TItemPowerStatusRaw::Ptr(new TItemPowerStatusRaw());
    string psRaw1Str = to_string(psRaw1Js);
    bool rresult = powerStatusRaw2->parse(psRaw1Str);
    LDEBUG("powerStatusRaw2=", powerStatusRaw2->getJson());
    REQUIRE(rresult == true);
    REQUIRE(powerStatusRaw2->getMotorVoltage() == rmv);
    REQUIRE(powerStatusRaw2->getMotorCurrent() == rmc);
    REQUIRE(powerStatusRaw2->getCommVoltage() == rcv);
    REQUIRE(powerStatusRaw2->getCommCurrent() == rcc);

    LDEBUG("TelemetryItem::test() end");
}

TEST_CASE("Test TelemetryCom", "[TelemetryCom]") {
    {
        LINFO("Creating serv");
        int const port = 10081;
        auto servTelemetryMap = TelemetryMap::Ptr(new TelemetryMap());
        auto serv = TelemetryCom::create(servTelemetryMap, port);

        serv->startServer();
        REQUIRE(serv->waitForServerRunning(5) == true);

        servTelemetryMap->getPowerStatus()->setMotorVoltage(-3.0);
        servTelemetryMap->getPowerStatus()->setMotorCurrent(4.0);
        servTelemetryMap->getPowerStatus()->setCommVoltage(-5.0);
        servTelemetryMap->getPowerStatus()->setCommCurrent(6.0);

        servTelemetryMap->getPowerStatusRaw()->setMotorVoltage(17.0);
        servTelemetryMap->getPowerStatusRaw()->setMotorCurrent(27.0);
        servTelemetryMap->getPowerStatusRaw()->setCommVoltage(30.0);
        servTelemetryMap->getPowerStatusRaw()->setCommCurrent(25.0);

        LDEBUG("Running clients");
        std::vector<TelemetryCom::Ptr> clients;
        std::vector<thread> clientThreads;

        for (int j = 0; j < 10; ++j) {
            auto clientTelemetryMap = TelemetryMap::Ptr(new TelemetryMap());
            REQUIRE(clientTelemetryMap->compareMaps(*servTelemetryMap) == false);
            TelemetryCom::Ptr client = TelemetryCom::create(clientTelemetryMap, port);
            clients.push_back(client);
            clientThreads.emplace_back(&TelemetryCom::client, client, j);
        }
        sleep(2);
        LDEBUG("Stopping server");
        serv->shutdownCom();
        LDEBUG("serv joined");
        for (auto& thrd : clientThreads) {
            LDEBUG("client joining");
            thrd.join();
        }
        LDEBUG("clients joined");

        // Test that client data matches the server data.
        for (auto const& client : clients) {
            TelemetryMap::Ptr clientMap = client->getTMap();
            LDEBUG("comparing client map");
            REQUIRE(clientMap->compareMaps(*servTelemetryMap) == true);
        }
    }
}
