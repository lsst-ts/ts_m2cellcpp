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

// 3rd party headers
#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_session.hpp>

// Project headers
#include "system/Config.h"
#include "vis/TangentLoadCellFaultDetection.h"
#include "util/Log.h"

using namespace std;
using namespace LSST::m2cellcpp::vis;

TEST_CASE("Test VIs", "[VIs]") {
    LSST::m2cellcpp::util::Log::getLog().useEnvironmentLogLvl();
    string cfgPath = LSST::m2cellcpp::system::Config::getEnvironmentCfgPath("../configs");

    {
        TangentLoadCellFaultDetection tangCellFaultDetect;
        tangCellFaultDetect.readTestFile("../testFiles/io_TangentLoadCell_v03.csv");
        REQUIRE(tangCellFaultDetect.runTest() == true);
    }
}
