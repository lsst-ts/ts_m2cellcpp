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
#include "control/MotionCtrl.h"

// System headers

// Project headers
#include "util/Log.h"

using namespace std;

namespace LSST {
namespace m2cellcpp {
namespace control {

/// &&& doc
void MotionCtrl::mCtrlStart() {
    LINFO("MotionCtrl::mCtrlStart() running thread");
    thread thrd(&MotionCtrl::_mCtrlAction, this);
    _mCtrlThread = std::move(thrd);
}



/// This is the function that is run by the `_mCtrlThread`.
void MotionCtrl::_mCtrlAction() {
    LINFO("MotionCtrl::_mCtrlAction() running");
    // &&&x This will probably all be in a single separate MotionCtrl thread.
    // &&&x - Ctrlr.start Processor:Controler->startupProcesses.vi ** Important startup **
    // &&&O   - Controller->startupFaultManager.vi - starts the fault manager in a separate thread - async call and other things that I don't quite understand ???
    // &&&O   - Controller->startupMotionEngine.vi - starts the motion engine (see MotionEngine->motionEngineMain.vi) - again, don't quite understand ???
    // &&&FUTURE: not sure what will need to happen with this     -Model->initMotionEngine.vi - sends the init-MotionEngineCmd User Event to the motion engine.
    // &&&O   - Controller->startScriptEngine.vi - start the script engine - putting this on the back burner FUTURE-FAR
    // &&&O   - Controller->startupCellCommunications - Sets several initial states, listed below.
    // &&&O     (See CellCommunications.vi, which appears to be a major control loop that seems to do a LOT more than its name would imply ???)
    // &&&O     - FeedForward = true
    // &&&O     - FeedBack = true
    // &&&O     - AxialDeadband = false
    // &&&O     - OutputEnabled = false
    // &&&O     - In Position = false
    // &&&O     - Tuning Log (off) = false
    // &&&O     - Begin Soft Start = false
    // &&&O     - Clear, Reset = true
    // &&&x   - Controller->startupTDMS_Logging.vi - logging handled elsewhere
    // &&&FUTURE   - Controller->startupDAQ_Process.vi - startup DAQ - this can't be accessed until much later FUTURE-MODERATE
    // &&&   - Controller->startupPowerControlSubsystem.vi - *** Trying to start with a version of this with real loops and fake power outputs ***
    // &&&     - *** &&& come back to this, examine PowerSubsystem->power_subsystem_main.vi &&& ***
    // &&&   - Controller->startupNetworkInterface.vi
    // &&& This is the control loop: ****
    // &&& - Enter the while loop - the while loop handles incoming User Events
    // &&&   - 0 - <Command>:User Event - note This state will stop this loop if Model.state == OfflineState
    // &&&       - ??? important things to know ???
    // &&&   - 1 - <GeneralCommand>:User Event - This will always stop this loop
    // &&&       - ??? important things to know ???
    // &&&   - 2 - <GenRead>:User Event - Handle SAL TcpIp commands - not needed in c++
    // &&&   - 3 - <GenRead>:User Event - Handle GUI TcpIp commands - c++ ComConnection class should handle these
    // &&&   - 4 - <Telemetry>:User Event - Handle telemetry events - c++ TelemetryCom class should handle these
    // &&&   - 5 - <State>:User Event - Sets system state
    // &&&       - StandbyState
    // &&&          - cRIO State:standby
    // &&&          - MotionEngineState:idle
    // &&&       - FaultState
    // &&&          - cRIO State:fault
    // &&&          - MotionEngineState:idle
    // &&&       - ReadyIdle
    // &&&          - cRIO State:ready
    // &&&          - MotionEngineState:idle
    // &&&       - ReadyInMotion
    // &&&          - cRIO State:ready
    // &&&          - MotionEngineState:moving
    // &&&       - ReadyPause
    // &&&          - cRIO State:offline
    // &&&          - MotionEngineState:paused
    // &&&       - OfflineState
    // &&&          - cRIO State:offline
    // &&&          - MotionEngineState:idle
    // &&& - Exit while loop
    // &&& Control shutdown ****
    // &&& - Model:shutdn H/W:Model->shutdownHW.vi - *** Arguably one of the most important things to have working, but can't truly implement until FPGA code exists ***
    // &&& - SystemController->uninitFPGA.vi - FPGA doesn't exist, wil try to simu the outputs.
    // &&&   - ILC_Motor_power_On - set to false
    // &&&   - ILC_Comm_power_On - set to false
    // &&&   - cRIO_Interlock_Enable - set to false
    // &&&   - Reset_Motor_Power_Breakers - set to false
    // &&&   - Reset_Comm_Power_Breakers - set to false
    // &&&   - Mod4/CH5 - set to false  - not that important, but what is this doing?
    // &&&   - Mod4/CH6 - set to false  - not that important, but what is this doing?
    // &&&   - Mod4/CH7 - set to false  - not that important, but what is this doing?
    // &&&   - Globals - I'd rather not implement these in c++ as they may disagree with actual values and breed red herrings.
    // &&&     - G_ILC_Motor_power_On(Ch0) - set to false
    // &&&     - G_ILC_Comm_power_On(Ch1) - set to false
    // &&&     - G_cRIO_Interlock_Enable(Ch2) - set to false
    // &&&     - G_Reset_Motor_Power_Breakers(Ch3) - set to false
    // &&&     - G_Reset_Comm_Power_Breakers(Ch4) - set to false

    // &&& ***** Should be in safe mode at this point, aka ReadyIdle.
}



}  // namespace control
}  // namespace m2cellcpp
}  // namespace LSST
