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

#include <fstream>
#include <iostream>

#include <unistd.h>
#include <limits.h>

#define CATCH_CONFIG_MAIN
#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_session.hpp>
#include <catch2/catch_all.hpp>

#include "util/Bug.h"
#include "util/Log.h"

using namespace std;
using namespace LSST::m2cellcpp::util;

TEST_CASE("Test Bug", "[Bug]") {
    char cwd[PATH_MAX];
    if (getcwd(cwd, sizeof(cwd)) != NULL) {
        LCRITICAL("Current working dir:", cwd);
    } else {
        LCRITICAL("getcwd() error");
    }
    Log::getLog().useEnvironmentLogLvl();

    bool thrown = false;
    string str;
    int line;
    /// Only way to test is to catch
    try {
        line = __LINE__ + 1;
        throw Bug(ERR_LOC, "not really a bug");
    } catch (Bug const& ex) {
        thrown = true;
        str = ex.what();
    }
    REQUIRE(thrown);
    /// find "test_Bug.cpp:"+to_string(line) in str.
    string expected("test_Bug.cpp:");
    expected += to_string(line);
    LTRACE("expected=", expected, " what=", str);
    bool found = false;
    if (str.find(expected) != std::string::npos) {
        found = true;
    }
    REQUIRE(found);
}
