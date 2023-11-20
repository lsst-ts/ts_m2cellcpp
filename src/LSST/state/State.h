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

#ifndef LSST_M2CELLCPP_STATE_STATE_H
#define LSST_M2CELLCPP_STATE_STATE_H

// System headers
#include <functional>
#include <map>
#include <memory>

// Project headers
#include "util/Bug.h"

namespace LSST {
namespace m2cellcpp {
namespace state {

class Model;
class StateMap;

/// This is the base class for system states. It determines what commands the
/// are allowed to be used. Each child class has overridden functions for the
/// commands that can be used.
/// In the LabView code, the most critical element to be aware of is
/// "Command.lvclass.::exec.vi" which is used in "Controller.lvclass:controllerMain.vi"
/// within
class State {
public:
    using Ptr = std::shared_ptr<State>;

    /// Enum for the state name, one for each possible state.
    enum StateEnum {
        OFFLINESTATE  = 0,
        STARTUPSTATE  = 1,
        STANDBYSTATE  = 2,
        IDLESTATE     = 3,
        PAUSESTATE    = 4,
        INMOTIONSTATE = 5
    };

    /// Return the string version of `stEnum`.
    static std::string getStateEnumStr(StateEnum stEnum);

    State(StateEnum stateId,  Model *const model_) : modelPtr(model_), _stateId(stateId), _name(getStateEnumStr(_stateId)) {}
    State() = delete;
    State(State const&) = delete;
    State& operator=(State const&) = delete;
    virtual ~State() = default;

    /// Return the name of the state.
    std::string getName() const { return _name; }

    /// Return the state identifier.
    StateEnum getId() const { return _stateId; }

    /// Do things that need to be done every time entering a state.
    /// This calls `enterState`.
    void onEnterState(Ptr const& oldName);

    /// Do things specific to entering a state.
    /// Unless overriden, this will shut off power and stop motion.
    virtual void enterState(Ptr const& oldName);

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

    /// Return true if power could be set to 'on'.
    /// VI-PH  setPowerVI;
    virtual bool setPower(bool on) {
        errorWrongStateMsg("setPower");
        return false;
    }
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

    /// Log a message indicating a problem. Possibly send an error message back
    /// to the client.
    void errorMsg(std::string const& msg);

    /// Log a message indicating a problem that this `action` cannot be performed
    /// in the current `State`
    void errorWrongStateMsg(std::string const& action);

protected:
    /// Constant pointer to the model, which can be nullptr in unit tests.
    Model *const modelPtr;

private:
    StateEnum const _stateId; ///< The type of this state.
    std::string const _name;  ///< Name of this state.

};

}  // namespace state
}  // namespace m2cellcpp
}  // namespace LSST

#endif  // LSST_M2CELLCPP_STATE_STATE_H
