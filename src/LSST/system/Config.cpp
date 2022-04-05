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

void Config::setup(std::string const& source) {
    lock_guard<mutex> lock(_thisMtx);
    if (_thisPtr) {
        LWARN("Config already setup");
        return;
    }
    _thisPtr = Ptr(new Config(source));
}

Config::Config(std::string const& source) {
    if (source == "UNIT_TEST") {
        // source ignored in this case.
        _setValuesUnitTests();
    } else {
        throw invalid_argument("Config had invalid source " + source);
    }
}

Config& Config::get() {
    if (_thisPtr == nullptr) {
        throw runtime_error("Config has not been setup.");
    }
    return *_thisPtr;
}

void Config::_setValuesUnitTests() {
    _setValue("server", "port", "12678");
    _setValue("server", "threads", "1");
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

}  // namespace system
}  // namespace m2cellcpp
}  // namespace LSST
