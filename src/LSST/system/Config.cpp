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
#include "system/Config.h"
#include "util/Log.h"

// System headers

// Project headers

using namespace std;

namespace LSST {
namespace m2cellcpp {
namespace system {

// static Config members
Config::Ptr Config::_thisPtr;
std::mutex Config::_thisMtx;

string Config::getEnvironmentCfgPath(string const& defaultPath) {
    string result = defaultPath;
    char* path = getenv("M2CELL_CFG_PATH");
    if (path != nullptr) {
        result = path;
    }
    result += "/";
    return result;
}

void Config::setup(std::string const& source) {
    lock_guard<mutex> lock(_thisMtx);
    if (_thisPtr) {
        LERROR("Config already setup");
        return;
    }
    _thisPtr = Ptr(new Config(source));
}

void Config::reset() {
    LCRITICAL("Config reseting global configuration!!!");
    lock_guard<mutex> lock(_thisMtx);
    _thisPtr.reset();
}

Config::Config(std::string const& source) : _source(source) {
    try {
        LINFO("Config trying to load yaml file ", _source);
        _yaml = YAML::LoadFile(_source);
    } catch (YAML::BadFile& ex) {
        throw ConfigException(ERR_LOC, string("YAML::BadFile ") + ex.what());
    }
    verifyRequiredElements();
}

void Config::verifyRequiredElements() {
    try {
        LINFO("Config::verifyRequiredElements ", _source);
        string host = getControlServerHost();
        LINFO("ControlServer:host=", host);

        int port = getControlServerPort();
        LINFO("ControlServer:port=", port);

        int threads = getControlServerThreads();
        LINFO("ControlServer:threads=", threads);

        host = getTelemetryServerHost();
        LINFO("TelemetryServer:host=", host);

        port = getTelemetryServerPort();
        LINFO("TelemetryServer:port=", port);

        threads = getTelemetryServerThreads();
        LINFO("TelemetryServer:threads=", threads);

    } catch (exception const& ex) {
        LCRITICAL("Config::verifyRequiredElements config file ", _source, " needs valid ", ex.what());
        throw;
    }
}

Config& Config::get() {
    if (_thisPtr == nullptr) {
        throw ConfigException(ERR_LOC, "Config has not been setup.");
    }
    return *_thisPtr;
}

int Config::getControlServerPort() {
    string section = "ControlServer";
    string key = "port";
    return getSectionKeyAsInt(section, key, 1, 65535);
}

int Config::getControlServerThreads() {
    string section = "ControlServer";
    string key = "threads";
    return getSectionKeyAsInt(section, key, 1, 3000);
}

string Config::getControlServerHost() {
    string section = "ControlServer";
    string key = "host";
    return getSectionKeyAsString(section, key);
}

int Config::getTelemetryServerPort() {
    string section = "TelemetryServer";
    string key = "port";
    return getSectionKeyAsInt(section, key, 1, 65535);
}

int Config::getTelemetryServerThreads() {
    string section = "TelemetryServer";
    string key = "threads";
    return getSectionKeyAsInt(section, key, 1, 3000);
}

string Config::getTelemetryServerHost() {
    string section = "TelemetryServer";
    string key = "host";
    return getSectionKeyAsString(section, key);
}

int Config::getSectionKeyAsInt(string const& section, string const& key) {
    if (!_yaml[section][key]) {
        throw ConfigException(ERR_LOC, string("Config") + section + ": " + key + " is missing");
    }
    try {
        int val = _yaml[section][key].as<int>();
        return val;
    } catch (exception const& ex) {
        throw ConfigException(ERR_LOC, string("Config") + section + ": " + key + " failed int " + ex.what());
    }
}

double Config::getSectionKeyAsDouble(string const& section, string const& key) {
    if (!_yaml[section][key]) {
        throw ConfigException(ERR_LOC, string("Config") + section + ": " + key + " is missing");
    }
    try {
        double val = _yaml[section][key].as<double>();
        return val;
    } catch (exception const& ex) {
        throw ConfigException(ERR_LOC,
                              string("Config") + section + ": " + key + " failed double " + ex.what());
    }
}

double Config::getSectionKeyAsDouble(string const& section, string const& key, double min, double max) {
    double val = getSectionKeyAsDouble(section, key);
    if (val < min || val > max) {
        throw ConfigException(ERR_LOC, section + ":" + key + "=" + to_string(val) + " must be between " +
                                               to_string(min) + " & " + to_string(max));
    }
    return val;
}

string Config::getSectionKeyAsString(string const& section, string const& key) {
    if (!_yaml[section][key]) {
        throw ConfigException(ERR_LOC, string("Config") + section + ": " + key + " is missing");
    }
    try {
        string str = _yaml[section][key].as<string>();
        return str;
    } catch (exception const& ex) {
        throw ConfigException(ERR_LOC,
                              string("Config") + section + ": " + key + " failed string " + ex.what());
    }
}

int Config::getSectionKeyAsInt(string const& section, string const& key, int min, int max) {
    int val = getSectionKeyAsInt(section, key);
    if (val < min || val > max) {
        throw ConfigException(ERR_LOC, section + ":" + key + "=" + to_string(val) + " must be between " +
                                               to_string(min) + " & " + to_string(max));
    }
    return val;
}

}  // namespace system
}  // namespace m2cellcpp
}  // namespace LSST
