// -*- LSST-C++ -*-
/*
 *  This file is part of LSST M2 support system package.
 *
 * This product includes software developed by the
 * LSST Project (http://www.lsst.org/).
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the LSST License Statement and
 * the GNU General Public License along with this program.  If not,
 * see <http://www.lsstcorp.org/LegalNotices/>.
 */

// Class header
#include "util/Log.h"

#include <fstream>
// Project headers
#include "util/Bug.h"

// 3rd party headers
#include "spdlog/sinks/stdout_color_sinks.h"
#include "spdlog/sinks/rotating_file_sink.h"

using namespace std;

namespace LSST {
namespace m2cellcpp {
namespace util {

/// Pointer to the spdlog instance to use.
std::shared_ptr<spdlog::logger> Log::speedLog;

Log& Log::getLog() {
    // Static initialization is generally very bad, but this needs
    // to be high performance. This avoids mutex complications.
    static Log logSingleton;
    return logSingleton;
}

Log::Log() {
    speedLog = spdlog::stdout_color_mt("console");
    setLogLvl(spdlog::level::trace);
    setOutputDest(CONSOLE);
}

Log::~Log() {}

bool Log::setupFileRotation(std::string const& fileName, size_t fileSize, size_t maxFiles) {
    LINFO("Log::setupFileRotation ", fileName, " size=", fileSize, " max=", maxFiles);
    lock_guard<mutex> lck(_mtx);
    if (fileName == "") {
        LERROR("Log::setupFileRotation fileName was empty.");
        return false;
    }
    try {
        bool rotateOnStart = true;
        auto logger = spdlog::rotating_logger_mt("rt", fileName, fileSize, maxFiles, rotateOnStart);
    } catch (const spdlog::spdlog_ex& ex) {
        std::cout << "Log init failed: " << ex.what() << std::endl;
        return false;
    }
    return true;
}

void Log::logW(spdlog::level::level_enum const lvl, std::stringstream& msg) {
    lock_guard<mutex> lck(_mtx);
    switch (_outputDest) {
        case CONSOLE:
            [[fallthrough]];
        case SPEEDLOG:
            // Having 'str' fill in '{}' prevents issues with curly brackets in 'str'.
            speedLog->log(lvl, "{}", msg.str());
            break;
        case BUFFER:
            _buffers.emplace_back(msg.str());
            _reduceBuffers();
            break;
        case MIRRORED: {
            cout << msg.str() << endl;
            _buffers.emplace_back(msg.str());
            _reduceBuffers();
        } break;
        case COUT:
            [[fallthrough]];
        default:
            std::cout << msg.str() << std::endl;
    }
}

void Log::setMaxBuffers(int maxBuffers) {
    lock_guard<mutex> lck(_mtx);
    if (maxBuffers < 0) {
        maxBuffers = 0;
    }
    _maxBuffers = maxBuffers;
    _reduceBuffers();
}

void Log::_reduceBuffers() {
    while (_buffers.size() > _maxBuffers) {
        _buffers.pop_front();
    }
}

std::string Log::getBufferLast() {
    lock_guard<mutex> lck(_mtx);
    if (_buffers.empty()) {
        return "";
    }
    return _buffers.back();
}

uint Log::getBuffersSize() const {
    lock_guard<mutex> lck(_mtx);
    return _buffers.size();
}

void Log::_bufferDump() {
    // _mtx must be held before calling
    if (_outputDest == BUFFER || _outputDest == MIRRORED) {
        return;
    }
    while (!_buffers.empty()) {
        auto& str = _buffers.front();
        if (_outputDest == SPEEDLOG || _outputDest == CONSOLE) {
            // Having 'str' fill in '{}' prevents issues with curly brackets in 'str'.
            speedLog->log(_logLvl, "{}", str);
        } else {
            cout << str << std::endl;
        }
        _buffers.pop_front();
    }
}

bool Log::setOutputDest(OutputDest dest) {
    {
        unique_lock<mutex> uLock(_mtx);
        switch (dest) {
            case CONSOLE:
                // "console" logger defined in Log constructor.
                speedLog = spdlog::get("console");
                speedLog->set_level(_logLvl);
                _outputDest = SPEEDLOG;
                return true;
            case BUFFER:
                _outputDest = dest;
                return true;
            case MIRRORED:
                _outputDest = dest;
                return true;
            case SPEEDLOG:
                // Use the rotating file logger, if available.
                {
                    auto spl = spdlog::get("rt");
                    if (spl != nullptr) {
                        speedLog = spl;
                        speedLog->info("using speedLog");
                    } else {
                        speedLog = spdlog::get("console");
                        string eStr("Log::setOutputDest logger 'rt' was not found.");
                        speedLog->error(eStr);
                        cout << eStr << endl;
                    }
                }
                _outputDest = SPEEDLOG;
                speedLog->set_level(_logLvl);
                _bufferDump();
                return true;
            case COUT:
                _outputDest = COUT;
                return true;
            default:
                cout << "Log::setOuputDest unknown dest=" << dest << " using COUT\n";
                _outputDest = COUT;
        }
    }
    /// Throwing with _mtx locked will cause deadlock.
    string bMsg = string("Log::setOutputDest unknown dest=") + to_string(dest);
    throw Bug(ERR_LOC, bMsg);
}

const char* Log::getLogLvl(spdlog::level::level_enum const lvl) {
    switch (lvl) {
        case spdlog::level::level_enum::trace:
            return "TRACE";
        case spdlog::level::level_enum::debug:
            return "DEBUG";
        case spdlog::level::level_enum::info:
            return "INFO";
        case spdlog::level::level_enum::warn:
            return "WARN";
        case spdlog::level::level_enum::err:
            return "ERROR";
        case spdlog::level::level_enum::critical:
            return "CRITICAL";
        default:
            return "unknown";
    }
}

void Log::setLogLvl(spdlog::level::level_enum logLvl) {
    // Changing the log level mid log function may have
    // undesirable consequences.
    lock_guard<mutex> lck(_mtx);
    _logLvl = logLvl;
    speedLog->set_level(_logLvl);
}

spdlog::level::level_enum Log::getEnvironmentLogLvl() {
    char* chp = getenv("LOGLVL");
    if (chp != nullptr) {
        string inStr = chp;
        int inVal = stoi(inStr);
        switch (inVal) {
            case 1:
                return spdlog::level::trace;
            case 2:
                return spdlog::level::debug;
            case 3:
                return spdlog::level::info;
            case 4:
                return spdlog::level::warn;
            case 5:
                return spdlog::level::err;
            case 6:
                return spdlog::level::critical;
        }
    }
    return spdlog::level::trace;
}

void Log::useEnvironmentLogLvl() {
    auto eLogLvl = getEnvironmentLogLvl();
    LCRITICAL("using environment LOGLVL ", Log::getLogLvl(eLogLvl));
    setLogLvl(eLogLvl);
}

}  // namespace util
}  // namespace m2cellcpp
}  // namespace LSST
