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
#include "util/InstanceCount.h"

// System Headers

// Project headers
#include "system/Log.h"

using namespace std;

namespace LSST {
namespace m2cellcpp {
namespace util {

map<string, int> InstanceCount::_instances;
recursive_mutex InstanceCount::_mx;


InstanceCount::InstanceCount(string const& className) :_className{className} {
    _increment("con");
}


InstanceCount::InstanceCount(InstanceCount const& other) : _className{other._className} {
    _increment("cpy");
}


InstanceCount::InstanceCount(InstanceCount &&origin) : _className{origin._className} {
    _increment("mov");
}


void InstanceCount::_increment(string const& source) {
    lock_guard<recursive_mutex> lg(_mx);
    pair<string const, int> entry(_className, 0);
    auto ret = _instances.insert(entry);
    auto iter = ret.first;
    iter->second += 1;
    LDEBUG("InstanceCount ", source, " ", iter->first, "=",iter->second);
}


InstanceCount::~InstanceCount() {
    lock_guard<recursive_mutex> lg(_mx);
    auto iter = _instances.find(_className);
    if (iter != _instances.end()) {
        iter->second -= 1;
        LDEBUG("~InstanceCount ", iter->first, "=", iter->second, " : ", dump());
    } else {
        LERROR("~InstanceCount ", _className, " was not found! : ", dump());
    }
}


int InstanceCount::getCount() {
    lock_guard<recursive_mutex> lg(_mx);
    auto iter = _instances.find(_className);
    if (iter == _instances.end()) {
        return 0;
    }
    return iter->second;
}


ostream& operator<<(ostream &os, InstanceCount const& instanceCount) {
    return instanceCount.dump(os);
}

ostream& InstanceCount::dump(ostream &os) const {
    lock_guard<recursive_mutex> lg(_mx);
    for (auto const& entry : _instances) {
        if (entry.second != 0) {
            os << entry.first << "=" << entry.second << " ";
        }
    }
    return os;
}

string InstanceCount::dump() const {
    ostringstream os;
    dump(os);
    return os.str();
}

}}} // namespace LSST::m2cellcpp::util
