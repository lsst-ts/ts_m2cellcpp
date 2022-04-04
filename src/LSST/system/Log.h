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
#ifndef LSST_M2CELLCPP_SYSTEM_LOG_H
#define LSST_M2CELLCPP_SYSTEM_LOG_H

// System headers
#include <atomic>
#include <deque>
#include <iostream>
#include <memory>
#include <mutex>
#include <sstream>
#include <string>
#include <thread>

// Project headers

// Third party headers

// evil macros
#define LTRACE(...) LSST::m2cellcpp::system::Log::logW(LSST::m2cellcpp::system::Log::TRACE, \
     __FILE__, __LINE__, __VA_ARGS__)
#define LDEBUG(...) LSST::m2cellcpp::system::Log::logW(LSST::m2cellcpp::system::Log::DEBUG, \
     __FILE__, __LINE__, __VA_ARGS__)
#define LINFO(...) LSST::m2cellcpp::system::Log::logW(LSST::m2cellcpp::system::Log::INFO, \
     __FILE__, __LINE__, __VA_ARGS__)
#define LWARN(...) LSST::m2cellcpp::system::Log::logW(LSST::m2cellcpp::system::Log::WARN, \
     __FILE__, __LINE__, __VA_ARGS__)
#define LERROR(...) LSST::m2cellcpp::system::Log::logW(LSST::m2cellcpp::system::Log::ERROR, \
     __FILE__, __LINE__, __VA_ARGS__)
#define LCRITICAL(...) LSST::m2cellcpp::system::Log::logW(LSST::m2cellcpp::system::Log::CRITICAL, \
     __FILE__, __LINE__, __VA_ARGS__)


namespace LSST {
namespace m2cellcpp {
namespace system {

/// &&& doc
/// unit test: test_Log.cpp
class Log {
public:
    enum LogLvl { TRACE = 1, DEBUG, INFO, WARN, ERROR, CRITICAL };
    enum OutputDest { COUT = 1, FILE, BUFFER, MIRRORED};

    /// Select the destination for log messages.
    /// @return true if the opperation was successful.
    /// @param dest  Destination type one of: 
    ///             COUT - standard out
    ///             BUFFER - internal buffer
    ///             MIRRORED - both COUT and internal buffer
    ///             FILE - to a file indicated by `fileName`. If
    ///               `fileName` is invalid or empty, COUT will be used and
    ///               false will be returned.
    /// @param fileName Must be set to a valid file name when 'dest' is FILE.
    ///
    /// When FILE is selected, the contents of the internal buufer
    /// are written to the file and the internal buffer is deleted.
    bool setOutputDest(OutputDest dest, std::string const& fileName="");

    Log(Log const&) = delete;
    Log& operator=(Log const&) = delete;

    ~Log();

    /// @return a reference to the single instance of Log.
    static Log& getLog();

    /// @return the system wide log level.
    /// Only log messages with a LogLvl greater than _logLvl will be recorded.
    LogLvl getLogLvl() { return _logLvl; }

    /// @return a string version of the LogLvl.
    static const char* getLogLvl(LogLvl lvl);

    /// The base logging function called by macros.
    template<typename... Args>
    static void logW(LogLvl lvl, const char* file, int line, const Args&... args) {
        std::stringstream msg;
        Log& lg = Log::getLog();
        if (lvl < lg.getLogLvl()) {
            return;
        } 
        std::thread::id tid = std::this_thread::get_id();
        msg << file << ":" << line << " tid:" << std::hex << tid << std::dec << " " << getLogLvl(lvl) << " ";
        lg.logW(msg, args...); 
    }

    /// The terminating function called in the recursive `logW` call.
    /// This function will write the contents of msg to the 
    /// location indicated by `_outputDest`
    void logW(std::stringstream& msg);

    /// This function recursively adds the `argss` to `msg`.
    /// When `args` is empty, `logW(std::stringstream& msg)` is called.
    template<typename T, typename... Args>
    void logW(std::stringstream& msg, T val, const Args&... args) {
        msg << val;
        logW(msg, args...);
    }

    /// @return the last string in _buffers.
    /// Used for testing.
    std::string getBufferLast();

    /// @return the number of strings in _buffers.
    /// Used for testing.
    uint getBuffersSize() const;

    /// Set the maximum number of buffers stored when using
    /// BUFFER or MIRRORED as the destination.
    void setMaxBuffers(int maxBuffers);

    //&&& delete
    static void log(LogLvl lvl, std::string const& str)  {
        std::cout << lvl << " "  << str << std::endl;
    }

private:
    /// Private constructor as only instance of Log should be availabel through getLog().
    /// Log needs to be available before the configuration is read so that configuration
    /// values can be logged.
    Log() = default;

    /// &&& doc
    /// @param uLock - unique_lock holding _mtx that will be unlocked before this 
    ///                function finishes.
    /// note: _mtx must be held before calling
    bool _setOutputToFile(std::string const& fileName, std::unique_lock<std::mutex> &uLock);

    /// &&& doc
    /// note: _mtx must be held before calling
    void _closeLogFiles();

    ///&&& doc
    /// note: _mtx must be held before calling
    void _bufferDump();

    ///&&& doc
    /// note: _mtx must be held before calling
    void _reduceBuffers();

    std::atomic<LogLvl> _logLvl{TRACE}; ///< System wide log level.

    OutputDest _outputDest{COUT}; ///< &&& doc

    std::string _logFileBaseName;
    std::unique_ptr<std::ofstream> _logFile;
    std::deque<std::string> _buffers; ///< &&& doc
    uint _maxBuffers = 2000; ///< maximum size of _buffers

    mutable std::mutex _mtx; ///< protects all member variables
};

}  // namespace system
}  // namespace m2cellcpp
}  // namespace LSST

#endif  // LSST_M2CELLCPP_SYSTEM_COMCLIENT_H
