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
#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_session.hpp>

// Project headers
#include "control/FpgaIo.h"
#include "control/PowerSubsystem.h"
#include "control/PowerSystem.h"
#include "faultmgr/FaultMgr.h"
#include "simulator/SimCore.h"
#include "system/Config.h"
#include "system/Globals.h"
#include "util/Log.h"

using namespace std;
using namespace LSST::m2cellcpp::control;
using namespace LSST::m2cellcpp;



TEST_CASE("Test PowerSystem", "[PowerSystem]") {
    util::Log::getLog().useEnvironmentLogLvl();
    string cfgPath = system::Config::getEnvironmentCfgPath("../configs");
    system::Config::setup(cfgPath + "unitTestCfg.yaml");
    auto& config = LSST::m2cellcpp::system::Config::get();
    system::Globals::setup(config);

    // Approximate minimum voltage fault value.
    const double approxMinVoltageFault = 21.0;

    // Voltage off value
    const double voltageOffLevel = 12.0;

    // Number of iterations to wait for telemetry count.
    const int telemWait = 10;

    // Number of iterations to wait for simulator.
    const int simWait = 3;

    // Number of iterations for long wait for simulator.
    const int simWaitLong = 25;

    simulator::SimCore::Ptr simCore(new simulator::SimCore());

    faultmgr::FaultMgr::setup();

    LDEBUG("test_SimCore start");
    simCore->start();

    while (simCore->getIterations() < 1) {
        this_thread::sleep_for(0.1s);
    }

    {
        FpgaIo::setup(simCore);

        PowerSystem::Ptr powerSys = PowerSystem::Ptr(new PowerSystem());

        PowerSubsystem& motorSubSys = powerSys->getMotor();

        PowerSubsystem& commSubSys = powerSys->getComm();

        REQUIRE(motorSubSys.getSystemType() == MOTOR);
        REQUIRE(commSubSys.getSystemType() == COMM);

        // Give some time for threads to startup.
        simCore->waitForUpdate(2);
        this_thread::sleep_for(0.5s);

        // motor
        // verify power off
        SysInfo sInfo = simCore->getSysInfo();
        LDEBUG(sInfo.dump());
        REQUIRE(sInfo.outputPort.getBitAtPos(OutputPortBits::MOTOR_POWER_ON) == 0);
        REQUIRE(sInfo.outputPort.getBitAtPos(OutputPortBits::RESET_MOTOR_BREAKERS) != 0);
        REQUIRE(sInfo.outputPort.getBitAtPos(OutputPortBits::ILC_COMM_POWER_ON) == 0);
        REQUIRE(sInfo.outputPort.getBitAtPos(OutputPortBits::RESET_COMM_BREAKERS) != 0);
        REQUIRE(sInfo.motorVoltage < voltageOffLevel); // _voltageOffLevel
        REQUIRE(motorSubSys.getTargPowerState() == PowerSubsystem::OFF);
        simCore->waitForUpdate(telemWait);
        REQUIRE(motorSubSys.getActualPowerState() == PowerSubsystem::UNKNOWN);

        // Register the PowerSystem with FpgaIo so that it gets updates.
        FpgaIo::get().registerPowerSys(powerSys);
        powerSys->writeCrioInterlockEnable(true);
        simCore->waitForUpdate(telemWait);
        REQUIRE(motorSubSys.getActualPowerState() == PowerSubsystem::OFF);

        // turn power on
        LDEBUG("Turn motor power on!!!");
        motorSubSys.setPowerOn();
        simCore->waitForUpdate(simWait);
        sInfo = simCore->getSysInfo();
        REQUIRE(sInfo.outputPort.getBitAtPos(OutputPortBits::MOTOR_POWER_ON) != 0);
        REQUIRE(sInfo.outputPort.getBitAtPos(OutputPortBits::RESET_MOTOR_BREAKERS) != 0);
        REQUIRE(sInfo.outputPort.getBitAtPos(OutputPortBits::ILC_COMM_POWER_ON) == 0);
        REQUIRE(sInfo.outputPort.getBitAtPos(OutputPortBits::RESET_COMM_BREAKERS) != 0);


        // verify power on
        simCore->waitForUpdate(telemWait); // wait for telemetry count
        this_thread::sleep_for(1.0s);// Need to wait for voltage rise and phases.
        sInfo = simCore->getSysInfo();
        REQUIRE(sInfo.outputPort.getBitAtPos(OutputPortBits::MOTOR_POWER_ON) != 0);
        REQUIRE(sInfo.outputPort.getBitAtPos(OutputPortBits::RESET_MOTOR_BREAKERS) != 0);
        REQUIRE(sInfo.motorVoltage > approxMinVoltageFault);
        LDEBUG("Test motor power on!!!");
        REQUIRE(motorSubSys.getActualPowerState() == PowerSubsystem::ON);

        // turn power off
        motorSubSys.setPowerOff("test motor power off");
        simCore->waitForUpdate(5);
        sInfo = simCore->getSysInfo();
        REQUIRE(sInfo.outputPort.getBitAtPos(OutputPortBits::MOTOR_POWER_ON) == 0);
        REQUIRE(sInfo.outputPort.getBitAtPos(OutputPortBits::RESET_MOTOR_BREAKERS) != 0);
        REQUIRE(sInfo.outputPort.getBitAtPos(OutputPortBits::ILC_COMM_POWER_ON) == 0);
        REQUIRE(sInfo.outputPort.getBitAtPos(OutputPortBits::RESET_COMM_BREAKERS) != 0);

        // verify power off
        simCore->waitForUpdate(simWaitLong); // need to wait for voltage fall
        sInfo = simCore->getSysInfo();
        REQUIRE(sInfo.outputPort.getBitAtPos(OutputPortBits::MOTOR_POWER_ON) == 0);
        REQUIRE(sInfo.outputPort.getBitAtPos(OutputPortBits::RESET_MOTOR_BREAKERS) != 0);
        REQUIRE(sInfo.motorVoltage < voltageOffLevel); // _voltageOffLevel
        REQUIRE(motorSubSys.getActualPowerState() == PowerSubsystem::OFF);

        // comm
        // turn power on
        LDEBUG("Turn comm power on!!!");
        commSubSys.setPowerOn();
        simCore->waitForUpdate(simWait);
        sInfo = simCore->getSysInfo();
        REQUIRE(sInfo.outputPort.getBitAtPos(OutputPortBits::MOTOR_POWER_ON) == 0);
        REQUIRE(sInfo.outputPort.getBitAtPos(OutputPortBits::RESET_MOTOR_BREAKERS) != 0);
        REQUIRE(sInfo.outputPort.getBitAtPos(OutputPortBits::ILC_COMM_POWER_ON) != 0);
        REQUIRE(sInfo.outputPort.getBitAtPos(OutputPortBits::RESET_COMM_BREAKERS) != 0);

        // verify power on
        simCore->waitForUpdate(telemWait); // wait for telemetry count
        this_thread::sleep_for(1.0s);// Need to wait for voltage rise and phases.
        sInfo = simCore->getSysInfo();
        REQUIRE(sInfo.outputPort.getBitAtPos(OutputPortBits::ILC_COMM_POWER_ON) != 0);
        REQUIRE(sInfo.outputPort.getBitAtPos(OutputPortBits::RESET_COMM_BREAKERS) != 0);
        REQUIRE(sInfo.commVoltage > approxMinVoltageFault); // approximate _minVoltageFault
        REQUIRE(commSubSys.getActualPowerState() == PowerSubsystem::ON);

        // turn power off
        commSubSys.setPowerOff("test comm power off");
        simCore->waitForUpdate(simWait);
        sInfo = simCore->getSysInfo();
        REQUIRE(sInfo.outputPort.getBitAtPos(OutputPortBits::MOTOR_POWER_ON) == 0);
        REQUIRE(sInfo.outputPort.getBitAtPos(OutputPortBits::RESET_MOTOR_BREAKERS) != 0);
        REQUIRE(sInfo.outputPort.getBitAtPos(OutputPortBits::ILC_COMM_POWER_ON) == 0);
        REQUIRE(sInfo.outputPort.getBitAtPos(OutputPortBits::RESET_COMM_BREAKERS) != 0);
        REQUIRE(commSubSys.getTargPowerState() == PowerSubsystem::OFF);

        // verify power off
        simCore->waitForUpdate(simWaitLong); // need to wait for voltage fall
        sInfo = simCore->getSysInfo();
        REQUIRE(sInfo.outputPort.getBitAtPos(OutputPortBits::ILC_COMM_POWER_ON) == 0);
        REQUIRE(sInfo.outputPort.getBitAtPos(OutputPortBits::RESET_MOTOR_BREAKERS) != 0);
        REQUIRE(sInfo.commVoltage < voltageOffLevel); // _voltageOffLevel
        REQUIRE(commSubSys.getActualPowerState() == PowerSubsystem::OFF);


        // Test breaker reset
        // turn power on
        motorSubSys.setPowerOn();
        simCore->waitForUpdate(simWait);
        sInfo = simCore->getSysInfo();
        REQUIRE(sInfo.outputPort.getBitAtPos(OutputPortBits::MOTOR_POWER_ON) != 0);
        REQUIRE(sInfo.outputPort.getBitAtPos(OutputPortBits::RESET_MOTOR_BREAKERS) != 0);

        // verify power on
        simCore->waitForUpdate(telemWait); // wait for telemetry count
        this_thread::sleep_for(1.0s);// Need to wait for voltage rise and phases.
        sInfo = simCore->getSysInfo();
        REQUIRE(sInfo.outputPort.getBitAtPos(OutputPortBits::MOTOR_POWER_ON) != 0);
        REQUIRE(sInfo.outputPort.getBitAtPos(OutputPortBits::RESET_MOTOR_BREAKERS) != 0);
        REQUIRE(sInfo.motorVoltage > approxMinVoltageFault);
        REQUIRE(motorSubSys.getActualPowerState() == PowerSubsystem::ON);

        // Trip one breaker, power should remain on &&&
        simCore->writeInputPortBit(control::InputPortBits::J2_W10_1_MTR_PWR_BRKR_OK, true);

        // verify power on &&&
        simCore->waitForUpdate(15);
        sInfo = simCore->getSysInfo();
        REQUIRE(sInfo.outputPort.getBitAtPos(OutputPortBits::MOTOR_POWER_ON) != 0);
        REQUIRE(sInfo.outputPort.getBitAtPos(OutputPortBits::RESET_MOTOR_BREAKERS) != 0);
        REQUIRE(sInfo.motorVoltage > approxMinVoltageFault); // approximate _minVoltageFault
        REQUIRE(motorSubSys.getActualPowerState() == PowerSubsystem::ON);
        // FUTURE: DM-??? check for FaultMgr warning &&&

        // Trip a second breaker, power should go off
        simCore->writeInputPortBit(control::InputPortBits::J2_W10_3_MTR_PWR_BRKR_OK, true);

        // verify power off
        simCore->waitForUpdate(simWaitLong); // need to wait for voltage rise
        sInfo = simCore->getSysInfo();
        REQUIRE(sInfo.outputPort.getBitAtPos(OutputPortBits::MOTOR_POWER_ON) == 0);
        REQUIRE(sInfo.outputPort.getBitAtPos(OutputPortBits::RESET_MOTOR_BREAKERS) != 0);
        REQUIRE(sInfo.motorVoltage < voltageOffLevel); // _voltageOffLevel
        REQUIRE(motorSubSys.getActualPowerState() == PowerSubsystem::OFF);


        // Try to turn on, this should result in reset logic being used &&&


        // brief wait for reset mode &&&

        // clear breaker issues &&&

        // wait for power to turn on &&&


    }

    LDEBUG("test_PowerSystem simCore stop");
    simCore->stop();

    LDEBUG("test_PowerSystem simCore joining");
    simCore->join();
    LINFO("test_PowerSystem done");
}
