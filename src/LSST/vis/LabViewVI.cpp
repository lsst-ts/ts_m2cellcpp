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
#include "vis/LabViewVI.h"

// LSST headers
#include "system/Config.h"
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
        LINFO("runTest file=", _testFile->getFileName(), " row=", j, " ", dumpStr());
        run();
        // While only the outMap should be interesting, none of the other values
        // should have changed.
        if (!checkMap(inMap, j)) {
            LERROR("inMap failure ", getViNameId(), " ", dumpStr());
            return false;
        }
        if (!checkMap(constMap, j)) {
            LERROR("constMap failure ", getViNameId(), " ", dumpStr());
            return false;
        }
        if (!checkMap(outMap, j)) {
            LERROR("outMap failure ", getViNameId(), " ", dumpStr());
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
            LERROR("checkMap failed for ", getViNameId(), " row=", row, " test=", nVal->dumpStr());
            success = false;
        }
    }
    return success;
}

void LabViewVI::_searchConfig(util::NamedValue::Map& undefCMap, string const& section) {
    auto& cfg = system::Config::get();
    vector<string> found;
    for (auto const& elem : undefCMap) {
        util::NamedValue::Ptr nv = elem.second;
        string key = nv->getName();
        try {
            string val = cfg.getSectionKeyAsString(section, key);
            nv->setFromString(val);
            found.push_back(key);
        } catch (system::ConfigException const& ex) {
            LDEBUG("LabViewVI::setConstFromConfig no local key for ", section, ", ", key);
        }
    }
    // Remove elements that were found
    for (auto const& key : found) {
        undefCMap.erase(key);
    }
}

void LabViewVI::setConstFromConfig() {
    // Copy the constMap
    util::NamedValue::Map undefConsts = constMap;

    // First try to set elements from the local version
    string section = getViNameId();
    _searchConfig(undefConsts, section);

    // Check the Globals section of the configurtation for all undefined elements.
    section = "Globals";
    _searchConfig(undefConsts, section);

    if (!undefConsts.empty()) {
        string eMsg = "LabViewVI::setConstFromConfig() udefined constants for " + getViNameId() + ": ";
        for (auto const& elem : undefConsts) {
            eMsg += elem.second->getName() + ", ";
        }
        LERROR(eMsg);
        throw system::ConfigException(ERR_LOC, eMsg);
    }
}

}  // namespace vis
}  // namespace m2cellcpp
}  // namespace LSST
