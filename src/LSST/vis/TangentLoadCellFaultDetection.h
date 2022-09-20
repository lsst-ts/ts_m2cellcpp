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

#ifndef LSST_M2CELLCPP_VIS_TANGENTLOADCELLFAULTDETECTION_H
#define LSST_M2CELLCPP_VIS_TANGENTLOADCELLFAULTDETECTION_H

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

/// &&&
class TangentLoadCellFaultDetection {
public:
    TangentLoadCellFaultDetection();
    std::vector<double> inFA;

    /// &&& doc move to parent, virtual and override
    std::string getTestName() { return "TangentLoadFaultDetection"; }

    /// &&& doc
    void run();

    /// &&& doc
    void readTestFile(std::string const& fileName);

    /// &&& doc
    bool runTest();

    /// &&& doc
    bool checkMap(util::NamedValue::Map& nvMap, int row);

private:
    // &&& delete following comment
    //   in_fA1,   in_fA2,  in_fA3,   in_fA4, in_fA5,  in_fA6, in_elevation_angle,
    //   out_tangential_total_weight, out_load_bearing_fA2, out_load_bearing_fA3, out_load_bearing_fA5,
    //   out_load_bearing_fA6, out_net_moment_forces, const_tan_weight_error, const_load_bearing_error,
    //   const_net_moment_error, const_not_load_bearing_error,
    //   out_tan_weight_bool,out_load_bearing_bool,out_net_moment_bool,out_non_load_bearing_bool,out_tan_load_cell_bool
    // -325.307, -447.377, 1128.37, -1249.98, 458.63, 267.627,                 80, -2589.83, 269.818,
    // 1634.455,             -260.072,             -425.486,              -168.037,                   2000,
    // 1000,                   1000,                         2500,                TRUE,                 TRUE,
    // FALSE,                    FALSE,                  TRUE
    //    150.8,   -100.3,   450.8,    -52.5,  102.5,   309.4,                 50, -9678.62, 2346.087,
    //    2823.354,-            2344.182,            -2165.001,                 860.7,                   2000,
    //    1000,                   1000,                         2500,                TRUE, TRUE, FALSE, FALSE,
    //    TRUE
    util::NamedValue::Map _inMap;             ///< &&& doc
    util::NamedDouble::Ptr _inFA1;            ///< &&& doc VI->"TangentMeasured Forces 1"
    util::NamedDouble::Ptr _inFA2;            ///< &&& doc VI->"TangentMeasured Forces 2"
    util::NamedDouble::Ptr _inFA3;            ///< &&& doc VI->"TangentMeasured Forces 3"
    util::NamedDouble::Ptr _inFA4;            ///< &&& doc VI->"TangentMeasured Forces 4"
    util::NamedDouble::Ptr _inFA5;            ///< &&& doc VI->"TangentMeasured Forces 5"
    util::NamedDouble::Ptr _inFA6;            ///< &&& doc VI->"TangentMeasured Forces 6"
    util::NamedAngle::Ptr _inElevationAngle;  ///< &&& doc

    // All items in the `_constMap` should be set from the Config, except during unit tests.
    util::NamedValue::Map _constMap;                   ///< &&& doc
    util::NamedDouble::Ptr _constTanWeightError;       ///< &&& doc
    util::NamedDouble::Ptr _constLoadBearingError;     ///< &&& doc
    util::NamedDouble::Ptr _constNetMomentError;       ///< &&& doc
    util::NamedDouble::Ptr _constNotLoadBearingError;  ///< &&& doc VI->"Non Load Bearing Link Threshold"

    util::NamedValue::Map _outMap;                     ///< &&& doc
    util::NamedDouble::Ptr _outTangentialTotalWeight;  ///< &&& doc ??? VI->"Total Weight Error [N]"
    util::NamedDouble::Ptr _outLoadBearingFA2;         ///< &&& doc VI->"Individual Weight Error 2"
    util::NamedDouble::Ptr _outLoadBearingFA3;         ///< &&& doc VI->"Individual Weight Error 3"
    util::NamedDouble::Ptr _outLoadBearingFA5;         ///< &&& doc VI->"Individual Weight Error 5"
    util::NamedDouble::Ptr _outLoadBearingFA6;         ///< &&& doc VI->"Individual Weight Error 6"
    util::NamedDouble::Ptr _outNetMomentForces;        ///< &&& doc
    // &&& ???in VI "Tangent Sum [N]"

    // &&& ???in VI "Non Load Bearing Forces 1  (index 0)"
    // &&& ???in VI "Non Load Bearing Forces 4  (index 3)"

    util::NamedBool::Ptr _outTanWeightBool;       ///< &&& doc
    util::NamedBool::Ptr _outLoadBearingBool;     ///< &&& doc
    util::NamedBool::Ptr _outNetMomentBool;       ///< &&& doc
    util::NamedBool::Ptr _outNonLoadBearingBool;  ///< &&& doc in VI "Non Load Bearing Link Error"
    util::NamedBool::Ptr _outTanLoadCellBool;     ///< &&& doc

    util::CsvFile::Ptr _testFile;

    /// Weight of the mirror
    /// TODO: this should be a constant in the csv file and otherwise set from the config.
    double _mirrorWeight = 15140.0;
};

}  // namespace vis
}  // namespace m2cellcpp
}  // namespace LSST

#endif  // LSST_M2CELLCPP_VIS_TANGENTLOADCELLFAULTDETECTION_H
