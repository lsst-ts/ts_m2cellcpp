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
    for (Feed::Ptr const& feed:_feeds) {
        auto [status, str] = feed->checkBreakers(info.inputPort);
        inactiveInputs += str;
        /// Result should contain the worst SysStatus found.
        if (status < result) {
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
    else if (count == 2) breakerStatus = WARN;
    if (bitmap != _feedBitmap) {
        LDEBUG("BreakerStatus change to ", (int)bitmap, " from ", (int)_feedBitmap,
              " status=", getSysStatusStr(breakerStatus), " low inputs=", inactiveStr);
    }
    _feedBitmap = bitmap;
    return make_tuple(breakerStatus, inactiveStr);
}

std::tuple<SysStatus, std::string> PowerSubsystemConfig::checkBreakers(SysInfo sysInfo) {
    if (_breakerFeedGroup == nullptr) {
        return make_tuple(SysStatus::FAULT, "nullptr");
    }
    return _breakerFeedGroup->checkBreakers(sysInfo);
}


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


PowerSubsystem::~PowerSubsystem() {
    setPowerOff(__func__);
}


PowerSubsystem::PowerSubsystem(PowerSystemType sysType) : _systemType(sysType), _psCfg(_systemType),
        _fpgaIo(FpgaIo::getPtr()) {
    setPowerOff(__func__);
}

double PowerSubsystem::getVoltage() const {
    switch (_systemType) {
    case MOTOR: return _sysInfo.motorVoltage;
    case COMM: return _sysInfo.commVoltage;
    }
    LERROR(getClassName(), " getVoltage unexpected _systemType=", _systemType, " ", getPowerSystemTypeStr(_systemType));
    return 0.0;
}

double PowerSubsystem::getCurrent() const {
    switch (_systemType) {
    case MOTOR: return _sysInfo.motorCurrent;
    case COMM: return _sysInfo.commCurrent;
    }
    LERROR(getClassName(), " getCurrent unexpected _systemType=", _systemType, " ", getPowerSystemTypeStr(_systemType));
    return 0.0;
}


bool PowerSubsystem::_getRelayControlOutputOn() const {
    /// “Relay Control Output On”
    bool relayControlOutputOn = _sysInfo.outputPort.getBitAtPos(_psCfg.getOutputPowerOnBitPos());
    return relayControlOutputOn;
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
        // active low signal.
        interlockRelayControlOutputOn = !(_sysInfo.inputPort.getBitAtPos(InputPortBits::INTERLOCK_POWER_RELAY));
        return interlockRelayControlOutputOn;
    case COMM:
        // DAQ_to_comm_telemetry.vi
        return true;
        break;
    }
    throw util::Bug(ERR_LOC, string(__func__) + " unexpected systemType");
}


bool PowerSubsystem::_powerShouldBeOn() {
    // BasePowerOutput->output_should_be_on.vi
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

string PowerSubsystem::_getPowerShouldBeOnStr() {
    string str = getClassName() + " _powerShouldBeOn() relay=" + to_string(_getRelayControlOutputOn())
                 + " cRioReady=" + to_string(_getCrioReadyOutputOn())
                 + " interlock=" + to_string(_getInterlockRelayControlOutputOn());
    return str;
}

PowerSubsystem::PowerState PowerSubsystem::getActualPowerState() const {
    VMUTEX_NOT_HELD(_powerStateMtx);
    lock_guard<util::VMutex> lg(_powerStateMtx);
    return _actualPowerState;
}

PowerSubsystem::PowerState PowerSubsystem::getTargPowerState() const {
    VMUTEX_NOT_HELD(_powerStateMtx);
    lock_guard<util::VMutex> lg(_powerStateMtx);
    return _targPowerState;
}


void PowerSubsystem::setPowerOn() {
    VMUTEX_NOT_HELD(_powerStateMtx);
    auto outputPort = _fpgaIo->getOutputPort();
    lock_guard<util::VMutex> lg(_powerStateMtx);
    _setPowerOn();
}

void PowerSubsystem::_setPowerOn() {
    VMUTEX_HELD(_powerStateMtx);

    if (_checkForFaults()) {
        LERROR(getClassName(), " _setPowerOn cannot turn on due to faults");
        _sendFaultMgrError(500003, "Internal ERROR: Faults preventing operation to proceed");
        _setPowerOff("fault during _setPowerOn");
        return;
    }

    if (!_getCrioReadyOutputOn()) {
        LERROR("_setPowerOn() cannot turn due to CRIO_INTERLOCK_ENABLE");
        _setPowerOff("_setPowerOn called without CRIO_INTERLOCK_ENABLE");
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


void PowerSubsystem::setPowerOff(std::string const& note) {
    VMUTEX_NOT_HELD(_powerStateMtx);
    lock_guard<util::VMutex> lg(_powerStateMtx);
    _setPowerOff(note);
}

void PowerSubsystem::_setPowerOff(std::string const& note) {
    VMUTEX_HELD(_powerStateMtx);

    LINFO(getClassName(), " Turning power off ", note);
    _fpgaIo->writeOutputPortBitPos(_psCfg.getOutputBreakerBitPos(), true);
    _fpgaIo->writeOutputPortBitPos(_psCfg.getOutputPowerOnBitPos(), false);
    if (_targPowerState != OFF) {
        _targPowerState = OFF;
        _phase = 1;
        _powerOffStart = util::CLOCK::now();
        _phaseStartTime = _powerOffStart;
    }
}


SysStatus PowerSubsystem::processDaq(SysInfo const& info) {
    _sysInfo = info;

    // Check for faults
    bool systemFaults = _checkForFaults();
    if (systemFaults) {
        _setPowerOff("processDaq had system faults");
    }

    if (_targPowerStatePrev != _targPowerState
        || _actualPowerStatePrev != _actualPowerState) {
        LINFO(getClassName(), " power state change prev(targ=", getPowerStateStr(_targPowerStatePrev),
            " act=", getPowerStateStr(_actualPowerStatePrev),
            ") new(targ=", getPowerStateStr(_targPowerState), " act=", getPowerStateStr(_actualPowerState), ")");
    }
    _targPowerStatePrev = _targPowerState;
    _actualPowerStatePrev = _actualPowerState;

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
        _setPowerOff("processDaq had unexpected _targPowerState=" + getPowerStateStr(_targPowerState));
        [[fallthrough]];
    case OFF: [[fallthrough]]; // OFF is the default.
    default:
        _processPowerOff();
    }


    return FAULT;
}


bool PowerSubsystem::_checkForPowerOnBreakerFault(double voltage) {
    // Breakers only matter (and have correct input) when voltage is
    // above `_breakerOperatingVoltage`, there's been time for them
    // to stabilize, and `_targPowerOn` is true.

    // Is the voltage high enough to to check the breakers? breaker_status_is_Active.vi
    if (voltage >= _psCfg.getBreakerOperatingVoltage()) {
        SysStatus breakerStatus;
        string inactiveInputs;
        tie(breakerStatus, inactiveInputs) = _psCfg.checkBreakers(_sysInfo);
        if (breakerStatus == GOOD) {
            return false; // no faults
        }
        if (breakerStatus <= FAULT) {
            LWARN(getClassName(), " _checkForPowerOnBreakerFault a breakerStatus=", breakerStatus,
                    " ", getSysStatusStr(breakerStatus), " inactiveInputs=", inactiveInputs);
            _sendFaultMgrSetBit(_psCfg.getBreakerFault()); //"breaker fault"
            _setPowerOff(string(__func__) + " breaker fault");
            return true;
        }
        _sendFaultMgrSetBit(_psCfg.getBreakerWarn()); //"breaker warning"
        return false; // no faults, just warnings
    }

    // The power should be on and stable by this point.
    _sendBreakerVoltageFault();
    return true;
}

void PowerSubsystem::_sendBreakerVoltageFault() {
    _sendFaultMgrSetBit(_psCfg.getVoltageFault()); // "voltage fault"
    _sendFaultMgrSetBit(faultmgr::FaultStatusBits::HARDWARE_FAULT); // "hardware fault"
    _setPowerOff(__func__);
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
        _setPowerOff(string(__func__) + "voltage too high");
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
            LDEBUG(_getPowerShouldBeOnStr(), " ON");
            _setPowerOff(string(__func__) + " output is not on when it should be on");
            return;
        }

        if (_checkForPowerOnBreakerFault(voltage)) {
            LERROR("Breaker fault while _actualPowerState == ON");
            _setPowerOff(string(__func__) + " Breaker fault _actualPowerState == ON");
            return;
        }

        if (_phase <= 1) {
            LDEBUG(getClassName(), " ON phase 1");
            // ouput_voltage_is_stable.vi
            double timeSincePhaseStartInSec = util::timePassedSec(_phaseStartTime, now);
            double minTimeToStablize = _psCfg.getVoltageSettlingTime() - _psCfg.getBreakerOperatingVoltageRiseTime();
            if(timeSincePhaseStartInSec > minTimeToStablize) {
                _phase = 2; // advance to next phase.
                _phaseStartTime = util::CLOCK::now();
                LINFO(getClassName(), " ON phase 2 reached");
            }
        } else if (_phase == 2) {
            if (voltage < _psCfg.getMinVoltageWarn()) {
                LWARN(getClassName(), " voltage(", voltage, ") below warning level ", _psCfg.getMinVoltageWarn());
                _sendFaultMgrWarn();
            }
            if (voltage < _psCfg.getMinVoltageFault()) {
                LWARN(getClassName(), " voltage(", voltage, ") below fault level ", _psCfg.getMinVoltageFault());
                _sendFaultMgrError();
                _setPowerOff(string(__func__) + " voltage too low");
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
        double timeSincePhaseStartInSec = util::timePassedSec(_phaseStartTime, now);
        LDEBUG(getClassName(), " TURNING_ON phase=", _phase, " timeInPhase=", timeSincePhaseStartInSec,
               " telemCount=", _telemCounter);
        if (_phase == 1) {
            // Presumably, waiting for the signal to stabilize.
            ++_telemCounter;
            // Value of 10 comes from telemetry_is_stable.vi, with no explanation
            // in LabView for the source of the value or its direct purpose.
            if (_telemCounter >= 10) {
                _phase = 2; // advance to the next phase
                _phaseStartTime = now;
                LINFO(getClassName(), " TURNING_ON moved to phase 2");
            }
        }

        if (_phase > 1 && !outputIsOn) {
            // At this point, the power output bits should be on. If they
            // aren't, give up and turn off power.
            if (!outputIsOn) {
                LDEBUG(_getPowerShouldBeOnStr(), " ON phase=", _phase);
                _setPowerOff(string(__func__) + " TURNING_ON and not outputIsOn");
                return;
            }
        }

        if (_phase == 2) {
            LDEBUG(getClassName(), " phase 2 timeInPhase=", timeSincePhaseStartInSec, " wait=", _psCfg.outputOnMaxDelay());
            if (timeSincePhaseStartInSec > _psCfg.outputOnMaxDelay()) {
                _phase = 3; // advance to the next phase
                _phaseStartTime = now;
                LINFO(getClassName(), " TURNING_ON moved to phase 3");
            } else {
                return;
            }
        }

        if (_phase >= 3) {
            LDEBUG(getClassName(), " phase 3");
            // If the voltage isn't high enough to read the breakers, give up, turn power off
            if (voltage < _psCfg.getBreakerOperatingVoltage()) {
                LERROR(getClassName(), " TURNING_ON voltage too low volt=", voltage);
                _sendBreakerVoltageFault(); // calls _setPowerOff()
                return;
            }
            // If the breakers have any faults or warnings, try to reset them.
            auto [breakerStatus, inactiveInputs] = _psCfg.checkBreakers(_sysInfo);
            LDEBUG(getClassName(), " phase 3 breaker=", breakerStatus, " ", inactiveInputs);
            if (breakerStatus == GOOD) {
                _actualPowerState = ON;
                _phase = 1;
                _phaseStartTime = now;
                LINFO(getClassName(), " is now ON");
                return;
            } else {
                _actualPowerState = RESET;
                _phaseStartTime = now;
                _phase = 1;
                LWARN(getClassName(), " breaker RESET starting");
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
            LDEBUG(_getPowerShouldBeOnStr(), " breaker RESET");
            _fpgaIo->writeOutputPortBitPos(_psCfg.getOutputBreakerBitPos(), true);
            _setPowerOff(string(__func__) + " cannot RESET breakers when not outputIsOn");
            return;
        }
        if (voltage < _psCfg.getBreakerOperatingVoltage()) {
            _sendBreakerVoltageFault(); // calls _setPowerOff()
            return;
        }
        double timeSincePhaseStartInSec = util::timePassedSec(_phaseStartTime, now);
        if (timeSincePhaseStartInSec > _psCfg.getResetBreakerPulseWidth()) {
            // Enough time passed, restore breaker the output
            LINFO(getClassName(), " breaker RESET restoring");
            _fpgaIo->writeOutputPortBitPos(_psCfg.getOutputBreakerBitPos(), true);
            // Need to wait longer for faults to clear
            if (timeSincePhaseStartInSec > (_psCfg.getResetBreakerPulseWidth() * 2.0)) {
                // Check breakers again (fault sets power to off).
                if (_checkForPowerOnBreakerFault(voltage)) {
                    LERROR("Breaker RESET fault while _actualPowerState == RESET");
                    return;
                }
                // It worked, set power on.
                _actualPowerState = ON;
                _phaseStartTime = now;
                LINFO(getClassName(), " breaker RESET success");
                return;
            }
        }
        break;
    }
    default:
        string eMsg = string(" unexpected _actualPowerState=") + getPowerStateStr(_actualPowerState);
        LERROR(getClassName(), eMsg);
        _setPowerOff(string(__func__) + eMsg);
    }
}


void PowerSubsystem::_processPowerOff() {
    VMUTEX_HELD(_powerStateMtx);

    bool powerOn = _getRelayControlOutputOn();
    if (powerOn) {
        _setPowerOff(string(__func__) + " need to unset powerOn bit");
        return;
    }

    util::TIMEPOINT now = util::CLOCK::now();
    double voltage = getVoltage();
    switch (_actualPowerState) {
    case UNKNOWN: [[fallthrough]];
    case RESET: [[fallthrough]];
    case ON: [[fallthrough]];
    case TURNING_ON:
        _setPowerOff(string(__func__) + " _actualPowerState was not TURNING_OFF or OFF");
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
                _setPowerOff(string(__func__) + " timeout TURNING_OFF");
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


bool PowerSubsystem::_checkForFaults() {
    return faultmgr::FaultMgr::get().checkForPowerSubsystemFaults(_psCfg.getSubsystemFaultMask(), getClassName());
}

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
