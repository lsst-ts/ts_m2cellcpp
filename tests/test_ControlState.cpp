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

#include "state/StateMap.h"
#include "system/Config.h"
#include "util/Log.h"

using namespace std;
using namespace LSST::m2cellcpp::state;
using namespace LSST::m2cellcpp;

TEST_CASE("Test ControlState", "[ControlState]") {
    util::Log::getLog().useEnvironmentLogLvl();
    string cfgPath = system::Config::getEnvironmentCfgPath("../configs");
    system::Config::setup(cfgPath + "unitTestCfg.yaml");

    StateMap sMap(nullptr);
    State::Ptr currentState = sMap.getCurrentState();
    StartupState::Ptr startupState = dynamic_pointer_cast<StartupState>(currentState);
    REQUIRE(currentState->getName() == "StartupState");
    REQUIRE(currentState == sMap.getStartupState());
    REQUIRE(startupState != nullptr);

    // Test advancing through existing states transistions.

    // go from startup to idle
    REQUIRE(sMap.changeState(State::IDLESTATE));
    REQUIRE(sMap.getCurrentState()->getName() == "IdleState");
    REQUIRE(sMap.getCurrentState() == sMap.getIdleState());

    // go from idle to standby
    REQUIRE(sMap.changeState(State::STANDBYSTATE));
    REQUIRE(sMap.getCurrentState()->getName() == "StandbyState");
    REQUIRE(sMap.getCurrentState() == sMap.getStandbyState());

    // go from standby to idle
    REQUIRE(sMap.changeState(State::IDLESTATE));  // "IdleState"
    REQUIRE(sMap.getCurrentState()->getName() == "IdleState");

    // go from idle to inMotion
    REQUIRE(sMap.changeState(State::INMOTIONSTATE));
    REQUIRE(sMap.getCurrentState()->getName() == "InMotionState");
    REQUIRE(sMap.getCurrentState() == sMap.getInMotionState());

    // go from inMotion to idle
    REQUIRE(sMap.changeState(State::IDLESTATE));
    REQUIRE(sMap.getCurrentState()->getName() == "IdleState");

    // go from idle to inMotion
    REQUIRE(sMap.changeState(State::INMOTIONSTATE));
    REQUIRE(sMap.getCurrentState()->getName() == "InMotionState");

    // got from inMotion to pause
    REQUIRE(sMap.changeState(State::PAUSESTATE));
    REQUIRE(sMap.getCurrentState()->getName() == "PauseState");
    REQUIRE(sMap.getCurrentState() == sMap.getPauseState());

    // go from pause to inMotion
    REQUIRE(sMap.changeState(State::INMOTIONSTATE));
    REQUIRE(sMap.getCurrentState()->getName() == "InMotionState");

    // go from inMotion to pause
    REQUIRE(sMap.changeState(State::PAUSESTATE));
    REQUIRE(sMap.getCurrentState()->getName() == "PauseState");

    // go from pause to idle
    REQUIRE(sMap.changeState(State::IDLESTATE));
    REQUIRE(sMap.getCurrentState()->getName() == "IdleState");

    // go from idle to standby
    REQUIRE(sMap.changeState(State::STANDBYSTATE));
    REQUIRE(sMap.getCurrentState()->getName() == "StandbyState");
    REQUIRE(sMap.getCurrentState() == sMap.getStandbyState());

    // go from standby to offline
    REQUIRE(sMap.changeState(State::OFFLINESTATE));
    REQUIRE(sMap.getCurrentState()->getName() == "OfflineState");
    REQUIRE(sMap.getCurrentState() == sMap.getOfflineState());

    // goToASafeState tests (starts with currentState being OfflineState
    REQUIRE(sMap.goToASafeState(State::STANDBYSTATE, "test") == false);  // should stay in OfflineState.
    REQUIRE(sMap.changeState(State::IDLESTATE));                         // change to IdleState for next test
    REQUIRE(sMap.goToASafeState(State::STANDBYSTATE, "test") == true);
    REQUIRE(sMap.goToASafeState(State::INMOTIONSTATE, "test") == false);
    REQUIRE(sMap.getCurrentState()->getName() == "StandbyState");
}
