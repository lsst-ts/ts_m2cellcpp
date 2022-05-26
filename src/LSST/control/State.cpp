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
#include "control/State.h"

// Project headers
#include "control/Model.h"
#include "control/StateMap.h"
#include "util/Bug.h"
#include "util/Log.h"

using namespace std;

namespace LSST {
namespace m2cellcpp {
namespace control {

void State::onEnterState(State::Ptr const& oldState) {
    LINFO("Entering state=", getName(), " from oldState=", oldState->getName());
    enterState(oldState);
}

void State::onExitState(State::Ptr const& newState) {
    LINFO("Leaving state=", getName(), " to go to newState=", newState->getName());
    exitState(newState);
}

FaultState::Ptr FaultState::create(StateMap& stateMap) {
    auto state = shared_ptr<FaultState>(new FaultState());
    stateMap.insertIntoMap(state);
    return state;
}

IdleState::Ptr IdleState::create(StateMap& stateMap) {
    auto state = shared_ptr<IdleState>(new IdleState());
    stateMap.insertIntoMap(state);
    return state;
}

void IdleState::goToInMotionVI() { throw util::Bug(ERR_LOC, "IdleState::goToInMotion PLACEHOLDER"); }

void IdleState::goToStandbyVI() { throw util::Bug(ERR_LOC, "IdleState::goToInMotion PLACEHOLDER"); }

InMotionState::Ptr InMotionState::create(StateMap& stateMap) {
    auto state = shared_ptr<InMotionState>(new InMotionState());
    stateMap.insertIntoMap(state);
    return state;
}

void InMotionState::goToIdleReadyVI() { throw util::Bug(ERR_LOC, "goToInMotion PLACEHOLDER"); }

void InMotionState::goToPauseVI() { throw util::Bug(ERR_LOC, "goToPauseVI PLACEHOLDER"); }

OfflineState::Ptr OfflineState::create(StateMap& stateMap) {
    auto state = shared_ptr<OfflineState>(new OfflineState());
    stateMap.insertIntoMap(state);
    return state;
}

PauseState::Ptr PauseState::create(StateMap& stateMap) {
    auto state = shared_ptr<PauseState>(new PauseState());
    stateMap.insertIntoMap(state);
    return state;
}

void PauseState::goToIdleReadyVI() { throw util::Bug(ERR_LOC, "goToIdleReady PLACEHOLDER"); }

void PauseState::goToInMotionVI() { throw util::Bug(ERR_LOC, "goToInMotion PLACEHOLDER"); }

StandbyState::Ptr StandbyState::create(StateMap& stateMap) {
    auto state = shared_ptr<StandbyState>(new StandbyState());
    stateMap.insertIntoMap(state);
    return state;
}

StartupState::Ptr StartupState::create(StateMap& stateMap) {
    auto state = shared_ptr<StartupState>(new StartupState());
    stateMap.insertIntoMap(state);
    return state;
}

}  // namespace control
}  // namespace m2cellcpp
}  // namespace LSST
