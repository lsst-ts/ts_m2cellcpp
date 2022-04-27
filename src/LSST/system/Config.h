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
// TODO #include "nlohmann/json.hpp" or yaml &&&
#include "yaml-cpp/yaml.h"

// This header declarations
namespace LSST {
namespace m2cellcpp {
namespace system {

/// This class reads and stores the system configuration.
/// Eventually, this class will read in a configuration file,
/// maybe yaml or json. Until then, it will just store a map
/// of values and return those values requested.
/// Config::setup() should be called as early in the program
/// as possible as many objects will require this class.
/// unit test: test_config.cpp
class Config {
public:
    typedef std::shared_ptr<Config> Ptr;

    /// Setup the global instance of Config.
    /// Calling this function is expected to be the first
    /// thing that happens in the executable
    /// (probably after Log setup).
    /// @param source the source for configuration values.
    ///     "UNIT_TEST" indicates a set of default values for unit tests.
    /// @throws invalid_argument when source is invalid.
    static void setup(std::string const& source);

    /// Get access to the global configuration.
    /// Calling this before setup() will throw.
    /// @return a reference to the global Config object.
    /// @throws runtime_error when setup() has not been called.
    static Config& get();

    ///&&&doc
    std::string getControlServerHost();

    /// &&&doc
    int getControlServerPort();

    /// &&&doc
    int getControlServerThreads();

    /// @throws `util::Bug` if any required elements are missing or fail conversion.
    void verifyRequiredElements();

    /// Temporary method to access values.
    /// TODO: methods to access specific keys that return typed values
    std::string getValue(std::string const& section, std::string const& key);

private:
    /// @see setup.
    Config(std::string const& source);

    /// @return the `key` value in `section` as an integer.
    /// @throws `util::Bug` if the key is missing or is not an `int`.
    int _getSectionKeyAsInt(std::string const& section, std::string const& key);

    /// @return the `key` value in `section` as a string.
    /// @throws `util::Bug` if the key is missing or is not a `string`.
    std::string _getSectionKeyAsString(std::string const& section, std::string const& key);

    /// Set config values for unit tests. &&&
    void _setValuesUnitTests();  //&&&

    void _setValue(std::string const& section, std::string const& key, std::string const& val); //&&&

    static Ptr _thisPtr;  ///< Pointer to the global instance of Config.
    static std::mutex _thisMtx;
    std::map<std::string, std::string> _map;  ///< map of section:key -> values.

    YAML::Node _yaml;  ///< yaml storage object
};

}  // namespace system
}  // namespace m2cellcpp
}  // namespace LSST

#endif  // LSST_M2CELLCPP_SYSTEM_CONFIG_H
