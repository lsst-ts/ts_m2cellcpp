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
    inFa1N = util::NamedDouble::create("inFa1N", inMap);
    inFa2N = util::NamedDouble::create("inFa2N", inMap);
    inFa3N = util::NamedDouble::create("inFa3N", inMap);
    inFa4N = util::NamedDouble::create("inFa4N", inMap);
    inFa5N = util::NamedDouble::create("inFa5N", inMap);
    inFa6N = util::NamedDouble::create("inFa6N", inMap);
    inElevationAngleD = util::NamedAngle::create("inElevationAngleD", inMap);

    _constTanWeightErrorN = util::NamedDouble::create("constTanWeightErrorN", constMap);
    _constLoadBearingErrorN = util::NamedDouble::create("constLoadBearingErrorN", constMap);
    _constNetMomentErrorN = util::NamedDouble::create("constNetMomentErrorN", constMap);
    _constNotLoadBearingErrorN = util::NamedDouble::create("constNotLoadBearingErrorN", constMap);
    _constMirrorWeightN = util::NamedDouble::create("constMirrorWeightN", constMap);

    // The limits for these outputs are 1000 to 2500. tolerance of +/- should be acceptable.
    double tolerance = 0.05;
    outTangentialTotalWeightN = util::NamedDouble::create("outTangentialTotalWeightN", outMap, tolerance);
    outLoadBearingFa2N = util::NamedDouble::create("outLoadBearingFa2N", outMap, tolerance);
    outLoadBearingFa3N = util::NamedDouble::create("outLoadBearingFa3N", outMap, tolerance);
    outLoadBearingFa5N = util::NamedDouble::create("outLoadBearingFa5N", outMap, tolerance);
    outLoadBearingFa6N = util::NamedDouble::create("outLoadBearingFa6N", outMap, tolerance);
    outNetMomentForcesN = util::NamedDouble::create("outNetMomentForcesN", outMap, tolerance);
    outFa1N = util::NamedDouble::create("outFa1N", outMap);
    outFa4N = util::NamedDouble::create("outFa4N", outMap);

    outTanWeightBool = util::NamedBool::create("outTanWeightBool", outMap);
    outLoadBearingBool = util::NamedBool::create("outLoadBearingBool", outMap);
    outNetMomentBool = util::NamedBool::create("outNetMomentBool", outMap);
    outNonLoadBearingBool = util::NamedBool::create("outNonLoadBearingBool", outMap);
    outTanLoadCellBool = util::NamedBool::create("outTanLoadCellBool", outMap);

    util::NamedValue::insertMapElements(inMap, completeMap);
    util::NamedValue::insertMapElements(outMap, completeMap);
    util::NamedValue::insertMapElements(constMap, completeMap);

    setConstFromConfig();
}

void TangentLoadCellFaultDetection::run() {
    LDEBUG("TangentLoadCellFaultDetection::run start");
    constexpr double cos30Deg = cos(30.0 * util::NamedAngle::PI / 180.0);
    double zeta = util::NamedAngle::PI / 2.0 - inElevationAngleD->val;  // 90Deg - elevation
    double elevationComp = sin(zeta) * _constMirrorWeightN->val;
    double mirrorWeightCompDiv4 = elevationComp / 4.0;

    // Tangential weight error
    double fa2W = (-inFa2N->val * cos30Deg);
    double fa3W = (-inFa3N->val * cos30Deg);
    double fa5W = (inFa5N->val * cos30Deg);
    double fa6W = (inFa6N->val * cos30Deg);
    outTangentialTotalWeightN->val = fa2W + fa3W + fa5W + fa6W - elevationComp;
    outTanWeightBool->val = fabs(outTangentialTotalWeightN->val) >= _constTanWeightErrorN->val;

    // Individual load bearing error
    outLoadBearingFa2N->val = cos30Deg * inFa2N->val + mirrorWeightCompDiv4;
    outLoadBearingFa3N->val = cos30Deg * inFa3N->val + mirrorWeightCompDiv4;
    outLoadBearingFa5N->val = cos30Deg * inFa5N->val - mirrorWeightCompDiv4;
    outLoadBearingFa6N->val = cos30Deg * inFa6N->val - mirrorWeightCompDiv4;

    bool individualWeightError2 = fabs(outLoadBearingFa2N->val) >= _constLoadBearingErrorN->val;
    bool individualWeightError3 = fabs(outLoadBearingFa3N->val) >= _constLoadBearingErrorN->val;
    bool individualWeightError5 = fabs(outLoadBearingFa5N->val) >= _constLoadBearingErrorN->val;
    bool individualWeightError6 = fabs(outLoadBearingFa6N->val) >= _constLoadBearingErrorN->val;
    outLoadBearingBool->val = individualWeightError2 || individualWeightError3 || individualWeightError5 ||
                              individualWeightError6;

    // Tangent Sum, Theta Z Moment Error
    outNetMomentForcesN->val =
            inFa1N->val + inFa2N->val + inFa3N->val + inFa4N->val + inFa5N->val + inFa6N->val;
    outNetMomentBool->val = fabs(outNetMomentForcesN->val) > _constNetMomentErrorN->val;

    // Non-load bearing
    outFa1N->val = inFa1N->val;
    outFa4N->val = inFa4N->val;
    outNonLoadBearingBool->val = (fabs(outFa1N->val) > _constNotLoadBearingErrorN->val) ||
                                 (fabs(outFa4N->val) > _constNotLoadBearingErrorN->val);

    outTanLoadCellBool->val = outTanWeightBool->val || outLoadBearingBool->val ||
                              outNonLoadBearingBool->val || outNetMomentBool->val;

    LDEBUG("TangentLoadCellFaultDetection::run() ", dumpStr());
}

}  // namespace vis
}  // namespace m2cellcpp
}  // namespace LSST
