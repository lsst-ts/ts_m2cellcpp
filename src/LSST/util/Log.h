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
#define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_TRACE
#include <spdlog/spdlog.h>

// evil macros
#define LTRACE(...) LSST::m2cellcpp::util::Log::logW(spdlog::level::trace, __FILE__, __LINE__, __VA_ARGS__)
#define LDEBUG(...) LSST::m2cellcpp::util::Log::logW(spdlog::level::debug, __FILE__, __LINE__, __VA_ARGS__)
#define LINFO(...) LSST::m2cellcpp::util::Log::logW(spdlog::level::info, __FILE__, __LINE__, __VA_ARGS__)
#define LWARN(...) LSST::m2cellcpp::util::Log::logW(spdlog::level::warn, __FILE__, __LINE__, __VA_ARGS__)
#define LERROR(...) LSST::m2cellcpp::util::Log::logW(spdlog::level::err, __FILE__, __LINE__, __VA_ARGS__)
#define LCRITICAL(...) \
    LSST::m2cellcpp::util::Log::logW(spdlog::level::critical, __FILE__, __LINE__, __VA_ARGS__)

// These macros are the same as the 'SPDLOG_LOGGER_TRACE' macros, just shorter names and the
// logger is Log::speedLog.
// Unlike calling Log::speedlog->trace(...); these include file and line number.
#define SPDTRACE(...) \
    SPDLOG_LOGGER_CALL(LSST::m2cellcpp::util::Log::speedLog, spdlog::level::trace, __VA_ARGS__)
#define SPDDEBUG(...) \
    SPDLOG_LOGGER_CALL(LSST::m2cellcpp::util::Log::speedLog, spdlog::level::debug, __VA_ARGS__)
#define SPDINFO(...) \
    SPDLOG_LOGGER_CALL(LSST::m2cellcpp::util::Log::speedLog, spdlog::level::info, __VA_ARGS__)
#define SPDWARN(...) \
    SPDLOG_LOGGER_CALL(LSST::m2cellcpp::util::Log::speedLog, spdlog::level::warn, __VA_ARGS__)
#define SPDERROR(...) \
    SPDLOG_LOGGER_CALL(LSST::m2cellcpp::util::Log::speedLog, spdlog::level::err, __VA_ARGS__)
#define SPDCRITICAL(...) \
    SPDLOG_LOGGER_CALL(LSST::m2cellcpp::util::Log::speedLog, spdlog::level::critical, __VA_ARGS__)

namespace LSST {
namespace m2cellcpp {
namespace util {

/// This class is for writing log messages efficiently, and is using spdlog.
/// Changes to `Log::speedLog` and the loggers defined by `Log` should be made
/// through `Log`.
/// Log messages may be writen to disk, stdout, or temporarily stored in a buffer.
/// See `Log::outputDest()` for details.
/// It is recommended to use MIRRORED until the configuration file has been
/// read so that log messages show up on stdout and then are writen to the
/// log file when it is open.
/// Once the log file has been opened, there should be no need to change the
/// `Log::_outputDest`, and it should not be set to nullptr.
/// The varadic template function are convenient, but have a tendancy to
/// make copies despite being references. This can cause issues when
/// logging references with deleted copy constructor.
/// The most expensive frequent operation is the creation of `stringstream msg;`
/// in `logW()`. Making it a member is possible but has other complications.
/// unit test: test_Log.cpp
///
/// Using spdlog has some pros and cons, and this class did not originally use it.
///  spdlog pros:
///      - fast
///      - includes log file rotation and other nice features
///      - can be setup to work nicely with other libraries that use spdlog
///      - fast way to get time and date into the logs
///  cons:
///      - somewhat slower compile time
///      - The format syntax it uses is prone to runtime errors with missing
///        or extra elements or unmatched curly brackets, such as
///        `SPDDEBUG("test msg {", 34.2);`, which results in nothing being
///        put in the log. This can be caught at runtime by using `FMT_STRING`, like
///        `SPDDEBUG(FMT_STRING("test msg {"), 34.2);`
///        `FMT_STRING` should be used in all calls using spdlog directly, and
///        can't be used inside of macros.
///      - Using spdlog::set_pattern() to get the thread id in the log results
///        in the spdlog macros (anything that uses SPDLOG_LOGGER_CALL) dropping
///        the file and line number (?!). This can probably be fixed, but is a currently a
///        mystery. So anything using spdlog directly will not have the
///        thread id unless added in the log statement.
///        Also, using spdlog::set_pattern causes `LDEBUG` and its kin to slow significantly,
///        another mystery. Thankfully, the default pattern is adequate for `LDEBUG`.
/// At this time, using `LINFO` instead of `SPDINFO` is about 0.2-0.4 microseconds slower, but includes
/// thread id. The contents of the log message, surprisingly, seem to have little impact (big
/// messages with lots of conversions still only take about 0.3 microseconds longer).
/// Testing code is defined out in tests/test_Log.cpp.
///
/// In most cases, `LINFO` and its ilk should be fast enough, include thread id, and are not
/// prone to runtime errors.
/// If speed is essential, `SPDINFO(FMT_STRING())` should be used.
///
/// unit tests in tests/test_Log.cpp
class Log {
public:
    enum OutputDest { COUT = 1, CONSOLE, SPEEDLOG, BUFFER, MIRRORED };

    /// Select the destination for log messages.
    /// @return true if the opperation was successful.
    /// @param dest  Destination type one of:
    ///             CONSOLE - standard out using spdlog with logger "console"
    ///             BUFFER - internal buffer
    ///             MIRRORED - both COUT and internal buffer
    ///             SPEEDLOG - to a file indicated by `fileName` using logger "rt".
    ///                      The logger must be setup, by calling setupFileRotation(...), before
    ///                      calling this function.
    ///             COUT - standard out - in the very unlikely case spdlog has an issue.
    /// @param fileName Must be set to a valid file name when 'dest' is FILE.
    ///
    /// When SPEEDLOG is selected, the contents of the internal buffer
    /// are written to the file and the internal buffer is deleted.
    ///
    /// Note that CONSOLE and SPEEDLOG both write to Log::speedLog. If the value
    /// of Log::speedLog is changed, output will go to the new value of Log::speedLog.
    bool setOutputDest(OutputDest dest);

    Log(Log const&) = delete;
    Log& operator=(Log const&) = delete;

    ~Log();

    /// A static pointer to the `spdlog::logger` that `Log` is using.
    /// Always use `Log` to change its value.
    static std::shared_ptr<spdlog::logger> speedLog;

    /// @return a reference to the single instance of Log.
    static Log& getLog();

    /// Flush speedLog to disk, if it exists.
    void flush() {
        if (speedLog) speedLog->flush();
    }

    /// Setup a `spdlg::logger` named `rt` for rotating log files.
    /// This only setups up the rotating file logger, Log::setOutputDest(SPEEDLOG)
    /// must be called for it to be used.
    /// This function can only be called once. If it needs to be called
    /// a second time, the "rt" logger needs to be removed from spdlog,
    /// which will likely cause difficult race conditions.
    bool setupFileRotation(std::string const& fileName, size_t fileSize, size_t maxFiles);

    /// Only log messages with a LogLvl greater than or equal to _logLvl will be recorded.
    /// @return the system wide log level.
    spdlog::level::level_enum getLogLvl() const { return speedLog->level(); }

    /// Set the minimum logLvl to log. logLvl uses spdlog::level::level_enum.
    /// This value is passed to `speedLog`.
    void setLogLvl(spdlog::level::level_enum logLvl);

    /// Return a string version of the LogLvl.
    static const char* getLogLvl(spdlog::level::level_enum const lvl);

    /// The base logging function called by `LINFO` style macros.
    /// It adds the thread id to the log message.
    template <typename... Args>
    static void logW(spdlog::level::level_enum lvl, const char* file, int line, const Args&... args) {
        Log& lg = Log::getLog();
        if (lvl < lg.getLogLvl()) {
            return;
        }
        std::stringstream msg;  // This is a likely target for optimization.
        std::thread::id tid = std::this_thread::get_id();
        msg << "[" << file << ":" << line << "] tid:" << std::hex << tid << std::dec << " ";
        lg.logW(lvl, msg, args...);
    }

    /// This function recursively adds the `args` to `msg`.
    /// When `args` is empty, `logW(std::stringstream& msg)` is called.
    template <typename T, typename... Args>
    void logW(spdlog::level::level_enum lvl, std::stringstream& msg, T val, const Args&... args) {
        msg << val;
        logW(lvl, msg, args...);
    }

    /// The terminating function called in the recursive `logW` call.
    /// This function will write the contents of msg to the
    /// location indicated by `_outputDest`.
    void logW(spdlog::level::level_enum lvl, std::stringstream& msg);

    /// Return the last string in _buffers, used for testing.
    std::string getBufferLast();

    /// Return the number of strings in _buffers, used for testing.
    uint getBuffersSize() const;

    /// Set the maximum number of buffers stored when using
    /// BUFFER or MIRRORED as the destination.
    void setMaxBuffers(int maxBuffers);

    /// Return the value of environment variabe LOGLVL, defaults to TRACE (1).
    /// values are
    /// TRACE=1, DEBUG=2, INFO=3, WARN=4, ERROR=5, CRITICAL=6. Values are
    /// constrained such that values less than TRACE are set to TRACE, and
    /// values greater than CRITCAL are set to CRITICAL.
    static spdlog::level::level_enum getEnvironmentLogLvl();

    /// Set the current _logLvl to the environment LOGLVL
    /// Useful for quieting down local unit tests.
    /// @see Log::getEnvironmentLogLvl()
    void useEnvironmentLogLvl();

private:
    /// Private constructor as only instance of Log should be availabel through getLog().
    /// Log needs to be available before the configuration is read so that configuration
    /// values can be logged.
    Log();

    /// Remove the strings from `_buffers` and write them to `_outputDest`.
    /// note: _mtx must be held before calling
    void _bufferDump();

    /// Limit the number of string in `_buffers` to _maxBuffers.
    /// Oldest strings thrown out first.
    /// note: _mtx must be held before calling
    void _reduceBuffers();

    /// System wide log level, passed to `speedlog`.
    std::atomic<spdlog::level::level_enum> _logLvl;

    OutputDest _outputDest{COUT};  ///< Where the log messages will be stored.

    std::string _logFileBaseName;  ///< Log file name when `_outputDest == FILE`.

    /// List of strings to temporarily store log messages.
    std::deque<std::string> _buffers;
    uint _maxBuffers = 2000;  ///< maximum size of _buffers (need enough to read config)

    mutable std::mutex _mtx;  ///< protects all member variables
};

}  // namespace util
}  // namespace m2cellcpp
}  // namespace LSST

#endif  // LSST_M2CELLCPP_UTIL_LOG_H
