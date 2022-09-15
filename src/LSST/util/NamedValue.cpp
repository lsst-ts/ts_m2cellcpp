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

/// System headers
#include <fstream>
#include <sstream>
#include <system_error>
#include <utility>

// Class header
#include "util/NamedValue.h"

// LSST headers
#include "util/Log.h"

using namespace std;

namespace LSST {
namespace m2cellcpp {
namespace util {

void NamedBool::setFromString(string const& str) {
    string upper;
    for (size_t j = 0; j < str.size(); ++j) {
        upper += toupper(str[j]);
    }
    if (upper == "TRUE") {
        setValue(true);
    } else if (upper == "FALSE") {
        setValue(false);
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
    setValue(v);
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
    setValue(v);
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

bool NamedAngle::approxEqualRad(double const& val) const { return NamedDouble::approxEqual(val); }

bool NamedAngle::approxEqualDeg(double const& val) const {
    double v = val * RADPERDEG;
    return NamedDouble::approxEqual(v);
}

bool NamedAngle::approxEqual(double const& val) const {
    switch (_expectedUnits) {
        case RADIAN:
            return approxEqualRad(val);
        case DEGREE:
            return approxEqualDeg(val);
    }
    LERROR("NamedAngle::approxEqual ", val, " unknown units", _expectedUnits, " ", getName());
    return false;
}

}  // namespace util
}  // namespace m2cellcpp
}  // namespace LSST
