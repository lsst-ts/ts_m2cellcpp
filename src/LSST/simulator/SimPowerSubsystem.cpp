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

// Class header
#include "simulator/SimPowerSubsystem.h"

// System headers

// Project headers
#include "util/Log.h"

using namespace std;

namespace LSST {
namespace m2cellcpp {
namespace simulator {

SimPowerSubsystem::SimPowerSubsystem(control::OutputPortBits::Ptr const& outputPort, int powerOnBitPos,
        control::InputPortBits::Ptr const& inputPort,  std::vector<int> const& breakerBitPositions)
        : _outputPort(outputPort),  _powerOnBitPos(powerOnBitPos), _inputPort(inputPort), _breakerBitPositions(breakerBitPositions) {

}


void SimPowerSubsystem::calcBreakers(system::CLOCK::time_point ts) {
    //&&&
}


void SimPowerSubsystem::calcVoltageCurrent(system::CLOCK::time_point ts) {
    // &&&
}



}  // namespace simulator
}  // namespace m2cellcpp
}  // namespace LSST
