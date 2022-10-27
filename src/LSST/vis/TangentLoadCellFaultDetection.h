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
#include "vis/LabViewVI.h"

namespace LSST {
namespace m2cellcpp {
namespace vis {

/// This class represnets the logic in the `TangentLoadCellFaultDetection.vi`.
/// Its purpose is to detect unsafe loads on the secondary mirror.
/// For a description of the logic, please see:
///     `docs/viDocs/LTS-TangentLoadCellFaultDetection.pdf`
/// For a mapping of inputs, outputs, and constants to the original VI frontpanel see
///     `testFiles/io_TangentLoadCellConnMap_v02.csv`
/// Unit testing is done in `test_TangentLoadCellFaultDetection.cpp` using
///     `testFiles/io_TangentLoadCell_v03.csv`
class TangentLoadCellFaultDetection : public LabViewVI {
public:
    using Ptr = std::shared_ptr<TangentLoadCellFaultDetection>;

    TangentLoadCellFaultDetection();

    TangentLoadCellFaultDetection(TangentLoadCellFaultDetection const&) = delete;
    TangentLoadCellFaultDetection& operator=(TangentLoadCellFaultDetection const&) = delete;
    ~TangentLoadCellFaultDetection() = default;

    /// This function does the actual checking of the loads.
    void run() override;

    // Inputs
    util::NamedDouble::Ptr inFa1N;            ///< Tangential Actuator force 1 VI->"TangentMeasured Forces 1"
    util::NamedDouble::Ptr inFa2N;            ///< Tangential Actuator force 2 VI->"TangentMeasured Forces 2"
    util::NamedDouble::Ptr inFa3N;            ///< Tangential Actuator force 3 VI->"TangentMeasured Forces 3"
    util::NamedDouble::Ptr inFa4N;            ///< Tangential Actuator force 4 VI->"TangentMeasured Forces 4"
    util::NamedDouble::Ptr inFa5N;            ///< Tangential Actuator force 5 VI->"TangentMeasured Forces 5"
    util::NamedDouble::Ptr inFa6N;            ///< Tangential Actuator force 6 VI->"TangentMeasured Forces 6"
    util::NamedAngle::Ptr inElevationAngleD;  ///< Elevation VI->"Inclination Angle"

    // Outputs
    /// Total weight on the load bearing Tangential Actuators VI->"Total Weight Error [N]".
    /// This is the load on the tangential actuators not parallel to the ground:
    ///  `inFa2N`, `inFa3N`, `inFa5N`, and `inFa6N`,
    util::NamedDouble::Ptr outTangentialTotalWeightN;
    /// Load Bearing for `inFa2N` VI->"Individual Weight Error [N], index 0" FA2
    util::NamedDouble::Ptr outLoadBearingFa2N;
    /// Load Bearing for `inFa3N` VI->"Individual Weight Error [N], index 1" FA3
    util::NamedDouble::Ptr outLoadBearingFa3N;
    /// Load Bearing for `inFa5N` VI->"Individual Weight Error [N], index 2" FA5
    util::NamedDouble::Ptr outLoadBearingFa5N;
    /// Load Bearing for `inFa6N` VI->"Individual Weight Error [N], index 4" FA6
    util::NamedDouble::Ptr outLoadBearingFa6N;

    /// Sum of all Tangential actuator forces VI->"Tangent Sum [N]"
    util::NamedDouble::Ptr outNetMomentForcesN;

    /// Same as `inFa1N` VI->"Non Load Bearing Forces [N], index 0".
    /// This actuator is parallel to the ground, so load should not change with elevation.
    util::NamedDouble::Ptr outFa1N;
    /// Same as `inFa4N` VI->"Non Load Bearing Forces [N], index 1".
    /// This actuator is parallel to the ground, so load should not change with elevation.
    util::NamedDouble::Ptr outFa4N;

    ///< True if the sum of load bearing actuator forces were out of range VI->"Total Weight Error [N]".
    util::NamedBool::Ptr outTanWeightBool;
    /// True if any individual load bearing actuator forces were out of range VI->"Individual Weight Error".
    util::NamedBool::Ptr outLoadBearingBool;
    /// True if the sum of all Tangent Actuator forces were out of range VI->"Tangent Sum Error".
    util::NamedBool::Ptr outNetMomentBool;
    /// True if any non load bearing values were out of range VI->"Non Load Bearing Link Error"
    util::NamedBool::Ptr outNonLoadBearingBool;
    /// True if any other errors were tripped VI->"Tangent Load Fault"
    util::NamedBool::Ptr outTanLoadCellBool;

    /// Return mirror weight constant.
    double getMirrorWeightN() const { return _constMirrorWeightN->val; }

    /// Return the tangent weight error constant.
    double getTangentWeightErrorN() const { return _constTanWeightErrorN->val; }

private:
    // All items in the `constMap` should be set from the Config, except during unit tests. TODO:DM-36500
    /// Maximum allowable value for `outTangentialTotalWeight` VI->"Theta Z Moment Error Threshold [N]".
    util::NamedDouble::Ptr _constTanWeightErrorN;
    /// Maximum allowable value for individual `_outLoadBearingFA#` VI->"Load Bearing Link Threshold [N]"
    util::NamedDouble::Ptr _constLoadBearingErrorN;
    /// Maximum allowable value for `outNetMomentForces` VI->"Total Weight Error Threshold [N]"
    util::NamedDouble::Ptr _constNetMomentErrorN;
    /// Maximum allowable value for `outFA1` and `outFA4`  VI->"Non Load Bearing Link Threshold [N]"
    util::NamedDouble::Ptr _constNotLoadBearingErrorN;
    /// Weight of the mirror in Newtons VI->"Mirror Weight [N]"
    util::NamedDouble::Ptr _constMirrorWeightN;
};

}  // namespace vis
}  // namespace m2cellcpp
}  // namespace LSST

#endif  // LSST_M2CELLCPP_VIS_TANGENTLOADCELLFAULTDETECTION_H
