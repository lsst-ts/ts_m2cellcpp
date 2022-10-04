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
/// see https://confluence.lsstcorp.org/display/LTS/Tangent+Load+Cell+Fault+Detection
/// Unit testing done in `test_TangentLoadCellFaultDetection.cpp`
class TangentLoadCellFaultDetection : public LabViewVI {
public:
    using Ptr = std::shared_ptr<TangentLoadCellFaultDetection>;

    TangentLoadCellFaultDetection();

    TangentLoadCellFaultDetection(TangentLoadCellFaultDetection const&) = delete;
    TangentLoadCellFaultDetection& operator=(TangentLoadCellFaultDetection const&) = delete;
    ~TangentLoadCellFaultDetection() = default;

    /// This function does the actual checking of the loads.
    void run() override;

    /// Return a log appropriate string describing the contents of this class.

private:
    util::NamedDouble::Ptr _inFA1;            ///< Tangential Actuator force 1 VI->"TangentMeasured Forces 1"
    util::NamedDouble::Ptr _inFA2;            ///< Tangential Actuator force 2 VI->"TangentMeasured Forces 2"
    util::NamedDouble::Ptr _inFA3;            ///< Tangential Actuator force 3 VI->"TangentMeasured Forces 3"
    util::NamedDouble::Ptr _inFA4;            ///< Tangential Actuator force 4 VI->"TangentMeasured Forces 4"
    util::NamedDouble::Ptr _inFA5;            ///< Tangential Actuator force 5 VI->"TangentMeasured Forces 5"
    util::NamedDouble::Ptr _inFA6;            ///< Tangential Actuator force 6 VI->"TangentMeasured Forces 6"
    util::NamedAngle::Ptr _inElevationAngle;  ///< Elevation VI->"Inclination Angle"

    // All items in the `constMap` should be set from the Config, except during unit tests.
    /// Maximum allowable value for `_outTangentialTotalWeight` VI->"Theta Z Moment Error Threshold [N]".
    util::NamedDouble::Ptr _constTanWeightError;
    /// Maximum allowable value for individual `_outLoadBearingFA#` VI->"Load Bearing Link Threshold [N]"
    util::NamedDouble::Ptr _constLoadBearingError;
    /// Maximum allowable value for `_outNetMomentForces` VI->"Total Weight Error Threshold [N]"
    util::NamedDouble::Ptr _constNetMomentError;
    /// Maximum allowable value for `_outFA1` and `_outFA4`  VI->"Non Load Bearing Link Threshold [N]"
    util::NamedDouble::Ptr _constNotLoadBearingError;
    /// Weight of the mirror VI->"Mirror Weight [N]"
    util::NamedDouble::Ptr _constMirrorWeight;

    /// Total weight on the load bearing Tangential Actuators VI->"Total Weight Error [N]".
    /// This is the load on the tangential actuators not parallel to the ground:
    ///  `_inFA2`, `_inFA3`, `_inFA5`, and `_inFA6`,
    util::NamedDouble::Ptr _outTangentialTotalWeight;
    /// Load Bearing for `_inFA2` VI->"Individual Weight Error [N], index 0"
    util::NamedDouble::Ptr _outLoadBearingFA2;
    /// Load Bearing for `_inFA3` VI->"Individual Weight Error [N], index 1" FA3
    util::NamedDouble::Ptr _outLoadBearingFA3;
    /// Load Bearing for `_inFA5` VI->"Individual Weight Error [N], index 2" FA5
    util::NamedDouble::Ptr _outLoadBearingFA5;
    /// Load Bearing for `_inFA6` VI->"Individual Weight Error [N], index 4" FA6
    util::NamedDouble::Ptr _outLoadBearingFA6;

    /// Sum of all Tangential actuator forces VI->"Tangent Sum [N]"
    util::NamedDouble::Ptr _outNetMomentForces;

    /// Same as `_inFA1` VI->"Non Load Bearing Forces [N], index 0".
    /// This actuator is parallel to the ground, so load should not change with elevation.
    util::NamedDouble::Ptr _outFA1;
    /// Same as `_inFA4` VI->"Non Load Bearing Forces [N], index 1".
    /// This actuator is parallel to the ground, so load should not change with elevation.
    util::NamedDouble::Ptr _outFA4;

    ///< True if the sum of load bearing actuator forces were out of range VI->"Total Weight Error [N]".
    util::NamedBool::Ptr _outTanWeightBool;
    /// True if any individual load bearing actuator forces were out of range VI->"Individual Weight Error".
    util::NamedBool::Ptr _outLoadBearingBool;
    /// True if the sum of all Tangent Actuator forces were out of range VI->"Tangent Sum Error".
    util::NamedBool::Ptr _outNetMomentBool;
    /// True if any non load bearing values were out of range VI->"Non Load Bearing Link Error"
    util::NamedBool::Ptr _outNonLoadBearingBool;
    /// True if any other errors were tripped VI->"Tangent Load Fault"
    util::NamedBool::Ptr _outTanLoadCellBool;
};

}  // namespace vis
}  // namespace m2cellcpp
}  // namespace LSST

#endif  // LSST_M2CELLCPP_VIS_TANGENTLOADCELLFAULTDETECTION_H
