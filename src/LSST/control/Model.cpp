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
#include "control/Model.h"

// System headers

// Project headers
#include "control/PowerSystem.h"
#include "control/State.h"
#include "util/Log.h"

namespace LSST {
namespace m2cellcpp {
namespace control {

Model::Model() {
    LDEBUG("Model::Model() creating PowerSystem");
    _powerSystem = PowerSystem::Ptr(new PowerSystem());
}

State::Ptr Model::getCurrentState() { return _stateMap.getCurrentState(); }

std::shared_ptr<State> Model::getState(std::string const& stateName) { return _stateMap.getState(stateName); }

bool Model::changeState(std::shared_ptr<State> const& newState) { return _stateMap.changeState(newState); }

bool Model::goToSafeMode(std::string const& note) {
    if (_powerSystem == nullptr) {
        LERROR("Model::goToSafeMode _powerSystem is nullptr");
        return false;
    }
    // TODO: DM-40339 Add code to put the system in safe mode,
    //       If the system is already in safe mode, return false
    LCRITICAL("Model::goToSafeMode needs code note=", note);
    // Put the system in safe mode.
    // This includes setting "Closed loop control" (aka "CLC Mode") to Idle
    // Turning motor power off
    _powerSystem->getMotor().setPowerOff("safe mode");
    // Turning comm power off
    _powerSystem->getComm().setPowerOff("safe mode");
    return true;
}

}  // namespace control
}  // namespace m2cellcpp
}  // namespace LSST
