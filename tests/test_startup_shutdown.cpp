/*
 * This file is part of LSST ts_m2cellcpp test suite.
 *
 * Developed for the Vera C. Rubin Observatory Telescope & Site Software Systems.
 * This product includes software developed by the Vera C.Rubin Observatory Project
 * (https://www.lsst.org). See the COPYRIGHT file at the top-level directory of
 * this distribution for details of code ownership.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation, either version 3 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program. If not, see <https://www.gnu.org/licenses/>.
 */

#define CATCH_CONFIG_MAIN

// System headers
#include <exception>

// 3rd party headers
#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_session.hpp>

// Project headers
#include "control/Context.h"
#include "control/ControlMain.h"
#include "control/FpgaIo.h"
#include "control/MotionEngine.h"
#include "simulator/SimCore.h"
#include "system/Config.h"
#include "util/Bug.h"
#include "util/Log.h"

using namespace std;
using namespace LSST::m2cellcpp::control;

TEST_CASE("Test startup shutdown", "[CSV]") {
    LSST::m2cellcpp::util::Log::getLog().useEnvironmentLogLvl();
    string cfgPath = LSST::m2cellcpp::system::Config::getEnvironmentCfgPath("../configs");

    LSST::m2cellcpp::simulator::SimCore::Ptr simCore(new LSST::m2cellcpp::simulator::SimCore());
    FpgaIo::setup(simCore);
    MotionEngine::setup();
    Context::setup();

    Context::Ptr context = Context::get();
    REQUIRE(context != nullptr);
    context->model.ctrlSetup();

    ControlMain::setup();
    ControlMain::Ptr control = ControlMain::getPtr();
    control->startThrd();

    // &&& test thread is running

    REQUIRE(context->model.getCurrentState() == context->model.getState("StartupState"));

    auto newState = context->model.getState("IdleState");
    REQUIRE(context->model.changeState(newState));
    REQUIRE(context->model.getCurrentState() == context->model.getState("IdleState"));





}

/* &&&
 *
M2CellCtrlrTopVI.vi - Creates the "User Events" just waits for "User Event: State" == "OfflineState" at which point it starts shutdown by
  stopping its loop and then sending User Event  "General Commands" = "shutdown"
  It also contains the static instance of Controller->controllerMain.vi, which is what we really care about when it comes to startup.


States to look for
StandbyState - ** stop motion when entering StandbyState by calling Model->stopMotion.vi *** -
   - set by
       Controller->controllerMain.vi
       FaultState->clearError.vi
       FaultState->goToStandby.vi
       IdleState->GoToStandby.vi
   - read by - this really needs to search on "cRIO State" with value "standby"  and then "MotionEngineState" set to "idle"
      - Controller->controllerMain.vi
      - NetworkINterface.vi

FaultState -
   - set by - only seems to be set in the NetworkINterface.vi
   -read by - this really needs to search on "cRIO State" with value "fault"  and then "MotionEngineState" set to "idle"

ReadyIdle -
   - set by
      - Controller->controllerMain.vi -- system starting state
      - PauseState->goToReadyIdle.vi
      - StandbyState->start.vi
   -read by - this really needs to search on "cRIO State" with value "ready"  and then "MotionEngineState" set to "idle"
      - Controller->controllerMain.vi
      - NetworkINterface.vi

ReadyInMotion -
   - set by
       - Controller->controllerMain.vi
       - IdleSate->goToInMotion.vi
       - PauseState->goToInMotion.vi
       -
   -read by - this really needs to search on "cRIO State" with value "ready"  and then "MotionEngineState" set to "moving"
      - Controller->controllerMain.vi
      - NetworkINterface.vi

ReadyPause -
   - set by
       - Controller->controllerMain.vi
       - InMotionState->goToPause.vi
   -read by - this really needs to search on "cRIO State" with value "ready"  and then "MotionEngineState" set to "paused"
       - Controller->controllerMain.vi
       - NetworkINterface.vi

OfflineState - This usually kills the thread loops in LabView
   - set by
       - Controller->controllerMain.vi
       - NetworkInterface.vi
       - StandbyState->exit.vi
   - read by
       - M2CellCtrlrTopVI.vi - stops loop
       - Controller->controllerMain.vi - stops loop
       - NetworkINterface.vi

Model->startMotion.vi - stuff



Command.lvclass:exec.vi
Support_VIs->Commands:
 - CLC_Command
 - EnableFaults
 - GotoIdleReadyState
 - GotoInMotionState
 - Power
 - Reboot
 - ResetFaults
 - Restart
 - StartActuatorMotion
 - StartMotion
 - StopScript




/// This is the function that is run by the `_mCtrlThread`.
void MotionEngine::_mCtrlAction() {
    LINFO("MotionEngine::_mCtrlAction() running");
    // &&&x This will probably all be in a single separate MotionCtrl thread.
    // &&&x - Ctrlr.start Processor:Controler->startupProcesses.vi ** Important startup **
    // &&&x done   - Controller->startupFaultManager.vi - starts the fault manager in a separate thread - async call and other things that I don't quite understand ???
    // &&&***  - Controller->startupMotionEngine.vi - starts the motion engine (see MotionEngine->motionEngineMain.vi) - again, don't quite understand ???
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
    // &&&x nope  - Controller->startupTDMS_Logging.vi - logging handled elsewhere
    // &&&x done  - Controller->startupDAQ_Process.vi - startup DAQ - this can't be accessed until much later FUTURE-MODERATE
    // &&&x done  - Controller->startupPowerControlSubsystem.vi - Trying to start with a version of this with real loops and fake power outputs
    // &&&x done  - PowerSubsystem->power_subsystem_main.vi
    // &&&x done  - Controller->startupNetworkInterface.vi
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
&&& */
