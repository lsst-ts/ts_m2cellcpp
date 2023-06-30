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
    powerStatus1->getMotorVoltage().setVal(mv);
    powerStatus1->getMotorCurrent().setVal(mc);
    powerStatus1->getCommVoltage().setVal(cv);
    powerStatus1->getCommCurrent().setVal(cc);

    auto powerStatusRaw1 = TItemPowerStatusRaw::Ptr(new TItemPowerStatusRaw());
    double rmv = 7.0;
    double rmc = 8.0;
    double rcv = -9.0;
    double rcc = -12.0;
    powerStatusRaw1->getMotorVoltage().setVal(rmv);
    powerStatusRaw1->getMotorCurrent().setVal(rmc);
    powerStatusRaw1->getCommVoltage().setVal(rcv);
    powerStatusRaw1->getCommCurrent().setVal(rcc);

    REQUIRE(powerStatus1->getMotorVoltage().getVal() == mv);
    REQUIRE(powerStatus1->getMotorCurrent().getVal() == mc);
    REQUIRE(powerStatus1->getCommVoltage().getVal() == cv);
    REQUIRE(powerStatus1->getCommCurrent().getVal() == cc);
    auto ps1Js = powerStatus1->getJson();
    LDEBUG("powerStatus1=", ps1Js);

    REQUIRE(powerStatusRaw1->getMotorVoltage().getVal() == rmv);
    REQUIRE(powerStatusRaw1->getMotorCurrent().getVal() == rmc);
    REQUIRE(powerStatusRaw1->getCommVoltage().getVal() == rcv);
    REQUIRE(powerStatusRaw1->getCommCurrent().getVal() == rcc);
    auto psRaw1Js = powerStatusRaw1->getJson();
    LDEBUG("powerStatusRaw1=", psRaw1Js);

    // Set values of powerStatus2 from powerStatus1 and test that they match.
    auto powerStatus2 = TItemPowerStatus::Ptr(new TItemPowerStatus());
    string ps1Str = to_string(ps1Js);
    bool result = powerStatus2->parse(ps1Str);
    LDEBUG("powerStatus2=", powerStatus2->getJson());
    REQUIRE(result == true);
    REQUIRE(powerStatus2->getMotorVoltage().getVal() == mv);
    REQUIRE(powerStatus2->getMotorCurrent().getVal() == mc);
    REQUIRE(powerStatus2->getCommVoltage().getVal() == cv);
    REQUIRE(powerStatus2->getCommCurrent().getVal() == cc);

    // Set values of powerStatusRaw2 from powerStatusRaw1 and test that they match.
    auto powerStatusRaw2 = TItemPowerStatusRaw::Ptr(new TItemPowerStatusRaw());
    string psRaw1Str = to_string(psRaw1Js);
    bool rresult = powerStatusRaw2->parse(psRaw1Str);
    LDEBUG("powerStatusRaw2=", powerStatusRaw2->getJson());
    REQUIRE(rresult == true);
    REQUIRE(powerStatusRaw2->getMotorVoltage().getVal() == rmv);
    REQUIRE(powerStatusRaw2->getMotorCurrent().getVal() == rmc);
    REQUIRE(powerStatusRaw2->getCommVoltage().getVal() == rcv);
    REQUIRE(powerStatusRaw2->getCommCurrent().getVal() == rcc);

    // Test TItemTangentForce and TItemVectorDouble
    auto tangForceIn = TItemTangentForce::Ptr(new TItemTangentForce());
    vector<double> tfInLutGravity{0.6, -0.5, -0.4, 0.3, 0.2, -0.1};
    vector<double> tfInLutTemperature{};
    vector<double> tfInApplied{-6.0, -1.5, 3.4, 7.3, 9.2, -2.1};
    vector<double> tfInMeasured{8.6, -3.5, -1.4, 9.3, 4.2, -5.1};
    vector<double> tfInHardpointCorrection{9.6, -7.5, -2.4, 6.3, 1.2, -7.1};
    tangForceIn->getLutGravity().setVals(tfInLutGravity);
    tangForceIn->getLutTemperature().setVals(tfInLutTemperature);
    tangForceIn->getApplied().setVals(tfInApplied);
    tangForceIn->getMeasured().setVals(tfInMeasured);
    tangForceIn->getHardpointCorrection().setVals(tfInHardpointCorrection);
    auto tangFInJs = tangForceIn->getJson();
    string tangFInStr = to_string(tangFInJs);
    LDEBUG("tangForceIn=", tangFInStr);
    REQUIRE(tangForceIn->getLutGravity().getVals() == tfInLutGravity);
    REQUIRE(tangForceIn->getLutTemperature().getVals() == tfInLutTemperature);
    REQUIRE(tangForceIn->getApplied().getVals() == tfInApplied);
    REQUIRE(tangForceIn->getMeasured().getVals() == tfInMeasured);
    REQUIRE(tangForceIn->getHardpointCorrection().getVals() == tfInHardpointCorrection);
    REQUIRE(tangForceIn->getLutTemperature().getVals() != tfInHardpointCorrection);
    REQUIRE(tangForceIn->getApplied().getVals() != tfInMeasured);

    // Create a new TangentForce object that is different that tangForceIn
    // to check comparisons.
    auto tangForceOut = TItemTangentForce::Ptr(new TItemTangentForce());
    REQUIRE(tangForceIn->compareItem(*tangForceOut) == false);
    REQUIRE(tangForceIn->compareItem(*powerStatusRaw2) == false);
    // Set the new object to the old object using json and check that they match
    bool tfResult = tangForceOut->parse(tangFInStr);
    REQUIRE(tfResult);
    REQUIRE(tangForceIn->compareItem(*tangForceOut) == true);

    // Test TItemAxialActuatorSteps as it contains a TItemVectorInt item
    auto axialActuatorStepsIn = TItemAxialActuatorSteps::Ptr(new TItemAxialActuatorSteps());
    vector<int> aaSteps;
    for(int j=0; j<72; ++j) {
        aaSteps.push_back(j);
    }
    axialActuatorStepsIn->getSteps().setVals(aaSteps);
    auto axialActInJs = axialActuatorStepsIn->getJson();
    string axialActInStr = to_string(axialActInJs);
    LDEBUG("axialActInStr=", axialActInStr);
    // Make a new  object and verify that they are different
    auto axialActuatorStepsOut = TItemAxialActuatorSteps::Ptr(new TItemAxialActuatorSteps());
    REQUIRE(axialActuatorStepsOut->compareItem(*axialActuatorStepsIn) == false);
    // Set the new object to the old object using json and check that they match
    bool aaResult = axialActuatorStepsOut->parse(axialActInStr);
    REQUIRE(aaResult);
    REQUIRE(axialActuatorStepsIn->compareItem(*axialActuatorStepsOut) == true);

    // Test TItemBoolean as memeber of TItemM2AssemblyInPosition
    auto m2AssemblyInPositionA = TItemM2AssemblyInPosition::Ptr(new TItemM2AssemblyInPosition());

    bool inPosA = true;
    m2AssemblyInPositionA->getInPosition().setVal(inPosA);

    REQUIRE(m2AssemblyInPositionA->getInPosition().getVal() == inPosA);
    auto m2AssembInPosAJs = m2AssemblyInPositionA->getJson();
    LDEBUG("m2AssemblyInPositionA=", m2AssembInPosAJs);


    // Set values of m2AssemblyInPositionB from m2AssemblyInPositionA and test that they match.
    auto m2AssemblyInPositionB = TItemM2AssemblyInPosition::Ptr(new TItemM2AssemblyInPosition());
    LDEBUG("m2AssemblyInPositionB=", m2AssemblyInPositionB->getJson());
    REQUIRE(m2AssemblyInPositionB->compareItem(*m2AssemblyInPositionA) == false);
    string m2AssembInPosAStr = to_string(m2AssembInPosAJs);
    bool m2AInPosResult = m2AssemblyInPositionB->parse(m2AssembInPosAStr);
    LDEBUG("m2AssemblyInPositionB=", m2AssemblyInPositionB->getJson());
    REQUIRE(m2AInPosResult == true);
    REQUIRE(m2AssemblyInPositionB->getInPosition().getVal() == inPosA);
    REQUIRE(m2AssemblyInPositionB->compareItem(*m2AssemblyInPositionA) == true);

    LDEBUG("TelemetryItem::test() end");
}

TEST_CASE("Test TelemetryCom", "[TelemetryCom]") {
    LINFO("Creating serv");
    int const port = 10081;
    auto servTelemetryMap = TelemetryMap::Ptr(new TelemetryMap());
    auto serv = TelemetryCom::create(servTelemetryMap, port);

    serv->startServer();
    REQUIRE(serv->waitForServerRunning(5) == true);

    servTelemetryMap->getPowerStatus()->getMotorVoltage().setVal(-3.0);
    servTelemetryMap->getPowerStatus()->getMotorCurrent().setVal(4.0);
    servTelemetryMap->getPowerStatus()->getCommVoltage().setVal(-5.0);
    servTelemetryMap->getPowerStatus()->getCommCurrent().setVal(6.0);

    servTelemetryMap->getPowerStatusRaw()->getMotorVoltage().setVal(17.0);
    servTelemetryMap->getPowerStatusRaw()->getMotorCurrent().setVal(27.0);
    servTelemetryMap->getPowerStatusRaw()->getCommVoltage().setVal(30.0);
    servTelemetryMap->getPowerStatusRaw()->getCommCurrent().setVal(25.0);

    vector<double> tfInLutGravity{0.6, -0.5, -0.4, 0.3, 0.2, -0.1};
    vector<double> tfInLutTemperature{};
    vector<double> tfInApplied{-6.0, -1.5, 3.4, 7.3, 9.2, -2.1};
    vector<double> tfInMeasured{8.6, -3.5, -1.4, 9.3, 4.2, -5.1};
    vector<double> tfInHardpointCorrection{9.6, -7.5, -2.4, 6.3, 1.2, -7.1};
    servTelemetryMap->getTangentForce()->getLutGravity().setVals(tfInLutGravity);
    servTelemetryMap->getTangentForce()->getLutTemperature().setVals(tfInLutTemperature);
    servTelemetryMap->getTangentForce()->getApplied().setVals(tfInApplied);
    servTelemetryMap->getTangentForce()->getMeasured().setVals(tfInMeasured);
    servTelemetryMap->getTangentForce()->getHardpointCorrection().setVals(tfInHardpointCorrection);

    servTelemetryMap->getForceBalance()->getFx().setVal(3.0);
    servTelemetryMap->getForceBalance()->getFy().setVal(7.0);
    servTelemetryMap->getForceBalance()->getFz().setVal(2.0);
    servTelemetryMap->getForceBalance()->getMx().setVal(6.0);
    servTelemetryMap->getForceBalance()->getMy().setVal(9.0);
    servTelemetryMap->getForceBalance()->getMz().setVal(4.0);

    servTelemetryMap->getPosition()->getX().setVal(6.0);
    servTelemetryMap->getPosition()->getY().setVal(5.0);
    servTelemetryMap->getPosition()->getZ().setVal(3.0);
    servTelemetryMap->getPosition()->getXRot().setVal(1.0);
    servTelemetryMap->getPosition()->getYRot().setVal(7.0);
    servTelemetryMap->getPosition()->getZRot().setVal(9.0);

    servTelemetryMap->getPositionIMS()->getX().setVal(2.0);
    servTelemetryMap->getPositionIMS()->getY().setVal(5.0);
    servTelemetryMap->getPositionIMS()->getZ().setVal(8.0);
    servTelemetryMap->getPositionIMS()->getXRot().setVal(1.0);
    servTelemetryMap->getPositionIMS()->getYRot().setVal(7.0);
    servTelemetryMap->getPositionIMS()->getZRot().setVal(4.0);

    servTelemetryMap->getTemperature()->getRing().setVals({12.0, 11.0, 10.0, 9.0, 8.0, 7.0, 6.0, 5.0, 4.0, 3.0, 2.0, 1.0});
    servTelemetryMap->getTemperature()->getIntake().setVals({6.0, 2.0});
    servTelemetryMap->getTemperature()->getExhaust().setVals({27.0, 9.0});

    servTelemetryMap->getZenithAngle()->getMeasured().setVal(13.0);
    servTelemetryMap->getZenithAngle()->getInclinometerRaw().setVal(19.0);
    servTelemetryMap->getZenithAngle()->getInclinometerProcessed().setVal(5.0);

    vector<double> posVectIn;
    vector<int> stepVectIn;
    for(int j=1; j<=72; ++j) {
        posVectIn.push_back(j);
        stepVectIn.push_back(j - 4);
    }
    servTelemetryMap->getAxialEncoderPositions()->getPosition().setVals(posVectIn);

    servTelemetryMap->getTangentEncoderPositions()->getPosition().setVals({11.0, 10.0, 9.0, 8.0, 7.0, 6.0, 5.0, 4.0, 3.0, 2.0, 1.0, 12.0});

    servTelemetryMap->getAxialActuatorSteps()->getSteps().setVals(stepVectIn);

    servTelemetryMap->getTangentActuatorSteps()->getSteps().setVals({5, 2, 3, 1, 6, 4});

    servTelemetryMap->getForceErrorTangent()->getForce().setVals({21.0, 22.0, -23.0, 24.0, 25.0, 26.0});
    servTelemetryMap->getForceErrorTangent()->getWeight().setVal(8.0);
    servTelemetryMap->getForceErrorTangent()->getSum().setVal(99.0);

    servTelemetryMap->getInclinometerAngleTma()->getInclinometer().setVal(4.0);

    servTelemetryMap->getM2AssemblyInPosition()->getInPosition().setVal(true);

    servTelemetryMap->getDisplacementSensors()->getThetaZ().setVals({6.1, 5.2, 4.3, 3.4, 2.5, 1.6});
    servTelemetryMap->getDisplacementSensors()->getDeltaZ().setVals({6.0, 5.0, 4.0, 3.0, 2.5, 1.4});

    servTelemetryMap->getIlcData()->getStatus().setVals(
        {7.8, 7.7, 7.6, 7.5, 7.4, 7.3, 7.2, 7.1,
         6.0, .9, .8, .7, .6, .5, .4, .3, .2, .1,
         5.0, .9, .8, .7, .6, .5, .4, .3, .2, .1,
         4.0, .9, .8, .7, .6, .5, .4, .3, .2, .1,
         3.0, .9, .8, .7, .6, .5, .4, .3, .2, .1,
         2.0, .9, .8, .7, .6, .5, .4, .3, .2, .1,
         1.0, .9, .8, .7, .6, .5, .4, .3, .2, .1 });

    servTelemetryMap->getNetForcesTotal()->getFx().setVal(9.0);
    servTelemetryMap->getNetForcesTotal()->getFy().setVal(5.0);
    servTelemetryMap->getNetForcesTotal()->getFz().setVal(8.0);

    servTelemetryMap->getNetMomentsTotal()->getMx().setVal(3.0);
    servTelemetryMap->getNetMomentsTotal()->getMy().setVal(7.0);
    servTelemetryMap->getNetMomentsTotal()->getMz().setVal(1.0);

    servTelemetryMap->getAxialForce()->getLutGravity().setVals(
        {3.5, 4.3, 0.3, 0.7, 3.0, 1.9, 4.3, 0.3, 0.7, 3.1,
         3.4, 4.1, 0.3, 0.7, 3.2, 1.9, 3.9, 0.4, 0.7, 3.2,
         3.2, 3.8, 0.4, 0.7, 3.1, 1.9, 4.0, 0.3, 0.7, 3.1,
         0.5, 4.8, 4.6, 0.5, 0.5, 4.8, 4.7, 0.4, 0.6, 4.7,
         4.7, 0.3, 0.7, 4.7, 4.7, 0.2, 0.6, 4.7, 4.7, 0.3,
         0.5, 4.7, 4.7, 0.5, 0.2, 2.3, 2.2, 0.2, 2.3, 2.3,
         0.2, 2.3, 2.3, 0.2, 2.2, 2.4, 0.1, 2.2, 2.3, 0.1,
         2.2, 2.2});

    servTelemetryMap->getAxialForce()->getLutTemperature().setVals(
        {3.5, 4.3, 0.3, 0.7, 3.0, 1.9, 4.3, 0.3, 0.7, 3.1,
         3.4, 4.1, 0.3, 0.7, 3.2, 1.9, 3.9, 0.4, 0.7, 3.2,
         3.2, 9.8, 0.4, 0.7, 3.1, 1.9, 4.0, 0.3, 0.7, 3.1,
         0.5, 4.8, 4.6, 3.5, 0.5, 4.8, 4.7, 0.4, 0.6, 4.7,
         4.7, 0.3, 0.7, 4.7, 4.7, 0.2, 0.6, 4.7, 4.7, 0.3,
         0.5, 4.7, 4.7, 0.5, 0.2, 2.3, 2.2, 0.2, 2.3, 2.3,
         0.2, 2.3, 2.3, 0.2, 2.2, 2.4, 0.1, 2.2, 2.3, 0.1,
         5.2, 2.2});

    servTelemetryMap->getAxialForce()->getApplied().setVals(
        {3.5, 4.3, 0.3, 0.7, 3.0, 1.9, 4.3, 0.3, 0.7, 3.1,
         3.4, 4.1, 0.3, 0.7, 3.2, 1.9, 3.9, 0.4, 0.7, 3.2,
         3.2, 3.8, 0.4, 0.7, 3.1, 1.9, 4.0, 0.3, 0.7, 3.1,
         0.5, 4.8, 4.6, 0.5, 0.5, 4.8, 4.7, 0.4, 0.6, 4.7,
         4.7, 0.3, 0.7, 4.7, 4.7, 0.2, 0.6, 4.7, 4.7, 0.3,
         3.5, 4.7, 4.7, 0.5, 0.2, 2.3, 2.2, 0.2, 2.3, 2.3,
         0.2, 2.3, 2.3, 0.2, 2.2, 2.4, 0.1, 2.2, 2.3, 0.1,
         2.2, 2.2});

    servTelemetryMap->getAxialForce()->getMeasured().setVals(
        {3.5, 4.3, 0.3, 0.7, 3.0, 1.9, 4.3, 0.3, 0.7, 3.1,
         3.4, 4.1, 0.3, 0.7, 3.2, 1.9, 3.9, 0.4, 0.7, 3.2,
         3.2, 3.8, 0.4, 0.7, 3.1, 1.9, 4.0, 0.3, 0.7, 3.1,
         0.5, 4.8, 4.6, 0.5, 0.5, 4.8, 4.7, 0.4, 0.6, 4.7,
         4.7, 0.3, 0.7, 4.7, 4.7, 0.2, 0.6, 4.7, 4.7, 0.3,
         0.5, 4.7, 4.7, 0.5, 0.2, 2.3, 2.2, 0.2, 2.3, 2.3,
         8.2, 2.3, 2.3, 0.2, 2.2, 2.4, 0.1, 2.2, 2.3, 0.1,
         2.2, 2.2});

    servTelemetryMap->getAxialForce()->getHardpointCorrection().setVals(
        {3.5, 4.3, 0.3, 0.7, 3.0, 1.9, 4.3, 0.3, 0.7, 3.1,
         3.4, 4.1, 0.3, 0.7, 3.2, 1.9, 3.9, 0.4, 0.7, 3.2,
         3.2, 3.8, 0.4, 0.7, 3.1, 1.9, 4.0, 0.3, 0.7, 3.1,
         0.5, 4.8, 4.6, 0.5, 0.5, 4.8, 4.7, 0.4, 0.6, 4.7,
         0.7, 0.3, 0.7, 4.7, 4.7, 0.2, 0.6, 4.7, 4.7, 0.3,
         0.5, 4.7, 4.7, 0.5, 0.2, 2.3, 2.2, 0.2, 2.3, 2.3,
         0.2, 2.3, 2.3, 0.2, 2.2, 2.4, 0.1, 2.2, 2.3, 0.1,
         2.2, 2.2});

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

