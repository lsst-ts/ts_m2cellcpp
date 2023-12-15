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
#include "control/Context.h"
#include "control/FpgaIo.h"
#include "control/MotionEngine.h"
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

    // Need to wait for voltage rise and phases.
    const chrono::duration voltageRiseWait = 1.0s;

    simulator::SimCore::Ptr simCore(new simulator::SimCore());
    faultmgr::FaultMgr::setup();
    control::FpgaIo::setup(simCore);
    control::MotionEngine::setup();
    control::Context::setup();

    LDEBUG("test_SimCore start");
    simCore->start();

    control::Context::Ptr context = control::Context::get();
    context->model.ctrlSetup();

    // At this point, MotionCtrl and FpgaCtrl should be configured. Start the control loops
    context->model.ctrlStart();

    while (simCore->getIterations() < 1) {
        this_thread::sleep_for(0.1s);
    }

    {
        PowerSystem::Ptr powerSys = control::Context::get()->model.getPowerSystem();
        PowerSubsystem& motorSubSys = powerSys->getMotor();
        PowerSubsystem& commSubSys = powerSys->getComm();

        // FUTURE: TODO: put this item into the configuration (or where does it really belong?)
        powerSys->writeCrioInterlockEnable(true);

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
        REQUIRE(sInfo.motorVoltage < voltageOffLevel);  // _voltageOffLevel
        REQUIRE(motorSubSys.getTargPowerState() == PowerState::OFF);
        simCore->waitForUpdate(telemWait);
        REQUIRE(motorSubSys.getActualPowerState() == PowerState::OFF);

        // Try to turn power on with any network connections
        LDEBUG("Turn motor power on, expected failure due to no network connections");
        motorSubSys.setPowerOn();
        simCore->waitForUpdate(simWait);
        sInfo = simCore->getSysInfo();
        REQUIRE(sInfo.outputPort.getBitAtPos(OutputPortBits::MOTOR_POWER_ON) == 0);

        // Pretend there's at least one TCP/IP connection so the
        // power can be turned on.
        faultmgr::FaultMgr::get().reportComConnectionCount(1);

        // comm,  turn power on
        LDEBUG("Turn comm power on!!!");
        commSubSys.setPowerOn();
        simCore->waitForUpdate(simWait);
        sInfo = simCore->getSysInfo();
        REQUIRE(sInfo.outputPort.getBitAtPos(OutputPortBits::MOTOR_POWER_ON) == 0);
        REQUIRE(sInfo.outputPort.getBitAtPos(OutputPortBits::RESET_MOTOR_BREAKERS) != 0);
        REQUIRE(sInfo.outputPort.getBitAtPos(OutputPortBits::ILC_COMM_POWER_ON) != 0);
        REQUIRE(sInfo.outputPort.getBitAtPos(OutputPortBits::RESET_COMM_BREAKERS) != 0);

        // comm, verify power on
        simCore->waitForUpdate(telemWait);        // wait for telemetry count
        this_thread::sleep_for(voltageRiseWait);  // Need to wait for voltage rise and phases.
        sInfo = simCore->getSysInfo();
        REQUIRE(sInfo.outputPort.getBitAtPos(OutputPortBits::ILC_COMM_POWER_ON) != 0);
        REQUIRE(sInfo.outputPort.getBitAtPos(OutputPortBits::RESET_COMM_BREAKERS) != 0);
        REQUIRE(sInfo.commVoltage > approxMinVoltageFault);  // approximate _minVoltageFault
        REQUIRE(commSubSys.getActualPowerState() == PowerState::ON);

        // motor, turn power on
        // motor power can only be turned on if comm power is already on.
        LDEBUG("Turn motor power on!!!");
        motorSubSys.setPowerOn();
        simCore->waitForUpdate(simWait);
        sInfo = simCore->getSysInfo();
        REQUIRE(sInfo.outputPort.getBitAtPos(OutputPortBits::MOTOR_POWER_ON) != 0);
        REQUIRE(sInfo.outputPort.getBitAtPos(OutputPortBits::RESET_MOTOR_BREAKERS) != 0);
        REQUIRE(sInfo.outputPort.getBitAtPos(OutputPortBits::ILC_COMM_POWER_ON) != 0);
        REQUIRE(sInfo.outputPort.getBitAtPos(OutputPortBits::RESET_COMM_BREAKERS) != 0);

        // motor, verify power on
        simCore->waitForUpdate(telemWait);        // wait for telemetry count
        this_thread::sleep_for(voltageRiseWait);  // Need to wait for voltage rise and phases.
        sInfo = simCore->getSysInfo();
        LDEBUG("Verify motor power on!!!");
        REQUIRE(sInfo.outputPort.getBitAtPos(OutputPortBits::MOTOR_POWER_ON) != 0);
        REQUIRE(sInfo.outputPort.getBitAtPos(OutputPortBits::RESET_MOTOR_BREAKERS) != 0);
        REQUIRE(sInfo.motorVoltage > approxMinVoltageFault);
        LDEBUG("Test motor power on!!!");
        REQUIRE(motorSubSys.getActualPowerState() == PowerState::ON);

        // motor, turn power off
        motorSubSys.setPowerOff("test motor power off");
        simCore->waitForUpdate(5);
        sInfo = simCore->getSysInfo();
        REQUIRE(sInfo.outputPort.getBitAtPos(OutputPortBits::MOTOR_POWER_ON) == 0);
        REQUIRE(sInfo.outputPort.getBitAtPos(OutputPortBits::RESET_MOTOR_BREAKERS) != 0);
        REQUIRE(sInfo.outputPort.getBitAtPos(OutputPortBits::ILC_COMM_POWER_ON) != 0);
        REQUIRE(sInfo.outputPort.getBitAtPos(OutputPortBits::RESET_COMM_BREAKERS) != 0);

        // verify power off
        simCore->waitForUpdate(simWaitLong);  // need to wait for voltage fall
        sInfo = simCore->getSysInfo();
        REQUIRE(sInfo.outputPort.getBitAtPos(OutputPortBits::MOTOR_POWER_ON) == 0);
        REQUIRE(sInfo.outputPort.getBitAtPos(OutputPortBits::RESET_MOTOR_BREAKERS) != 0);
        REQUIRE(sInfo.motorVoltage < voltageOffLevel);  // _voltageOffLevel
        REQUIRE(motorSubSys.getActualPowerState() == PowerState::OFF);

        // comm, turn power off
        commSubSys.setPowerOff("test comm power off");
        simCore->waitForUpdate(simWait);
        sInfo = simCore->getSysInfo();
        REQUIRE(sInfo.outputPort.getBitAtPos(OutputPortBits::MOTOR_POWER_ON) == 0);
        REQUIRE(sInfo.outputPort.getBitAtPos(OutputPortBits::RESET_MOTOR_BREAKERS) != 0);
        REQUIRE(sInfo.outputPort.getBitAtPos(OutputPortBits::ILC_COMM_POWER_ON) == 0);
        REQUIRE(sInfo.outputPort.getBitAtPos(OutputPortBits::RESET_COMM_BREAKERS) != 0);
        REQUIRE(commSubSys.getTargPowerState() == PowerState::OFF);

        // verify power off
        simCore->waitForUpdate(simWaitLong);  // need to wait for voltage fall
        sInfo = simCore->getSysInfo();
        REQUIRE(sInfo.outputPort.getBitAtPos(OutputPortBits::ILC_COMM_POWER_ON) == 0);
        REQUIRE(sInfo.outputPort.getBitAtPos(OutputPortBits::RESET_MOTOR_BREAKERS) != 0);
        REQUIRE(sInfo.commVoltage < voltageOffLevel);  // _voltageOffLevel
        REQUIRE(commSubSys.getActualPowerState() == PowerState::OFF);

        // comm,  turn power on as motor power tests require comm power on.
        LDEBUG("Turn comm power on so motor breakers can be tested");
        commSubSys.setPowerOn();
        simCore->waitForUpdate(simWait);
        sInfo = simCore->getSysInfo();
        REQUIRE(sInfo.outputPort.getBitAtPos(OutputPortBits::MOTOR_POWER_ON) == 0);
        REQUIRE(sInfo.outputPort.getBitAtPos(OutputPortBits::RESET_MOTOR_BREAKERS) != 0);
        REQUIRE(sInfo.outputPort.getBitAtPos(OutputPortBits::ILC_COMM_POWER_ON) != 0);
        REQUIRE(sInfo.outputPort.getBitAtPos(OutputPortBits::RESET_COMM_BREAKERS) != 0);

        // comm, verify power on
        simCore->waitForUpdate(telemWait);        // wait for telemetry count
        this_thread::sleep_for(voltageRiseWait);  // Need to wait for voltage rise and phases.
        sInfo = simCore->getSysInfo();
        REQUIRE(sInfo.outputPort.getBitAtPos(OutputPortBits::ILC_COMM_POWER_ON) != 0);
        REQUIRE(sInfo.outputPort.getBitAtPos(OutputPortBits::RESET_COMM_BREAKERS) != 0);
        REQUIRE(sInfo.commVoltage > approxMinVoltageFault);  // approximate _minVoltageFault
        REQUIRE(commSubSys.getActualPowerState() == PowerState::ON);

        // Test breaker reset
        // turn power on
        motorSubSys.setPowerOn();
        simCore->waitForUpdate(simWait);
        sInfo = simCore->getSysInfo();
        REQUIRE(sInfo.outputPort.getBitAtPos(OutputPortBits::MOTOR_POWER_ON) != 0);
        REQUIRE(sInfo.outputPort.getBitAtPos(OutputPortBits::RESET_MOTOR_BREAKERS) != 0);

        // verify power on
        simCore->waitForUpdate(telemWait);
        this_thread::sleep_for(voltageRiseWait);
        sInfo = simCore->getSysInfo();
        REQUIRE(sInfo.outputPort.getBitAtPos(OutputPortBits::MOTOR_POWER_ON) != 0);
        REQUIRE(sInfo.outputPort.getBitAtPos(OutputPortBits::RESET_MOTOR_BREAKERS) != 0);
        REQUIRE(sInfo.motorVoltage > approxMinVoltageFault);
        REQUIRE(motorSubSys.getActualPowerState() == PowerState::ON);

        // Trip one breaker, power should remain on
        LINFO("Trip J2_W10_1_MTR_PWR_BRKR_OK=", control::InputPortBits::J2_W10_1_MTR_PWR_BRKR_OK);
        simCore->writeInputPortBit(control::InputPortBits::J2_W10_1_MTR_PWR_BRKR_OK, false);

        // verify power on
        simCore->waitForUpdate(simWaitLong);
        simCore->waitForUpdate(simWaitLong);
        sInfo = simCore->getSysInfo();
        REQUIRE(sInfo.outputPort.getBitAtPos(OutputPortBits::MOTOR_POWER_ON) != 0);
        REQUIRE(sInfo.outputPort.getBitAtPos(OutputPortBits::RESET_MOTOR_BREAKERS) != 0);
        REQUIRE(sInfo.motorVoltage > approxMinVoltageFault);  // approximate _minVoltageFault
        REQUIRE(motorSubSys.getActualPowerState() == PowerState::ON);
        // FUTURE: DM-40909 check for FaultMgr warning

        // Trip a second breaker, power should go off
        LINFO("Trip J2_W10_3_MTR_PWR_BRKR_OK=", control::InputPortBits::J2_W10_3_MTR_PWR_BRKR_OK);
        simCore->writeInputPortBit(control::InputPortBits::J2_W10_3_MTR_PWR_BRKR_OK, false);

        // verify power off
        simCore->waitForUpdate(simWaitLong);  // need to wait for voltage fall
        sInfo = simCore->getSysInfo();
        REQUIRE(sInfo.outputPort.getBitAtPos(OutputPortBits::MOTOR_POWER_ON) == 0);
        REQUIRE(sInfo.outputPort.getBitAtPos(OutputPortBits::RESET_MOTOR_BREAKERS) != 0);
        REQUIRE(sInfo.motorVoltage < voltageOffLevel);  // _voltageOffLevel
        REQUIRE(motorSubSys.getActualPowerState() == PowerState::OFF);

        // Try to turn on, this should result in reset logic being used
        LDEBUG("Turning on for RESET test");
        motorSubSys.setPowerOn();
        simCore->waitForUpdate(simWait);
        sInfo = simCore->getSysInfo();
        REQUIRE(sInfo.outputPort.getBitAtPos(OutputPortBits::MOTOR_POWER_ON) != 0);

        // long wait for
        // voltage to rise high enough to read breaker fault.
        simCore->waitForUpdate(telemWait);
        this_thread::sleep_for(voltageRiseWait);

        // wait for reset (minimum of .4 seconds)
        this_thread::sleep_for(voltageRiseWait);

        // wait for power to turn on
        simCore->waitForUpdate(telemWait);
        this_thread::sleep_for(voltageRiseWait);
        // Verify motor power on
        sInfo = simCore->getSysInfo();
        REQUIRE(sInfo.outputPort.getBitAtPos(OutputPortBits::MOTOR_POWER_ON) != 0);
        REQUIRE(sInfo.outputPort.getBitAtPos(OutputPortBits::RESET_MOTOR_BREAKERS) != 0);
        REQUIRE(sInfo.motorVoltage > approxMinVoltageFault);  // approximate _minVoltageFault
        REQUIRE(motorSubSys.getActualPowerState() == PowerState::ON);

        // verify tripped breaker bit are back to ok
        REQUIRE(sInfo.inputPort.getBitAtPos(control::InputPortBits::J2_W10_3_MTR_PWR_BRKR_OK) == true);
        REQUIRE(sInfo.inputPort.getBitAtPos(control::InputPortBits::J2_W10_1_MTR_PWR_BRKR_OK) == true);

        // Remove the pretend TCP/IP connections and verify power goes off.
        faultmgr::FaultMgr::get().reportComConnectionCount(0);
        simCore->waitForUpdate(simWait);
        sInfo = simCore->getSysInfo();
        REQUIRE(sInfo.outputPort.getBitAtPos(OutputPortBits::MOTOR_POWER_ON) == 0);
        // verify power off
        simCore->waitForUpdate(simWaitLong);  // need to wait for voltage fall
        sInfo = simCore->getSysInfo();
        REQUIRE(sInfo.outputPort.getBitAtPos(OutputPortBits::MOTOR_POWER_ON) == 0);
        REQUIRE(sInfo.outputPort.getBitAtPos(OutputPortBits::RESET_MOTOR_BREAKERS) != 0);
        REQUIRE(sInfo.motorVoltage < voltageOffLevel);  // _voltageOffLevel
        REQUIRE(motorSubSys.getActualPowerState() == PowerState::OFF);

        /// Test for power off on over voltage.
        faultmgr::FaultMgr::get().reportComConnectionCount(1);  // need at least 1 TCP/IP
        LDEBUG("Turn power on for over voltage test.");
        commSubSys.setPowerOn();
        simCore->waitForUpdate(simWait);
        motorSubSys.setPowerOn();
        simCore->waitForUpdate(simWait);
        sInfo = simCore->getSysInfo();
        REQUIRE(sInfo.outputPort.getBitAtPos(OutputPortBits::MOTOR_POWER_ON) != 0);
        REQUIRE(sInfo.outputPort.getBitAtPos(OutputPortBits::RESET_MOTOR_BREAKERS) != 0);
        REQUIRE(sInfo.outputPort.getBitAtPos(OutputPortBits::ILC_COMM_POWER_ON) != 0);
        REQUIRE(sInfo.outputPort.getBitAtPos(OutputPortBits::RESET_COMM_BREAKERS) != 0);

        // verify power on
        simCore->waitForUpdate(telemWait);        // wait for telemetry count
        this_thread::sleep_for(voltageRiseWait);  // Need to wait for voltage rise and phases.
        sInfo = simCore->getSysInfo();
        REQUIRE(sInfo.outputPort.getBitAtPos(OutputPortBits::MOTOR_POWER_ON) != 0);
        REQUIRE(sInfo.outputPort.getBitAtPos(OutputPortBits::RESET_MOTOR_BREAKERS) != 0);
        REQUIRE(sInfo.motorVoltage > approxMinVoltageFault);
        LDEBUG("Test motor power on");
        REQUIRE(motorSubSys.getActualPowerState() == PowerState::ON);

        // Let motor power run high
        LDEBUG("Motor voltage too high test");
        simCore->getMotorSub()->forceOverVoltage(true);
        simCore->waitForUpdate(telemWait);        // wait for telemetry count
        this_thread::sleep_for(voltageRiseWait);  // Need to wait for voltage rise and phases.
        sInfo = simCore->getSysInfo();
        REQUIRE(sInfo.outputPort.getBitAtPos(OutputPortBits::MOTOR_POWER_ON) == 0);
        REQUIRE(motorSubSys.setPowerOn() == false);

        // verify power off
        simCore->waitForUpdate(simWaitLong);  // need to wait for voltage fall
        sInfo = simCore->getSysInfo();
        REQUIRE(sInfo.outputPort.getBitAtPos(OutputPortBits::MOTOR_POWER_ON) == 0);
        REQUIRE(sInfo.outputPort.getBitAtPos(OutputPortBits::RESET_MOTOR_BREAKERS) != 0);
        REQUIRE(sInfo.motorVoltage < voltageOffLevel);  // _voltageOffLevel
        REQUIRE(motorSubSys.getActualPowerState() == PowerState::OFF);

        // Reset fault
        simCore->getMotorSub()->forceOverVoltage(false);
        uint64_t tmap = 0;
        uint64_t const resetmap = ~tmap;
        faultmgr::FaultMgr::get().resetFaults(resetmap);
        REQUIRE(motorSubSys.setPowerOn() == true);

        // Let motor current run high
        LDEBUG("Motor current too high test");
        simCore->getMotorSub()->forceOverCurrent(true);
        simCore->waitForUpdate(telemWait);        // wait for telemetry count
        this_thread::sleep_for(voltageRiseWait);  // Need to wait for voltage rise and phases.
        sInfo = simCore->getSysInfo();
        REQUIRE(sInfo.outputPort.getBitAtPos(OutputPortBits::MOTOR_POWER_ON) == 0);
        REQUIRE(motorSubSys.setPowerOn() == false);

        // verify power off
        simCore->waitForUpdate(simWaitLong);  // need to wait for voltage fall
        sInfo = simCore->getSysInfo();
        REQUIRE(sInfo.outputPort.getBitAtPos(OutputPortBits::MOTOR_POWER_ON) == 0);
        REQUIRE(sInfo.outputPort.getBitAtPos(OutputPortBits::RESET_MOTOR_BREAKERS) != 0);
        REQUIRE(sInfo.motorVoltage < voltageOffLevel);  // _voltageOffLevel
        REQUIRE(motorSubSys.getActualPowerState() == PowerState::OFF);

        // Reset fault
        simCore->getMotorSub()->forceOverCurrent(false);
        faultmgr::FaultMgr::get().resetFaults(resetmap);
        REQUIRE(motorSubSys.setPowerOn() == true);
    }

    LDEBUG("test_PowerSystem simCore stop");
    simCore->stop();

    LDEBUG("test_PowerSystem simCore joining");
    simCore->join();
    LINFO("test_PowerSystem done");
}
