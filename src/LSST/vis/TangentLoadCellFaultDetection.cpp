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
    _inFA1 = util::NamedDouble::create("in_fA1", inMap);
    _inFA2 = util::NamedDouble::create("in_fA2", inMap);
    _inFA3 = util::NamedDouble::create("in_fA3", inMap);
    _inFA4 = util::NamedDouble::create("in_fA4", inMap);
    _inFA5 = util::NamedDouble::create("in_fA5", inMap);
    _inFA6 = util::NamedDouble::create("in_fA6", inMap);
    _inElevationAngle = util::NamedAngle::create("in_elevation_angle", inMap);

    _constTanWeightError = util::NamedDouble::create("const_tan_weight_error", constMap);
    _constLoadBearingError = util::NamedDouble::create("const_load_bearing_error", constMap);
    _constNetMomentError = util::NamedDouble::create("const_net_moment_error", constMap);
    _constNotLoadBearingError = util::NamedDouble::create("const_not_load_bearing_error", constMap);
    _hmmMirrorWeightN = util::NamedDouble::create("&&& need a column name for mirror weight &&&",
                                                  constMap);  // &&& check
    _hmmNonLoadBearingLinkThresholdN = util::NamedDouble::create(
            "&&& need a column name for Non Loda bearing link threshold N &&&", constMap);  // &&& check

    _outTangentialTotalWeight = util::NamedDouble::create("out_tangential_total_weight", outMap);
    _outLoadBearingFA2 = util::NamedDouble::create("out_load_bearing_fA2", outMap);
    _outLoadBearingFA3 = util::NamedDouble::create("out_load_bearing_fA3", outMap);
    _outLoadBearingFA5 = util::NamedDouble::create("out_load_bearing_fA5", outMap);
    _outLoadBearingFA6 = util::NamedDouble::create("out_load_bearing_fA6", outMap);
    _outNetMomentForces = util::NamedDouble::create("out_net_moment_forces", outMap);
    _hmmNonLoadBearing1 =
            util::NamedDouble::create("&&& need col name for non-load bearing fA1", outMap);  // &&& check
    _hmmNonLoadBearing4 =
            util::NamedDouble::create("&&& need col name for non-load bearing fA4", outMap);  // &&& check

    _outTanWeightBool = util::NamedBool::create("out_tan_weight_bool", outMap);
    _outLoadBearingBool = util::NamedBool::create("out_load_bearing_bool", outMap);
    _outNetMomentBool = util::NamedBool::create("out_net_moment_bool", outMap);
    _outNonLoadBearingBool = util::NamedBool::create("out_non_load_bearing_bool", outMap);
    _outTanLoadCellBool = util::NamedBool::create("out_tan_load_cell_bool", outMap);

    util::NamedValue::insertMapElements(completeMap, inMap);
    util::NamedValue::insertMapElements(completeMap, outMap);
    util::NamedValue::insertMapElements(completeMap, constMap);
}

void TangentLoadCellFaultDetection::run() {
    constexpr double cos30Deg = cos(30.0 * util::NamedAngle::PI / 180.0);
    double elevationComp =
            sin(util::NamedAngle::PI / 2.0 - _inElevationAngle->val);  // sin(90Deg - elevation)
    double mirrorWeightCompDiv4 = elevationComp * _hmmMirrorWeightN->val / 4.0;

    // Tangential weight error
    double fa2W = (-_inFA2->val * cos30Deg) - elevationComp;
    double fa3W = (-_inFA3->val * cos30Deg) - elevationComp;
    double fa5W = (_inFA5->val * cos30Deg) - elevationComp;
    double fa6W = (_inFA6->val * cos30Deg) - elevationComp;
    _outTangentialTotalWeight->val = fa2W + fa3W + fa5W + fa6W;
    _outTanWeightBool->val = fabs(_outTangentialTotalWeight->val) >= _constNetMomentError->val;

    // Individual load bearing error
    _outLoadBearingFA2->val = cos30Deg * _inFA2->val + mirrorWeightCompDiv4;
    _outLoadBearingFA3->val = cos30Deg * _inFA3->val + mirrorWeightCompDiv4;
    _outLoadBearingFA5->val = cos30Deg * _inFA5->val - mirrorWeightCompDiv4;
    _outLoadBearingFA6->val = cos30Deg * _inFA6->val - mirrorWeightCompDiv4;

    bool individualWeightError2 = fabs(_outLoadBearingFA2->val) >= _constLoadBearingError->val;
    bool individualWeightError3 = fabs(_outLoadBearingFA3->val) >= _constLoadBearingError->val;
    bool individualWeightError5 = fabs(_outLoadBearingFA5->val) >= _constLoadBearingError->val;
    bool individualWeightError6 = fabs(_outLoadBearingFA6->val) >= _constLoadBearingError->val;
    _outLoadBearingBool->val = individualWeightError2 || individualWeightError3 || individualWeightError5 ||
                               individualWeightError6;

    // Tangent Sum, Theta Z Moment Error
    _outNetMomentForces->val =
            _inFA1->val + _inFA2->val + _inFA3->val + _inFA4->val + _inFA5->val + _inFA6->val;
    _outNetMomentBool->val = fabs(_outNetMomentForces->val) > _constTanWeightError->val;

    // Non-load bearing
    _hmmNonLoadBearing1->val = _inFA1->val;
    _hmmNonLoadBearing4->val = _inFA4->val;
    _outNonLoadBearingBool->val = (fabs(_inFA1->val) > _hmmNonLoadBearingLinkThresholdN->val) ||
                                  (fabs(_inFA4->val) > _hmmNonLoadBearingLinkThresholdN->val);

    _outTanLoadCellBool->val = _outTanWeightBool->val || _outLoadBearingBool->val ||
                               _outNonLoadBearingBool->val || _outNetMomentBool->val;

    LDEBUG("TangentLoadCellFaultDetection::run() ", dumpStr());
}

}  // namespace vis
}  // namespace m2cellcpp
}  // namespace LSST
