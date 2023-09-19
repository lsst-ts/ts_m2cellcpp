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

// Class header
#include "control/PowerSubsystem.h"

// system headers
#include <bitset>
#include <sstream>
#include <mutex>
#include <stdexcept>
#include <string>

// project headers

#include "control/FpgaIo.h"
#include "util/Bug.h"
#include "util/Log.h"

#include "faultmgr/FaultMgr.h"


using namespace std;

namespace LSST {
namespace m2cellcpp {
namespace control {

BreakerFeedGroup::BreakerFeedGroup(Feed::Ptr const& feed1, Feed::Ptr const& feed2, Feed::Ptr const& feed3)
        : _feed1(feed1), _feed2(feed2), _feed3(feed3){
    _feeds.push_back(_feed1);
    _feeds.push_back(_feed2);
    _feeds.push_back(_feed3);
}


tuple<SysStatus, string> BreakerFeedGroup::checkBreakers(SysInfo const& info) {
    SysStatus result = GOOD;
    string inactiveInputs = "";

    for (auto& feed:_feeds) {
        auto [status, str] = feed->checkBreakers(info.inputPort);
        inactiveInputs += str;
        /// Result should contain the worst SysStatus found.
        if (status > result) {
            result = status;
        }
    }

    return make_tuple(result, inactiveInputs);
}


tuple<SysStatus, string> BreakerFeedGroup::Feed::checkBreakers(InputPortBits const& input) {
    bool bit0 = input.getBitAtPos(_bit0Pos);
    bool bit1 = input.getBitAtPos(_bit1Pos);
    bool bit2 = input.getBitAtPos(_bit2Pos);

    // All 3 bits should be high
    int count = 0;
    uint8_t bitmap = 0;
    string inactiveStr = "";
    if (bit0) {
        ++count;
        bitmap |= 0b001;
    } else {
        inactiveStr += InputPortBits::getEnumString(_bit0Pos) + ",";
    }
    if (bit1) {
        ++count;
        bitmap |= 0b010;
    } else {
        inactiveStr += InputPortBits::getEnumString(_bit1Pos) + ",";
    }
    if (bit2) {
        ++count;
        bitmap |= 0b100;
    } else {
        inactiveStr += InputPortBits::getEnumString(_bit2Pos);
    }

    SysStatus breakerStatus = FAULT;
    if (count == 3) breakerStatus = GOOD;
    if (count == 2) breakerStatus = WARN;
    if (bitmap != _feedBitmap) {
        LDEBUG("BreakerStatus change to ", bitmap, " from ", _feedBitmap,
              " status=", sysStatusStr(breakerStatus), " low inputs=", inactiveStr);
    }
    _feedBitmap = bitmap;

    return make_tuple(breakerStatus, inactiveStr);
}


std::string PowerSubsystem::getPowerStateStr(PowerState powerState) {
    int val = powerState;
    string valStr = string(" ") + to_string(val);
    switch (powerState) {
    case ON: return "ON" + valStr;
    case TURNING_ON: return "TURNING_ON" + valStr;
    case TURNING_OFF: return "TURNING_OFF" + valStr;
    case OFF: return "OFF" + valStr;
    case RESET: return "RESET" + valStr;
    case UNKNOWN: return "UNKNOWN" + valStr;
    }
    return "unknown" + valStr;
};



PowerSubsystemConfig::PowerSubsystemConfig(PowerSystemType systemType) : _systemType(systemType),
        _subsystemFaultMask(faultmgr::FaultStatusBits::getMaskPowerSubsystemFaults(_systemType)) {
    switch (_systemType) {
    case MOTOR:
        _setupMotor();
        break;
    case COMM:
        _setupComm();
        break;
    default:
        throw util::Bug(ERR_LOC,"unexpected systemType=" + to_string(_systemType));
    }
    _setupCalculated();
}

void PowerSubsystemConfig::_setupMotor() {
    // - U32 output on max delay (ms) (unsigned long [32-bit integer (0 to 4,294,967,295)])  =
    //   “relay close delay” (50ms) + “breaker on time” (500ms) + “interlock output on delay”(50ms)= 600ms
    _relayCloseDelay = 0.050; ///< "relay close delay" 50ms, in seconds
    _breakerOnTime = 0.5; ///< “breaker on time” (500ms), in seconds
    _interlockOutputOnDelay = 0.050; ///< “interlock output on delay”(50ms), in seconds

    // - U32 output off max delay (ms) (unsigned long [32-bit integer (0 to 4,294,967,295)])
    //   “relay open delay” (30ms) + “interlock output off delay”(50ms) = 80ms
    _relayOpenDelay = 0.030; ///< “relay open delay” (30ms), in seconds
    _interlockOutputOffDelay = 0.050; ///< “interlock output off delay”(50ms), in seconds

    // - U32 reset breaker pulse width (ms) (unsigned long [32-bit integer (0 to 4,294,967,295)])  = 400ms
    _resetBreakerPulseWidth = 0.400; ///< "reset breaker pulse width" (400ms), in seconds

    // - DBL breaker operating voltage (double [64-bit real (~15 digit precision)] [V])  = 19V
    _breakerOperatingVoltage = 19.0; ///< breaker operating voltage 19V, in volts.

    // - CLUSTER output voltage warning level (volts) (cluster of 2 elements) = output voltage nominal level = 24V
    _nominalVoltage = 24.0; ///< "output voltage nominal level" = 24V, in volts.

    // - U32 breaker operating voltage rise time (ms) (unsigned long [32-bit integer (0 to 4,294,967,295)])  = 85ms
    _breakerOperatingVoltageRiseTime = 0.085; ///< breaker operating voltage rise time (85ms), in seconds.

    // - U32 output voltage settling time (ms) (unsigned long [32-bit integer (0 to 4,294,967,295)])  = 20ms
    _voltageSettlingTime = 0.020; ///< output voltage settling time (20ms), in seconds

    // - U32 output voltage fall time (ms) (unsigned long [32-bit integer (0 to 4,294,967,295)])  = 300ms
    _voltageFallTime = 0.3; ///< output voltage fall time (300ms), in seconds

    // - DBL output voltage off level (double [64-bit real (~15 digit precision)] [V])  = 12V
    _voltageOffLevel = 12.0; ///< output voltage off level 12V, in volts

    // - DBL maximum output current (double [64-bit real (~15 digit precision)] [A])  = 20A
    _maxCurrent = 20.0; ///< maximum output current 20A, in amps.

    BreakerFeedGroup::Feed::Ptr motorFeed1 = make_shared<BreakerFeedGroup::Feed>(
        InputPortBits::J1_W9_1_MTR_PWR_BRKR_OK, InputPortBits::J1_W9_2_MTR_PWR_BRKR_OK,
        InputPortBits::J1_W9_3_MTR_PWR_BRKR_OK);
    BreakerFeedGroup::Feed::Ptr motorFeed2 = make_shared<BreakerFeedGroup::Feed>(
        InputPortBits::J2_W10_1_MTR_PWR_BRKR_OK, InputPortBits::J2_W10_2_MTR_PWR_BRKR_OK,
        InputPortBits::J2_W10_3_MTR_PWR_BRKR_OK);
    BreakerFeedGroup::Feed::Ptr motorFeed3 = make_shared<BreakerFeedGroup::Feed>(
        InputPortBits::J3_W11_1_MTR_PWR_BRKR_OK, InputPortBits::J3_W11_2_MTR_PWR_BRKR_OK,
        InputPortBits::J3_W11_3_MTR_PWR_BRKR_OK);

    _breakerFeedGroup = make_shared<BreakerFeedGroup>(motorFeed1, motorFeed2, motorFeed3);

    _outputPowerOnBitPos = OutputPortBits::MOTOR_POWER_ON;
    _outputBreakerBitPos = OutputPortBits::RESET_MOTOR_BREAKERS;

    // Values from BasePowerSubsystem.lvclass:configure_subsystem.vi
    _subsytemName = "motor";
    _voltageFault = faultmgr::FaultStatusBits::MOTOR_VOLTAGE_FAULT;
    _voltageWarn = faultmgr::FaultStatusBits::MOTOR_VOLTAGE_WARN;
    _excessiveCurrent = faultmgr::FaultStatusBits::MOTOR_OVER_CURRENT;
    _relayFault = faultmgr::FaultStatusBits::POWER_RELAY_OPEN_FAULT;
    _breakerFault = faultmgr::FaultStatusBits::MOTOR_MULTI_BREAKER_FAULT;
    _breakerWarn = faultmgr::FaultStatusBits::SINGLE_BREAKER_TRIP;
    _relayInUse = faultmgr::FaultStatusBits::MOTOR_RELAY;
}


void PowerSubsystemConfig::_setupComm() {
    // - Comm Power Subsystem Configuration Information (typedef 'PowerSubsystem (cluster of 11 elements)
    //   Values found in “PowerSubsystemCommonConfig.vi”  and “CommPowerBusConfigurationParameters.vi”
    // - U32 output on max delay (ms) (unsigned long [32-bit integer (0 to 4,294,967,295)])
    //   “relay close delay” (50ms) + “breaker on time” (500ms) = 550ms
    _relayCloseDelay = 0.050; ///< "relay close delay" 50ms, in seconds
    _breakerOnTime = 0.5; ///< “breaker on time” (500ms), in seconds
    _interlockOutputOnDelay = 0.000; ///< “interlock output on delay”(ms), in seconds, not used in COMM

    // - U32 output off max delay (ms) (unsigned long [32-bit integer (0 to 4,294,967,295)])
    //   “relay open delay” (30ms) = 30ms
    _relayOpenDelay = 0.030; ///< “relay open delay” (30ms), in seconds
    _interlockOutputOffDelay = 0.0; ///< “interlock output off delay”(ms), in seconds, not used in COMM

    // - U32 reset breaker pulse width (ms) (unsigned long [32-bit integer (0 to 4,294,967,295)])  = 400ms
    _resetBreakerPulseWidth = 0.400; ///< "reset breaker pulse width" (400ms), in seconds

    // - DBL breaker operating voltage (double [64-bit real (~15 digit precision)] [V])  = 19V
    _breakerOperatingVoltage = 19.0; ///< breaker operating voltage 19V, in volts.

    // - CLUSTER output voltage warning level (volts) (cluster of 2 elements)  = “output voltage nominal level” = 24V
    _nominalVoltage = 24.0; ///< "output voltage nominal level" = 24V, in volts.

    // - U32 breaker operating voltage rise time (ms) (unsigned long [32-bit integer (0 to 4,294,967,295)])  = 30ms
    _breakerOperatingVoltageRiseTime = 0.030; ///< breaker operating voltage rise time (30ms), in seconds.

    // - U32 output voltage settling time (ms) (unsigned long [32-bit integer (0 to 4,294,967,295)])  = 10ms
    _voltageSettlingTime = 0.020; ///< output voltage settling time (20ms), in seconds

    // - U32 output voltage fall time (ms) (unsigned long [32-bit integer (0 to 4,294,967,295)])  = 50ms
    _voltageFallTime = 0.050; ///< output voltage fall time (50ms), in seconds

    // - DBL output voltage off level (double [64-bit real (~15 digit precision)] [V])  = 12V
    _voltageOffLevel = 12.0; ///< output voltage off level 12V, in volts

    // - DBL maximum output current (double [64-bit real (~15 digit precision)] [A])  = 10A
    _maxCurrent = 10.0; ///< maximum output current 10A, in amps.

    BreakerFeedGroup::Feed::Ptr commFeed1 = make_shared<BreakerFeedGroup::Feed>(
        InputPortBits::J1_W12_1_COMM_PWR_BRKR_OK, InputPortBits::J1_W12_2_COMM_PWR_BRKR_OK,
        InputPortBits::ALWAYS_HIGH);
    BreakerFeedGroup::Feed::Ptr commFeed2 = make_shared<BreakerFeedGroup::Feed>(
        InputPortBits::J2_W13_1_COMM_PWR_BRKR_OK, InputPortBits::J2_W13_2_COMM_PWR_BRKR_OK,
        InputPortBits::ALWAYS_HIGH);
    BreakerFeedGroup::Feed::Ptr commFeed3 = make_shared<BreakerFeedGroup::Feed>(
        InputPortBits::J3_W14_1_COMM_PWR_BRKR_OK, InputPortBits::J3_W14_2_COMM_PWR_BRKR_OK,
        InputPortBits::ALWAYS_HIGH);
    _breakerFeedGroup = make_shared<BreakerFeedGroup>(commFeed1, commFeed2, commFeed3);

    _outputPowerOnBitPos = OutputPortBits::ILC_COMM_POWER_ON;
    _outputBreakerBitPos = OutputPortBits::RESET_COMM_BREAKERS;

    // Values from BasePowerSubsystem.lvclass:configure_subsystem.vi
    _subsytemName = "comm";
    _voltageFault = faultmgr::FaultStatusBits::COMM_VOLTAGE_FAULT;
    _voltageWarn = faultmgr::FaultStatusBits::COMM_VOLTAGE_WARN;
    _excessiveCurrent = faultmgr::FaultStatusBits::COMM_OVER_CURRENT;
    _relayFault = faultmgr::FaultStatusBits::POWER_RELAY_OPEN_FAULT;
    _breakerFault = faultmgr::FaultStatusBits::COMM_MULTI_BREAKER_FAULT;
    _breakerWarn = faultmgr::FaultStatusBits::SINGLE_BREAKER_TRIP;
    _relayInUse = faultmgr::FaultStatusBits::COMM_RELAY;

}

void PowerSubsystemConfig::_setupCalculated() {
    //  Based on “output voltage nominal level” * x%   x=”output voltage warning threshold level(%)”= 5%
    //   - DBL Minimum (double [64-bit real (~15 digit precision)])  = 24V * 0.95 = 22.8
    //   - DBL Maximum (double [64-bit real (~15 digit precision)])  = 24V * 1.05 = 25.2
    _minVoltageWarn = _nominalVoltage * 0.95; ///< minimum voltage warning level in volts.
    _maxVoltageWarn = _nominalVoltage * 1.05; ///< maximum voltage warning level in volts.

    // -  CLUSTER output voltage fault level (volts) (cluster of 2 elements)
    //   Based on “output voltage nominal level” * x%   x=”output voltage fault threshold level(%)”= 10%
    //    - DBL Minimum (double [64-bit real (~15 digit precision)])  = 24V * 0.90 = 21.6
    //    - DBL Maximum (double [64-bit real (~15 digit precision)])  = 24V * 1.10 = 26.4
    _minVoltageFault = _nominalVoltage * 0.90; ///< minimum voltage fault level in volts.
    _maxVoltageFault = _nominalVoltage * 1.10; ///< maximum voltage fault level in volts.
}

double PowerSubsystemConfig::outputOnMaxDelay() const {
    if (_systemType == MOTOR) return _relayCloseDelay + _breakerOnTime + _interlockOutputOnDelay;
    if (_systemType == COMM) return _relayCloseDelay + _breakerOnTime;;
    throw util::Bug(ERR_LOC, "PowerSubsystemConfig unexpected _systemType=" + to_string(_systemType));
}


double PowerSubsystemConfig::outputOffMaxDelay() const {
    if (_systemType == MOTOR) return _relayOpenDelay + _interlockOutputOffDelay;
    if (_systemType == COMM) return _relayOpenDelay;
    throw util::Bug(ERR_LOC, "PowerSubsystemConfig unexpected _systemType=" + to_string(_systemType));
}


PowerSubsystem::PowerSubsystem(PowerSystemType sysType) : _systemType(sysType), _psCfg(_systemType),
        _fpgaIo(FpgaIo::getPtr()) {

}

double PowerSubsystem::getVoltage() {
    switch (_systemType) {
    case MOTOR: return _sysInfo.motorVoltage;
    case COMM: return _sysInfo.commVoltage;
    }
    LERROR(getClassName(), " getVoltage unexpected _systemType=", _systemType, " ", getPowerSystemTypeStr(_systemType));
    return 0.0;
}

double PowerSubsystem::getCurrent() {
    switch (_systemType) {
    case MOTOR: return _sysInfo.motorCurrent;
    case COMM: return _sysInfo.commCurrent;
    }
    LERROR(getClassName(), " getCurrent unexpected _systemType=", _systemType, " ", getPowerSystemTypeStr(_systemType));
    return 0.0;
}


bool PowerSubsystem::_getRelayControlOutputOn() const {
    /// “Relay Control Output On”
    bool relayControlOutputOn = false;
    switch(_systemType) {
    case MOTOR:
        // DAQ_to_motor_telemetry.vi
        relayControlOutputOn = _sysInfo.outputPort.getBitAtPos(OutputPortBits::MOTOR_POWER_ON);
        return relayControlOutputOn;
    case COMM:
        // DAQ_to_comm_telemetry.vi
        relayControlOutputOn = _sysInfo.outputPort.getBitAtPos(OutputPortBits::ILC_COMM_POWER_ON);
        return relayControlOutputOn;
    }
    throw util::Bug(ERR_LOC, string(__func__) + " unexpected systemType");
}


bool PowerSubsystem::_getCrioReadyOutputOn() const {
    // “cRIO Ready Output On”
    bool crioReadyOutputOn = false;
    switch(_systemType) {
    case MOTOR:
        // DAQ_to_motor_telemetry.vi
        crioReadyOutputOn = _sysInfo.outputPort.getBitAtPos(OutputPortBits::CRIO_INTERLOCK_ENABLE);
        return  crioReadyOutputOn;
    case COMM:
        // DAQ_to_comm_telemetry.vi
        return true;
    }
    throw util::Bug(ERR_LOC, string(__func__) + " unexpected systemType");
}


bool PowerSubsystem::_getInterlockRelayControlOutputOn() const {
    // ”Interlock Relay Control Output On”
    bool interlockRelayControlOutputOn = false;
    switch(_systemType) {
    case MOTOR:
        // DAQ_to_motor_telemetry.vi
        interlockRelayControlOutputOn = _sysInfo.inputPort.getBitAtPos(InputPortBits::INTERLOCK_POWER_RELAY);
        return interlockRelayControlOutputOn;
    case COMM:
        // DAQ_to_comm_telemetry.vi
        return true;
        break;
    }
    throw util::Bug(ERR_LOC, string(__func__) + " unexpected systemType");
}


bool PowerSubsystem::_powerShouldBeOn() {
    if (_getRelayControlOutputOn() && _getCrioReadyOutputOn()) {
        if (_getInterlockRelayControlOutputOn()) {
            return true;
        } else {
            // Set "Fault Status" "interlock fault"
            _sendFaultMgrSetBit(faultmgr::FaultStatusBits::INTERLOCK_FAULT);
        }
    }
    return false;
}


void PowerSubsystem::setPowerOn() {
    VMUTEX_NOT_HELD(_powerStateMtx);
    auto outputPort = _fpgaIo->getOutputPort();
    lock_guard<util::VMutex> lg(_powerStateMtx);
    _setPowerOn();
}

void PowerSubsystem::_setPowerOn() {
    VMUTEX_HELD(_powerStateMtx);
    // &&& NEED code
    if (_checkForFaults()) {
        LERROR(getClassName(), " _setPowerOn cannot turn on due to faults");
        _sendFaultMgrError(500003, "Internal ERROR: Faults preventing operation to proceed");
        _setPowerOff();
        return;
    }

    LINFO(getClassName(), " Turning power on");
    _fpgaIo->writeOutputPortBitPos(_psCfg.getOutputBreakerBitPos(), true);
    _fpgaIo->writeOutputPortBitPos(_psCfg.getOutputPowerOnBitPos(), true);
    _targPowerState = ON;
    _phase = 1;
    _powerOnStart = util::CLOCK::now();
    _phaseStartTime = _powerOnStart;
    _telemCounter = 0;
}


void PowerSubsystem::setPowerOff() {
    VMUTEX_NOT_HELD(_powerStateMtx);
    lock_guard<util::VMutex> lg(_powerStateMtx);
    _setPowerOff();
}

void PowerSubsystem::_setPowerOff() {
    VMUTEX_HELD(_powerStateMtx);

    LINFO(getClassName(), " Turning power off");
    _fpgaIo->writeOutputPortBitPos(_psCfg.getOutputBreakerBitPos(), true);
    _fpgaIo->writeOutputPortBitPos(_psCfg.getOutputPowerOnBitPos(), false);
    if (_targPowerState != OFF) {
        _targPowerState = OFF;
        _phase = 1;
        _powerOffStart = util::CLOCK::now();
        _phaseStartTime = _powerOffStart;
    }
}


SysStatus PowerSubsystem::processDAQ(SysInfo const& info) {
    _sysInfo = info;

    // Check for faults
    bool systemFaults = _checkForFaults();
    if (systemFaults) {
        _setPowerOff();
    }

    // Breakers only matter (and have correct input) when voltage is
    // above `_breakerOperatingVoltage`, there's been time for them
    // to stabilize, and `_targPowerOn` is true.
    lock_guard<util::VMutex> lg(_powerStateMtx);
    switch (_targPowerState) {
    case  ON:
        _processPowerOn();
        break;
    case RESET: [[fallthrough]]; // invalid case
    case TURNING_ON: [[fallthrough]]; // invalid case
    case TURNING_OFF: [[fallthrough]]; // invalid case
    case UNKNOWN: // invalid case
        LERROR(getClassName() ," unexpected _targPowerState=", getPowerStateStr(_targPowerState), "turning off");
        _setPowerOff();
        [[fallthrough]];
    case OFF: [[fallthrough]]; // OFF is the default.
    default:
        _processPowerOff();
    }


    return FAULT;
}

/* &&&
               - MotorPowerSubsystem used with MST in call to BasePowerSubsystem->process_telemetry.vi
                 - Calls PSS_State.lvclass:PS_process_telemetry.vi   It has versions for these states: ***LB015***
                   - PS_Powered_On.lvclass:PS_process_telemetry.vi
                     See below NOTE: Powering on comment from “PS_Powered_On.lvclass:PS_Process_telemetry.vi”
                     - get output for BasePowerOutput->output_should_be_on.vi (values from MST or CST above)  ***LB011***
                         return (“Relay Control Output On” && “cRIO Ready Output On” && ”Interlock Relay Control Output On”)
                                   Also if 
                                      (“Relay Control Output On” && “cRIO Ready Output On”) && !”Interlock Relay Control Output On” 
                                      then set the “Fault Status” “interlock fault” bit
                        - If that returned false - call “PSS_State.lvclass.goto_powering_off.vi”  ***LB012***
                            - BasePowerSubsystem->set_breaker_output.vi (TRUE)  (see LB004)
                            - BasePowerSubsystem->set_relay_output.vi (FALSE) (see LB003)
                            - BasePowerSubsystem->set_state.vi (“powering off”) and exit vi
                        - if that returned true 
                           - If the substate == “phase 1”
                               - then call “BasePowerSubsystem->output_voltage_is_stable.vi  (it just returns true if enough time has passed) 
                                   Return (“starting time (msecs)” 
                                                + “Subsystem Configuration Information.output voltage settling time (ms)” 
                                                - “Subsystem Configuration Information.breaker operating voltage rise time (ms)”
                                               ) > now                            NOTE: it does NOT check the voltage.
                                   - if that returned true, then substate is set to “phase_2” and “starting time (msecs)” is set to now.
                           - If the substate == “phase 2”
                               - then call “BasePowerSubsystem->breaker_status_is_active.vi  ***LB013***
                                  Return (“Output Voltage” >= “Subsystem Configuration Information.breaker operating voltage”)
                                   - If that was false [voltage was too low change to powering off]
                                       - “BasePowerSubsystem->signal_voltage_hardware_fault.vi”  ***LB014***
                                           - “BasePowerSubsystem->signal_voltage_fault.vi”   ***LB009***
                                               - set “voltage fault” bit of “Fault Status”
                                           - “BasePowerSubsystem->signal_hardware_fault.vi
                                               - set “hardware fault” bit of “Fault Status”
                                       - call “PSS_State.lvclass.goto_powering_off.vi” to set state to “powering off” and exit vi  (see LB012)
                                   - If that was true [voltage was ok] then make further checks on power
                                      - “BasePowerSubsystem->check_output_for_faults_and_warnings.vi
                                         - “BasePowerSubsystem->check_for_output_current_fault.vi  ***LB008***
                                            -if (“Output Current” >
                                                 “Subsystem Configuration Information.maximum output current”) then 
                                               - [current too high]
                                                -“BasePowerSubsystem->signal_current_fault.vi”
                                                   - set “excessive current” bit of “Fault Status”
                                                - “BasePowerSubsytem->turn_power_off.vi” and exit this vi 
                                                        [why not  “PSS_State.lvclass.goto_powering_off.vi”???, or why don’t the others call this???]
                             - PS_Powered_On.lvclass:PS_turn_power_off.vi
                                                        - turn off power for this system, set “telemetry counter” =0, 
                                                             set state to “powering off”, “phase_1”, “starting time (msecs)” = now 
                                               - [current too ok] - continue on
                               - everything ok so far, call BasePowerSubsystem->check_output_conditions_for_faults_and_warnings.vi
                                   - “BasePowerSubsystem->check_for_output_current_fault.vi  (see LB008 above)
                                   - “BasePowerSubsystem->check_for_output_voltage_faults_and_warnings.vi 
                                      - if (“Output Voltage” > “Subsystem Configuration Information.output voltage fault level (volts).Minimum”
                                            && “Output Voltage” > “Subsystem Configuration Information.output voltage fault level (volts).Maximum”)
                                           - FALSE (voltage fault) -  
                                             - “BasePowerSubsystem->signal_voltage_fault.vi”   (see LB009 above)
                                             - “BasePowerSubsystem->turn_power_off.vi”   (see LB010 above)
                                           - TRUE (voltage ok, check for warning) -
                                      - if (“Output Voltage” > “Subsystem Configuration Information.output voltage warning level (volts).Minimum”
                                            && “Output Voltage” > “Subsystem Configuration Information.output voltage warning level
                                                    (volts).Maximum”)
                                           - FALSE (voltage warning) -
                                              -  “BasePowerSubsystem->signal_voltage_warning.vi”, set “voltage warning” bit of “Fault Status”
                                           - TRUE (voltage good) -
&&& */

bool PowerSubsystem::_checkForPowerOnBreakerFault(double voltage) {
    // Is the voltage high enough to to check the breakers? breaker_status_is_Active.vi
    if (voltage >= _psCfg.getBreakerOperatingVoltage()) {
        auto [breakerStatus, inactiveInputs] = _breakerFeeds->checkBreakers(_sysInfo);
        if (breakerStatus == GOOD) {
            return true;
        } else {
            if (breakerStatus == FAULT) {
                _sendFaultMgrSetBit(_psCfg.getBreakerFault()); //"breaker fault"
                _setPowerOff();
            } else {
                _sendFaultMgrSetBit(_psCfg.getBreakerWarn()); //"breaker warning"
            }
        }
    } else {
        // The power should be on and stable.
        _sendBreakerVoltageFault();
    }
    return false;
}

void PowerSubsystem::_sendBreakerVoltageFault() {
    _sendFaultMgrSetBit(_psCfg.getVoltageFault()); // "voltage fault"
    _sendFaultMgrSetBit(faultmgr::FaultStatusBits::HARDWARE_FAULT); // "hardware fault"
    _setPowerOff();
}


void PowerSubsystem::_processPowerOn() {
    VMUTEX_HELD(_powerStateMtx);


    // Check that the outputs are appropriate for turning power on
    // BasePowerOutput->output_should_be_on.vi
    bool outputIsOn = _powerShouldBeOn();

    double voltage = getVoltage();
    if (voltage > _psCfg.getMaxVoltageFault()) {
        LERROR(getClassName(), " voltage(", voltage, ") is too high, turning off");
        _sendFaultMgrError();
        _setPowerOff();
        return;
    }
    if (voltage > _psCfg.getMaxVoltageWarn()) {
        LWARN(getClassName(), " voltage(", voltage, ") above warning level ", _psCfg.getMaxVoltageWarn());
        _sendFaultMgrWarn();
    }

    util::TIMEPOINT now = util::CLOCK::now();
    switch (_actualPowerState) {
    case ON:
    {
        if (!outputIsOn) {
            _setPowerOff();
            return;
            if (_phase <= 1) {
                // ouput_voltage_is_stable.vi
                double timeSincePhaseStartInSec = util::timePassedSec(_phaseStartTime, now);
                double minTimeToStablize = _psCfg.getVoltageSettlingTime() - _psCfg.getBreakerOperatingVoltageRiseTime();
                if (_checkForPowerOnBreakerFault(voltage)) {
                    LERROR("Breaker fault while _actualPowerState == ON");
                    return;
                }
                if(timeSincePhaseStartInSec > minTimeToStablize) {
                    _phase = 2;
                    _phaseStartTime = util::CLOCK::now();
                }
            } else if (_phase == 2) {
                if (voltage < _psCfg.getMinVoltageWarn()) {
                    LWARN(getClassName(), " voltage(", voltage, ") below warning level ", _psCfg.getMinVoltageWarn());
                    _sendFaultMgrWarn();
                }
                if (voltage < _psCfg.getMinVoltageFault()) {
                    LWARN(getClassName(), " voltage(", voltage, ") below fault level ", _psCfg.getMinVoltageFault());
                    _sendFaultMgrError();
                    _setPowerOff();
                }
            }
        }
        break;
    }
    case OFF: [[fallthrough]];
    case TURNING_OFF:
        _actualPowerState = TURNING_ON;
        _phase = 1;
        _phaseStartTime = now;
        [[fallthrough]];
    case TURNING_ON:
    {
        if (_phase == 1) {
            // Presumably, waiting for the signal to stabilize.
            ++_telemCounter;
            // Value of 10 comes from telemetry_is_stable.vi, no explanation.
            if (_telemCounter >= 10) {
                _phase = 2;
                _phaseStartTime = now;
            }
        }

        if (_phase > 1 && !outputIsOn) {
            // At this point, the power output bits should be on. If they
            // aren't, give up and turn off power.
            if (!outputIsOn) {
                _setPowerOff();
                return;
            }
        }

        if (_phase == 2) {
            double timeSincePhaseStartInSec = util::timePassedSec(_phaseStartTime, now);
            if (timeSincePhaseStartInSec > _psCfg.outputOnMaxDelay()) {
                _phase = 3;
                _phaseStartTime = now;
            } else {
                return;
            }
        }

        if (_phase == 3) {
            // If the voltage isn't high enough to read the breakers, give up, turn power off
            if (voltage < _psCfg.getBreakerOperatingVoltage()) {
                _sendBreakerVoltageFault(); // calls _setPowerOff()
                return;
            }
            // If the breakers have any faults or warnings, try to reset them.
            auto [breakerStatus, inactiveInputs] = _breakerFeeds->checkBreakers(_sysInfo);
            if (breakerStatus == GOOD) {
                _actualPowerState = ON;
                _phaseStartTime = now;
                LINFO(getClassName(), " is now ON");
                return;
            } else {
                _actualPowerState = RESET;
                _phaseStartTime = now;
                _phase = 1;
                LINFO(getClassName(), " reset needed");
                // Start resetting the breakers by setting the output low.
                _fpgaIo->writeOutputPortBitPos(_psCfg.getOutputBreakerBitPos(), false);
            }

        }
        break;
    }
    case RESET:
    {
        // Trying to reset the breakers, this can only be done while trying to turn power on.
        if (!outputIsOn) {
            // Something happened, give up.
            _fpgaIo->writeOutputPortBitPos(_psCfg.getOutputBreakerBitPos(), true);
            _setPowerOff();
            return;
        }
        if (voltage < _psCfg.getBreakerOperatingVoltage()) {
            _sendBreakerVoltageFault(); // calls _setPowerOff()
            return;
        }
        double timeSincePhaseStartInSec = util::timePassedSec(_phaseStartTime, now);
        if (timeSincePhaseStartInSec > _psCfg.getResetBreakerPulseWidth()) {
            // Enough time passed, restore breaker the output
            _fpgaIo->writeOutputPortBitPos(_psCfg.getOutputBreakerBitPos(), true);
            // Check breakers again (failure sets power to off).
            if (_checkForPowerOnBreakerFault(voltage)) {
                LERROR("Breaker fault while _actualPowerState == RESET");
                return;
            }
            // It worked, set power on.
            _actualPowerState = ON;
            _phaseStartTime = now;
            LINFO(getClassName(), " reset success");
            return;
        }
        break;
    }
    default:
        LERROR(getClassName(), " unexpected _actualPowerState=", getPowerStateStr(_actualPowerState));
        _setPowerOff();
    }
}


void PowerSubsystem::_processPowerOff() {
    VMUTEX_HELD(_powerStateMtx);

    bool powerOn = _getRelayControlOutputOn();
    if (powerOn) {
        _setPowerOff();
        return;
    }

    util::TIMEPOINT now = util::CLOCK::now();
    double voltage = getVoltage();
    switch (_actualPowerState) {
    case UNKNOWN: [[fallthrough]];
    case RESET: [[fallthrough]];
    case ON: [[fallthrough]];
    case TURNING_ON:
        _setPowerOff();
        _phaseStartTime = now;
        [[fallthrough]];
    case TURNING_OFF:
    {
        if (voltage < _psCfg.getVoltageOffLevel()) {
            _actualPowerState = OFF;
            _phaseStartTime = now;
            LINFO(getClassName(), " is now OFF");
            return;
        } else {
            double timeInPhaseSec = util::timePassedSec(_phaseStartTime, now);
            if (timeInPhaseSec > _psCfg.outputOffMaxDelay()) {
                _sendFaultMgrSetBit(_psCfg.getRelayFault()); // "relay fault"
                _sendFaultMgrSetBit(_psCfg.getRelayInUse()); // "relay in use"
                _setPowerOff();
            }
        }
        break;
    }
    case OFF:
        // do nothing
        if (voltage > _psCfg.getVoltageOffLevel()) {
            LWARN(getClassName(), " voltage high for OFF state voltage=", voltage);
        }
    }

}

/* &&&
void PowerSubsystem::_sendFaultMgrError() {
    LERROR(getClassName(), " _sendFaultMgrError &&& NEEDS CODE");
}

void PowerSubsystem::_sendFaultMgrError(int errId, string note) {
    LERROR(getClassName(), " _sendFaultMgrError &&& NEEDS CODE ", errId, note);
}

void PowerSubsystem::_sendFaultMgrWarn() {
    LERROR(getClassName(), " _sendFaultMgrWarn &&& NEEDS CODE");
}
*/

bool PowerSubsystem::_checkForFaults() {
    return faultmgr::FaultMgr::get().checkForPowerSubsystemFaults(_psCfg.getSubsystemFaultMask());
}


/* &&&

 - “process DAQ telemetry”
   - Power Subsystem Cmd - Parameters provide crucial information for this, including:
     - PowerSubsystem->process_DAQ_telemetry.vi
        - DAQ Telemetry which is broken in to three parts by PowerSubsystem->disassemble_DAQ_telemetry.vi
           - DAQ_to_motor_telemetry.vi - Assemble vi output “Motor Subsystem Telemetry” (shortening to MST here)
             - “Power Control/Status Telemetry.Processed Motor Voltage” -> sets units as “V” -> “MST.Output Voltage”
             - “Power Control/Status Telemetry.Processed Motor Current” -> sets units as “A” -> “MST.Output Current”
             - “Power Control/Status Telemetry.Digital Outputs” (active high)
                 AND with “Output Port Bit Masks.ILC Motor Power On Bit” -> convert to bool (true if != 0) -> “MST.Relay Control Output On”
             - “Power Control/Status Telemetry.Digital Outputs” (active high)
                 AND with “Output Port Bit Masks.cRIO Interlock Enable Bit” -> convert to bool (true if != 0) -> “MST.cRIO Ready Output On”
             - “Power Control/Status Telemetry.Digital Inputs” (active low)
                  AND with “Input Port Bit Masks.Interlocal Power Relay On Bit” - > convert to bool (true if == 0) -> “MST.Interlock Relay Control Output On”
              - Assemble vi output “MST.Breaker Power Feed Status” (shortening to MST.BPFS here)
                - “Power Control/Status Telemetry.Digital Inputs”
                   -  AND with “Input Port Bit Masks.J1-WE9-1-MtrPwrBrkr OK Bit” -> if !=0 set bit 0b001 of “MST.BPFS.Feed 1”
                   -  AND with “Input Port Bit Masks.J1-WE9-2-MtrPwrBrkr OK Bit” -> if !=0 set bit 0b010 of “MST.BPFS.Feed 1”
                   -  AND with “Input Port Bit Masks.J1-WE9-3-MtrPwrBrkr OK Bit” -> if !=0 set bit 0b100 of “MST.BPFS.Feed 1”
                   -  AND with “Input Port Bit Masks.J2-WE10-1-MtrPwrBrkr OK Bit” -> if !=0 set bit 0b001 of “MST.BPFS.Feed 2”
                   -  AND with “Input Port Bit Masks.J2-WE10-2-MtrPwrBrkr OK Bit” -> if !=0 set bit 0b010 of “MST.BPFS.Feed 2”
                   -  AND with “Input Port Bit Masks.J2-WE10-3-MtrPwrBrkr OK Bit” -> if !=0 set bit 0b100 of “MST.BPFS.Feed 2”
                   -  AND with “Input Port Bit Masks.J3-WE11-1-MtrPwrBrkr OK Bit” -> if !=0 set bit 0b001 of “MST.BPFS.Feed 3”
                   -  AND with “Input Port Bit Masks.J3-WE11-2-MtrPwrBrkr OK Bit” -> if !=0 set bit 0b010 of “MST.BPFS.Feed 3”
                   -  AND with “Input Port Bit Masks.J3-WE11-3-MtrPwrBrkr OK Bit” -> if !=0 set bit 0b100 of “MST.BPFS.Feed 3”
            - DAQ_to_comm_telemetry.vi - Assemble vi output “Comm Subsystem Telemetry” (shortening to CST here)
              - “Power Control/Status Telemetry.Processed Comm Voltage” -> sets units as “V” -> “CST.Output Voltage”
              - “Power Control/Status Telemetry.Processed Motor Current” -> sets units as “A” -> “CST.Output Current”
              - “Power Control/Status Telemetry.Digital Outputs” (active high)
                 AND with “Output Port Bit Masks.ILC Comm Power On Bit” -> convert to bool (true if != 0) -> “CST.Relay Control Output On”
              - always set to TRUE -> “CST.cRIO Ready Output On”
              - always set to TRUE -> “CST.Interlock Relay Control Output On”
              - Assemble vi output “CST.Breaker Power Feed Status” (shortening to CST.BPFS here)
                - “Power Control/Status Telemetry.Digital Inputs”
                   -  AND with “Input Port Bit Masks.J1-WE12-1-MtrPwrBrkr OK Bit” -> if !=0 set bit 0b001 of “CST.BPFS.Feed 1”
                   -  AND with “Input Port Bit Masks.J1-WE12-2-MtrPwrBrkr OK Bit” -> if !=0 set bit 0b010 of “CST.BPFS.Feed 1”
                   -  always set bit 0b100 of “CST.BPFS.Feed 1”
                   -  AND with “Input Port Bit Masks.J2-WE13-1-MtrPwrBrkr OK Bit” -> if !=0 set bit 0b001 of “CST.BPFS.Feed 2”
                   -  AND with “Input Port Bit Masks.J2-WE13-2-MtrPwrBrkr OK Bit” -> if !=0 set bit 0b010 of “CST.BPFS.Feed 2”
                   -  always set bit 0b100 of “CST.BPFS.Feed 2”
                   -  AND with “Input Port Bit Masks.J3-WE14-1-MtrPwrBrkr OK Bit” -> if !=0 set bit 0b001 of “CST.BPFS.Feed 3”
                   -  AND with “Input Port Bit Masks.J3-WE14-2-MtrPwrBrkr OK Bit” -> if !=0 set bit 0b010 of “CST.BPFS.Feed 3”
                   -  always set bit 0b100 of “CST.BPFS.Feed 3”
            - DAQ_to_PS_health_telemetry.vi - assemble vi output “Power Subsystem Common Telemetry” (shortening to PSCT)
              - “Power Control/Status Telemetry.Digital Inputs”
                - AND with “Input Port Bit Masks.RedundancyOK Bit” (active high)
                    -> convert to bool (true if !=0) -> “PSCT.Redundancy OK”
                - AND with “Input Port Bit Masks.Load Distribution OK Bit” (active high)
                    -> convert to bool (true if !=0) -> “PSCT.Load Distribution OK”
                - AND with “Input Port Bit Masks.Power Supply #1 DC OK Bit” (active high)
                    -> convert to bool (true if !=0) -> “PSCT.P/S 1 DC OK”
                - AND with “Input Port Bit Masks.Power Supply #2 DC OK Bit” (active high)
                    -> convert to bool (true if !=0) -> “PSCT.P/S 2 DC OK”
                - AND with “Input Port Bit Masks.Power Supply #1 Current OK Bit” (active low)
                    -> convert to bool (true if ==0) -> “PSCT.P/S 1 Boost Current ON”
                - AND with “Input Port Bit Masks.Power Supply #2 Current OK Bit” (active low)
                    -> convert to bool (true if ==0) -> “PSCT.P/S 2 Boost Current ON”
         - MotorPowerSubsystem used with MST in call to BasePowerSubsystem->process_telemetry.vi
           - Calls PSS_State.lvclass:PS_process_telemetry.vi   It has versions for these states: **LB015**
             - PS_Powered_On.lvclass:PS_process_telemetry.vi
               See below NOTE: Powering on comment from “PS_Powered_On.lvclass:PS_Process_telemetry.vi”
               - get output for BasePowerOutput->output_should_be_on.vi (values from MST or CST above)  **LB011***
                   return (“Relay Control Output On” && “cRIO Ready Output On” && ”Interlock Relay Control Output On”)
                             Also if
                                (“Relay Control Output On” && “cRIO Ready Output On”) && !”Interlock Relay Control Output On”
                                then set the “Fault Status” “interlock fault” bit
                  - If that returned false - call “PSS_State.lvclass.goto_powering_off.vi”  **LB012***
                      - BasePowerSubsystem->set_breaker_output.vi (TRUE)  (see LB004)
                      - BasePowerSubsystem->set_relay_output.vi (FALSE) (see LB003)
                      - BasePowerSubsystem->set_state.vi (“powering off”) and exit vi
                  - if that returned true
                     - If the substate == “phase 1”
                         - then call “BasePowerSubsystem->output_voltage_is_stable.vi  (it just returns true if enough time has passed)
                             Return (“starting time (msecs)”
                                          + “Subsystem Configuration Information.output voltage settling time (ms)”
                                          - “Subsystem Configuration Information.breaker operating voltage rise time (ms)”
                                         ) > now                            NOTE: it does NOT check the voltage.
                             - if that returned true, then substate is set to “phase_2” and “starting time (msecs)” is set to now.
                     - If the substate == “phase 2”
                         - then call “BasePowerSubsystem->breaker_status_is_active.vi  **LB013***
                            Return (“Output Voltage” >= “Subsystem Configuration Information.breaker operating voltage”)
                             - If that was false [voltage was too low change to powering off]
                                 - “BasePowerSubsystem->signal_voltage_hardware_fault.vi”  **LB014***
                                     - “BasePowerSubsystem->signal_voltage_fault.vi”   **LB009***
                                         - set “voltage fault” bit of “Fault Status”
                                     - “BasePowerSubsystem->signal_hardware_fault.vi
                                         - set “hardware fault” bit of “Fault Status”
                                 - call “PSS_State.lvclass.goto_powering_off.vi” to set state to “powering off” and exit vi  (see LB012)
                             - If that was true [voltage was ok] then make further checks on power
                                - “BasePowerSubsystem->check_output_for_faults_and_warnings.vi
                                   - “BasePowerSubsystem->check_for_output_current_fault.vi  **LB008***
                                      -if (“Output Current” >
                                           “Subsystem Configuration Information.maximum output current”) then
                                         - [current too high]
                                          -“BasePowerSubsystem->signal_current_fault.vi”
                                             - set “excessive current” bit of “Fault Status”
                                          - “BasePowerSubsytem->turn_power_off.vi” and exit this vi
                                                  [why not  “PSS_State.lvclass.goto_powering_off.vi”???, or why don’t the others call this???]
                       - PS_Powered_On.lvclass:PS_turn_power_off.vi
                                                  - turn off power for this system, set “telemetry counter” =0,
                                                       set state to “powering off”, “phase_1”, “starting time (msecs)” = now
                                         - [current too ok] - continue on
                         - everything ok so far, call BasePowerSubsystem->check_output_conditions_for_faults_and_warnings.vi
                             - “BasePowerSubsystem->check_for_output_current_fault.vi  (see LB008 above)
                             - “BasePowerSubsystem->check_for_output_voltage_faults_and_warnings.vi
                                - if (“Output Voltage” > “Subsystem Configuration Information.output voltage fault level (volts).Minimum”
                                      && “Output Voltage” > “Subsystem Configuration Information.output voltage fault level (volts).Maximum”)
                                     - FALSE (voltage fault) -
                                       - “BasePowerSubsystem->signal_voltage_fault.vi”   (see LB009 above)
                                       - “BasePowerSubsystem->turn_power_off.vi”   (see LB010 above)
                                     - TRUE (voltage ok, check for warning) -
                                - if (“Output Voltage” > “Subsystem Configuration Information.output voltage warning level (volts).Minimum”
                                      && “Output Voltage” > “Subsystem Configuration Information.output voltage warning level
                                              (volts).Maximum”)
                                     - FALSE (voltage warning) -
                                        -  “BasePowerSubsystem->signal_voltage_warning.vi”, set “voltage warning” bit of “Fault Status”
                                     - TRUE (voltage good) -
             - PS_Powering_Off.lvclass:PS_process_telemetry.vi
                -  “BasePowerSubsystem->output_voltage_is_off.vi”
                      Return (“Output Voltage < Subsystem Configuration Information.output voltage off level”)
                   - TRUE - power is off - set state to “powered off” with “BasePowerSubsystem->outputvoltage_is_off.vi”
                   - FALSE - power is not off yet
                      - “BasePowerSubsystem->output_off_time_expired.vi”
                          - return (“starting time (msecs)”
                                         + “Subsystem Configuration Information.output off max delay (ms)”
                                         + “Subsystem Configuration Information.output voltage fall time (ms)”) > now
                            - TRUE [timed out] -
                              -“BasePowerSubsystem->signal_relay_fault.vi” - set “relay fault” bit of “Fault Status”
                              -“BasePowerSubsystem->signal_which_relay.vi” - set “relay in use” bit of “Fault Status”
                             - set state “powered off” with “BasePowerSubsystem->set_state.vi”
                                  (substate=”phase 1”, “starting time(msecs)” = now)
                            - FALSE [keep waiting for voltage to fall] -
             - PS_Powering_On.lvclass:PS_process_telemetry.vi
                - substate == “phase_1”  - waiting for telemetry to stabilize.
                - “BasePowerSubsystem->telemetry_is_stable.vi” [wait for the count to reach 10, that’s it]
                        - “telemetry counter” += 1
                        - Return (“telemetry counter” >= 10)
                   - FALSE - do nothing
                   - TRUE - “BasePowerSubsystem->set_substate.vi” (substate = “phase_2”, “starting time (msecs)” = now
                - substate == “phase_2”  - waiting for relay to close.
                   - “BasePowerSubsystem->output_should_be_on.vi” (see LB011 above)
                     - FALSE [something went wrong, go to power off state]
                        - “PSS_State.lvclass:goto_powering_off.vi”   (see LB012)
                     - TRUE - “BasePowerSubsystem->set_substate.vi” (substate = “phase_3”, “starting time (msecs)” = now
                - substate == “phase_3”  - wait for output voltage to rise to breaker operating level
                   - “BasePowerSubsystem->output_should_be_on.vi” (see LB011 above)
                     - FALSE [something went wrong, go to power off state]
                        - “PSS_State.lvclass:goto_powering_off.vi”   (see LB012)
                     - TRUE -
                        - “BasePowerSubsystem->breaker_status_is_active.vi  (see LB013)
                          -FALSE - [breaker voltage not high enough for them to report status]
                              - “BasePowerSubsystem->output_voltage_risetime_expired.vi”
                              - return (“starting time (msecs)”
                                         + “Subsystem Configuration Information.breaker operating voltage rise time (ms)”
                                         ) > now
                                 -FALSE - [do nothing, wait for next message]
                                 -TRUE [timed out, set power off]
                                    - “BasePowerSubsystem->signal_voltage_hardware_fault.vi” (see LB014)
                                    - “PSS_State.lvclass:goto_powering_off.vi”   (see LB012)
                                    - exit vi
                          -TRUE - [breaker voltage high enough for them to report status]
                            -“BasePowerSubsystem->check_breaker_status.vi” - [make sure all “BPFS.Feed ”’s are == 7]
                              - call “decode_breaker_status.vi” with “Breaker Power Feed Status.Feed 1”
                              - call “decode_breaker_status.vi” with “Breaker Power Feed Status.Feed 2”
                              - call “decode_breaker_status.vi” with “Breaker Power Feed Status.Feed 3”
            - returns 3 bool , “Breakers OK”, “Breakers Warning”, “Breakers Fault”
                                       “Feed” == 7  -> TRUE, FALSE, FALSE
                                       “Feed” == 0, 1, 2, 4 -> FALSE, FALSE, TRUE
                                       “Feed” == 3, 5, 6 -> FALSE, TRUE, FALSE
                               - return “Breakers OK” == AND  all “Breakers OK” for Feeds 1, 2, and 3
                               - return “Breakers Warning” == OR all “Breakers Warning” for Feeds 1, 2, and 3
                               - return “Breakers Fault” == OR all “Breakers fault” for Feeds 1, 2, and 3
                           - if “Breakers OK” == TRUE
                               - “BasePowerSubsystem->set_state.vi” set state to “powered on”
                            - if “Breakers OK” == FALSE
                               - “PSS_State.lvclass:reset_breakers.vi”
                                   - “BasePowerSubsytem->set_breaker_output.vi” (with FALSE)   (see LB004)
                                   - “BasePowerSubsytem->set_state.vi (with “resetting breakers”)
                                       - “State” = “resetting breakers”
                                       - substate = “phase_1”
                                       - “starting time (msec)” = now
             - PS_Resetting_Breakers.lvclass:PS_process_telemetry.vi [send the signal to the breaker of the proper width]
                - “BasePowerOutput->output_should_be_on.vi”  (see LB011)
                   - FALSE [something went wrong, go to power off state]
                      - “PSS_State.lvclass:goto_powering_off.vi”   (see LB012)
                   - TRUE [ output should be on]
                      - “BasePowerSubsystem->breaker_status_is_active.vi  (see LB013)
                          - FALSE [wait longer, do nothing, exit vi]
   - TRUE [reset breaker output breaker so disable breaker output line]
                             - BasePowerSubsystem->set_breaker_output.vi (TRUE) (see LB002)
                             - “BasePowerSubsytem->set_state.vi (with “powered on”)
                                       - “State” = “powered on”
                                       - substate = “phase_1”
                                       - “starting time (msec)” = now
             - PS_init.lvclass:PS_process_telemetry.vi - EMPTY
             - PS_Powered_Off.lvclass:PS_process_telemetry.vi - EMPTY
         - CommPowerSubsystem used with CST in call to BasePowerSubsystem->process_telemetry.vi
           - Calls PSS_State.lvclass:PS_process_telemetry.vi  SAME as (see LB015) except CST
         -  PS_GeneralHealth->process_telemetry.vi
              [ “Power Subsystem Common Telemetry” (shortening to PSCT)]
             - if (“PCST.Redundancy OK” AND “PCST.Load Distribution OK”)
                 -FALSE [if either one was false, it’s a fault]
                     - “Fault Status” set bit “power supply load share error”
                 -TRUE [do nothing]
             - if ((!“PCST.P/S 1 DC OK” OR !“PCST.P/S 2 DC OK”)
                   OR (“Boost Current Fault Enabled”
                          AND (“PCST.P/S 1 Boost Current On” OR “PCST.P/S 2 Boost Current On”)))
                -TRUE [if any are not OK or (boost fault is enabled and anything has boost on]
                    - “Fault Status” set bit “power supply health fault”
                    &&& */

void PowerSubsystem::_sendFaultMgrError() {
    LERROR("PowerSubsystem::", __func__, " PLACEHOLDER NEEDS CODE");
}


void PowerSubsystem::_sendFaultMgrError(int errId, std::string note) {
    LERROR("PowerSubsystem::", __func__, " PLACEHOLDER NEEDS CODE");
}

void PowerSubsystem::_sendFaultMgrWarn() {
    LERROR("PowerSubsystem::", __func__, " PLACEHOLDER NEEDS CODE");
}

void PowerSubsystem::_sendFaultMgrSetBit(int bitPos) {
    LERROR("PowerSubsystem::", __func__, " PLACEHOLDER NEEDS CODE");
}

}  // namespace control
}  // namespace m2cellcpp
}  // namespace LSST
