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
#include "system/Log.h"

#include <fstream>
// Project headers
#include "util/Bug.h"

using namespace std;

namespace LSST {
namespace m2cellcpp {
namespace system {

Log& Log::getLog() {
    // Static initialization is generally very bad, but this needs
    // to be high performance. This avoids mutex complications.
    static Log logSingleton;
    return logSingleton;
}


Log::~Log() {
    lock_guard<mutex> lck(_mtx);
    _closeLogFiles();
}


void Log::logW(std::stringstream& msg) {
    lock_guard<mutex> lck(_mtx);
    switch (_outputDest) {
        case FILE:
            *_logFile << msg.str() << std::endl;
            break;
        case BUFFER:
            _buffers.emplace_back(msg.str());
            _reduceBuffers();
            break;
        case MIRRORED:
            {
                cout << msg.str() << endl;
                _buffers.emplace_back(msg.str());
                _reduceBuffers();
            }
            break;
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
    while (!_buffers.empty()) {
        auto& str = _buffers.front();
        if (_outputDest == FILE) {
            *_logFile << str << std::endl;
        } else {
            cout << str << std::endl;
        }
        _buffers.pop_front();
    }
}


bool Log::setOutputDest(OutputDest dest, std::string const& fileName) {
    {
        unique_lock<mutex> uLock(_mtx);
        switch (dest) {
            case COUT:
            case BUFFER:
            case MIRRORED:
                _outputDest = dest;
                _closeLogFiles();
                return true;
            case FILE:
                // _setOutputToFile will unLock uLock
                _outputDest = FILE;
                return _setOutputToFile(fileName, uLock);
            default:
                cout << "Log::setOuputDest unknown dest=" << dest;
        }
    }
    /// Throwing with _mtx locked will cause deadlock.
    //throw util::Bug(string("Log::setOutputDest unknown dest=") + dest) //&&& uncomment
    return false; //&&& delete when bug thrown
}


bool Log::_setOutputToFile(std::string const& fileName, std::unique_lock<std::mutex> &uLock) {
    if (fileName == "") {
        uLock.unlock();
        LERROR("Log::_setOutputToFile fileName was empty");
        return false;
    }
    if (_logFile != nullptr && _logFile->is_open()) {
        _logFile->close();
    }
    // TODO: Just truncate the existing file until log rotation is
    //       implemented in DM-34304
    _logFile = make_unique<ofstream>(fileName, ios::out | ios::trunc); 
    if (_logFile->fail()) {
        _outputDest = COUT;
        _bufferDump();
        uLock.unlock();
        LERROR("Log::_setOutputToFile failed to open ", fileName);
        return false;
    }
    _bufferDump();
    uLock.unlock();
    return true;
}


void Log::_closeLogFiles() {
    if (_logFileBaseName == "") return;
    _logFile->close();
}


const char* Log::getLogLvl(LogLvl lvl) {
    switch (lvl) {
    case TRACE:
        return "TRACE";
    case DEBUG:
        return "DEBUG";
    case INFO:
        return "INFO";
    case WARN:
        return "WARN";
    case ERROR:
        return "ERROR";
    case CRITICAL:
        return "CRITICAL";
    default:
        return "unknown"; 
    }
}

}}} // LSST::m2cellcpp::system
