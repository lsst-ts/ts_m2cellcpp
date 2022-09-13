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
#include <catch2/catch.hpp>

// Project headers
#include "system/Config.h"
#include "util/CsvFile.h"
#include "util/Log.h"
#include "util/NamedValue.h"

using namespace std;
using namespace LSST::m2cellcpp::util;

TEST_CASE("Test CsvFile", "[CsvFile]") {
    LSST::m2cellcpp::util::Log::getLog().useEnvironmentLogLvl();
    string cfgPath = LSST::m2cellcpp::system::Config::getEnvironmentCfgPath("../configs");

    CsvFile cFile("../testFiles/csvTest.csv");
    cFile.read();

    string dump = cFile.dumpStr();
    LINFO(dump);

    REQUIRE(cFile.getColumnCount() == 22);
    REQUIRE(cFile.getRowCount() == 10);

    REQUIRE(cFile.getValue("in_fA1", 0) == "-325.307");
    REQUIRE(cFile.getValue("in_fA1", 9) == "-0.150991");

    REQUIRE(cFile.getValue("in_fA6", 2) == "309.4");
    REQUIRE(cFile.getValue("in_fA6", 7) == "3692.16");

    REQUIRE_THROWS(cFile.getValue("in_fA6", 10));
    REQUIRE_THROWS(cFile.getValue("in_fA6", -1));
    REQUIRE_THROWS(cFile.getValue("i_fA6", 9));
    REQUIRE(cFile.getValue("out_tan_load_cell_bool", 9) == "FALSE");

    // NamedString tests
    {
        NamedString nvS("nv");
        REQUIRE(nvS.getName() == "nv");
        REQUIRE(nvS.getValue() == "");
        REQUIRE(nvS.approxEqual(""));
        REQUIRE(!nvS.approxEqual("r"));
        nvS.setFromString("Hello");
        REQUIRE(nvS.getValue() == "Hello");
        REQUIRE(nvS.approxEqual("Hello"));
        REQUIRE(!nvS.approxEqual("hello"));
        string str = nvS.dump();
        LDEBUG(str);
    }

    // NamedBool tests
    {
        NamedBool nvB("nvB");
        REQUIRE(nvB.getName() == "nvB");
        REQUIRE(nvB.getValue() == false);
        REQUIRE(nvB.approxEqual(false));
        REQUIRE(!nvB.approxEqual(true));
        nvB.setFromString("true");
        REQUIRE(nvB.getValue() == true);
        REQUIRE(nvB.approxEqual(true));
        REQUIRE(!nvB.approxEqual(false));
        REQUIRE_NOTHROW(nvB.setFromString("false"));
        REQUIRE_NOTHROW(nvB.setFromString("TRUE"));
        REQUIRE_NOTHROW(nvB.setFromString("FALSE"));
        REQUIRE_THROWS(nvB.setFromString("tr"));
        REQUIRE_THROWS(nvB.setFromString("fal"));
    }

    // NamedInt tests
    {
        NamedInt nvI("nvI");
        REQUIRE(nvI.getName() == "nvI");
        REQUIRE(nvI.getValue() == 0);
        REQUIRE(nvI.approxEqual(0));
        REQUIRE(!nvI.approxEqual(1));
        int j = 453;
        nvI.setValue(j);
        REQUIRE(nvI.getValue() == j);
        REQUIRE(nvI.approxEqual(j));
        REQUIRE(!nvI.approxEqual(1));
        nvI.setFromString("945");
        REQUIRE(nvI.getValue() == 945);
        nvI.setFromString("-945");
        REQUIRE(nvI.getValue() == -945);
        REQUIRE_THROWS(nvI.setFromString("945.4"));
    }

    // NamedDouble tests
    {
        double tolorance = 0.0000001;
        NamedDouble nvD("nvD", tolorance);
        REQUIRE(nvD.getName() == "nvD");
        REQUIRE(nvD.getValue() == 0.0);
        REQUIRE(nvD.approxEqual(0.0));
        REQUIRE(nvD.approxEqual(tolorance * 0.9));
        REQUIRE(nvD.approxEqual(tolorance * -0.9));
        REQUIRE(!nvD.approxEqual(tolorance * 1.1));
        REQUIRE(!nvD.approxEqual(tolorance * -1.1));
        double jj = 7238.8125;
        nvD.setValue(jj);
        REQUIRE(nvD.getValue() == jj);
        REQUIRE(nvD.approxEqual(jj));
        REQUIRE(nvD.approxEqual(jj + tolorance * 0.9));
        REQUIRE(nvD.approxEqual(jj + tolorance * -0.9));
        REQUIRE(!nvD.approxEqual(jj + tolorance * 1.1));
        REQUIRE(!nvD.approxEqual(jj + tolorance * -1.1));

        nvD.setFromString("9743.9872");
        REQUIRE(nvD.getValue() == 9743.9872);
        nvD.setFromString("-472.198");
        REQUIRE(nvD.getValue() == -472.198);
        REQUIRE_THROWS(nvD.setFromString("19.4aw"));
    }

    // NamedAngle tests
    {
        NamedAngle nvA("nvA");
        REQUIRE(nvA.getName() == "nvA");
        REQUIRE(nvA.getValue() == 0.0);
        REQUIRE(nvA.approxEqual(0.0));
        double radians = NamedAngle::PI + (30*NamedAngle::PI2);
        nvA.setRad(radians);
        REQUIRE(nvA.approxEqualRad(radians));
        NamedAngle nvA1("nvA1");
        nvA1.setRad(NamedAngle::PI);
        REQUIRE(nvA1.approxEqualRad(NamedAngle::constrain0to2Pi(nvA.getRad())));
        REQUIRE(nvA1.approxEqualRad(NamedAngle::constrain(nvA.getRad())));

        NamedAngle nvA2("nvA2");
        nvA2.setFromString("180.0");
        LINFO("nva2", nvA2);
        REQUIRE(nvA2.approxEqual(180.0));
        REQUIRE(nvA2.approxEqualDeg(180.0));
        REQUIRE(nvA2.approxEqualRad(NamedAngle::PI));

        NamedAngle nvA3("nvA3", NamedAngle::RADIAN);
        nvA3.setFromString("0.5342");
        LINFO("nva3", nvA3);
        REQUIRE(nvA3.approxEqual(0.5342));
        REQUIRE(!nvA3.approxEqualDeg(180.0));
        REQUIRE(nvA3.approxEqualRad(0.5342));

    }
}
