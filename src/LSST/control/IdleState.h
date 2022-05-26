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

#ifndef LSST_M2CELLCPP_CONTROL_IDLESTATE_H
#define LSST_M2CELLCPP_CONTROL_IDLESTATE_H

// System headers
#include <functional>
#include <map>
#include <memory>

// Project headers
#include "control/State.h"

namespace LSST {
namespace m2cellcpp {
namespace control {

/// Class representation of the "IdleState", aka ReadyIdle.
class IdleState : public State {
public:
    using Ptr = std::shared_ptr<IdleState>;

    /// Create an instance and insert it into `stateMap`.
    /// @throws Bug if there's already an instance of this class in `stateMap`.
    static Ptr create(StateMap& stateMap);

    IdleState(IdleState const&) = delete;
    IdleState& operator=(IdleState const&) = delete;
    virtual ~IdleState() = default;

    /// VI-PH  goToInMotionVI // calls Model::changeStateVI(ReadyInMotion)
    void goToInMotionVI() override;

    /// VI-PH  goToStandbyVI // calls Model::stopVI then Model::changeStateVI(StandbyState)
    void goToStandbyVI() override;

    // VI-PH  initScriptEngineVI // calls Model::initScriptEngineVI
    // VI-PH  loadScriptVI // calls Model::loadScriptVI(scriptFilename)
    // VI-PH  pauseScriptVI // calls Model::pauseScriptVI
    // VI-PH  resumeScriptVI // calls Model::resumeScriptVI
    // VI-PH  setPowerVI // calls Model::setPowerVI(CommPowerControl, MotorPowerControl)   (both power
    // controls are enums) VI-PH  shutdownCellCommVI // calls Model::shutdownCellComm VI-PH
    // shutdownMotionEngineVI   // calls Model::shutdownMotionEngine VI-PH  shutdownNetworkInterfaceVI  //
    // calls Model::shutdownNetworkInterfaceVI VI-PH  shutdownScriptEngineVI // calls
    // Model::shutdownScriptEngineVI VI-PH  startMotionVI // calls Model::startMotionVI(startMotionParam)
    // (StartMotionParam includes (Timestamp, MotionStepVector, etc...) VI-PH  startScriptVI // calls
    // Model::startScriptVI VI-PH stopScriptVI
    // // call Model::stopScriptVI
private:
    IdleState() : State("IdleState") {}
};

}  // namespace control
}  // namespace m2cellcpp
}  // namespace LSST

#endif  // LSST_M2CELLCPP_CONTROL_IDLESTATE_H
