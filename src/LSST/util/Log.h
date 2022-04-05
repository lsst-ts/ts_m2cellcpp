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
#ifndef LSST_M2CELLCPP_UTIL_LOG_H
#define LSST_M2CELLCPP_UTIL_LOG_H

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
#define LTRACE(...) \
    LSST::m2cellcpp::util::Log::logW(LSST::m2cellcpp::util::Log::TRACE, __FILE__, __LINE__, __VA_ARGS__)
#define LDEBUG(...) \
    LSST::m2cellcpp::util::Log::logW(LSST::m2cellcpp::util::Log::DEBUG, __FILE__, __LINE__, __VA_ARGS__)
#define LINFO(...) \
    LSST::m2cellcpp::util::Log::logW(LSST::m2cellcpp::util::Log::INFO, __FILE__, __LINE__, __VA_ARGS__)
#define LWARN(...) \
    LSST::m2cellcpp::util::Log::logW(LSST::m2cellcpp::util::Log::WARN, __FILE__, __LINE__, __VA_ARGS__)
#define LERROR(...) \
    LSST::m2cellcpp::util::Log::logW(LSST::m2cellcpp::util::Log::ERROR, __FILE__, __LINE__, __VA_ARGS__)
#define LCRITICAL(...) \
    LSST::m2cellcpp::util::Log::logW(LSST::m2cellcpp::util::Log::CRITICAL, __FILE__, __LINE__, __VA_ARGS__)

namespace LSST {
namespace m2cellcpp {
namespace util {

/// This class is for writing log messages efficiently.
/// Log messages may be writen to disk, stdout, or temporarily stored in a buffer.
/// See `Log::outputDest()` for details.
/// It is recommended to use MIRRORED until the configuration file has been
/// read so that log messages show up on stdout and then are writen to the
/// log file when it is open.
/// The varadic template function are convenient, but have a tendancy to
/// make copies despite being references. This can cause issues when
/// logging references with deleted copy constructor.
/// The most expensive frequent operation is the creation of `stringstream msg;`
/// in `logW()`. Making it a member is possible but has other complications.
/// unit test: test_Log.cpp
class Log {
public:
    enum LogLvl { TRACE = 1, DEBUG, INFO, WARN, ERROR, CRITICAL };
    enum OutputDest { COUT = 1, FILE, BUFFER, MIRRORED };

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
    bool setOutputDest(OutputDest dest, std::string const& fileName = "");

    Log(Log const&) = delete;
    Log& operator=(Log const&) = delete;

    ~Log();

    /// @return a reference to the single instance of Log.
    static Log& getLog();

    /// @return the system wide log level.
    /// Only log messages with a LogLvl greater than _logLvl will be recorded.
    LogLvl getLogLvl() const { return _logLvl; }

    /// Set the minimum LogLvl to log.
    void setLogLvl(LogLvl logLvl);

    /// @return a string version of the LogLvl.
    static const char* getLogLvl(LogLvl lvl);

    /// The base logging function called by macros.
    template <typename... Args>
    static void logW(LogLvl lvl, const char* file, int line, const Args&... args) {
        Log& lg = Log::getLog();
        if (lvl < lg.getLogLvl()) {
            return;
        }
        std::stringstream msg;
        std::thread::id tid = std::this_thread::get_id();
        msg << file << ":" << line << " tid:" << std::hex << tid << std::dec << " " << getLogLvl(lvl) << " ";
        lg.logW(msg, args...);
    }

    /// This function recursively adds the `argss` to `msg`.
    /// When `args` is empty, `logW(std::stringstream& msg)` is called.
    template <typename T, typename... Args>
    void logW(std::stringstream& msg, T val, const Args&... args) {
        msg << val;
        logW(msg, args...);
    }

    /// The terminating function called in the recursive `logW` call.
    /// This function will write the contents of msg to the
    /// location indicated by `_outputDest`
    void logW(std::stringstream& msg);

    /// @return the last string in _buffers.
    /// Used for testing.
    std::string getBufferLast();

    /// @return the number of strings in _buffers.
    /// Used for testing.
    uint getBuffersSize() const;

    /// Set the maximum number of buffers stored when using
    /// BUFFER or MIRRORED as the destination.
    void setMaxBuffers(int maxBuffers);

private:
    /// Private constructor as only instance of Log should be availabel through getLog().
    /// Log needs to be available before the configuration is read so that configuration
    /// values can be logged.
    Log() = default;

    /// Set the `_outputDest` to `FILE`, open `fileName`, and put `_buffers` into the file.
    /// If the filecannot be opened, `_outputDest` is set to `COUT`.
    /// @return true if the file was opened and messages could be written.
    /// @param uLock - unique_lock holding _mtx that will be unlocked before this
    ///                function finishes.
    /// note: _mtx must be held before calling
    bool _setOutputToFile(std::string const& fileName, std::unique_lock<std::mutex>& uLock);

    /// Close the log file.
    /// note: _mtx must be held before calling
    void _closeLogFile();

    /// Remove the strings from `_buffers` and write them to `_outputDest`.
    /// note: _mtx must be held before calling
    void _bufferDump();

    /// Limit the number of string in `_buffers` to _maxBuffers.
    /// Oldest strings thrown out first.
    /// note: _mtx must be held before calling
    void _reduceBuffers();

    std::atomic<LogLvl> _logLvl{TRACE};  ///< System wide log level.

    OutputDest _outputDest{COUT};  ///< Where the log messages will be stored.

    std::string _logFileBaseName;             ///< Log file name when `_outputDest == FILE`.
    std::unique_ptr<std::ofstream> _logFile;  ///< Handle for the log file.
    /// List of string to temporarily store log messages.
    std::deque<std::string> _buffers;
    uint _maxBuffers = 2000;  ///< maximum size of _buffers

    mutable std::mutex _mtx;  ///< protects all member variables
};

}  // namespace util
}  // namespace m2cellcpp
}  // namespace LSST

#endif  // LSST_M2CELLCPP_UTIL_LOG_H
