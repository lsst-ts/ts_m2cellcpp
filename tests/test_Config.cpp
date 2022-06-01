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
#include "system/Config.h"
#include "util/Bug.h"
#include "util/Log.h"

using namespace std;
using namespace LSST::m2cellcpp::system;

TEST_CASE("Test Config", "[Config]") {
    LSST::m2cellcpp::util::Log::getLog().useEnvironmentLogLvl();
    string cfgPath = Config::getEnvironmentCfgPath("../configs");

    LDEBUG("test get");
    REQUIRE_THROWS(Config::get());

    LDEBUG("test bad filename");
    REQUIRE_THROWS(Config::setup("junk"));

    LDEBUG("test valid file read");
    Config::setup(cfgPath + "unitTestCfg.yaml");
    int port = Config::get().getControlServerPort();
    REQUIRE(port == 12678);
    string host = Config::get().getControlServerHost();
    REQUIRE(host == "127.0.0.1");

    LDEBUG("test valid double");
    double pi = Config::get().getSectionKeyAsDouble("testconstant", "pi", 3.14, 3.15);
    REQUIRE(pi > 3.14159264);
    REQUIRE(pi < 3.14159266);

    LDEBUG("test bad double value");
    /// pi is not be between 4.1 and 5.1, so it should trigger the logic.
    REQUIRE_THROWS(pi = Config::get().getSectionKeyAsDouble("testconstant", "pi", 4.1, 5.1));

    LDEBUG("test that the int conversion of a double throws");
    int ipi;
    REQUIRE_THROWS(ipi = Config::get().getSectionKeyAsInt("testconstant", "pi"));

    LDEBUG("test bad int value");
    REQUIRE_THROWS(Config::get().getSectionKeyAsInt("ControlServer", "port", 100000, 200000));

    LDEBUG("test for string value not found");
    REQUIRE_THROWS(Config::get().getSectionKeyAsString("testconstant", "not_here"));

    Config::reset();
    LDEBUG("test file with missing required element");
    bool found = false;
    try {
        Config::setup(cfgPath + "unitTestCfgBad1.yaml");
    } catch (LSST::m2cellcpp::system::ConfigException const& cfgEx) {
        string msg(cfgEx.what());
        string expected = "is missing";
        if (msg.find(expected) != std::string::npos) {
            found = true;
        }
    }
    REQUIRE(found);
}
