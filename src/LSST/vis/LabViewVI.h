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

#ifndef LSST_M2CELLCPP_VIS_LABVIEWVI_H
#define LSST_M2CELLCPP_VIS_LABVIEWVI_H

// System headers
#include <memory>
#include <string>
#include <vector>

// Project headers
#include "util/CsvFile.h"
#include "util/NamedValue.h"

namespace LSST {
namespace m2cellcpp {
namespace vis {

/// This is the base class for classes based directly on VI's from LabView.
/// It's primary purpose is for testing the child class `run()` functions
/// against `CsvFile`s with input and output values obtained directly from
/// the source VI's. Many member functions will throw `runtime_error` if
/// there is a problem.
/// UML diagram doc/LabViewVIUML.txt
/// Unit testing done in `test_TangentLoadCellFaultDetection.cpp`
class LabViewVI {
public:
    using Ptr = std::shared_ptr<LabViewVI>;

    /// Constructor.
    /// @param viName - the name of th VI
    /// @param id - optional id if the VI occurs multiple times in the system.
    /// @throws system::ConfigException if a constant is not defined in the configuration.
    LabViewVI(std::string const& viName, std::string const& id = "") : _viName(viName), _id(id) {}
    LabViewVI() = delete;

    /// Return a log worthy string of this object, see `std::ostream& dump(std::ostream& os)`.
    std::string dumpStr() const {
        std::stringstream os;
        dump(os);
        return os.str();
    }

    /// Return a log worthy string of this object. Child classes should override
    /// this function. This is called by the `operator<<` function below.
    virtual std::ostream& dump(std::ostream& os) const {
        os << "VI " << getViNameId() << " constants(";
        util::NamedValue::mapDump(os, constMap);
        os << ")  inputs(";
        util::NamedValue::mapDump(os, inMap);
        os << ") outputs(";
        util::NamedValue::mapDump(os, outMap);
        os << ")";
        return os;
    }

    /// Return the name of this VI.
    std::string getViName() const { return _viName; }

    /// Return the id of this VI.
    std::string getId() const { return _id; }

    /// Return  the name and id of this VI.
    std::string getViNameId() const { return (_id.empty()) ? getViName() : getViName() + "_" + _id; }

    /// This is the function that does what the original VI does.
    virtual void run() = 0;

    /// Read the test file.
    void readTestFile(std::string const& fileName);

    /// Run the test by reading the CSV file to set inputs and then check all of the outputs.
    /// WARNING: This is only for use in unit tests, it will overwrite constants.
    /// @return false if any tests are failed.
    bool runTest();

    /// Return false if any of the `NamedValues` in the map are out of tolerance.
    bool checkMap(util::NamedValue::Map& nvMap, int row);

protected:
    /// Set constant values from the configuration file.
    /// @throw `system::ConfigException` if any constant cannot be defined.
    void setConstFromConfig();

    util::NamedValue::Map constMap;     ///< Map of constant values for this VI.
    util::NamedValue::Map inMap;        ///< Map of inputs for this VI.
    util::NamedValue::Map outMap;       ///< Map of outputs for this VI
    util::NamedValue::Map completeMap;  ///< A map of all inputs, outputs, and constants for this VI.

    util::CsvFile::Ptr _testFile;  ///< CSV file used for test data.

private:
    /// Helper function for finding constants in the Config.
    void _searchConfig(util::NamedValue::Map& undefCMap, std::string const& section);

    std::string const _viName;  ///< The name of the kind of VI (child class realted)
    std::string const _id;      ///< Id of the specific VI (instance related)
};

/// `operator<<` for `LabViewVI` and all of its derived classes.
inline std::ostream& operator<<(std::ostream& os, LabViewVI const& vi) {
    vi.dump(os);
    return os;
}

}  // namespace vis
}  // namespace m2cellcpp
}  // namespace LSST

#endif  // LSST_M2CELLCPP_VIS_LABVIEWVI_H
