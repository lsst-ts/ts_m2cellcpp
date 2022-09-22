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
#include "vis/TangentLoadCellFaultDetection.h"

// LSST headers
#include "util/Log.h"

using namespace std;

namespace {

/// &&& doc  move to different location
void setMapValuesFromFile(LSST::m2cellcpp::util::NamedValue::Map& nvMap,
                          LSST::m2cellcpp::util::CsvFile& csvFile, int row) {
    for (auto&& elem : nvMap) {
        LSST::m2cellcpp::util::NamedValue::Ptr const& nVal = elem.second;
        string valFileRowJ = csvFile.getValue(nVal->getName(), row);
        nVal->setFromString(valFileRowJ);
    }
}

}  // namespace

namespace LSST {
namespace m2cellcpp {
namespace vis {

TangentLoadCellFaultDetection::TangentLoadCellFaultDetection() {
    _inFA1 = util::NamedDouble::create("in_fA1", _inMap);
    _inFA2 = util::NamedDouble::create("in_fA2", _inMap);
    _inFA3 = util::NamedDouble::create("in_fA3", _inMap);
    _inFA4 = util::NamedDouble::create("in_fA4", _inMap);
    _inFA5 = util::NamedDouble::create("in_fA5", _inMap);
    _inFA6 = util::NamedDouble::create("in_fA6", _inMap);
    _inElevationAngle = util::NamedAngle::create("in_elevation_angle", _inMap);

    _constTanWeightError = util::NamedDouble::create("const_tan_weight_error", _constMap);
    _constLoadBearingError = util::NamedDouble::create("const_load_bearing_error", _constMap);
    _constNetMomentError = util::NamedDouble::create("const_net_moment_error", _constMap);
    _constNotLoadBearingError = util::NamedDouble::create("const_not_load_bearing_error", _constMap);

    _outTangentialTotalWeight = util::NamedDouble::create("out_tangential_total_weight", _outMap);
    _outLoadBearingFA2 = util::NamedDouble::create("out_load_bearing_fA2", _outMap);
    _outLoadBearingFA3 = util::NamedDouble::create("out_load_bearing_fA3", _outMap);
    _outLoadBearingFA5 = util::NamedDouble::create("out_load_bearing_fA5", _outMap);
    _outLoadBearingFA6 = util::NamedDouble::create("out_load_bearing_fA6", _outMap);
    _outNetMomentForces = util::NamedDouble::create("out_net_moment_forces", _outMap);

    _outTanWeightBool = util::NamedBool::create("out_tan_weight_bool", _outMap);
    _outLoadBearingBool = util::NamedBool::create("out_load_bearing_bool", _outMap);
    _outNetMomentBool = util::NamedBool::create("out_net_moment_bool", _outMap);
    _outNonLoadBearingBool = util::NamedBool::create("out_non_load_bearing_bool", _outMap);
    _outTanLoadCellBool = util::NamedBool::create("out_tan_load_cell_bool", _outMap);
}

void TangentLoadCellFaultDetection::readTestFile(std::string const& fileName) {
    _testFile = std::make_shared<util::CsvFile>(fileName);
    _testFile->read();
    LDEBUG("TangentLoadFaultDetection::readTestFile ", fileName, ":\n", _testFile->dumpStr());
}

bool TangentLoadCellFaultDetection::runTest() {
    if (_testFile == nullptr) {
        LERROR("TangentLoadFaultDetection::runTest no testFile.");
        return false;
    }
    int rows = _testFile->getRowCount();
    if (rows == 0) {
        LERROR("TangentLoadFaultDetection::runTest no rows to tests.");
        return false;
    }

    for (int j = 0; j < rows; ++j) {
        setMapValuesFromFile(_inMap, *_testFile, j);
        setMapValuesFromFile(_constMap, *_testFile, j);
        setMapValuesFromFile(_outMap, *_testFile, j);
        run();
        // While only the outMap should be interesting, none of the other values
        // should have changed.
        if (!checkMap(_inMap, j)) {
            LERROR("inMap failure ", getTestName());
            return false;
        }
        if (!checkMap(_constMap, j)) {
            LERROR("constMap failure ", getTestName());
            return false;
        }
        if (!checkMap(_outMap, j)) {
            LERROR("outMap failure ", getTestName());
            return false;
        }
    }
    return true;
}

// &&& move to parent
bool TangentLoadCellFaultDetection::checkMap(util::NamedValue::Map& nvMap, int row) {
    bool success = true;
    for (auto&& elem : nvMap) {
        util::NamedValue::Ptr const& nVal = elem.second;
        if (!nVal->check()) {
            LERROR("checkMap failed for ", getTestName(), " row=", row, " test=", nVal->dumpStr());
            success = false;
        }
    }
    return success;
}

void TangentLoadCellFaultDetection::run() {
    double tangentSum = _inFa1.val + _inFa2 + _inFa3 + _inFa4 + _inFa5 + _inFa6;
    bool tangentSumError = fabs(tangentSum) >
}

}  // namespace vis
}  // namespace m2cellcpp
}  // namespace LSST
