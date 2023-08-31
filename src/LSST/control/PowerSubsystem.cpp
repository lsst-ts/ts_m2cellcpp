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
#include "util/Log.h"


using namespace std;

namespace LSST {
namespace m2cellcpp {
namespace control {

void PowerSubsystemConfig::setupMotor() {
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

}


void PowerSubsystemConfig::setupCOMM() {
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
}

void PowerSubsystemConfig::setupCalculated() {
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

}  // namespace control
}  // namespace m2cellcpp
}  // namespace LSST
