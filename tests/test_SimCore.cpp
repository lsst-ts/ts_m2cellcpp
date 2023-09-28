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
#include "control/PowerSubsystem.h"
#include "simulator/SimCore.h"
#include "system/Config.h"
#include "system/Globals.h"
#include "util/Log.h"

using namespace std;
using namespace LSST::m2cellcpp::simulator;
using namespace LSST::m2cellcpp::control;
using namespace LSST::m2cellcpp;

void testPowerOn(PowerSubsystemConfig const& psCfg, double voltage, double current,
        string const& note, bool& success, bool& done) {
    if (voltage > psCfg.getMinVoltageFault()) {
        if (voltage < psCfg.getMaxVoltageFault()) {
            if (current > psCfg.getMaxCurrentFault()) {
                LERROR(note, "Current=", current, " too high");
                success = false;
                done = true;
            }
            success = true;
            done = true;
        } else {
            LDEBUG(note, "Voltage=", voltage, " too low");
        }
    } else {
        LDEBUG(note, "Voltage=", voltage, " too high");
    }
}

void testPowerOff(PowerSubsystemConfig const& psCfg, double voltage,
        string const& note, bool& success, bool& done) {
    if (voltage <= psCfg.getVoltageOffLevel()) {
        success = true;
        done = true;
    } else {
        LDEBUG(note, "Voltage=", voltage, " above offLevel");
    }
}

void checkTimeout(util::CLOCK::time_point tStart, double maxTime, bool& done, string const& note) {
    // check timeout
    util::CLOCK::time_point tEnd = util::CLOCK::now();
    const double timeDiff = util::timePassedSec(tStart, tEnd);
    if (timeDiff > maxTime) {
        LERROR(note, "Voltage timeout timeDiff=", timeDiff, " max=", maxTime);
        done = true;
    }
}

bool motorPowerOnTest(SimCore& simCore, PowerSubsystemConfig const& motorCfg) {
    util::CLOCK::time_point tStart = util::CLOCK::now();
    bool success = false;
    bool done = false;
    while (!done) {
        auto info = simCore.getSysInfo();
        if (info.outputPort.getBitAtPos(OutputPortBits::MOTOR_POWER_ON) == false) {
            LDEBUG("motor power not on");
        }
        testPowerOn(motorCfg, info.motorVoltage, info.motorCurrent, "motor", success, done);
        checkTimeout(tStart, motorCfg.outputOnMaxDelay(), done, "motor"); // Sets done = true on timeout.
        this_thread::sleep_for(0.04s);
    }
    return success;
}

bool commPowerOnTest(SimCore& simCore, PowerSubsystemConfig const& commCfg) {
    util::CLOCK::time_point tStart = util::CLOCK::now();
    bool success = false;
    bool done = false;
    while (!done) {
        auto info = simCore.getSysInfo();
        if (info.outputPort.getBitAtPos(OutputPortBits::ILC_COMM_POWER_ON) == false) {
            LDEBUG("comm power not on");
        }
        testPowerOn(commCfg, info.commVoltage, info.commCurrent, "comm", success, done);
        checkTimeout(tStart, commCfg.outputOnMaxDelay(), done, "comm"); // Sets done = true on timeout.
        this_thread::sleep_for(0.04s);
    }
    return success;
}

bool motorPowerOffTest(SimCore& simCore, PowerSubsystemConfig const& motorCfg) {
    util::CLOCK::time_point tStart = util::CLOCK::now();
    bool success = false;
    bool done = false;
    while (!done) {
        auto info = simCore.getSysInfo();
        if (info.outputPort.getBitAtPos(OutputPortBits::MOTOR_POWER_ON) != 0) {
            LDEBUG("motorPowerOff motor power is on");
        }
        if (info.outputPort.getBitAtPos(OutputPortBits::RESET_MOTOR_BREAKERS) != 0) {
            LDEBUG("motorPowerOff reset breakers is set");
        }
        testPowerOff(motorCfg, info.motorVoltage, "motor", success, done);
        // outputOffMaxDelay() is too short for the simulator to finish a single loop
        // given the expected frequency of updates from the configuration.
        checkTimeout(tStart, motorCfg.outputOffMaxDelay()*5.0, done, "motor"); // Sets done = true on timeout.
        this_thread::sleep_for(0.04s);
    }
    return success;
}

bool commPowerOffTest(SimCore& simCore, PowerSubsystemConfig const& commCfg) {
    util::CLOCK::time_point tStart = util::CLOCK::now();
    bool success = false;
    bool done = false;
    while (!done) {
        auto info = simCore.getSysInfo();
        if (info.outputPort.getBitAtPos(OutputPortBits::ILC_COMM_POWER_ON) != 0) {
            LDEBUG("commPowerOff motor power is on");
        }
        if (info.outputPort.getBitAtPos(OutputPortBits::RESET_COMM_BREAKERS) != 0) {
            LDEBUG("commPowerOff reset breakers is set");
        }
        testPowerOff(commCfg, info.commVoltage, "comm", success, done);
        // outputOffMaxDelay() is too short for the simulator to finish a single loop
        // given the expected frequency of updates from the configuration.
        checkTimeout(tStart, commCfg.outputOffMaxDelay()*5.0, done, "comm"); // Sets done = true on timeout.
        this_thread::sleep_for(0.04s);
    }
    return success;
}


TEST_CASE("Test SimCore", "[SimCore]") {
    util::Log::getLog().useEnvironmentLogLvl();
    string cfgPath = system::Config::getEnvironmentCfgPath("../configs");
    system::Config::setup(cfgPath + "unitTestCfg.yaml");
    auto& config = LSST::m2cellcpp::system::Config::get();
    system::Globals::setup(config);

    SimCore simCore;

    PowerSubsystemConfig motorCfg(control::MOTOR);
    PowerSubsystemConfig commCfg(control::COMM);

    LDEBUG("test_SimCore start");
    simCore.start();

    while (simCore.getIterations() < 1) {
        this_thread::sleep_for(0.1s);
    }

    auto info = simCore.getSysInfo();
    REQUIRE(info.motorBreakerClosed == false);
    REQUIRE(info.commCurrent < 1);
    REQUIRE(info.commVoltage < 1);
    REQUIRE(info.motorCurrent < 1);
    REQUIRE(info.motorVoltage < 1);
    REQUIRE(info.outputPort.getBitAtPos(OutputPortBits::MOTOR_POWER_ON) == 0);
    REQUIRE(info.outputPort.getBitAtPos(OutputPortBits::ILC_COMM_POWER_ON) == 0);
    REQUIRE(info.outputPort.getBitAtPos(OutputPortBits::CRIO_INTERLOCK_ENABLE) == 0);
    REQUIRE(info.outputPort.getBitAtPos(OutputPortBits::RESET_MOTOR_BREAKERS) == 0);
    REQUIRE(info.outputPort.getBitAtPos(OutputPortBits::RESET_COMM_BREAKERS) == 0);

    // motor power on test
    simCore.writeNewOutputPortBit(OutputPortBits::MOTOR_POWER_ON, true);
    REQUIRE(motorPowerOnTest(simCore, motorCfg));

    // comm power on test
    info = simCore.getSysInfo();
    REQUIRE(info.outputPort.getBitAtPos(OutputPortBits::ILC_COMM_POWER_ON) == 0);
    simCore.writeNewOutputPortBit(OutputPortBits::ILC_COMM_POWER_ON, true);
    REQUIRE(commPowerOnTest(simCore, commCfg));

    // motor breaker test
    simCore.writeNewOutputPortBit(OutputPortBits::RESET_MOTOR_BREAKERS, false);
    this_thread::sleep_for(0.2s);
    info = simCore.getSysInfo();
    REQUIRE(info.motorCurrent < 1.0);
    REQUIRE(info.inputPort.getBitAtPos(control::InputPortBits::J1_W9_3_MTR_PWR_BRKR_OK) == false);

    simCore.writeNewOutputPortBit(OutputPortBits::RESET_MOTOR_BREAKERS, true);
    this_thread::sleep_for(chrono::duration<double>(motorCfg.getBreakerOnTime()));
    REQUIRE(motorPowerOnTest(simCore, motorCfg) == true);
    info = simCore.getSysInfo();
    REQUIRE(info.motorCurrent > 1.0);
    REQUIRE(info.inputPort.getBitAtPos(control::InputPortBits::J1_W9_3_MTR_PWR_BRKR_OK));

    // comm breaker test
    simCore.writeNewOutputPortBit(OutputPortBits::RESET_COMM_BREAKERS, false);
    this_thread::sleep_for(chrono::duration<double>(commCfg.getBreakerOnTime()));
    info = simCore.getSysInfo();
    REQUIRE(info.commCurrent < 1.0);
    REQUIRE(info.inputPort.getBitAtPos(control::InputPortBits::J2_W13_2_COMM_PWR_BRKR_OK) == false);

    simCore.writeNewOutputPortBit(OutputPortBits::RESET_COMM_BREAKERS, true);
    this_thread::sleep_for(chrono::duration<double>(commCfg.getBreakerOnTime()));
    REQUIRE(commPowerOnTest(simCore, commCfg) == true);
    info = simCore.getSysInfo();
    REQUIRE(info.commCurrent > 1.0);
    REQUIRE(info.inputPort.getBitAtPos(control::InputPortBits::J2_W13_2_COMM_PWR_BRKR_OK));

    // motor power off test
    simCore.writeNewOutputPortBit(OutputPortBits::MOTOR_POWER_ON, false);
    REQUIRE(motorPowerOffTest(simCore, motorCfg));

    // comm power off test
    simCore.writeNewOutputPortBit(OutputPortBits::ILC_COMM_POWER_ON, false);
    REQUIRE(commPowerOffTest(simCore, motorCfg));

    LDEBUG("test_SimCore stop");
    simCore.stop();

    LDEBUG("test_SimCore joining");
    simCore.join();
    LINFO("test_SimCore done");
}
