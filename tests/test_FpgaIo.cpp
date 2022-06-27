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
#include "control/FpgaIo.h"
#include "util/Log.h"

using namespace std;
using namespace LSST::m2cellcpp::control;

TEST_CASE("Test FpgaIo", "[FpgaIo]") {
    LSST::m2cellcpp::util::Log::getLog().useEnvironmentLogLvl();
    {
        LDEBUG("Testing Ilc");
        Ilc ilc("test_1", 1);

        ilc.setStatus(0xe1);
        REQUIRE(ilc.getBroadcastCommCount() == 0x0e);
        REQUIRE(ilc.getFault() == true);
        REQUIRE(ilc.getCWLimit() == false);
        REQUIRE(ilc.getCCWLimit() == false);

        ilc.setStatus(0x14);
        REQUIRE(ilc.getBroadcastCommCount() == 0x01);
        REQUIRE(ilc.getFault() == false);
        REQUIRE(ilc.getCWLimit() == true);
        REQUIRE(ilc.getCCWLimit() == false);

        ilc.setStatus(0x38);
        REQUIRE(ilc.getBroadcastCommCount() == 0x03);
        REQUIRE(ilc.getFault() == false);
        REQUIRE(ilc.getCWLimit() == false);
        REQUIRE(ilc.getCCWLimit() == true);
    }

    {
        LDEBUG("Testing AllIlcs");
        AllIlcs ilcs(true);
        auto a = ilcs.getIlc(1);
        REQUIRE(a->getName() == "Tangent_1");
        REQUIRE(a->getIdNum() == 1);
        a = ilcs.getIlc(6);
        REQUIRE(a->getName() == "Tangent_6");
        REQUIRE(a->getIdNum() == 6);
        a = ilcs.getIlc(7);
        REQUIRE(a->getName() == "Axial_7");
        REQUIRE(a->getIdNum() == 7);
        a = ilcs.getIlc(78);
        REQUIRE(a->getName() == "Axial_78");
        REQUIRE(a->getIdNum() == 78);

        REQUIRE_THROWS(ilcs.getIlc(0));
        REQUIRE_THROWS(ilcs.getIlc(79));
    }

    { LDEBUG("Test DaqIn DaqOut &&&"); }

    { LDEBUG("Test DaqBoolIn DaqBoolOut &&&"); }
}
