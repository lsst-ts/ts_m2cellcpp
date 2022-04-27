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
#include "util/Bug.h"
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

void Config::setup(std::string const& source) {
    lock_guard<mutex> lock(_thisMtx);
    if (_thisPtr) {
        LERROR("Config already setup");
        return;
    }
    _thisPtr = Ptr(new Config(source));
}

Config::Config(std::string const& source) {
    if (source == "UNIT_TEST") {
        // source ignored in this case.
        _setValuesUnitTests();
    } else {
        try {
            LINFO("Config trying to load yaml file ", source);
            _yaml = YAML::LoadFile(source);
        } catch (YAML::BadFile& ex) {
            throw util::Bug(ERR_LOC, string("YAML::BadFile ") + ex.what());
        }
    }
}

Config& Config::get() {
    if (_thisPtr == nullptr) {
        throw util::Bug(ERR_LOC, "Config has not been setup.");
    }
    return *_thisPtr;
}

void Config::_setValuesUnitTests() {
    _setValue("controlServer", "port", "12678");
    _setValue("controlServer", "threads", "1");
}

void Config::_setValue(string const& section, string const& key, string const& val) {
    string secKey = section + ":" + key;
    auto iter = _map.find(secKey);
    if (iter != _map.end()) {
        LWARN("Config trying to reset ", secKey, " from ", iter->second, " to ", val);
    }
    _map[secKey] = val;
    LINFO("Config set ", secKey, "=", val);
}

string Config::getValue(string const& section, string const& key) {
    string secKey = section + ":" + key;
    auto iter = _map.find(secKey);
    if (iter == _map.end()) {
        throw invalid_argument("ERROR Configure unknown key " + secKey);
    }
    string ret = iter->second;
    return ret;
}

int Config::getControlServerPort() {
    string section = "ControlServer";
    string key = "port";
    return _getSectionKeyAsInt(section, key);
}

int Config::getControlServerThreads() {
    string section = "ControlServer";
    string key = "threads";
    return _getSectionKeyAsInt(section, key);
}

string Config::getControlServerHost() {
    string section = "ControlServer";
    string key = "host";
    return _getSectionKeyAsString(section, key);
}

int Config::_getSectionKeyAsInt(string const& section, string const& key) {
    if (!_yaml[section][key]) {
        throw util::Bug(ERR_LOC, string("Config") + section + ": " + key + " is missing");
    }
    try {
        int val = _yaml[section][key].as<int>();
        return val;
    } catch (exception const& ex) {
        throw util::Bug(ERR_LOC, string("Config") + section + ": " + key + " failed int " + ex.what());
    }
}

string Config::_getSectionKeyAsString(string const& section, string const& key) {
    if (!_yaml[section][key]) {
        throw util::Bug(ERR_LOC, string("Config") + section + ": " + key + " is missing");
    }
    try {
        string str = _yaml[section][key].as<string>();
        return str;
    } catch (exception const& ex) {
        throw util::Bug(ERR_LOC, string("Config") + section + ": " + key + " failed string " + ex.what());
    }
}

/*&&&
string Config::getControlServerPort() {
    string section = "ControlServer";
    string key = "host";
    if (!_yaml["ControlServer"]["host"]) {
        throw util::Bug(ERR_LOC, "ControlServer: host is missing");
    }
    try {
        int port = _yaml["ControlServer"]["host"].as<int>();
        return port;
    } catch (exception const& ex) {
        throw util::Bug(ERR_LOC, string("ControlServer: port failed non-int ") + ex.what());
    }
}
*/

}  // namespace system
}  // namespace m2cellcpp
}  // namespace LSST
