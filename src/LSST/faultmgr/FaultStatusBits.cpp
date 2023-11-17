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
#include "faultmgr/FaultStatusBits.h"

#include <bitset>
#include <sstream>
#include <mutex>
#include <stdexcept>
#include <string>

// Project headers
#include "util/Bug.h"

using namespace std;

namespace LSST {
namespace m2cellcpp {
namespace faultmgr {

FaultStatusBits::Ptr FaultStatusBits::_maskOpenLoopControl;
FaultStatusBits::Ptr FaultStatusBits::_maskTelemetryOnlyControl;
FaultStatusBits::Ptr FaultStatusBits::_maskFaults;
FaultStatusBits::Ptr FaultStatusBits::_maskWarn;
FaultStatusBits::Ptr FaultStatusBits::_maskInfo;

FaultStatusBits::Ptr FaultStatusBits::_maskSubsystemCommFault;
FaultStatusBits::Ptr FaultStatusBits::_maskSubsystemMotorFault;

FaultStatusBits::Ptr FaultStatusBits::_telemetryFaultManagerAffectedFaultMask;
FaultStatusBits::Ptr FaultStatusBits::_telemetryFaultManagerAffectedWarningMask;

FaultStatusBits::Ptr FaultStatusBits::_powerSubsystemFaultManagerAffectedFaultMask;
FaultStatusBits::Ptr FaultStatusBits::_powerSubsystemFaultManagerAffectedWarningMask;

FaultStatusBits::Ptr FaultStatusBits::_healthFaultMask;

/// `faultMaskCreationMtx` is only needed during the creation of fault
/// status masks. There's a slight race condition when creating masks
/// where more than one thread could get past the if ( == nullptr) latch.
/// This mutex prevents that from being an issue. If the pointer has not yet
/// been set, this mutex is locked and the pointer is checked a second
/// time. Only if the pointer is still nullptr then the pointer will be set.
/// Once the pointer is set, there should be no need to ever lock the mutex
/// again, so it it should have no performance penalty.
mutex faultMaskCreationMtx;

void FaultStatusBits::setBit64(uint64_t& bitmap, int pos, bool set)  {
    if (pos < 0 || pos >= 64) {
        throw std::range_error("setBit64 out of range pos=" + to_string(pos));
    }
    uint64_t bit = 1;
    bit <<= pos;
    if (set) {
        bitmap |= bit;
    } else {
        bitmap &= ~bit;
    }
}

void FaultStatusBits::setBit32(uint32_t& bitmap, int pos, bool set)  {
    if (pos < 0 || pos >= 32) {
        throw std::range_error("setBit32 out of range pos=" + to_string(pos));
    }
    uint32_t bit = 1;
    bit <<= pos;
    if (set) {
        bitmap |= bit;
    } else {
        bitmap &= ~bit;
    }
}

void FaultStatusBits::setBit8(uint8_t& bitmap, int pos, bool set)  {
    if (pos < 0 || pos >= 8) {
        throw std::range_error("setBit8 out of range pos=" + to_string(pos));
    }
    uint8_t bit = 1;
    bit <<= pos;
    if (set) {
        bitmap |= bit;
    } else {
        bitmap &= ~bit;
    }
}


void FaultStatusBits::setBitAt(int pos) {
    setBit64(_bitmap, pos, true);
}

void FaultStatusBits::unsetBitAt(int pos) {
    setBit64(_bitmap, pos, false);
}


uint64_t FaultStatusBits::getBit(int pos) {
    if (pos < 0 || pos >= 64 ) {
        return 0;
    }
    uint64_t mask = 1;
    mask <<= pos;
    return getBitsSetInMask(mask);
}

uint64_t FaultStatusBits::getMaskOpenLoopControl() {
    if (_maskOpenLoopControl == nullptr) {
        FaultStatusBits::Ptr fsm(new FaultStatusBits(getMaskClosedLoopControl()));
        lock_guard<mutex> lg(faultMaskCreationMtx);
        if (_maskOpenLoopControl == nullptr) {
            // - "open-loop control mask":
            fsm->setBitAt(ACTUATOR_LIMIT_CL);          // “Actuator limit CL”
            fsm->setBitAt(INCLINOMETER_W_LUT);         // “Inclinometer error w/ lut”
            fsm->setBitAt(CRIO_TIMING_FAULT);          // “cRIO timing fault”
            fsm->setBitAt(INCLINOMETER_RANGE_ERR);     // “Inclinometer range error”
            fsm->setBitAt(MIRROR_TEMP_SENSOR_FAULT);   // “mirror temp sensor fault”
            fsm->setBitAt(ELEVATION_ANGLE_DIFF_FAULT); // “elevation angle difference error fault”
            _maskOpenLoopControl = fsm;
        }
    }
    return  _maskOpenLoopControl->_bitmap;
}

uint64_t FaultStatusBits::getMaskTelemetryOnlyControl() {
    if (_maskTelemetryOnlyControl == nullptr) {
        FaultStatusBits::Ptr fsm(new FaultStatusBits(getMaskOpenLoopControl()));
        lock_guard<mutex> lg(faultMaskCreationMtx);
        if (_maskTelemetryOnlyControl == nullptr) {
            // - "telemetry-only control mask" - all of the _maskOpenLoopControl and
            fsm->setBitAt(FaultStatusBits::ACTUATOR_FAULT);             // “actuator fault”
            fsm->setBitAt(FaultStatusBits::EXCESSIVE_FORCE);            // “excessive force”
            fsm->setBitAt(FaultStatusBits::MOTOR_VOLTAGE_FAULT);        // “motor voltage error fault”
            fsm->setBitAt(FaultStatusBits::MOTOR_OVER_CURRENT);         // “Motor over current”
            fsm->setBitAt(FaultStatusBits::MOTOR_MULTI_BREAKER_FAULT);  // “Motor mult-breaker fault”
            fsm->setBitAt(FaultStatusBits::AXIAL_ACTUATOR_ENCODER_RANGE_FAULT); // “Axial actuator encoder range fault”
            fsm->setBitAt(FaultStatusBits::TANGENT_ACTUATOR_ENCODER_RANGE_FAULT); // “tangent actuator encoder range fault”
            fsm->setBitAt(FaultStatusBits::ILC_STATE_TRANSITION_FAULT); // “ILC state transition error fault”
            _maskTelemetryOnlyControl = fsm;
        }
    }

    return _maskTelemetryOnlyControl->_bitmap;
}

uint64_t FaultStatusBits::getMaskFaults() {
    if (_maskFaults == nullptr) {
        FaultStatusBits::Ptr fsm(new FaultStatusBits(getMaskTelemetryOnlyControl()));
        lock_guard<mutex> lg(faultMaskCreationMtx);
        if (_maskFaults == nullptr) {
            // - "Faults Mask" - all of the faults in getMaskTelemetryOnlyControl and the following
            fsm->setBitAt(FaultStatusBits::COMM_VOLTAGE_FAULT);          // “comm voltage error fault”
            fsm->setBitAt(FaultStatusBits::COMM_OVER_CURRENT);           // “comm over current”
            fsm->setBitAt(FaultStatusBits::POWER_RELAY_OPEN_FAULT);      // “power relay open fault”
            fsm->setBitAt(FaultStatusBits::POWER_HEALTH_FAULT);          // “power supply health fault”
            fsm->setBitAt(FaultStatusBits::COMM_MULTI_BREAKER_FAULT);    // “comm multi-breaker fault”
            fsm->setBitAt(FaultStatusBits::POWER_SUPPLY_LOAD_SHARE_ERR); // “power supply load share error”
            fsm->setBitAt(FaultStatusBits::INTERLOCK_FAULT);             // “interlock fault”
            fsm->setBitAt(FaultStatusBits::TANGENT_LOAD_CELL_FAULT);     // “tangent load cell fault”
            fsm->setBitAt(FaultStatusBits::LOSS_OF_TMA_COMM_ON_ENABLE_FAULT); // “loss of TMA comm on ENABLE fault”
            fsm->setBitAt(FaultStatusBits::CRIO_COMM_FAULT);             // “cRIO COMM error fault”
            fsm->setBitAt(FaultStatusBits::USER_GENERATED_FAULT);        // “user generated fault”
            fsm->setBitAt(FaultStatusBits::PARAMETER_FILE_READ_FAULT);   // “configurable parameter file read error fault”
            fsm->setBitAt(FaultStatusBits::POWER_SYSTEM_TIMEOUT);        // power system timeout
            _maskFaults = fsm;
        }
    }
    return _maskFaults->_bitmap;
}

uint64_t FaultStatusBits::getMaskWarn() {
    if (_maskWarn == nullptr) {
        FaultStatusBits::Ptr fsm(new FaultStatusBits(0));
        lock_guard<mutex> lg(faultMaskCreationMtx);
        if (_maskWarn == nullptr) {
            // - Warnings Mask - starting clean with all zeroes
            fsm->setBitAt(ACTUATOR_LIMIT_OL);       // “actuator limit OL”
            fsm->setBitAt(INCLINOMETER_WO_LUT);     // “inclinometer error w/o lut”
            fsm->setBitAt(MOTOR_VOLTAGE_WARN);      // “motor voltage error warning”
            fsm->setBitAt(COMM_VOLTAGE_WARN);       // “comm voltage error warning”
            fsm->setBitAt(SINGLE_BREAKER_TRIP);     // “single breaker trip”
            fsm->setBitAt(CRIO_TIMING_WARN);        // “cRIO timing warning”
            fsm->setBitAt(DISPLACEMENT_SENSOR_RANGE_ERR); // “displacement sensor range”
            fsm->setBitAt(MIRROR_TEMP_SENSOR_WARN); // “mirror temp sensor warning”
            fsm->setBitAt(CELL_TEMP_WARN);          // “cell temp warning”
            fsm->setBitAt(TEMP_DIFF_WARN);          // “excessive temperature differential warning”
            fsm->setBitAt(LOSS_OF_TMA_WARN);        // “loss of TMA comm warning”
            fsm->setBitAt(MONITOR_ILC_READ_WARN);   // “monitoring ILC read error warning”
            _maskWarn = fsm;
        }
    }
    return _maskWarn->_bitmap;
}

uint64_t FaultStatusBits::getMaskInfo() {
    if (_maskInfo == nullptr) {
        FaultStatusBits::Ptr fsm(new FaultStatusBits(0));
        lock_guard<mutex> lg(faultMaskCreationMtx);
        if (_maskInfo == nullptr) {
           // - Info Mask - starting with all zeroes
           fsm->setBitAt(BROADCAST_ERR);    // “broadcast error”
           fsm->setBitAt(MOTOR_RELAY);      // “motor relay”
           fsm->setBitAt(COMM_RELAY);       // “comm relay”
           fsm->setBitAt(HARDWARE_FAULT);   // “hardware fault”
           fsm->setBitAt(STALE_DATA_WARN);  // “stale data warning”
           fsm->setBitAt(STALE_DATA_FAULT); // “stale data fault”
           // The LabView code indicates it may unset some values, but doesn't seem to do so.
           _maskInfo = fsm;
        }
    }
    return _maskInfo->_bitmap;
}

uint64_t FaultStatusBits::getMaskPowerSubsystemFaults(control::PowerSystemType sysType) {
    switch (sysType) {
    case control::COMM:
    {
        if (_maskSubsystemCommFault == nullptr) {
            FaultStatusBits::Ptr fsm(new FaultStatusBits(0));
            lock_guard<mutex> lg(faultMaskCreationMtx);
            if (_maskSubsystemCommFault == nullptr) {
                // - comm subsystem fault mask - starting with all zeroes
                fsm->setBitAt(FaultStatusBits::COMM_VOLTAGE_FAULT); // "comm voltage error fault"
                fsm->setBitAt(FaultStatusBits::COMM_OVER_CURRENT); // "comm over current"
                fsm->setBitAt(FaultStatusBits::POWER_RELAY_OPEN_FAULT); // "power relay open fault"
                fsm->setBitAt(FaultStatusBits::COMM_MULTI_BREAKER_FAULT); // "comm multi-breaker fault"
                _maskSubsystemCommFault = fsm;
            }
        }
        return _maskSubsystemCommFault->_bitmap;
    }
    case control::MOTOR:
    {
        if (_maskSubsystemMotorFault == nullptr) {
            FaultStatusBits::Ptr fsm(new FaultStatusBits(0));
            lock_guard<mutex> lg(faultMaskCreationMtx);
            if (_maskSubsystemMotorFault == nullptr) {
                // - motor subsystem fault mask - starting with all zeroes
                fsm->setBitAt(FaultStatusBits::MOTOR_VOLTAGE_FAULT); // "motor voltage error fault"
                fsm->setBitAt(FaultStatusBits::MOTOR_OVER_CURRENT); // "motor over current"
                fsm->setBitAt(FaultStatusBits::POWER_RELAY_OPEN_FAULT); // "power relay open fault"
                fsm->setBitAt(FaultStatusBits::MOTOR_MULTI_BREAKER_FAULT); // "motor multi-breaker fault"
                _maskSubsystemMotorFault = fsm;
            }

        }
        return _maskSubsystemMotorFault->_bitmap;
    }
    }
    throw util::Bug(ERR_LOC, string("FaultStatusBits::getMaskPowerSubsystemFaults ")
            + getPowerSystemTypeStr(sysType) + " unexpected type");
}


uint64_t FaultStatusBits::getTelemetryFaultManagerAffectedFaultMask() {
    if (_telemetryFaultManagerAffectedFaultMask == nullptr) {
        // Starts with nothing set
        FaultStatusBits::Ptr fsm(new FaultStatusBits());
        lock_guard<mutex> lg(faultMaskCreationMtx);
        if (_telemetryFaultManagerAffectedFaultMask == nullptr) {
            // - "Affected Fault Mask":
            fsm->setBitAt(ACTUATOR_LIMIT_CL);          // “Actuator limit CL”
            fsm->setBitAt(INCLINOMETER_W_LUT);         // “Inclinometer error w/ lut”
            fsm->setBitAt(INCLINOMETER_RANGE_ERR);     // “Inclinometer range error”
            fsm->setBitAt(MIRROR_TEMP_SENSOR_FAULT);   // “mirror temp sensor fault”
            fsm->setBitAt(ELEVATION_ANGLE_DIFF_FAULT); // “elevation angle difference error fault”
            fsm->setBitAt(ACTUATOR_FAULT);             // “actuator fault”
            fsm->setBitAt(EXCESSIVE_FORCE);            // “excessive force”
            fsm->setBitAt(AXIAL_ACTUATOR_ENCODER_RANGE_FAULT);   // “axial actuator encoder range fault”
            fsm->setBitAt(TANGENT_ACTUATOR_ENCODER_RANGE_FAULT); // “tangent actuator encoder range fault”
            fsm->setBitAt(TANGENT_LOAD_CELL_FAULT);    // “tangent load cell fault”
            _telemetryFaultManagerAffectedFaultMask = fsm;
        }
    }
    return  _telemetryFaultManagerAffectedFaultMask->_bitmap;
}

uint64_t FaultStatusBits::getTelemetryFaultManagerAffectedWarningMask() {
    if (_telemetryFaultManagerAffectedWarningMask == nullptr) {
        // Starts with nothing set
        FaultStatusBits::Ptr fsm(new FaultStatusBits());
        lock_guard<mutex> lg(faultMaskCreationMtx);
        if (_telemetryFaultManagerAffectedWarningMask == nullptr) {
            // - "Affected Warning Mask":
            // warnings
            fsm->setBitAt(MONITOR_ILC_READ_WARN);         // “monitoring ILC read error warning”
            fsm->setBitAt(ACTUATOR_LIMIT_OL);             // “actuator limit OL” opened-loop
            fsm->setBitAt(INCLINOMETER_WO_LUT);           // “inclinometer error w/o lut”
            fsm->setBitAt(DISPLACEMENT_SENSOR_RANGE_ERR); // “displacement sensor range error”
            fsm->setBitAt(MIRROR_TEMP_SENSOR_WARN);       // “mirror temp sensor warning”
            fsm->setBitAt(CELL_TEMP_WARN);                // “cell temp warning”
            // info
            fsm->setBitAt(BROADCAST_ERR);    // “broadcast error”
            fsm->setBitAt(STALE_DATA_WARN);  // “stale data warning”
            fsm->setBitAt(STALE_DATA_FAULT); // “stale data fault”
            _telemetryFaultManagerAffectedWarningMask = fsm;
        }
    }
    return  _telemetryFaultManagerAffectedWarningMask->_bitmap;
}

uint64_t FaultStatusBits::getPowerSubsystemFaultManagerAffectedFaultMask() {
    if (_powerSubsystemFaultManagerAffectedFaultMask == nullptr) {
        // Starts with nothing set
        FaultStatusBits::Ptr fsm(new FaultStatusBits());
        lock_guard<mutex> lg(faultMaskCreationMtx);
        if (_powerSubsystemFaultManagerAffectedFaultMask == nullptr) {
            // - "Affected Fault Mask":
            fsm->setBitAt(MOTOR_VOLTAGE_FAULT);         // “motor voltage error fault”
            fsm->setBitAt(MOTOR_OVER_CURRENT);          // “motor over current”
            fsm->setBitAt(MOTOR_MULTI_BREAKER_FAULT);   // “motor multi-breaker fault”
            fsm->setBitAt(COMM_VOLTAGE_FAULT);          // “comm voltage error fault”
            fsm->setBitAt(COMM_OVER_CURRENT);           // “comm over current”
            fsm->setBitAt(POWER_RELAY_OPEN_FAULT);      // “power relay open fault”
            fsm->setBitAt(POWER_HEALTH_FAULT);          // “power supply health fault”
            fsm->setBitAt(COMM_MULTI_BREAKER_FAULT);    // “comm multi-breaker fault”
            fsm->setBitAt(POWER_SUPPLY_LOAD_SHARE_ERR); // “power supply load share error”
            fsm->setBitAt(INTERLOCK_FAULT);             // “interlock fault”
            fsm->setBitAt(POWER_SYSTEM_TIMEOUT);
            _powerSubsystemFaultManagerAffectedFaultMask = fsm;
        }
    }
    return  _powerSubsystemFaultManagerAffectedFaultMask->_bitmap;
}


uint64_t FaultStatusBits::getPowerSubsystemFaultManagerAffectedWarningMask() {
    if (_powerSubsystemFaultManagerAffectedWarningMask == nullptr) {
        // Starts with nothing set
        FaultStatusBits::Ptr fsm(new FaultStatusBits());
        lock_guard<mutex> lg(faultMaskCreationMtx);
        if (_powerSubsystemFaultManagerAffectedWarningMask == nullptr) {
            // - "Affected Warning Mask":
            // warnings
            fsm->setBitAt(MOTOR_VOLTAGE_WARN);  // “motor voltage error warning”
            fsm->setBitAt(COMM_VOLTAGE_WARN);   // “comm voltage error warning”
            fsm->setBitAt(SINGLE_BREAKER_TRIP); // “single breaker trip”
            // info
            fsm->setBitAt(MOTOR_RELAY);    // “motor relay”
            fsm->setBitAt(COMM_RELAY);     // “comm relay”
            fsm->setBitAt(HARDWARE_FAULT); // “hardware fault”
            _powerSubsystemFaultManagerAffectedWarningMask = fsm;
        }
    }
    return  _powerSubsystemFaultManagerAffectedWarningMask->_bitmap;
}

uint64_t FaultStatusBits::getMaskHealthFaults() {
    if (_healthFaultMask == nullptr) {
        // Starts with nothing set
        FaultStatusBits::Ptr fsm(new FaultStatusBits());
        lock_guard<mutex> lg(faultMaskCreationMtx);
        if (_healthFaultMask == nullptr) {
            fsm->setBitAt(faultmgr::FaultStatusBits::POWER_HEALTH_FAULT);
            fsm->setBitAt(faultmgr::FaultStatusBits::POWER_SUPPLY_LOAD_SHARE_ERR);
            _healthFaultMask = fsm;
        }
    }
    return _healthFaultMask->_bitmap;
}

string FaultStatusBits::getAllSetBitEnums() const {
    string str;
    uint64_t mask = 1;

    for (int j=0; j<64; ++j) {
        if (getBitsSetInMask(mask)) {
            str += getEnumString(j) + ",";
        }
         mask <<= 1;
    }
    return str;
}

string FaultStatusBits::getBinaryStr(uint64_t val) {
    stringstream os;
    os << std::bitset<64>(val);
    return os.str();
}

string FaultStatusBits::getEnumString(int enumVal) {
    switch(enumVal) {
    case STALE_DATA_WARN: return "STALE_DATA_WARN " + to_string(enumVal);
    case STALE_DATA_FAULT: return "STALE_DATA_FAULT " + to_string(enumVal);
    case BROADCAST_ERR: return "BROADCAST_ERR " + to_string(enumVal);
    case ACTUATOR_FAULT: return "ACTUATOR_FAULT " + to_string(enumVal);
    case EXCESSIVE_FORCE: return "EXCESSIVE_FORCE " + to_string(enumVal);
    case ACTUATOR_LIMIT_OL: return "ACTUATOR_LIMIT_OL " + to_string(enumVal);
    case ACTUATOR_LIMIT_CL: return "ACTUATOR_LIMIT_CL " + to_string(enumVal);
    case INCLINOMETER_W_LUT: return "INCLINOMETER_W_LUT " + to_string(enumVal);
    case INCLINOMETER_WO_LUT: return "INCLINOMETER_WO_LUT " + to_string(enumVal);
    case MOTOR_VOLTAGE_FAULT: return "MOTOR_VOLTAGE_FAULT " + to_string(enumVal);
    case MOTOR_VOLTAGE_WARN: return "MOTOR_VOLTAGE_WARN " + to_string(enumVal);
    case COMM_VOLTAGE_FAULT: return "COMM_VOLTAGE_FAULT " + to_string(enumVal);
    case COMM_VOLTAGE_WARN: return "COMM_VOLTAGE_WARN " + to_string(enumVal);
    case MOTOR_OVER_CURRENT: return "MOTOR_OVER_CURRENT " + to_string(enumVal);
    case COMM_OVER_CURRENT: return "COMM_OVER_CURRENT " + to_string(enumVal);
    case POWER_RELAY_OPEN_FAULT: return "POWER_RELAY_OPEN_FAULT " + to_string(enumVal);
    case POWER_HEALTH_FAULT: return "POWER_HEALTH_FAULT " + to_string(enumVal);
    case COMM_MULTI_BREAKER_FAULT: return "COMM_MULTI_BREAKER_FAULT " + to_string(enumVal);
    case MOTOR_MULTI_BREAKER_FAULT: return "MOTOR_MULTI_BREAKER_FAULT " + to_string(enumVal);
    case SINGLE_BREAKER_TRIP: return "SINGLE_BREAKER_TRIP " + to_string(enumVal);
    case POWER_SUPPLY_LOAD_SHARE_ERR: return "POWER_SUPPLY_LOAD_SHARE_ERR " + to_string(enumVal);
    case DISPLACEMENT_SENSOR_RANGE_ERR: return "DISPLACEMENT_SENSOR_RANGE_ERR " + to_string(enumVal);
    case INCLINOMETER_RANGE_ERR: return "INCLINOMETER_RANGE_ERR " + to_string(enumVal);
    case MIRROR_TEMP_SENSOR_FAULT: return "MIRROR_TEMP_SENSOR_FAULT " + to_string(enumVal);
    case MIRROR_TEMP_SENSOR_WARN: return "MIRROR_TEMP_SENSOR_WARN " + to_string(enumVal);
    case CELL_TEMP_WARN: return "CELL_TEMP_WARN " + to_string(enumVal);
    case AXIAL_ACTUATOR_ENCODER_RANGE_FAULT: return "AXIAL_ACTUATOR_ENCODER_RANGE_FAULT " + to_string(enumVal);
    case TANGENT_ACTUATOR_ENCODER_RANGE_FAULT: return "TANGENT_ACTUATOR_ENCODER_RANGE_FAULT " + to_string(enumVal);
    case MOTOR_RELAY: return "MOTOR_RELAY " + to_string(enumVal);
    case COMM_RELAY: return "COMM_RELAY " + to_string(enumVal);
    case HARDWARE_FAULT: return "HARDWARE_FAULT " + to_string(enumVal);
    case INTERLOCK_FAULT: return "INTERLOCK_FAULT " + to_string(enumVal);
    case TANGENT_LOAD_CELL_FAULT: return "TANGENT_LOAD_CELL_FAULT " + to_string(enumVal);
    case ELEVATION_ANGLE_DIFF_FAULT: return "ELEVATION_ANGLE_DIFF_FAULT " + to_string(enumVal);
    case MONITOR_ILC_READ_WARN: return "MONITOR_ILC_READ_WARN " + to_string(enumVal);
    case PARAMETER_FILE_READ_FAULT: return "PARAMETER_FILE_READ_FAULT " + to_string(enumVal);
    case ILC_STATE_TRANSITION_FAULT: return "ILC_STATE_TRANSITION_FAULT " + to_string(enumVal);
    case CRIO_COMM_FAULT: return "CRIO_COMM_FAULT " + to_string(enumVal);
    case LOSS_OF_TMA_WARN: return "LOSS_OF_TMA_WARN " + to_string(enumVal);
    case LOSS_OF_TMA_COMM_ON_ENABLE_FAULT: return "LOSS_OF_TMA_COMM_ON_ENABLE_FAULT " + to_string(enumVal);
    case TEMP_DIFF_WARN: return "TEMP_DIFF_WARN " + to_string(enumVal);
    case CRIO_TIMING_FAULT: return "CRIO_TIMING_FAULT " + to_string(enumVal);
    case CRIO_TIMING_WARN: return "CRIO_TIMING_WARN " + to_string(enumVal);
    case USER_GENERATED_FAULT: return "USER_GENERATED_FAULT " + to_string(enumVal);
    }
    return "unknown " + to_string(enumVal);
}

}  // namespace control
}  // namespace m2cellcpp
}  // namespace LSST
