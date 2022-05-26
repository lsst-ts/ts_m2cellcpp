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

#ifndef LSST_M2CELLCPP_CONTROL_STATEMAP_H
#define LSST_M2CELLCPP_CONTROL_STATEMAP_H

// System headers
#include <map>
#include <memory>

// Project headers
#include "control/State.h"

namespace LSST {
namespace m2cellcpp {
namespace control {

/// This class contains a map of all possible states and the current state.
/// unit test: test_StateMap.cpp
class StateMap {
public:
    using MapType = std::map<std::string, State::Ptr>;
    /// Constructor, initial _currentState is StandByState.
    StateMap();
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

    /// Return a pointer to the fault state.
    FaultState::Ptr getFaultState() { return _faultState; }

    /// Return a pointer to the Idle state.
    IdleState::Ptr getIdleState() { return _idleState; }

    /// Return a pointer to the inMotion state.
    InMotionState::Ptr getInMotionState() { return _inMotionState; }

    /// Return a pointer to the offline state.
    OfflineState::Ptr getOfflineState() { return _offlineState; }

    /// Return a pointer to the pause state.
    PauseState::Ptr getPauseState() { return _pauseState; }

    /// Insert `state` into the state map.
    void insertIntoMap(State::Ptr const& state);

private:
    /// Map of all possible states with their names as the keys.
    MapType _stateMap;

    StartupState::Ptr _startupState;    ///< The Startup state instance.
    StandbyState::Ptr _standbyState;    ///< The Standby state instance.
    FaultState::Ptr _faultState;        ///< The Fault state instance.
    IdleState::Ptr _idleState;          ///< The Idle state instance.
    InMotionState::Ptr _inMotionState;  ///< The InMotion state instance.
    OfflineState::Ptr _offlineState;    ///< The Offline state instance.
    PauseState::Ptr _pauseState;        ///< The Pause state instance.

    /// The current state of the system.
    State::Ptr _currentState;
};

}  // namespace control
}  // namespace m2cellcpp
}  // namespace LSST

#endif  // LSST_M2CELLCPP_CONTROL_STATEMAP_H
