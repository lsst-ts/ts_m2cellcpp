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
#ifndef LSST_M2CELLCPP_SYSTEM_CONFIG_H
#define LSST_M2CELLCPP_SYSTEM_CONFIG_H

// System headers
#include <assert.h>
#include <map>
#include <memory>
#include <mutex>
#include <string>

// Third party headers
#include "yaml-cpp/yaml.h"

// Project headers
#include "util/Issue.h"

namespace LSST {
namespace m2cellcpp {
namespace system {

/// Exception specific to Config.
/// unit test: test_NetCommand.cpp
class ConfigException : public util::Issue {
public:
    ConfigException(Context const& ctx, std::string const& msg) : util::Issue(ctx, msg) {}
};

/// This class reads and stores the system configuration from a yaml file.
/// Config::setup() should be called as early in the program
/// as possible as many objects will require this class.
/// Special functions should be made for critical values, like
/// `getControlServerPort()` and these should be tested in
/// `verifyRequiredElements()`. Missing or invalid elements
/// from the config file should cause program termination,
/// and this should happen before the program has done anything
/// significant.
/// unit test: test_config.cpp
class Config {
public:
    typedef std::shared_ptr<Config> Ptr;

    Config() = delete;
    Config(Config const&) = delete;
    Config& operator=(Config const&) = delete;

    ~Config() = default;

    /// Get the value of `M2CELL_CFG_PATH`, return `defaultPath` if it
    /// is undefined.
    static std::string getEnvironmentCfgPath(std::string const& defaultPath);

    /// Setup the global instance of Config.
    /// Calling this function is expected to be the first
    /// thing that happens in the executable
    /// (probably after Log setup).
    /// @param source the source yaml file for configuration values.
    /// @throws invalid_argument when source is invalid.
    static void setup(std::string const& source);

    /// Get access to the global configuration.
    /// Calling this before setup() will throw.
    /// @return a reference to the global Config object.
    /// @throws runtime_error when setup() has not been called.
    static Config& get();

    /// Get the `ControlServer: host` value from the config file.
    /// @return the `ControlServer: host` value.
    /// @throws `ConfigException` if it's missing.
    std::string getControlServerHost();

    /// Get the `Log: fileName` value from the config file.
    /// @return the `Log: fileName` value.
    /// @throws `ConfigException` if it's missing.
    std::string getLogFileName();

    /// Get the `Log: fileSizeMB` value from the config file.
    /// @return the `Log: fileSizeMB` value.
    /// @throws `ConfigException` if it's missing.
    int getLogFileSizeMB();

    /// Get the `Log: maxFiles` value from the config file.
    /// @return the `Log: maxFiles` value.
    /// @throws `ConfigException` if it's missing.
    int getLogMaxFiles();

    /// Get the `ControlServer: port` value from the config file.
    /// @return the `ControlServer: port` int value.
    /// @throws `ConfigException` if it's missing or out of range.
    int getControlServerPort();

    /// Get the `ControlServer: threads` value from the config file.
    /// @return the `ControlServer: threads` int value.
    /// @throws `ConfigException` if it's missing or out of range.
    int getControlServerThreads();

    /// Get the `TelemetryServer: host` value from the config file.
    /// @return the `TelemetryServer: host` value.
    /// @throws `ConfigException` if it's missing.
    std::string getTelemetryServerHost();

    /// Get the `TelemetryServer: port` value from the config file.
    /// @return the `TelemetryServer: port` int value.
    /// @throws `ConfigException` if it's missing or out of range.
    int getTelemetryServerPort();

    /// Get the `TelemetryServer: threads` value from the config file.
    /// @return the `TelemetryServer: threads` int value.
    /// @throws `ConfigException` if it's missing or out of range.
    int getTelemetryServerThreads();

    /// Check all required entries from the Config.
    /// @throws `ConfigException` if any required elements are missing or
    ///         fail conversion.
    /// If there are any problems with the configuration, they will be
    /// reported and the program should terminate.
    void verifyRequiredElements();

    /// Get the `key` value in `section` as an integer.
    /// @return the `key` value in `section` as an integer.
    /// @throws `ConfigException` if the key is missing, or is not an `int`.
    int getSectionKeyAsInt(std::string const& section, std::string const& key);

    /// Get the `key` value in `section` as an integer, with a range check.
    /// @return the `key` value in `section` as an integer, with a range check.
    /// @throws `ConfigException` if the key is missing, or is not an `int`, or
    ///         if it is outside of min and max.
    int getSectionKeyAsInt(std::string const& section, std::string const& key, int min, int max);

    /// Get the `key` value in `section` as a string.
    /// @return the `key` value in `section` as a string.
    /// @throws `ConfigException` if the key is missing, or is not a `string`.
    std::string getSectionKeyAsString(std::string const& section, std::string const& key);

    /// Get the `key` value in `section` as a double.
    /// @return the `key` value in `section` as a double.
    /// @throws `ConfigException` if the key is missing, or is not a `double`.
    double getSectionKeyAsDouble(std::string const& section, std::string const& key);

    /// Get the `key` value in `section` as a double, with a range check.
    /// @return the `key` value in `section` as a double, with a range check.
    /// @throws `util::Bug` if the key is missing, is not a `double`, or
    ///         if it is outside of min and max.
    double getSectionKeyAsDouble(std::string const& section, std::string const& key, double min, double max);

    /// Reset the global Config so a new configuration can be read.
    /// The only expected use for this is unit testing.
    static void reset();

private:
    /// @see setup.
    Config(std::string const& source);

    static Ptr _thisPtr;         ///< Pointer to the global instance of Config.
    static std::mutex _thisMtx;  ///< Protects _thisPtr.
    std::string _source;         ///< name of the source for config values.
    YAML::Node _yaml;            ///< yaml storage object
};

}  // namespace system
}  // namespace m2cellcpp
}  // namespace LSST

#endif  // LSST_M2CELLCPP_SYSTEM_CONFIG_H
