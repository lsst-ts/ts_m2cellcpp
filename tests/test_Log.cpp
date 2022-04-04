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

#include <fstream>
#include <iostream>
#define CATCH_CONFIG_MAIN
#include <catch2/catch.hpp>

#include "system/Log.h"

using namespace std;
using namespace LSST::m2cellcpp::system;

TEST_CASE("Test Log", "[Log]") {
    thread::id tid = this_thread::get_id();
    Log& lg = Log::getLog();
    lg.setOutputDest(Log::MIRRORED); ///&&& change to BUFFER
    int bufferLines = 0;
    auto file = __FILE__;
    auto line = __LINE__;
    REQUIRE_NOTHROW(Log::logW(Log::DEBUG, file, line, "dbg=", 9));
    ++bufferLines;
    string buff = Log::getLog().getBufferLast();
    std::stringstream sstm;
    sstm << file << ":" << line  << " tid:" << std::hex << tid << " DEBUG dbg=9";
    REQUIRE(buff == sstm.str());

    int x = 7;
    double y = 3.14;
    REQUIRE_NOTHROW(LDEBUG("simple"));
    REQUIRE_NOTHROW(LDEBUG("i x=", x ));
    REQUIRE_NOTHROW(LDEBUG("d y=", y, " x=", x));

    REQUIRE_NOTHROW(LTRACE("d y=", y, " x=", x));
    REQUIRE_NOTHROW(LINFO("d y=", y, " x=", x));
    REQUIRE_NOTHROW(LWARN("d y=", y, " x=", x));
    REQUIRE_NOTHROW(LERROR("d y=", y, " x=", x));
    REQUIRE_NOTHROW(LCRITICAL("d y=", y, " x=", x));

    uint expectedSz = 4; /// Must be smaller than number of log lines above.
    lg.setMaxBuffers(expectedSz);
    REQUIRE(expectedSz == lg.getBuffersSize());
    string tmpLog("/tmp/test_Log.log");
    // Contens of the log buffer should be put into the file.
    lg.setOutputDest(Log::FILE, tmpLog);
    // Test that normal log line gets into the file.
    LINFO("one more line");
    ++expectedSz;
    lg.setOutputDest(Log::COUT); /// This should close tmpLog
    REQUIRE(lg.getBuffersSize() == 0);
    ifstream tmp(tmpLog);
    REQUIRE(tmp.is_open() == true);
    uint lineCount = 0;
    string str;
    while (getline(tmp, str)) {
        ++lineCount;
    }
    tmp.close();
    REQUIRE(expectedSz == lineCount);    
}



