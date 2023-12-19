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
#include "state/StateMap.h"

// System headers

// Project headers
#include "util/Log.h"

using namespace std;

namespace LSST {
namespace m2cellcpp {
namespace state {

StateMap::StateMap(Model* const model) : _model(model) {
    _startupState = StartupState::create(*this, _model);
    _standbyState = StandbyState::create(*this, _model);
    _idleState = IdleState::create(*this, _model);
    _inMotionState = InMotionState::create(*this, _model);
    _offlineState = OfflineState::create(*this, _model);
    _pauseState = PauseState::create(*this, _model);

    _currentState = _startupState;

    /// Add safe states to the list.
    _safeStates.push_back(_standbyState);
    _safeStates.push_back(_offlineState);
}

void StateMap::insertIntoMap(State::Ptr const& state) {
    auto [iter, success] = _stateMap.emplace(state->getId(), state);
    if (!success) {
        string name = state->getName();
        throw util::Bug(ERR_LOC, name + " was already in stateMap!");
    }
}

bool StateMap::changeState(State::StateEnum newState) {
    LDEBUG("changeState newState=", State::getStateEnumStr(newState));

    // Find the state
    auto iter = _stateMap.find(newState);
    if (iter == _stateMap.end()) {
        LERROR("changeState unknown newState=", State::getStateEnumStr(newState));
        return false;
    }
    auto& newStatePtr = iter->second;
    return _changeState(newStatePtr);
}

bool StateMap::changeState(State::Ptr const& newState) {
    if (newState == nullptr) {
        LWARN("StateMap cannot change state to nullptr");
        return false;
    }

    // The new state much match a state in the state map.
    // Find the state
    auto iter = _stateMap.find(newState->getId());
    if (iter == _stateMap.end()) {
        LERROR("changeState unknown newState=", newState);
        return false;
    }
    if (iter->second != newState) {
        LWARN("StateMap newState=", newState->getName(), " is not in the map");
        return false;
    }
    return _changeState(newState);
}

bool StateMap::_changeState(State::Ptr const& newState) {
    if (newState == nullptr) {
        LWARN("StateMap cannot change state to nullptr");
        return false;
    }
    LDEBUG("StateMap::_changeState trying to change to ", newState->getName());

    // The new state should have been verified as being in the map.
    auto oldState = _currentState;
    if (newState == oldState) {
        // Re-run the enter state options. This is useful for for
        // states like StandbyState to make sure everything is turned off.
        // Other states should be checking the old state and taking appropriate
        // action.
        newState->onEnterState(oldState);
        return true;
    }

    // If the current state is "startupState", the state cannot be changed until
    // the Model believes all files have been loaded, etc.
    if (_currentState->getId() == State::STARTUPSTATE) {
        if (!_startupState->isStartupFinished()) {
            LERROR("StateMap::_changeState cannot leave StartupState as system isn't ready.");
            return false;
        }
    }

    // Safe state transitions come from "Support_VIs->States" in the LabView code.
    // Each state allows only certain actions as seen in the "Support_VIs->Commands"
    // section where several Commands exec.vi actions depend on the _currentState,
    // such as "Support_VIs->Commands->Pause.lvclass:exec.vi"

    bool acceptable = false;
    auto const currentStateId = _currentState->getId();

    if (isASafeState(newState->getId())) {
        // It's always safe to go to these states as they
        // turn off power and motion.
        acceptable = true;
    } else if (currentStateId == State::IDLESTATE) {
        if (newState->getId() == State::INMOTIONSTATE || newState->getId() == State::PAUSESTATE) {
            // Motion states are accessible from IdleState.
            acceptable = true;
        }
    } else if (currentStateId == State::INMOTIONSTATE) {
        if (newState->getId() == State::INMOTIONSTATE || newState->getId() == State::PAUSESTATE ||
            newState->getId() == State::IDLESTATE) {
            // InMotion state can go to idle or pause.
            acceptable = true;
        }
    } else if (currentStateId == State::PAUSESTATE) {
        if (newState->getId() == State::INMOTIONSTATE || newState->getId() == State::IDLESTATE) {
            // pause can go to idle or motion.
            acceptable = true;
        }
    } else if (isASafeState(currentStateId) || currentStateId == State::STARTUPSTATE) {
        if (newState->getId() == State::IDLESTATE) {
            // Any safe state can go to idle.
            acceptable = true;
        }
    }

    if (acceptable) {
        LINFO("Changing state from ", oldState->getName(), " to ", newState->getName());
        _currentState->onExitState(newState);
        _currentState = newState;
        newState->onEnterState(oldState);
    } else {
        LERROR("Cannot change state from ", oldState->getName(), " to ", newState->getName());
    }
    return acceptable;
}

State::Ptr StateMap::getState(State::StateEnum stateId) {
    auto iter = _stateMap.find(stateId);
    if (iter == _stateMap.end()) {
        LDEBUG("unknown state=", to_string(stateId));
        return nullptr;
    }
    return iter->second;
}

bool StateMap::isASafeState(State::StateEnum stateId) const {
    for (auto const& safe : _safeStates) {
        if (stateId == safe->getId()) {
            return true;
        }
    }
    return false;
}

bool StateMap::goToASafeState(State::StateEnum desiredState, string const& note) {
    LDEBUG("StateMap::goToASafeState ", State::getStateEnumStr(desiredState), " ", note);
    if (_currentState == _offlineState || desiredState == _offlineState->getId()) {
        changeState(_offlineState);
        return (desiredState == _offlineState->getId());
    }

    if (isASafeState(desiredState)) {
        changeState(desiredState);
        return true;
    }
    changeState(_standbyState);
    return false;
}

}  // namespace state
}  // namespace m2cellcpp
}  // namespace LSST
