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
#include <exception>

// 3rd party headers
#include <catch2/catch.hpp>

// Project headers
#include <util/InstanceCount.h>

using namespace std;
using namespace LSST::m2cellcpp::util;
using Catch::Matchers::StartsWith;

TEST_CASE("Test InstanceCount", "[InstanceCount]") {
    {
        InstanceCount instA0("a");
        InstanceCount instB0("b");
        REQUIRE(instA0.getCount() == 1);
        REQUIRE(instB0.getCount() == 1);
        {
            InstanceCount instA1("a");
            REQUIRE(instA0.getCount() == 2);
            REQUIRE(instA1.getCount() == 2);
            REQUIRE(instB0.getCount() == 1);
            // Behavior here is the same as if it was inside
            // another object being copied or moved, at 
            // least using the default move and copy constructors.
            {
                InstanceCount instA2 = instA1;
                REQUIRE(instA0.getCount() == 3);
                InstanceCount instB1 = move(instB0);
                REQUIRE(instB0.getCount() == 2);
            }
            REQUIRE(instA0.getCount() == 2);
            REQUIRE(instB0.getCount() == 1);
        }
        REQUIRE(instA0.getCount() == 1);
        REQUIRE(instB0.getCount() == 1);
    }
}
