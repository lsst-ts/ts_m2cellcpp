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

/// This class represents the state when the system is started.
/// THis class may be removed in the future if it duplicates the
/// functionality of another state.
class StartupState : public State {
public:
    using Ptr = std::shared_ptr<StartupState>;

    /// Create an instance and insert it into `stateMap`.
    /// @throws Bug if there's already an instance of this class in `stateMap`.
    static Ptr create(StateMap& stateMap);

    StartupState(StartupState const&) = delete;
    StartupState& operator=(StartupState const&) = delete;
    virtual ~StartupState() = default;

private:
    StartupState() : State("StartupState") {}
};

/// Class representation of the FaultState, this state appears to be unused in LabView!
class FaultState : public State {
public:
    using Ptr = std::shared_ptr<FaultState>;

    /// Create an instance and insert it into `stateMap`.
    /// @throws Bug if there's already an instance of this class in `stateMap`.
    static Ptr create(StateMap& stateMap);

    FaultState(FaultState const&) = delete;
    FaultState& operator=(FaultState const&) = delete;
    virtual ~FaultState() = default;
    // VI-PH  clearErrorVI   // calls Model::resetErrorCodeVI then Model::changeStateVI(StandbyState)
    // VI-PH  goToStandbyVI  // calls Model::resetErrorCodeVI then Model::stopVI then
    // Model::changeStateVI(StandbyState)

private:
    FaultState() : State("FaultState") {}
};

/// Class representation of the IdleState, aka ReadyIdle.
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

/// Class representation of the InMotionState, aka ReadyInMotion.
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

/// Class representation of the OfflineState, which leads to program termination.
class OfflineState : public State {
public:
    using Ptr = std::shared_ptr<OfflineState>;

    /// Create an instance and insert it into `stateMap`.
    /// @throws Bug if there's already an instance of this class in `stateMap`.
    static Ptr create(StateMap& stateMap);

    OfflineState(OfflineState const&) = delete;
    OfflineState& operator=(OfflineState const&) = delete;
    virtual ~OfflineState() = default;

    // nothing here in LabView
private:
    OfflineState() : State("OfflineState") {}
};

/// Class representation of the PauseState, aka ReadyPause.
class PauseState : public State {
public:
    using Ptr = std::shared_ptr<PauseState>;

    /// Create an instance and insert it into `stateMap`.
    /// @throws Bug if there's already an instance of this class in `stateMap`.
    static Ptr create(StateMap& stateMap);

    PauseState(PauseState const&) = delete;
    PauseState& operator=(PauseState const&) = delete;
    virtual ~PauseState() = default;

    /// VI-PH  goToIdleReadyVI // calls Model::changeStateVI(ReadyIdle)
    void goToIdleReadyVI() override;

    /// VI-PH  goToInMotionVI // calls Model::changeStateVI(ReadyInMotion)
    void goToInMotionVI() override;

    // VI-PH  resumeVI // calls Model::resumeMotionVI
    // VI-PH  shutdownMotionEngineVI // calls Model::shutdownMotionEngineVI
    // VI-PH  stopMotionVI // calls Model::stopMotionVI
private:
    PauseState() : State("PauseState") {}
};

class StandbyState : public State {
public:
    using Ptr = std::shared_ptr<StandbyState>;

    /// Create an instance and insert it into `stateMap`.
    /// @throws Bug if there's already an instance of this class in `stateMap`.
    static Ptr create(StateMap& stateMap);

    StandbyState(StandbyState const&) = delete;
    StandbyState& operator=(StandbyState const&) = delete;
    virtual ~StandbyState() = default;

    /// VI-PH  exitVI // calls Model::changeStateVI(OfflineState)
    void exitVI();

    /// VI-PH  startVI // calls Model::startVI  then  Model::stopMotionVI  then
    /// Model::changeStateVI(ReadyIdle)
    void startVI();

private:
    StandbyState() : State("StandbyState") {}
};

}  // namespace control
}  // namespace m2cellcpp
}  // namespace LSST

#endif  // LSST_M2CELLCPP_CONTROL_STATE
