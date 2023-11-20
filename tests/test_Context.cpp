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
#include "control/Context.h"
#include "control/FpgaIo.h"
#include "control/MotionEngine.h"
#include "simulator/SimCore.h"
#include "system/Config.h"
#include "util/Bug.h"
#include "util/Log.h"

using namespace std;
using namespace LSST::m2cellcpp::control;
using namespace LSST::m2cellcpp::state;

TEST_CASE("Test Csv", "[CSV]") {
    LSST::m2cellcpp::util::Log::getLog().useEnvironmentLogLvl();
    string cfgPath = LSST::m2cellcpp::system::Config::getEnvironmentCfgPath("../configs");

    LSST::m2cellcpp::simulator::SimCore::Ptr simCore(new LSST::m2cellcpp::simulator::SimCore());
    FpgaIo::setup(simCore);
    MotionEngine::setup();
    Context::setup();

    Context::Ptr context = Context::get();
    REQUIRE(context != nullptr);
    REQUIRE(context->model.getCurrentState() == context->model.getState(State::STARTUPSTATE)); // "StartupState"

    context->model.ctrlSetup();

    auto newState = context->model.getState(State::STANDBYSTATE); // "StandbyState"
    REQUIRE(context->model.changeState(newState));
    REQUIRE(context->model.getCurrentState() == context->model.getState(State::STANDBYSTATE)); //"StandbyState"

    newState = context->model.getState(State::IDLESTATE); //"IdleState"
    REQUIRE(context->model.changeState(newState));
    REQUIRE(context->model.getCurrentState() == context->model.getState(State::IDLESTATE)); //"IdleState"
}
