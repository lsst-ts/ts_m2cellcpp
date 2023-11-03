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
#include "state/IdleState.h"

// Project headers
#include "state/Model.h"
#include "state/StateMap.h"
#include "util/Bug.h"
#include "util/Log.h"

using namespace std;

namespace LSST {
namespace m2cellcpp {
namespace state {

IdleState::Ptr IdleState::create(StateMap& stateMap, Model *const model) {
    auto state = shared_ptr<IdleState>(new IdleState(model));
    stateMap.insertIntoMap(state);
    return state;
}

//&&&void IdleState::goToInMotionVI() { throw util::Bug(ERR_LOC, "IdleState::goToInMotion PLACEHOLDER"); }

//&&&void IdleState::goToStandbyVI() { throw util::Bug(ERR_LOC, "IdleState::goToInMotion PLACEHOLDER"); }

bool IdleState::setPower(bool on) {
    if (modelPtr == nullptr) {
        LERROR("IdleState modelPtr is NULL");
        return false;
    }
    return modelPtr->_setPower(on);
}


}  // namespace state
}  // namespace m2cellcpp
}  // namespace LSST
