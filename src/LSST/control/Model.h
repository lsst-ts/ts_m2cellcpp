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
#include "control/State.h"
#include "control/StateMap.h"

#ifndef LSST_M2CELLCPP_CONTROL_MODEL_H
#define LSST_M2CELLCPP_CONTROL_MODEL_H

namespace LSST {
namespace m2cellcpp {

namespace control {

class FpgaIo;
class PowerSystem;
class State;

/// Most of the higher level systems information is contained in this class.
/// This class represents the RT CompactRIO Target/Support_VIs/Model.
/// unit test: test_Context.cpp
class Model {
public:
    Model();
    Model(Model const&) = delete;
    Model& operator=(Model const&) = delete;
    virtual ~Model() = default;

    /// Return a pointer to the FpgaIo instance.
    std::shared_ptr<FpgaIo> getFpgaIo() { return _fpgaIo; }

    /// Change the current state to `newState`, taking rquired actions.
    /// @return false in `newState` is invalid.
    bool changeState(std::shared_ptr<State> const& newState);

    /// Get a pointer to the state with `stateName`.
    /// @return nullptr if the state is not found.
    std::shared_ptr<State> getState(std::string const& stateName);

    /// Get a pointer to the current State.
    /// VI-PH retrieveStateVI  // calls breaks out State and currentStateEnum and returns
    std::shared_ptr<State> getCurrentState();

    /// Go to safe mode.
    /// @param note - Description of reason for safe mode.
    /// @return true if the system was not already trying to reach.
    bool goToSafeMode(std::string const& note);

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

protected:
    // VI-PH startupTDMSLoggingVI
    // VI-PH stopTDMSLoggingVI
    // VI-PH stopCellCommunicationsVI

private:
    /// `_stateMap` contains all possible system states and the current system state.
    /// This memeber serves a similar purpose to the LabView `_commandFactory` and `_state`.
    StateMap _stateMap;

    // _cellCommRT_FIFO // systemElement VI-PH
    // _deltaForceRT_FIFO // systemElement VI-PH
    std::shared_ptr<FpgaIo> _fpgaIo; ///< The FpgaIo instance for communicating with hardware.

    // The LabView implementation of Model has the folowing systemElements that need to
    // acounted for in the C++ version.
    // applicationElements seems to be a container for all of these. it can probably be skipped by using the
    // appropriate Accessor VI
    // _userEvents // systemElement VI-PH
    // _systemController // systemElement VI-PH

    // _cellCommRef // systemElement VI-PH
    // _powerStatus // systemElement  _commPowerOn, _motorPowerOn VI-PH
    std::shared_ptr<PowerSystem> _powerSystem; ///< The PowerSystem control instance, contains power states.
};

}  // namespace control
}  // namespace m2cellcpp
}  // namespace LSST

#endif  // LSST_M2CELLCPP_CONTROL_MODEL_H
