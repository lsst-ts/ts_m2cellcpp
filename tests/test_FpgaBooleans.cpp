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
#include "control/FpgaBooleans.h"
#include "system/Config.h"
#include "util/Log.h"

using namespace std;
using namespace LSST::m2cellcpp::control;

TEST_CASE("Test FpgaBooleans", "[FpgaBooleans]") {
    LSST::m2cellcpp::util::Log::getLog().useEnvironmentLogLvl();
    string cfgPath = LSST::m2cellcpp::system::Config::getEnvironmentCfgPath("../configs");
    LSST::m2cellcpp::system::Config::setup(cfgPath + "unitTestCfg.yaml");
    {
        LDEBUG("Testing setBit & getBit");

        byte byt = byte{1};
        REQUIRE_THROWS(FpgaBooleans::getBit(byt, -1));
        REQUIRE_THROWS(FpgaBooleans::getBit(byt, 8));
        REQUIRE_THROWS(FpgaBooleans::setBit(byt, -1, false));
        REQUIRE_THROWS(FpgaBooleans::setBit(byt, 8, true));

        REQUIRE(FpgaBooleans::getBit(byt, 0) == true);
        for (int j = 1; j <= FpgaBooleans::BIT_MAX; ++j) {
            REQUIRE(FpgaBooleans::getBit(byt, j) == false);
        }

        byt = byte{0xFE};
        REQUIRE(FpgaBooleans::getBit(byt, 0) == false);
        for (int j = 1; j <= FpgaBooleans::BIT_MAX; ++j) {
            REQUIRE(FpgaBooleans::getBit(byt, j) == true);
        }

        byt = byte{0b01111111};
        REQUIRE(FpgaBooleans::getBit(byt, 7) == false);
        for (int j = 0; j < FpgaBooleans::BIT_MAX; ++j) {
            REQUIRE(FpgaBooleans::getBit(byt, j) == true);
        }

        byt = byte{0b10010000};
        for (int j = 0; j <= 3; ++j) {
            REQUIRE(FpgaBooleans::getBit(byt, j) == false);
        }
        REQUIRE(FpgaBooleans::getBit(byt, 4) == true);
        REQUIRE(FpgaBooleans::getBit(byt, 5) == false);
        REQUIRE(FpgaBooleans::getBit(byt, 6) == false);
        REQUIRE(FpgaBooleans::getBit(byt, 7) == true);

        // The REQUIRE macro doesn't compile with (std::byte == std::byte)
        LINFO("setBit a");
        FpgaBooleans::setBit(byt, 4, true);
        bool b = byt == byte{0b10010000};
        REQUIRE(b);

        LINFO("setBit b");
        FpgaBooleans::setBit(byt, 4, false);
        b = byt == byte{0b10000000};
        REQUIRE(b);

        LINFO("setBit c");
        FpgaBooleans::setBit(byt, 7, false);
        b = byt == byte{0};
        REQUIRE(b);

        LINFO("setBit d");
        FpgaBooleans::setBit(byt, 0, true);
        b = byt == byte{0b00000001};
        REQUIRE(b);

        LINFO("setBit d");
        byt = byte(0xFF);
        FpgaBooleans::setBit(byt, 0, false);
        b = byt == byte{0b11111110};
        REQUIRE(b);
        LINFO("setBit e");
        FpgaBooleans::setBit(byt, 2, false);
        b = byt == byte{0b11111010};
        REQUIRE(b);
        LINFO("setBit f");
        FpgaBooleans::setBit(byt, 7, false);
        b = byt == byte{0b01111010};
        REQUIRE(b);
    }
}
