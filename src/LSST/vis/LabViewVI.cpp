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
#include "vis/LabVIewVI.h"

// LSST headers
#include "util/Log.h"

using namespace std;

namespace LSST {
namespace m2cellcpp {
namespace vis {

bool LabViewVI::runTest() {
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
        LSST::m2cellcpp::util::NamedValue::setMapValuesFromFile(completeMap, *_testFile, j);
        /// For the output map ONLY, reset all the values so it can be shown
        /// the `run()` function set the outputs.
        LSST::m2cellcpp::util::NamedValue::voidValForTest(outMap);
        run();
        // While only the outMap should be interesting, none of the other values
        // should have changed.
        if (!checkMap(inMap, j)) {
            LERROR("inMap failure ", getViName());
            return false;
        }
        if (!checkMap(constMap, j)) {
            LERROR("constMap failure ", getViName());
            return false;
        }
        if (!checkMap(outMap, j)) {
            LERROR("outMap failure ", getViName());
            return false;
        }
    }
    return true;
}

void LabViewVI::readTestFile(std::string const& fileName) {
    _testFile = std::make_shared<util::CsvFile>(fileName);
    _testFile->read();
    LDEBUG(getViName(), " readTestFile ", fileName, ":\n", _testFile->dumpStr());
}

bool LabViewVI::checkMap(util::NamedValue::Map& nvMap, int row) {
    bool success = true;
    for (auto&& elem : nvMap) {
        util::NamedValue::Ptr const& nVal = elem.second;
        if (!nVal->check()) {
            LERROR("checkMap failed for ", getViName(), " row=", row, " test=", nVal->dumpStr());
            success = false;
        }
    }
    return success;
}

}  // namespace vis
}  // namespace m2cellcpp
}  // namespace LSST
