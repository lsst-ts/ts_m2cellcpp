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
#include "util/NamedValue.h"

/// System headers
#include <fstream>
#include <sstream>
#include <system_error>
#include <utility>

// Project headers
#include "util/CsvFile.h"
#include "util/Log.h"

using namespace std;

namespace LSST {
namespace m2cellcpp {
namespace util {

void NamedValue::logWarn(string const& msg) const { LWARN(msg); }

void NamedValue::setup(Ptr const& obj, Map& nvMap) {
    auto ret = nvMap.insert(std::make_pair(obj->getName(), obj));
    if (ret.second == false) {
        string eMsg = "NamedValue::setup duplicate entry " + obj->dumpStr();
        LERROR(eMsg);
        throw runtime_error(eMsg);
    }
}

void NamedValue::insertMapElements(Map& src, Map& dest) {
    for (auto&& elem : src) {
        NamedValue::Ptr sec = elem.second;
        auto ret = dest.insert(elem);
        if (ret.second == false) {
            string eMsg = "NamedValue::insertMapElements duplicate entry " + elem.second->dumpStr();
            LERROR(eMsg);
            throw runtime_error(eMsg);
        }
    }
}

ostream& NamedValue::mapDump(ostream& os, Map const& nvMap) {
    bool first = true;
    for (auto const& elem : nvMap) {
        if (first)
            first = false;
        else
            os << ", ";
        elem.second->dump(os);
    }
    return os;
}

std::string NamedValue::mapDumpStr(Map const& nvMap) {
    stringstream os;
    mapDump(os, nvMap);
    return os.str();
}

void NamedValue::setMapValuesFromFile(Map& nvMap, CsvFile& csvFile, int row) {
    for (auto&& elem : nvMap) {
        Ptr const& nVal = elem.second;
        string valFileRowJ = csvFile.getValue(nVal->getName(), row);
        nVal->setFromString(valFileRowJ);
        LINFO("set from file ", nVal->dumpStr());
    }
}

void NamedValue::voidValForTest(Map& outputMap) {
    for (auto&& elem : outputMap) {
        Ptr const& nVal = elem.second;
        nVal->voidValForTest();
    }
}

void NamedBool::setFromString(string const& str) {
    string upper;
    for (size_t j = 0; j < str.size(); ++j) {
        upper += toupper(str[j]);
    }
    if (upper == "TRUE") {
        setValueRead(true);
    } else if (upper == "FALSE") {
        setValueRead(false);
    } else {
        string eMsg = "NamedBool::setFromString " + str +
                      " is not an acceptable variant of 'true' or 'false'." + getName();
        LERROR(eMsg);
        throw runtime_error(eMsg);
    }
}

void NamedInt::setFromString(std::string const& str) {
    char* endPtr;
    int v = strtol(str.c_str(), &endPtr, 10);
    if (*endPtr != '\0') {
        // there were invalid characters in the string
        string eMsg = "NamedInt::setFromString " + str + " did not convert properly (" + to_string(v) + ")." +
                      getName();
        LERROR(eMsg);
        throw runtime_error(eMsg);
    }
    setValueRead(v);
}

double NamedDouble::getValOfString(std::string const& str) const {
    char* endPtr;
    double v = strtod(str.c_str(), &endPtr);
    if (*endPtr != '\0') {
        // there were invalid characters in the string
        string eMsg = "NamedDouble::setFromString " + str + " did not convert properly (" + to_string(v) +
                      ")." + getName();
        LERROR(eMsg);
        throw runtime_error(eMsg);
    }
    return v;
}

void NamedDouble::setFromString(std::string const& str) {
    double v = getValOfString(str);
    setValueRead(v);
}

void NamedAngle::setFromString(std::string const& str) {
    switch (_expectedUnits) {
        case RADIAN:
            return setFromStringRad(str);
        case DEGREE:
            return setFromStringDeg(str);
    }
    string eMsg = "NamedAngle::setFromString " + str + " unknown units" + to_string(_expectedUnits) + " " +
                  getName();
    LERROR(eMsg);
    throw runtime_error(eMsg);
}

bool NamedAngle::approxEqualRad(double const& inV) const { return NamedDouble::approxEqual(inV); }

bool NamedAngle::approxEqualDeg(double const& inV) const {
    double v = inV * RADPERDEG;
    return NamedDouble::approxEqual(v);
}

bool NamedAngle::approxEqual(double const& inV) const {
    switch (_expectedUnits) {
        case RADIAN:
            return approxEqualRad(inV);
        case DEGREE:
            return approxEqualDeg(inV);
    }
    LERROR("NamedAngle::approxEqual ", inV, " unknown units", _expectedUnits, " ", getName());
    return false;
}

}  // namespace util
}  // namespace m2cellcpp
}  // namespace LSST
