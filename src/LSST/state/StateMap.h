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

#ifndef LSST_M2CELLCPP_STATE_STATEMAP_H
#define LSST_M2CELLCPP_STATE_STATEMAP_H

// System headers
#include <map>
#include <memory>
#include <vector>

// Project headers
#include "state/FaultState.h"
#include "state/IdleState.h"
#include "state/InMotionState.h"
#include "state/OfflineState.h"
#include "state/PauseState.h"
#include "state/StandbyState.h"
#include "state/StartupState.h"
#include "state/State.h"

namespace LSST {
namespace m2cellcpp {
namespace state {

/// This class contains a map of all possible states and the current state.
/// unit test: test_StateMap.cpp
class StateMap {
public:
    /// The type for the map of states. TODO: it should be an enum instead of string.
    using MapType = std::map<std::string, State::Ptr>;

    /// Constructor, initial _currentState is StandByState.
    /// @param `model` pointer to the model.
    StateMap(Model *const model);

    StateMap() = delete;
    virtual ~StateMap() = default;

    /// Return the current state.
    State::Ptr getCurrentState() { return _currentState; }

    /// Change the state to `newState`, if possible.
    /// @return false if the state could not be changed.
    bool changeState(std::string const& newState);

    /// Change the state to `newState`, if possible.
    /// @return false if the state could not be changed.
    bool changeState(State::Ptr const& newState);

    /// Get the state with name `stateName`.
    /// @returns nullptr if `stateName` could not be found.
    State::Ptr getState(std::string const& stateName);

    /// Return a pointer to the startup state.
    StartupState::Ptr getStartupState() { return _startupState; }

    /// Return a pointer to the standby state.
    StandbyState::Ptr getStandbyState() { return _standbyState; }

    /* &&&
    /// Return a pointer to the fault state.
    FaultState::Ptr getFaultState() { return _faultState; }
    */

    /// Return a pointer to the Idle state.
    IdleState::Ptr getIdleState() { return _idleState; }

    /// Return a pointer to the inMotion state.
    InMotionState::Ptr getInMotionState() { return _inMotionState; }

    /// Return a pointer to the offline state.
    OfflineState::Ptr getOfflineState() { return _offlineState; }

    /// Return a pointer to the pause state.
    PauseState::Ptr getPauseState() { return _pauseState; }

    /// Go to a state that turns off power and motion. Multiple
    /// states match this criteria.
    /// @param `desiredState` - This state must be one of the states that has power
    /// and motion off (see `_safeStates`). If the current
    /// state is "OfflineState", `desiredState will be ignored and the system
    /// will stay in "OfflineState".
    /// @param `note` - A note to indicate what called this function.
    /// @return true if the `desiredState` was used to set `_currentState`.
    bool goToASafeState(std::string const& desiredState, std::string const& note);

    /// Insert `state` into the state map.
    void insertIntoMap(State::Ptr const& state);

    /// Returns true is state is in the `_safeStates` list.
    bool isASafeState(std::string const& state) const;

private:
    /// Change the state to `newState`, if possible. `newState`
    /// should have been checked as valid and exisiting in the
    /// `_stateMap` before this is called.
    /// @return false if the state could not be changed.
    bool _changeState(State::Ptr const& newState);

    /// Reference to the global model. Since StateMap only exists
    /// as a member of the Model, this is always safe.
    Model *const _model;

    /// Map of all possible states with their names as the keys.
    /// It will only include the items defined below.
    MapType _stateMap;

    StartupState::Ptr _startupState;    ///< The Startup state instance.
    StandbyState::Ptr _standbyState;    ///< The Standby state instance.
    // &&& FaultState::Ptr _faultState;        ///< The Fault state instance.
    IdleState::Ptr _idleState;          ///< The Idle state instance.
    InMotionState::Ptr _inMotionState;  ///< The InMotion state instance.
    OfflineState::Ptr _offlineState;    ///< The Offline state instance.
    PauseState::Ptr _pauseState;        ///< The Pause state instance.

    /// The current state of the system. This must always point
    /// one of the elements of `_stateMap`.
    State::Ptr _currentState;

    std::vector<State::Ptr> _safeStates; ///< List of safe states (power and motion off)
};

}  // namespace state
}  // namespace m2cellcpp
}  // namespace LSST

#endif  // LSST_M2CELLCPP_STATE_STATEMAP_H
