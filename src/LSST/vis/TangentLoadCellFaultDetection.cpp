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

namespace LSST {
namespace m2cellcpp {
namespace vis {

TangentLoadCellFaultDetection::TangentLoadCellFaultDetection() : LabViewVI("TangentLoadCellFaultDetection") {
    inFA1 = util::NamedDouble::create("in_fA1", inMap);
    inFA2 = util::NamedDouble::create("in_fA2", inMap);
    inFA3 = util::NamedDouble::create("in_fA3", inMap);
    inFA4 = util::NamedDouble::create("in_fA4", inMap);
    inFA5 = util::NamedDouble::create("in_fA5", inMap);
    inFA6 = util::NamedDouble::create("in_fA6", inMap);
    inElevationAngle = util::NamedAngle::create("in_elevation_angle", inMap);

    _constTanWeightError = util::NamedDouble::create("const_tan_weight_error", constMap);
    _constLoadBearingError = util::NamedDouble::create("const_load_bearing_error", constMap);
    _constNetMomentError = util::NamedDouble::create("const_net_moment_error", constMap);
    _constNotLoadBearingError = util::NamedDouble::create("const_not_load_bearing_error", constMap);
    _constMirrorWeight = util::NamedDouble::create("const_mirror_weight", constMap);

    // The limits for these outputs are 1000 to 2500. tolerance of +/- should be acceptable.
    double tolerance = 0.05;
    outTangentialTotalWeight = util::NamedDouble::create("out_tangential_total_weight", outMap, tolerance);
    outLoadBearingFA2 = util::NamedDouble::create("out_load_bearing_fA2", outMap, tolerance);
    outLoadBearingFA3 = util::NamedDouble::create("out_load_bearing_fA3", outMap, tolerance);
    outLoadBearingFA5 = util::NamedDouble::create("out_load_bearing_fA5", outMap, tolerance);
    outLoadBearingFA6 = util::NamedDouble::create("out_load_bearing_fA6", outMap, tolerance);
    outNetMomentForces = util::NamedDouble::create("out_net_moment_forces", outMap, tolerance);
    outFA1 = util::NamedDouble::create("out_fA1", outMap);
    outFA4 = util::NamedDouble::create("out_fA4", outMap);

    outTanWeightBool = util::NamedBool::create("out_tan_weight_bool", outMap);
    outLoadBearingBool = util::NamedBool::create("out_load_bearing_bool", outMap);
    outNetMomentBool = util::NamedBool::create("out_net_moment_bool", outMap);
    outNonLoadBearingBool = util::NamedBool::create("out_non_load_bearing_bool", outMap);
    outTanLoadCellBool = util::NamedBool::create("out_tan_load_cell_bool", outMap);

    util::NamedValue::insertMapElements(inMap, completeMap);
    util::NamedValue::insertMapElements(outMap, completeMap);
    util::NamedValue::insertMapElements(constMap, completeMap);
}

void TangentLoadCellFaultDetection::run() {
    LDEBUG("TangentLoadCellFaultDetection::run start");
    constexpr double cos30Deg = cos(30.0 * util::NamedAngle::PI / 180.0);
    double zeta = util::NamedAngle::PI / 2.0 - inElevationAngle->val;  // 90Deg - elevation
    double elevationComp = sin(zeta) * _constMirrorWeight->val;
    double mirrorWeightCompDiv4 = elevationComp / 4.0;

    // Tangential weight error
    double fa2W = (-inFA2->val * cos30Deg);
    double fa3W = (-inFA3->val * cos30Deg);
    double fa5W = (inFA5->val * cos30Deg);
    double fa6W = (inFA6->val * cos30Deg);
    outTangentialTotalWeight->val = fa2W + fa3W + fa5W + fa6W - elevationComp;
    outTanWeightBool->val = fabs(outTangentialTotalWeight->val) >= _constTanWeightError->val;

    // Individual load bearing error
    outLoadBearingFA2->val = cos30Deg * inFA2->val + mirrorWeightCompDiv4;
    outLoadBearingFA3->val = cos30Deg * inFA3->val + mirrorWeightCompDiv4;
    outLoadBearingFA5->val = cos30Deg * inFA5->val - mirrorWeightCompDiv4;
    outLoadBearingFA6->val = cos30Deg * inFA6->val - mirrorWeightCompDiv4;

    bool individualWeightError2 = fabs(outLoadBearingFA2->val) >= _constLoadBearingError->val;
    bool individualWeightError3 = fabs(outLoadBearingFA3->val) >= _constLoadBearingError->val;
    bool individualWeightError5 = fabs(outLoadBearingFA5->val) >= _constLoadBearingError->val;
    bool individualWeightError6 = fabs(outLoadBearingFA6->val) >= _constLoadBearingError->val;
    outLoadBearingBool->val = individualWeightError2 || individualWeightError3 || individualWeightError5 ||
                              individualWeightError6;

    // Tangent Sum, Theta Z Moment Error
    outNetMomentForces->val = inFA1->val + inFA2->val + inFA3->val + inFA4->val + inFA5->val + inFA6->val;
    outNetMomentBool->val = fabs(outNetMomentForces->val) > _constNetMomentError->val;

    // Non-load bearing
    outFA1->val = inFA1->val;
    outFA4->val = inFA4->val;
    outNonLoadBearingBool->val = (fabs(outFA1->val) > _constNotLoadBearingError->val) ||
                                 (fabs(outFA4->val) > _constNotLoadBearingError->val);

    outTanLoadCellBool->val = outTanWeightBool->val || outLoadBearingBool->val ||
                              outNonLoadBearingBool->val || outNetMomentBool->val;

    LDEBUG("TangentLoadCellFaultDetection::run() ", dumpStr());
}

}  // namespace vis
}  // namespace m2cellcpp
}  // namespace LSST
