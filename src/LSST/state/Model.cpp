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
#include "../state/Model.h"

#include "control/MotionEngine.h"
#include "control/PowerSystem.h"
#include "state/State.h"
#include "util/Log.h"

using namespace std;

namespace LSST {
namespace m2cellcpp {
namespace state {

Model::Model() {
    LDEBUG("Model::Model() creating PowerSystem");
    _powerSystem = control::PowerSystem::Ptr(new control::PowerSystem());
    _motionEngine = control::MotionEngine::getPtr();
    _fpgaCtrl = control::FpgaIo::getPtr();
}

state::State::Ptr Model::getCurrentState() {
    std::lock_guard<util::VMutex> lockg(_mtx);
    return _stateMap.getCurrentState();
}

std::shared_ptr<state::State> Model::getState(State::StateEnum const& stateId) {
    std::lock_guard<util::VMutex> lockg(_mtx);
    return _stateMap.getState(stateId);
}

bool Model::changeState(std::shared_ptr<state::State> const& newState) {
    std::lock_guard<util::VMutex> lockg(_mtx);
    return _stateMap.changeState(newState);
}

bool Model::goToSafeMode(std::string const& note) {
    VMUTEX_NOT_HELD(_mtx);
    std::lock_guard<util::VMutex> lockg(_mtx);
    return _goToSafeMode(note);
}

bool Model::_goToSafeMode(std::string const& note) {
    VMUTEX_HELD(_mtx);
    return _stateMap.goToASafeState(State::STANDBYSTATE, note);
}

bool Model::_turnOffAll(string const& note) {
    VMUTEX_HELD(_mtx);
    if (_powerSystem == nullptr) {
        LERROR("Model::", __func__, " _powerSystem is nullptr");
        return false;
    }
    // TODO: DM-41758 Add code to stop motion.

    // Turning motor power off
    _powerSystem->getMotor().setPowerOff(note);
    // Turning comm power off
    _powerSystem->getComm().setPowerOff(note);
    return true;
}

bool Model::_setPower(bool on) {
    VMUTEX_HELD(_mtx);
    if (_powerSystem == nullptr) {
        LERROR("Model::", __func__, " _powerSystem is nullptr");
        return false;
    }

    if (on) {
        _powerSystem->getMotor().setPowerOn();
        _powerSystem->getComm().setPowerOn();
    } else {
        _powerSystem->getMotor().setPowerOff(string(__func__));
        _powerSystem->getComm().setPowerOff(string(__func__));
    }
    return true;
}

void Model::ctrlSetup() {
    std::lock_guard<util::VMutex> lockg(_mtx);
    //  FPGA setup - Model->setupHW.vi
    //  Model create - Model->createCellComm_RT_FIFO.vi - probably not using FIFOa, but create the model
    // TODO: DM-40694 - get these values from configuration files.
    //
    //  This section has to with configuring the MotionCtrl(motion control loop objects) FpgaCtrl(FPGA setup,
    //  read/write, simu)
    // CellCtrlComm Controller->InitializeFunctionalGlobals.vi - The individual items should be done in a
    // separate class/function CellCtrlComm    getCellGeomFromFile.vi - Opens the cellGeom.json file and uses
    // it to set configuration file values. CellCtrlComm     - critical read values: CellCtrlComm        -
    // locAct_axial (2d array 64 bit double array in m) CellCtrlComm        - locAct_tangent (64 bit double
    // angle, file in deg, internal in rad) CellCtrlComm        - radiusActTangent (64 bit double in m, (demo
    // value of 1.780189734 which can probably be ignored) CellCtrlComm     - CellConfiguration->Write Axial
    // Actuator Locations(X, Y[m]).vi  - c++ probably just store in cell configuration data structure
    // CellCtrlComm     - CellConfiguration->Write Tangent Angular Locations.vi  - c++ probably just store in
    // cell configuration data structure CellCtrlComm     - CellConfiguration->Write Tangent Actuator
    // Radius.vi  - c++ probably just store in cell configuration data structure CellCtrlComm     -
    // CellConfiguration->Write numOfAxialAct.vi  - c++ probably just store in cell configuration data
    // structure CellCtrlComm     - CellConfiguration->Write numOfTangentAct.vi  - c++ probably just store in
    // cell configuration data structure CellCtrlComm     - CellConfiguration->InitializeWithDefaultData.vi  -
    // take the data from CellConfiguration and send to CellActuatorData (which stores it, but where is it
    // accessed ??? ) CellCtrlComm   - Control Loop Force Limits FG: - called with init parameter CellCtrlComm
    // - Sets the open and closed loop force limits (ask Te-Wei or Patricio about correct values) CellCtrlComm
    // - Max Closed-Loop Axial Force - default 100 lbf CellCtrlComm        - Max Closed-Loop Tangent Force -
    // default 1100 lbf CellCtrlComm        - Max Open-Loop Axial Force (default) - 110 lbf CellCtrlComm - Max
    // Open-Loop Tangent Force (default) - 1350 lbf   *** front pane of the
    // DefaultMaxTangentOpen-LoopForceLimit.vi is mislabled as closed-loop CellCtrlComm        - Max Open Loop
    // Force Limits CellCtrlComm           - Axial - 140 lbf CellCtrlComm           - Tangent - 1400 lbf
    // CellCtrlComm   - Step Speed Limit FG - init   - StepVelocityLimitsFG.vi
    // CellCtrlComm     - Axial - 40  (units ???)
    // CellCtrlComm     - Tangent - 127  (units ???)
    // CellCtrlComm   - CL Control Params FG - init - ControlLoopControlParametersFG.vi ** several config
    // values CellCtrlComm      * Stale data max count taken from json file item - ilcStaleDataLimit
    // CellCtrlComm      - Telemetry Control Parameters.Actuator Encoder Range
    // CellCtrlComm         - Axial ILC Min Encoder Value = -16000000
    // CellCtrlComm         - Axial ILC Max Encoder Value =  16000000
    // CellCtrlComm         - Tangent ILC Min Encoder Value = -16000000
    // CellCtrlComm         - Tangent ILC Max Encoder Value =  16000000
    // CellCtrlComm      - Telemetry Control Parameters.Inclinometer Sensor Range
    // CellCtrlComm         - Min Inclinometer Value = -0.001745329251994 rad
    // CellCtrlComm         - Max Inclinometer Value =  6.284930636432 rad
    // CellCtrlComm      - Telemetry Control Parameters.Temperature Sensor Range
    // CellCtrlComm         - Min Cell Temperature = 233.15 Cdeg
    // CellCtrlComm         - Max Cell Temperature = 318.15 Cdeg
    // CellCtrlComm         - Min Mirror Temperature = 233.15 Cdeg
    // CellCtrlComm         - Max Mirror Temperature = 313.15 Cdeg
    // CellCtrlComm      - Telemetry Control Parameters.Displacement Sensor Range
    // CellCtrlComm         - min = -0.0001  m
    // CellCtrlComm         - max =  0.04 m
    // CellCtrlComm   - Calib Data FG:CalibrationDataFG.vi
    // CellCtrlComm     - Motor Voltage, Comm Voltage, Com Current - all have the same values
    // CellCtrlComm        - Gain = 1.0
    // CellCtrlComm        - Offset = 0.0
    // CellCtrlComm     - Motor Current has
    // CellCtrlComm        - Gain = 5.0
    // CellCtrlComm        - offset = 0.0
    // PowerSystem   - System Config FG:SystemConfigurationFG.vi  ****** power up configuration values
    // ******** PowerSystem     There are a lot of configuration values in here having to do with power supply
    // very specific, PowerSystem     probably best to open up the vi and look at the set values. Elements set
    // listed below PowerSystem     - "Power Subsystem Configuration Parameters.Power Subsystem Common
    // Configuration Parameters" PowerSystem     - "Power Subsystem Configuration Parameters.Comm Power Bus
    // Configuration Parameters" PowerSystem     - "Power Subsystem Configuration Parameters.Motor Power Bus
    // Configuration Parameters" PowerSystem     - "Power Subsystem Configuration Parameters.Boost Current
    // Fault Enabled" - this is set to false. CellCtrlComm - Ctrlr.U/D F Limit:updateForceLimits.vi - Pulls
    // limitForce values for several items from a json file (??? which file) CellCtrlComm   -
    // limitForce_closedLoopAxial CellCtrlComm   - limitForce_closedLoopTangent CellCtrlComm   -
    // limitForce_openLoopAxial CellCtrlComm   - limitForce_openLoopTangent CellCtrlComm   -
    // limitForceMax_openLoopAxial CellCtrlComm   - limitForceMax_openLoopTangent

    _setupFinished = true;
}

void Model::ctrlStart() {
    std::lock_guard<util::VMutex> lockg(_mtx);

    // Start MotionEngine threads
    auto mEngine = _motionEngine.lock();
    if (mEngine == nullptr) {
        throw util::Bug(ERR_LOC, "Model _motionEngine is nullptr");
    }

    mEngine->engineStart();

    // TODO: Add the following to this and `waitForCtrlReady()` and `joinCtrl()`.
    // - _scriptEngine - Controller->startScriptEngine.vi - start the script engine - FUTURE-FAR
    // - _cellCtrlComm - Controller->startupCellCommunications
    // - Do not need to start - _faultMgr[no threads], _fpgaCtrl[started in constructor],
}

void Model::waitForCtrlReady() const {
    auto mEngine = _motionEngine.lock();
    if (mEngine == nullptr) {
        throw util::Bug(ERR_LOC, "Model _motionEngine is nullptr");
    }

    mEngine->waitForEngine();
}

void Model::ctrlStop() {
    auto mEngine = _motionEngine.lock();
    if (mEngine == nullptr) {
        throw util::Bug(ERR_LOC, "Model _motionEngine is nullptr");
    }

    mEngine->engineStop();
}

void Model::ctrlJoin() {
    auto mEngine = _motionEngine.lock();
    if (mEngine == nullptr) {
        throw util::Bug(ERR_LOC, "Model _motionEngine is nullptr");
    }

    mEngine->engineJoin();
}

}  // namespace state
}  // namespace m2cellcpp
}  // namespace LSST
