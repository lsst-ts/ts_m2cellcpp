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

#ifndef LSST_M2CELLCPP_CONTROL_INMOTIONSTATE_H
#define LSST_M2CELLCPP_CONTROL_INMOTIONSTATE_H

// System headers
#include <functional>
#include <map>
#include <memory>

// Project headers
#include "control/State.h"

namespace LSST {
namespace m2cellcpp {
namespace control {

/// Class representation of the "InMotionState", aka ReadyInMotion.
class InMotionState : public State {
public:
    using Ptr = std::shared_ptr<InMotionState>;

    /// Create an instance and insert it into `stateMap`.
    /// @throws Bug if there's already an instance of this class in `stateMap`.
    static Ptr create(StateMap& stateMap);

    InMotionState(InMotionState const&) = delete;
    InMotionState& operator=(InMotionState const&) = delete;
    virtual ~InMotionState() = default;

    /// VI-PH  goToIdleReadyVI // calls Model::changeStateVI(ReadyIdle)
    void goToIdleReadyVI() override;

    /// VI-PH  goToPauseVI // calls Model::changeStateVI(ReadyPause)
    void goToPauseVI() override;

    // VI-PH  pauseVI // calls Model::pauseVI
    // VI-PH  shutdownMotionEngineVI // calls Model::shutdownMotionEngineVI
    // VI-PH  stopMotionVI // calls Model::stopMotionVI
    // VI-PH  shutdownMotionEngineVI // calls Model::shutdownMotionEngineVI
    // VI-PH  stopMotionVI // calls Model::stopMotionVI
private:
    InMotionState() : State("InMotionState") {}
};

}  // namespace control
}  // namespace m2cellcpp
}  // namespace LSST

#endif  // LSST_M2CELLCPP_CONTROL_INMOTIONSTATE_H
