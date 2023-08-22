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
#include "control/Model.h"

// System headers

// Project headers
#include "control/PowerSystem.h"
#include "control/State.h"
///&&&<<<<<<< HEAD
#include "util/Log.h"
///&&&=======
#include "control/FpgaCtrl.h"
#include "control/MotionCtrl.h"
///&&&>>>>>>> Added class framework.

namespace LSST {
namespace m2cellcpp {
namespace control {

Model::Model() {
    ///&&&<<<<<<< HEAD
    LDEBUG("Model::Model() creating PowerSystem");
    _powerSystem = PowerSystem::Ptr(new PowerSystem());
    ///&&&=======
    _motionCtrl = MotionCtrl::Ptr(new MotionCtrl());
    _fpgaCtrl = FpgaIo::Ptr(new FpgaIo());
    ///&&&>>>>>>> Added class framework.
}

State::Ptr Model::getCurrentState() { return _stateMap.getCurrentState(); }

std::shared_ptr<State> Model::getState(std::string const& stateName) { return _stateMap.getState(stateName); }

bool Model::changeState(std::shared_ptr<State> const& newState) { return _stateMap.changeState(newState); }

///&&&<<<<<<< HEAD
bool Model::goToSafeMode(std::string const& note) {
    if (_powerSystem == nullptr) {
        LERROR("Model::goToSafeMode _powerSystem is nullptr");
        return false;
    }
    // TODO: DM-40339 Add code to put the system in safe mode,
    //       If the system is already in safe mode, return false
    LCRITICAL("Model::goToSafeMode needs code note=", note);
    // Put the system in safe mode.
    // This includes setting "Closed loop control" (aka "CLC Mode") to Idle
    // Turning motor power off
    _powerSystem->getMotor().setPowerOff("safe mode");
    // Turning comm power off
    _powerSystem->getComm().setPowerOff("safe mode");
    return true;
    ///&&&=======
void Model::ctrlSetup() {
    &&&; // &&& which parts can be implemented now???
    // &&& FPGA setup - Model->setupHW.vi

    // &&& Model create - Model->createCellComm_RT_FIFO.vi - probably not using FIFOa, but create the model

    // &&& This section has to with configuring the MotionCtrl(motion control loop objects) FpgaCtrl(FPGA setup, read/write, simu)
    // &&& Controller->InitializeFunctionalGlobals.vi - The individual items should be done in a separate class/function
    // &&&    getCellGeomFromFile.vi - Opens the cellGeom.json file and uses it to set configuration file values.
    // &&&     - critical read values:
    // &&&        - locAct_axial (2d array 64 bit double array in m)
    // &&&        - locAct_tangent (64 bit double angle, file in deg, internal in rad)
    // &&&        - radiusActTangent (64 bit double in m, (demo value of 1.780189734 which can probably be ignored)
    // &&&     - CellConfiguration->Write Axial Actuator Locations(X, Y[m]).vi  - c++ probably just store in cell configuration data structure
    // &&&     - CellConfiguration->Write Tangent Angular Locations.vi  - c++ probably just store in cell configuration data structure
    // &&&     - CellConfiguration->Write Tangent Actuator Radius.vi  - c++ probably just store in cell configuration data structure
    // &&&     - CellConfiguration->Write numOfAxialAct.vi  - c++ probably just store in cell configuration data structure
    // &&&     - CellConfiguration->Write numOfTangentAct.vi  - c++ probably just store in cell configuration data structure
    // &&&     - CellConfiguration->InitializeWithDefaultData.vi  - take the data from CellConfiguration and send to CellActuatorData (which stores it, but where is it accessed ??? )
    // &&&   - Control Loop Force Limits FG: - called with init parameter
    // &&&     - Sets the open and closed loop force limits (ask Te-Wei or Patricio about correct values)
    // &&&        - Max Closed-Loop Axial Force - default 100 lbf
    // &&&        - Max Closed-Loop Tangent Force - default 1100 lbf
    // &&&        - Max Open-Loop Axial Force (default) - 110 lbf
    // &&&        - Max Open-Loop Tangent Force (default) - 1350 lbf   *** front pane of the DefaultMaxTangentOpen-LoopForceLimit.vi is mislabled as closed-loop
    // &&&        - Max Open Loop Force Limits
    // &&&           - Axial - 140 lbf
    // &&&           - Tangent - 1400 lbf
    // &&&   - Step Speed Limit FG - init   - StepVelocityLimitsFG.vi
    // &&&     - Axial - 40  (units ???)
    // &&&     - Tangent - 127  (units ???)
    // &&&   - CL Control Params FG - init - ControlLoopControlParametersFG.vi ** several config values
    // &&&      * Stale data max count taken from json file item - ilcStaleDataLimit
    // &&&      - Telemetry Control Parameters.Actuator Encoder Range
    // &&&         - Axial ILC Min Encoder Value = -16000000
    // &&&         - Axial ILC Max Encoder Value =  16000000
    // &&&         - Tangent ILC Min Encoder Value = -16000000
    // &&&         - Tangent ILC Max Encoder Value =  16000000
    // &&&      - Telemetry Control Parameters.Inclinometer Sensor Range
    // &&&         - Min Inclinometer Value = -0.001745329251994 rad
    // &&&         - Max Inclinometer Value =  6.284930636432 rad
    // &&&      - Telemetry Control Parameters.Temperature Sensor Range
    // &&&         - Min Cell Temperature = 233.15 Cdeg
    // &&&         - Max Cell Temperature = 318.15 Cdeg
    // &&&         - Min Mirror Temperature = 233.15 Cdeg
    // &&&         - Max Mirror Temperature = 313.15 Cdeg
    // &&&      - Telemetry Control Parameters.Displacement Sensor Range
    // &&&         - min = -0.0001  m
    // &&&         - max =  0.04 m
    // &&&   - Calib Data FG:CalibrationDataFG.vi
    // &&&     - Motor Voltage, Comm Voltage, Com Current - all have the same values
    // &&&        - Gain = 1.0
    // &&&        - Offset = 0.0
    // &&&     - Motor Current has
    // &&&        - Gain = 5.0
    // &&&        - offset = 0.0
    // &&&   - System Config FG:SystemConfigurationFG.vi  ****** power up configuration values ********
    // &&&     There are a lot of configuration values in here having to do with power supply very specific,
    // &&&     probably best to open up the vi and look at the set values. Elements set listed below
    // &&&     - "Power Subsystem Configuration Parameters.Power Subsystem Common Configuration Parameters"
    // &&&     - "Power Subsystem Configuration Parameters.Comm Power Bus Configuration Parameters"
    // &&&     - "Power Subsystem Configuration Parameters.Motor Power Bus Configuration Parameters"
    // &&&     - "Power Subsystem Configuration Parameters.Boost Current Fault Enabled" - this is set to false.
    // &&& - Ctrlr.U/D F Limit:updateForceLimits.vi - Pulls limitForce values for several items from a json file (??? which file)
    // &&&   - limitForce_closedLoopAxial
    // &&&   - limitForce_closedLoopTangent
    // &&&   - limitForce_openLoopAxial
    // &&&   - limitForce_openLoopTangent
    // &&&   - limitForceMax_openLoopAxial
    // &&&   - limitForceMax_openLoopTangent

}


void Model::ctrlStart() {
    // &&&I   - Controller->startupFaultManager.vi - starts the fault manager in a separate thread
    _faultMgr.ctrlStart();
    _faultMgr.waitForRunning();

    _motionCtrl.ctrlStart();
    _motionCtrl.waitForRunning();

    // &&&I   - Controller->startupMotionEngine.vi - starts the motion engine (see MotionEngine->motionEngineMain.vi)
    _motionEngine.ctrlStart();
    _motionEngine.waitForRunning();

    _fpgaCtrl.ctrlStart();
    _fpgaCtrl.waitForRunning();

    // &&&I   - Controller->startScriptEngine.vi - start the script engine - putting this on the back burner FUTURE-FAR
    _scriptEngine.ctrlStart();
    _scriptEngine.waitForRunning();

    // &&&I   - Controller->startupCellCommunications - Sets several initial states, listed below.
    _cellCtrlComm.ctrlStart();
    _cellCtrlComm.waitForRunning();
    ///&&&>>>>>>> Added class framework.
}

}  // namespace control
}  // namespace m2cellcpp
}  // namespace LSST
