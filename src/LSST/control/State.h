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

#ifndef LSST_M2CELLCPP_CONTROL_STATE_H
#define LSST_M2CELLCPP_CONTROL_STATE_H

// System headers
#include <functional>
#include <map>
#include <memory>

// Project headers
#include "util/Bug.h"

namespace LSST {
namespace m2cellcpp {
namespace control {

class Model;
class StateMap;

/// This is the base class for system states.
class State {
public:
    using Ptr = std::shared_ptr<State>;
    State(std::string const& name) : _name(name) {}
    State() = delete;
    State(State const&) = delete;
    State& operator=(State const&) = delete;
    virtual ~State() = default;

    /// Return the name of the state.
    std::string getName() { return _name; }

    /// Do things that need to be done every time entering a state.
    /// This calls `enterState`.
    void onEnterState(Ptr const& oldName);

    /// Do things specific to entering a state.
    virtual void enterState(Ptr const& oldName) { return; }

    /// Do things that need to be done every time leaving a state.
    /// This calls `exitState`.
    void onExitState(Ptr const& newStateName);

    /// Do things specific to leaving a state.
    virtual void exitState(Ptr const& newStateName) { return; }

    /// Go from the current state to IdleState, taking required actions.
    /// @throws Bug if this has not been override by a child class.
    virtual void goToIdleReadyVI() { throw util::Bug(ERR_LOC, getName() + " unexpected goToOfflineVI call"); }

    /// Go from the current state to InMotionState, taking required actions.
    /// @throws Bug if this has not been override by a child class.
    virtual void goToInMotionVI() { throw util::Bug(ERR_LOC, getName() + " unexpected goToOfflineVI call"); }

    /// Go from the current state to PauseState, taking required actions.
    /// @throws Bug if this has not been override by a child class.
    virtual void goToPauseVI() { throw util::Bug(ERR_LOC, getName() + " unexpected goToOfflineVI call"); }

    /// Go from the current state to StandbyState, taking required actions.
    /// @throws Bug if this has not been override by a child class.
    virtual void goToStandbyVI() { throw util::Bug(ERR_LOC, getName() + " unexpected goToOfflineVI call"); }

    /// Go from the current state to FaultState, taking required actions.
    /// @throws Bug if this has not been override by a child class.
    /// FUTURE: Maybe delete as LabView code never uses this state.
    virtual void goToFaultVI() { throw util::Bug(ERR_LOC, getName() + " unexpected goToOfflineVI call"); }

    /// Go from the current state to OfflineState, taking required actions.
    /// @throws Bug if this has not been override by a child class.
    /// Note: Getting to this state is somewhat different in LabView code.
    ///      In LabView, this state is reached through a call to exit().
    virtual void goToOfflineVI() { throw util::Bug(ERR_LOC, getName() + " unexpected goToOfflineVI call"); }

    // Public VI's that need to be accounted for
    //   (VI-PH indicates placeholder for LabView VI)
    // VI-PH  clearErrorVI; // Does nothing
    // VI-PH  exitVI; // Nada
    // VI-PH  goToIdelReadyVI; // Nada  - status:minimal implementation
    // VI-PH  goToInMotionVI; // Nada  - status:minimal implementation
    // VI-PH  goToPauseVI; // Nada  - status:minimal implementation
    // VI-PH  goToStandbyVI; // Nada - status:minimal implementation
    // VI-PH  initScriptEngineVI; // ??? if case with no effect
    // VI-PH  loadScriptVI; // ??? unwired Script Filename
    // VI-PH  pauseVI; // Nada
    // VI-PH  pauseScriptVI; // ??? if case with no effect
    // VI-PH  rebootVI; // ??? if case with no effect
    // VI-PH  resetForceVectorVI; // ??? if case with no effect
    // VI-PH  restartVI; // ??? if case with no effect
    // VI-PH  resumeVI; // Nada
    // VI-PH  resumeScriptVI; // ??? if case with no effect
    // VI-PH  setDeltaForcVectorVI; // ??? if case with no effect
    // VI-PH  setElevationAngleVI; // ??? if case with no effect
    // VI-PH  setPowerVI; // Maybe does something, but looks incomplete
    // VI-PH  setSlewingStateVI; // ??? if case with no effect
    // VI-PH  shutdownCellCommVI; // ??? if case with no effect
    // VI-PH  shutdownLoggerVI; // Nada
    // VI-PH  shutdownMotionEngineVI; // nada
    // VI-PH  shutdownNetworkInterfaceVI; // nada
    // VI-PH  shutdownScriptEngineVI; // ??? if case with no effect
    // VI-PH  startVI; // nada
    // VI-PH  startMotionVI; // ??? unfinished
    // VI-PH  startScriptVI; // ??? if case with no effect
    // VI-PH  stopMotionVI; // nada
    // VI-PH  stopScriptVI; // ??? if case with no effect

private:
    std::string const _name;  ///< Name of this state.
};

}  // namespace control
}  // namespace m2cellcpp
}  // namespace LSST

#endif  // LSST_M2CELLCPP_CONTROL_STATE_H
