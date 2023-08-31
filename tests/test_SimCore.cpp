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
#include "simulator/SimCore.h"
#include "system/Config.h"
#include "system/Globals.h"
#include "util/Log.h"

using namespace std;
using namespace LSST::m2cellcpp::simulator;

TEST_CASE("Test SimCore", "[SimCore]") {
    LSST::m2cellcpp::util::Log::getLog().useEnvironmentLogLvl();
    string cfgPath = LSST::m2cellcpp::system::Config::getEnvironmentCfgPath("../configs");
    LSST::m2cellcpp::system::Config::setup(cfgPath + "unitTestCfg.yaml");
    auto& config = LSST::m2cellcpp::system::Config::get();
    LSST::m2cellcpp::system::Globals::setup(config);

    SimCore simCore;

    LDEBUG("test_SimCore start");
    simCore.start();

    this_thread::sleep_for(10s);

    LDEBUG("test_SimCore stop");
    simCore.stop();

    LDEBUG("test_SimCore joining");
    simCore.join();
    LINFO("test_SimCore done");
}
