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

// System headers
#include <memory>

// Project headers
#include "control/control_defs.h"
#include "state/State.h"
#include "state/StateMap.h"
#include "util/VMutex.h"

#ifndef LSST_M2CELLCPP_STATE_MODEL_H
#define LSST_M2CELLCPP_STATE_MODEL_H

namespace LSST {
namespace m2cellcpp {

namespace control {
class FpgaIo;
class MotionEngine;
class PowerSystem;
}  // namespace control

namespace state {

/// Most of the higher level systems information is contained in this class.
/// This class represents the RT CompactRIO Target/Support_VIs/Model.
/// `Model`, combined with `StateMap`, `State`, and the child classes of `State`,
/// controls access to what modes and actions are available, as well as moving from
/// one state to the next.
/// unit test: test_Context.cpp
class Model {
public:
    Model();
    Model(Model const&) = delete;
    Model& operator=(Model const&) = delete;
    virtual ~Model() = default;

    /// Return a pointer to the `_powerSystem`.
    std::shared_ptr<control::PowerSystem> getPowerSystem() { return _powerSystem; }

    /// Shutdown the entire system.
    /// This function will first turn off power and then attempt
    /// to shutdown the rest of the system in an orderly manner.
    void systemShutdown();

    /// Read configuration files that haven't yet been read and use those
    /// values to setup various aspects of the system.
    void ctrlSetup();

    /// Return true if sytem setup has finished.
    bool isSetupFinished() const { return _setupFinished; }

    /// Start MotionEngine and other threads that aren't started in their constructors.
    /// These are generally objects that require the essential services to  already
    /// be running.
    void ctrlStart();

    /// Wait for threads started in `ctrlStart()` to be ready.
    void waitForCtrlReady() const;

    /// Stop threads started in `ctrlStart()`.
    void ctrlStop();

    /// Join threads started in `ctrlStart()`.
    void ctrlJoin();

    /// Change the current state to `newState`, taking required actions.
    /// @return false in `newState` is invalid.
    bool changeState(std::shared_ptr<state::State> const& newState);

    /// Get a pointer to the `State` with `stateId`.
    /// @return nullptr if the `State` is not found.
    std::shared_ptr<state::State> getState(State::StateEnum const& stateId);

    /// Get a pointer to the current State.
    /// VI-PH retrieveStateVI  // calls breaks out State and currentStateEnum and returns
    std::shared_ptr<state::State> getCurrentState();

    /// Go to safe mode, which includes turning off MOTOR and COMM power.
    /// @param note - Description of reason for safe mode.
    /// @return true if the system was not already trying to reach.
    bool goToSafeMode(std::string const& note);

    /// Return a pointer to the power system.
    std::shared_ptr<control::PowerSystem> getPowerSystem() const;

    /// This function allows PowerSubsytem instances to inform the model that their stat has changed.
    /// The changes in the PowerSystem can result in changes being made to the overal system state in
    /// `state::Model`.
    /// @param systemType - the type of the power subsytem reporting a state change.
    /// @param `targePowerState` - the new target state for the subsystem (should be ON of OFF).
    /// @param `actualPowerState` - the current state for the subsystem.
    void reportPowerSystemStateChange(control::PowerSystemType systemType, control::PowerState targPowerState,
                                      control::PowerState actualPowerState);

    /// Accessors
    // VI-PH readApplicationElementsVI  // can probably skip this one
    // VI-PH writeApplicationElementsVI // can probably skip this one

    // VI-PH readCellCommRefVI
    // VI-PH writeCellCommRefVI

    // VI-PH readCommandFactoryVI
    // VI-PH writeCommandFactoryVI

    // VI-PH readLoggingFIFOsVI

    // VI-PH readStateVI
    // VI-PH writeStateVI

    // VI-PH readSystemControllerVI

    // VI-PH readUserEventsVI
    // VI-PH writeUserEventsVI

    // VI-PH writeStateFactoryVI

    /// Public VIs
    // VI-PH changeStateVI
    // VI-PH checkSystemExceptionVI
    // VI-PH checkTestCompleteVI
    // VI-PH closeLogVI
    // VI-PH closeSysLogVI
    // VI-PH createCellCommRT_FIFOVI
    // VI-PH createDeltaCommRT_FIFOVI
    // VI-PH creatLogginFIFOsVI
    // VI-PH createNewSyslogVI
    // VI-PH destroyCellCommRT_FIFOVI
    // VI-PH destroyDeltaCommRT_FIFOVI
    // VI-PH destroyLoggingFIFOsVI
    // VI-PH getFpgaReferenceVI
    // VI-PH getCellCommRT_FIFOVI
    // VI-PH getDeltaCommRT_FIFOVI
    // VI-PH getPowerStatusVI
    // VI-PH initMotionEngineVI
    // VI-PH initScriptEngineVI
    // VI-PH loadScriptVI
    // VI-PH logMessageVI
    // VI-PH logPowerStatusVI
    // VI-PH pauseVI
    // VI-PH pauseScriptVI
    // VI-PH publishStateVI
    // VI-PH readClosedLoopControlEnablesVI
    // VI-PH reinitializeControllerVI
    // VI-PH resetErrorCodeVI
    // VI-PH resumeVI
    // VI-PH resumeScriptVI
    // VI-PH sendCellCommCommandVI
    // VI-PH sendGeneralCommandVI
    // VI-PH setPowerVI
    // VI-PH setPowerStatusVI
    // VI-PH setupHWVI
    // VI-PH shutdownCellCommVI
    // VI-PH shutdownHWVI
    // VI-PH shutdownMotionEngineVI
    // VI-PH shutdownNetworkInterfaceVI
    // VI-PH shutdownScriptEngineVI
    // VI-PH startVI
    // VI-PH startMotionVI
    // VI-PH startScriptVI
    // VI-PH staryupCellCommunicationsVI
    // VI-PH stopVI
    // VI-PH stopMotionVI
    // VI-PH stopScriptVI
    // VI-PH updateErrorCodeVI
    // VI-PH writeClosedLoopContolEnablesVI

    // This is not ideal, but the various State children need
    // access to `Model` members but the mutex `_mtx` has already
    // been locked. For the sake of consistency, and to prevent
    // accidental calls to functions where `_mtx` must be locked
    // before calling, all of the Model members that `State` needs
    // to call are private.
    friend class state::State;
    friend class state::IdleState;

protected:
    // VI-PH startupTDMSLoggingVI
    // VI-PH stopTDMSLoggingVI
    // VI-PH stopCellCommunicationsVI

private:
    /// Go to safe mode, which includes turning off MOTOR and COMM power.
    /// @param note - Description of reason for safe mode.
    /// @return true if the system was not already trying to reach.
    bool _goToSafeMode(std::string const& note);

    /// Stop motion and turn off all power that can be turned off.
    bool _turnOffAll(std::string const& note);

    /// `_stateMap` contains all possible system states and the current system state.
    /// This member serves a similar purpose to the LabView `_commandFactory` and `_state`.
    StateMap _stateMap{this};

    // _cellCommRT_FIFO // systemElement VI-PH
    // _deltaForceRT_FIFO // systemElement VI-PH
    std::weak_ptr<control::FpgaIo> _fpgaIo;  ///< The FpgaIo instance for communicating with hardware.

    std::weak_ptr<control::MotionEngine> _motionEngine;  ///< The MotionEngine instance.

    // The LabView implementation of Model has the folowing systemElements that need to
    // accounted for in the C++ version.
    // applicationElements seems to be a container for all of these. it can probably be skipped by using the
    // appropriate Accessor VI
    // _userEvents // systemElement VI-PH
    // _systemController // systemElement VI-PH

    // _cellCommRef // systemElement VI-PH
    // _powerStatus // systemElement  _commPowerOn, _motorPowerOn VI-PH

    /// The PowerSystem control instance, contains power states.
    std::shared_ptr<control::PowerSystem> _powerSystem;

    std::shared_ptr<control::FpgaIo> _fpgaCtrl;  ///< pointer to the global instance of FpgaIo.

    util::VMutex _mtx;  ///< Protects all members.

    std::atomic<bool> _setupFinished{false};  ///< Set to true when system setup is finished.
};

}  // namespace state
}  // namespace m2cellcpp
}  // namespace LSST

#endif  // LSST_M2CELLCPP_STATE_MODEL_H
