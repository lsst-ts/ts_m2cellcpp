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
#define CATCH_CONFIG_MAIN
#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_session.hpp>

#include "util/Log.h"

using namespace std;
using namespace LSST::m2cellcpp::util;

TEST_CASE("Test Log", "[Log]") {
    thread::id tid = this_thread::get_id();
    Log& lg = Log::getLog();
    lg.setOutputDest(Log::MIRRORED);
    int bufferLines = 0;
    auto file = __FILE__;
    auto line = __LINE__;
    REQUIRE_NOTHROW(Log::logW(spdlog::level::debug, file, line, "dbg=", 9));
    ++bufferLines;
    string buff = Log::getLog().getBufferLast();
    std::stringstream sstm;
    sstm << "[" << file << ":" << line << "] tid:" << std::hex << tid << " dbg=9";
    REQUIRE(buff == sstm.str());

    int x = 7;
    double y = 3.14;
    REQUIRE_NOTHROW(LDEBUG("simple"));
    REQUIRE_NOTHROW(LDEBUG("i x=", x));
    REQUIRE_NOTHROW(LDEBUG("d y=", y, " x=", x));

    REQUIRE_NOTHROW(LTRACE("d y=", y, " x=", x));
    REQUIRE_NOTHROW(LINFO("d y=", y, " x=", x));
    REQUIRE_NOTHROW(LWARN("d y=", y, " x=", x));
    REQUIRE_NOTHROW(LERROR("d y=", y, " x=", x));
    REQUIRE_NOTHROW(LCRITICAL("d y=", y, " x=", x));

    uint expectedSz = 4;  /// Must be smaller than number of log lines above.
    lg.setMaxBuffers(expectedSz);
    REQUIRE(expectedSz == lg.getBuffersSize());
    string tmpLog("/tmp/test_Log.log");
    // Contens of the log buffer should be put into the file.
    int const megabyte = 1024 * 1024;
    lg.setupFileRotation(tmpLog, 11 * megabyte, 10);  // Keep ten 11MB files.
    lg.setOutputDest(Log::SPEEDLOG);
    ++expectedSz;  // setOutputDest(Log::SPEEDLOG) adds an info level log message.
    // Test that normal log line gets into the file.
    LINFO("one more line");
    ++expectedSz;
    lg.flush();
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

    // Test log levels
    lg.setOutputDest(Log::MIRRORED);
    auto startSize = lg.getBuffersSize();
    lg.setLogLvl(spdlog::level::debug);
    LTRACE("a");
    REQUIRE(startSize == lg.getBuffersSize());
    lg.setLogLvl(spdlog::level::info);
    LDEBUG("b");
    REQUIRE(startSize == lg.getBuffersSize());
    lg.setLogLvl(spdlog::level::warn);
    LINFO("c");
    REQUIRE(startSize == lg.getBuffersSize());
    lg.setLogLvl(spdlog::level::err);
    LWARN("d");
    REQUIRE(startSize == lg.getBuffersSize());
    lg.setLogLvl(spdlog::level::critical);
    LERROR("e");
    REQUIRE(startSize == lg.getBuffersSize());
    LCRITICAL("f");
    REQUIRE(startSize + 1 == lg.getBuffersSize());
    lg.setLogLvl(spdlog::level::debug);
    LDEBUG("g");
    REQUIRE(startSize + 2 == lg.getBuffersSize());

    // spdlog testing of inmproperly formed messages. `FMT_STRING` catches all at compile time,
    //    but this is what log file output looks like in some improperly formed cases.
    // spdlog::info("spdlog test3 {", 34.2); /// message missing from output
    // spdlog::info("spdlog test4 }", 34.2); /// no number in message
    // spdlog::info("spdlog test5 {} {}", 34.2);
    //        /// [*** LOG ERROR #0001 ***] [2022-08-11 12:56:10] [] {invalid format string}

    // Test 'SPD' macros.
    REQUIRE_NOTHROW(SPDTRACE(FMT_STRING("basic test {} {} {}"), "df } {", 6, 9.8765));
    REQUIRE_NOTHROW(SPDDEBUG(FMT_STRING("basic test {} {} {}"), "df } {", 7, 9.8765));
    REQUIRE_NOTHROW(SPDINFO(FMT_STRING("basic test {} {} {}"), "df } {", 8, 9.8765));
    REQUIRE_NOTHROW(SPDWARN(FMT_STRING("basic test {} {} {}"), "df } {", 9, 9.8765));
    REQUIRE_NOTHROW(SPDERROR(FMT_STRING("basic test {} {} {}"), "df } {", 1, 9.8765));
    REQUIRE_NOTHROW(SPDCRITICAL(FMT_STRING("basic test {} {} {}"), "df } {", 2, 9.8765));
    REQUIRE_NOTHROW(SPDINFO(FMT_STRING("spdlog test11 {}, {}"), 34.2, "bob"));
    REQUIRE_NOTHROW(SPDINFO(FMT_STRING("{}"), "spdlog test12 THIS { is valid?"));

#if 0   // Timing tests
    // Before running test `rm /tmp/test_Lo*` as log file rotation takes time and can
    // foul the results. It's also important the log file rotation set above has
    // a max filesize large enough not to cause a rotation (see setupFileRotation).
    // The tests run twice and make 1 million similar entries using similar input
    // and records the time spent.
    lg.setOutputDest(Log::SPEEDLOG);
    lg.setLogLvl(spdlog::level::debug);
    spdlog::set_level(spdlog::level::debug);

    int limit= 1'000'000;
    string loglogTimes("loglog:");
    string spdlogTimes("spdlog:");
    for (int trial=0; trial<2; ++trial) {
        auto begin = chrono::steady_clock::now();
        for (int j=0;j<limit;++j) {
            LINFO("LogLog ", j, " ", trial, " ", 3.145674312465, " ", 9823.1234345, " ", 987654321);
            //LINFO("LogLog ", j, " ", trial, " ", 3.145674312465);
            //LINFO("LogLog ", j);
        }
        auto end = chrono::steady_clock::now();
        loglogTimes += to_string(chrono::duration_cast<chrono::milliseconds>(end - begin).count()) + ", ";

        begin = chrono::steady_clock::now();
        for (int j=0;j<limit;++j) {
            SPDINFO(FMT_STRING("InfLog {} {} {} {} {}"), j, trial, 3.145674312465, 9823.1234345, 987654321);
            //SPDINFO(FMT_STRING("InfLog {} {} {}"), j, trial, 3.145674312465);
            //SPDINFO(FMT_STRING("InfLog {}"), j);
        }
        end = chrono::steady_clock::now();
        spdlogTimes += to_string(chrono::duration_cast<chrono::milliseconds>(end - begin).count()) + ", ";
    }

    cout << loglogTimes << endl;
    cout << spdlogTimes << endl;
#endif  // end timing tests
}
