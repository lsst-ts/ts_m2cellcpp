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
#include "control/StateMap.h"

// System headers

// Project headers
#include "util/Log.h"

using namespace std;

namespace LSST {
namespace m2cellcpp {
namespace control {

StateMap::StateMap() {
    _startupState = StartupState::create(*this);
    _standbyState = StandbyState::create(*this);
    _faultState = FaultState::create(*this);
    _idleState = IdleState::create(*this);
    _inMotionState = InMotionState::create(*this);
    _offlineState = OfflineState::create(*this);
    _pauseState = PauseState::create(*this);

    _currentState = _startupState;
}

void StateMap::insertIntoMap(State::Ptr const& state) {
    auto [iter, success] = _stateMap.emplace(state->getName(), state);
    if (!success) {
        string name = state->getName();
        throw util::Bug(ERR_LOC, name + " was already in stateMap!");
    }
}

bool StateMap::changeState(string const& newState) {
    LDEBUG("changeState newState=", newState);

    // Find the state
    auto iter = _stateMap.find(newState);
    if (iter == _stateMap.end()) {
        LERROR("changeState unknown newState=", newState);
        return false;
    }
    auto& newStatePtr = iter->second;
    return changeState(newStatePtr);
}

bool StateMap::changeState(State::Ptr const& newState) {
    if (newState == nullptr) {
        LWARN("StateMap cannot change state to nullptr");
        return false;
    }

    // FUTURE: The LabView code doesn't make checks for valid
    // state transitions, but this may be a good check to
    // add here.
    auto oldState = _currentState;
    _currentState->onExitState(newState);
    _currentState = newState;
    newState->onEnterState(oldState);
    return true;
}

State::Ptr StateMap::getState(std::string const& stateName) {
    auto iter = _stateMap.find(stateName);
    if (iter == _stateMap.end()) {
        LDEBUG("unknown state=", stateName);
        return nullptr;
    }
    return iter->second;
}

}  // namespace control
}  // namespace m2cellcpp
}  // namespace LSST
