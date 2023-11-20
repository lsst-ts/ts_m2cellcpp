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
#include "state/State.h"

// Project headers
#include "state/Model.h"
#include "state/StateMap.h"
#include "util/Bug.h"
#include "util/Log.h"

using namespace std;

namespace LSST {
namespace m2cellcpp {
namespace state {

std::string State::getStateEnumStr(StateEnum stEnum) {
    switch (stEnum) {
    case OFFLINESTATE: return string("OfflineState");
    case STARTUPSTATE: return string("StartupState");
    case STANDBYSTATE: return string("StandbyState");
    case IDLESTATE: return string("IdleState");
    case PAUSESTATE: return string("PauseState");
    case INMOTIONSTATE: return string("InMotionState");
    }
    throw util::Bug(ERR_LOC, "State::getStateEnumStr unknown state " +  to_string(stEnum));
}

void State::errorMsg(std::string const& msg) {
    LERROR("State error ", msg);
}

/// Log a message indicating a problem that this `action` cannot be performed
/// in the current `State`
void State::errorWrongStateMsg(std::string const& action) {
    stringstream os;
    os << action << " is not a valid option while in State " << getName();
    errorMsg(os.str());
}

void State::enterState(State::Ptr const& oldState) {
    string msg("State::enterState " + getName());
    LINFO(msg);
    if (modelPtr == nullptr) {
        LERROR("State::enterState ignoring due to unit test.");
        return;
    }
    modelPtr->_turnOffAll(msg);
}


void State::onEnterState(State::Ptr const& oldState) {
    LINFO("Entering state=", getName(), " from oldState=", oldState->getName());
    enterState(oldState);
}

void State::onExitState(State::Ptr const& newState) {
    LINFO("Leaving state=", getName(), " to go to newState=", newState->getName());
    exitState(newState);
}

}  // namespace state
}  // namespace m2cellcpp
}  // namespace LSST
