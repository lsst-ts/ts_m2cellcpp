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
#include "state/PauseState.h"

// Project headers
#include "state/Model.h"
#include "state/StateMap.h"
#include "util/Bug.h"
#include "util/Log.h"

using namespace std;

namespace LSST {
namespace m2cellcpp {
namespace state {

PauseState::Ptr PauseState::create(StateMap& stateMap, Model* const model) {
    auto state = shared_ptr<PauseState>(new PauseState(model));
    stateMap.insertIntoMap(state);
    return state;
}

void PauseState::enterState(State::Ptr const& oldState) { enterStateBase(oldState); }

void PauseState::goToIdleReadyVI() { throw util::Bug(ERR_LOC, "goToIdleReady PLACEHOLDER"); }

void PauseState::goToInMotionVI() { throw util::Bug(ERR_LOC, "goToInMotion PLACEHOLDER"); }

}  // namespace state
}  // namespace m2cellcpp
}  // namespace LSST
