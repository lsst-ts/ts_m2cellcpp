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


#ifndef LSST_M2CELLCPP_CONTROL_POWERSUBSYTEM_H
#define LSST_M2CELLCPP_CONTROL_POWERSUBSYTEM_H

// System headers
#include <memory>
#include <stdint.h>
#include <string>
#include <vector>


// Project headers
#include "control/control_defs.h"
#include "control/InputPortBits.h"
#include "control/OutputPortBits.h"
#include "control/SysInfo.h"
#include "faultmgr/FaultStatusBits.h"
#include "util/clock_defs.h"
#include "util/Log.h"
#include "util/VMutex.h"


namespace LSST {
namespace m2cellcpp {
namespace control {

class FpgaIo;

/* PowerSubsystemConfig values based on this.
      - Motor Power Subsystem Configuration Information (typedef 'PowerSubsystem (cluster of 11 elements)
          Values found in “PowerSubsystemCommonConfig.vi”  and “MotorPowerBusConfigurationParameters.vi”
          - U32 output on max delay (ms) (unsigned long [32-bit integer (0 to 4,294,967,295)])  =
              “relay close delay” (50ms) + “breaker on time” (500ms) + “interlock output on delay”(50ms)= 600ms
          - U32 output off max delay (ms) (unsigned long [32-bit integer (0 to 4,294,967,295)])
              “relay open delay” (30ms) + “interlock output off delay”(50ms) = 80ms
          - U32 reset breaker pulse width (ms) (unsigned long [32-bit integer (0 to 4,294,967,295)])  = 400ms
          - DBL breaker operating voltage (double [64-bit real (~15 digit precision)] [V])  = 19V
          - CLUSTER output voltage warning level (volts) (cluster of 2 elements) = output voltage nominal level = 24V
             Based on “output voltage nominal level” * x%   x=”output voltage warning threshold level(%)”= 5%
             - DBL Minimum (double [64-bit real (~15 digit precision)])  = 24V * 0.95 = 22.8
             - DBL Maximum (double [64-bit real (~15 digit precision)])  = 24V * 1.05 = 25.2
          -  CLUSTER output voltage fault level (volts) (cluster of 2 elements)
            Based on “output voltage nominal level” * x%   x=”output voltage fault threshold level(%)”= 10%
              - DBL Minimum (double [64-bit real (~15 digit precision)])  = 24V * 0.90 = 21.6
              - DBL Maximum (double [64-bit real (~15 digit precision)])  = 24V * 1.10 = 26.4
          - U32 breaker operating voltage rise time (ms) (unsigned long [32-bit integer (0 to 4,294,967,295)])  = 85ms
          - U32 output voltage settling time (ms) (unsigned long [32-bit integer (0 to 4,294,967,295)])  = 20ms
          - U32 output voltage fall time (ms) (unsigned long [32-bit integer (0 to 4,294,967,295)])  = 300ms
          - DBL output voltage off level (double [64-bit real (~15 digit precision)] [V])  = 12V
          - DBL maximum output current (double [64-bit real (~15 digit precision)] [A])  = 20A
      - Comm Power Subsystem Configuration Information (typedef 'PowerSubsystem (cluster of 11 elements)
            Values found in “PowerSubsystemCommonConfig.vi”  and “CommPowerBusConfigurationParameters.vi”
         - U32 output on max delay (ms) (unsigned long [32-bit integer (0 to 4,294,967,295)])
              “relay close delay” (50ms) + “breaker on time” (500ms) = 550ms
         - U32 output off max delay (ms) (unsigned long [32-bit integer (0 to 4,294,967,295)])
              “relay open delay” (30ms) = 30ms
         - U32 reset breaker pulse width (ms) (unsigned long [32-bit integer (0 to 4,294,967,295)])  = 400ms
         - DBL breaker operating voltage (double [64-bit real (~15 digit precision)] [V])  = 19V
         - CLUSTER output voltage warning level (volts) (cluster of 2 elements)  = “output voltage nominal level” = 24V
           Based on “output voltage nominal level” * x%   x=”output voltage warning threshold level(%)”= 5%
         - DBL Minimum (double [64-bit real (~15 digit precision)])  =  24V * 0.95 = 22.8
         - DBL Maximum (double [64-bit real (~15 digit precision)])  = 24V * 1.05 = 25.2
         -  CLUSTER output voltage fault level (volts) (cluster of 2 elements)
            Based on “output voltage nominal level” * x%   x=”output voltage fault threshold level(%)”= 10%
         - DBL Minimum (double [64-bit real (~15 digit precision)])  = 24V * 0.90 = 21.6
         - DBL Maximum (double [64-bit real (~15 digit precision)])  = 24V * 1.10 = 26.4
         - U32 breaker operating voltage rise time (ms) (unsigned long [32-bit integer (0 to 4,294,967,295)])  = 30ms
         - U32 output voltage settling time (ms) (unsigned long [32-bit integer (0 to 4,294,967,295)])  = 10ms
         - U32 output voltage fall time (ms) (unsigned long [32-bit integer (0 to 4,294,967,295)])  = 50ms
         - DBL output voltage off level (double [64-bit real (~15 digit precision)] [V])  = 12V
         - DBL maximum output current (double [64-bit real (~15 digit precision)] [A])  = 10A
*/


/// Each PowerSubsystem has a group of breaker hardware inputs that indicate if the power supply
/// is working correctly. The individual bit inputs are kind of a mystery, but fault and warning
/// conditions are defined.
class BreakerFeedGroup {
public:
    using Ptr = std::shared_ptr<BreakerFeedGroup>;

    /// Each breaker group contains three feeds, which each have three binary inputs that are read from
    /// InputPortBits.
    class Feed {
    public:
        using Ptr = std::shared_ptr<Feed>;

        Feed() = delete;
        Feed(int bit0Pos, int bit1Pos, int bit2Pos) : _bit0Pos(bit0Pos), _bit1Pos(bit1Pos), _bit2Pos(bit2Pos) {}

        /// All of the associated bits for the breaker in `input` should be 1.
        /// @return GOOD - if all three bits are 1.
        ///         WARN - if only 2 of the 3 bits are 1.
        ///         FAULT - if less than 2 bits of the 3 are 1
        ///       and a string containing the names of the problem bits.
        std::tuple<control::SysStatus, std::string> checkBreakers(InputPortBits const& input);

    private:
        uint8_t _feedBitmap = 0; /// Representation of imported bits.

        int _bit0Pos; /// InputPortBits bit position for bit 0 of `_feedBitmap`, active high.
        int _bit1Pos; /// InputPortBits bit position for bit 1 of `_feedBitmap`, active high.
        int _bit2Pos; /// InputPortBits bit position for bit 2 of `_feedBitmap`, active high.
    };

    BreakerFeedGroup(Feed::Ptr const& feed1, Feed::Ptr const& feed2, Feed::Ptr const& feed3);
    BreakerFeedGroup() = delete;
    BreakerFeedGroup(BreakerFeedGroup const&) = delete;
    BreakerFeedGroup& operator=(BreakerFeedGroup const&) = delete;

    ~BreakerFeedGroup() = default;

    /// Check the status of the breakers.
    /// @return - SysStatus of the worst Feed checked and a string
    ///     containing the names of the problem Feed input bits as defined in InputPortBits.
    std::tuple<control::SysStatus, std::string> checkBreakers(control::SysInfo const& info);

private:
    Feed::Ptr _feed1; /// First set of breaker inputs.
    Feed::Ptr _feed2; /// Second set of breaker inputs.
    Feed::Ptr _feed3; /// Third set of breaker inputs.

    std::vector<Feed::Ptr> _feeds; ///< vector containing all 3 breaker feeds.
};


/// The motor power system and comm power system are the same except for configuration values. This class
/// sets configuration values appropriately for each.
/// Once set, these values are not expected to change.
/// unit test: test_SimCore.cpp
/// FUTURE: DM-40694 set the values using a configuration file.
class PowerSubsystemConfig {
public:
    using Ptr = std::shared_ptr<PowerSubsystemConfig>;


    PowerSubsystemConfig() = delete;
    PowerSubsystemConfig(PowerSubsystemConfig const&) = delete;
    PowerSubsystemConfig operator=(PowerSubsystemConfig const&) = delete;

    ~PowerSubsystemConfig() = default;

    /// Create the appropriate configuration according to `systemType`.
    PowerSubsystemConfig(PowerSystemType systemType);

    /// Returns delay for turning power on, in seconds.
    double outputOnMaxDelay() const;

    /// Returns delay for turning power off.
    double outputOffMaxDelay() const;

    /// Return `_nominalVoltage` in volts
    double getNominalVoltage() const { return _nominalVoltage; }

    /// Return `_maxCurrent` in amps
    double getMaxCurrentFault() const { return _maxCurrent; }

    /// Return `_breakerOnTime` in seconds.
    double getBreakerOnTime() const { return _breakerOnTime; }

    /// Return `_minVoltageWarn`
    double getMinVoltageWarn() const { return _minVoltageWarn; }

    /// Return `_maxVoltageWarn`
    double getMaxVoltageWarn() const { return _maxVoltageWarn; }

    /// Return `_minVoltageFault`
    double getMinVoltageFault() const { return _minVoltageFault; }

    /// Return `_maxVoltageFault`
    double getMaxVoltageFault() const { return _maxVoltageFault; }

    /// Return `_voltageOffLevel`
    double getVoltageOffLevel() const { return _voltageOffLevel; }

    /// Return minimum voltage where breaker outputs function in volts.
    double getBreakerOperatingVoltage() const { return _breakerOperatingVoltage; }

    /// Return `_breakerOperatingVoltageRiseTime` in seconds.
    double getBreakerOperatingVoltageRiseTime() const { return _breakerOperatingVoltageRiseTime; }

    /// Return `_voltageSettlingTime` in seconds.
    double getVoltageSettlingTime() const { return _voltageSettlingTime; }

    /// Return `_resetBreakerPulseWidth` in seconds.
    double getResetBreakerPulseWidth() const { return _resetBreakerPulseWidth; }

    /// Return `_outputPowerOnBitPos`
    int getOutputPowerOnBitPos() const { return _outputPowerOnBitPos; }

    /// Return `_outputBreakerBitPos`
    int getOutputBreakerBitPos() const { return _outputBreakerBitPos; }

    /// Return a copy of `_subsystemFaultMask`
    faultmgr::FaultStatusBits getSubsystemFaultMask() const { return _subsystemFaultMask; }

    /// Return a copy of `_subsytemName`
    std::string get_subsytemName() const { return _subsytemName; }

    /// Return `_voltageFault`.
    int getVoltageFault() const { return _voltageFault; }

    /// Return `_voltageWarn`.
    int getVoltageWarn() const { return _voltageWarn; }

    /// Return `_excessiveCurrent`.
    int getExcessiveCurrent() const { return _excessiveCurrent; }

    /// Return `_relayFault`.
    int getRelayFault() const { return _relayFault; }

    /// Return `_breakerFault`.
    int getBreakerFault() const { return _breakerFault; }

    /// Return `_breakerWarn`.
    int getBreakerWarn() const { return _breakerWarn; }

    /// Return `_relayInUse`.
    int getRelayInUse() const { return _relayInUse; }

    /// Check the status of the breakers, according to `sysInfo`.
    /// @return - SysStatus of the worst Feed checked and a string
    ///     containing the names of the problem Feed input bits as defined in InputPortBits.
    ///     If `_breakerFeeds` is nullptr, return FAULT and "nullptr".
    /// The status of the breakers comes from `sysInfo`, `_breakerGroup` only
    /// knows which bits in `sysInfo` are important.
    std::tuple<SysStatus, std::string> checkBreakers(SysInfo sysInfo);

private:
    PowerSystemType _systemType; ///< indicates if this is the MOTOR or COMM system.

    /// Set values for the MOTOR subsystem.
    void _setupMotor();

    /// Set values for the COMM subsystem.
    void _setupComm();

    /// Setup values calculated from other configuration items.
    void _setupCalculated();

    double _relayCloseDelay; ///< "relay close delay" in seconds
    double _breakerOnTime; ///< “breaker on time” in seconds
    double _interlockOutputOnDelay; ///< “interlock output on delay” in seconds

    double _relayOpenDelay; ///< “relay open delay” in seconds
    double _interlockOutputOffDelay; ///< “interlock output off delay” in seconds

    double _resetBreakerPulseWidth; ///< "reset breaker pulse width" in seconds
    double _breakerOperatingVoltage; ///< "breaker operating voltage" in volts.

    double _nominalVoltage; ///< "output voltage nominal level" in volts.
    double _minVoltageWarn; ///< "minimum voltage warning level" in volts.
    double _maxVoltageWarn; ///< "maximum voltage warning level" in volts.
    double _minVoltageFault; ///< "minimum voltage fault level" in volts.
    double _maxVoltageFault; ///< "maximum voltage fault level" in volts.

    double _breakerOperatingVoltageRiseTime; ///< "breaker operating voltage rise time" in seconds.
    double _voltageSettlingTime; ///< output voltage settling time in seconds

    double _voltageFallTime; ///< output voltage fall time in seconds

    double _voltageOffLevel; ///< output voltage off level in volts

    double _maxCurrent; ///< maximum output current in amps.

    /// Contains the bit positions that are connected to the 3 breaker feeds,
    /// but does not store any data.
    BreakerFeedGroup::Ptr _breakerFeedGroup;

    int _outputPowerOnBitPos; ///< Output bit that turns power on/off for this subsystem, active high.
    int _outputBreakerBitPos; ///< Output bit that will reset breaker for this subsystem when low/high???.

    faultmgr::FaultStatusBits _subsystemFaultMask; ///< Mask for bits

    // Values from BasePowerSubsystem.lvclass:configure_subsystem.vi
    std::string _subsytemName; ///< string name of the subsytem
    int _voltageFault;         ///< "voltage fault"
    int _voltageWarn;          ///< "voltage warning"
    int _excessiveCurrent;     ///< "excessive current"
    int _relayFault;           ///< "relay fault"
    int _breakerFault;         ///< "breaker fault"
    int _breakerWarn;          ///< "breaker warning"
    int _relayInUse;           ///< "relay in use"
};


/// This class represents the MOTOR and COMM power subsystems. The physical
/// hardware for both systems is the same, the differences are found in the
/// PowerSubsystemConfig `_psCfg` for each type.
///
/// The hardware has built in breakers which can trigger power being
/// turned off if more than one breaker in a Feed is tripped.
/// See class `BreakerFeedGroup`.
///
/// Turning power off is simply a matter of calling `setPowerOff()` and
/// OutputPortBits will be set to turn the power off.
///
/// Turning the power on is done by calling `setPowerOn()` which begins
/// a sequence of events that turns the power on. Numerous checks
/// are made while `TURNING_ON` the power. Most of these simply
/// give up and call `setPowerOff()` if there is a problem. However,
/// breaker faults cause `setPowerOn()` to try to reset the breakers
/// by going into `RESET` mode. This is complicated by the fact that
/// breakers can only be RESET while the power is on.
/// See `_processPowerOn()` and `processDaq()`.
///
/// When power has been turned on successfully, `getActualPowerState()`
/// should return `ON`.
///
/// unit tests: test_PowerSystem.cpp
class PowerSubsystem {
public:

    enum PowerState {
        OFF,
        TURNING_OFF,
        TURNING_ON,
        ON,
        RESET,
        UNKNOWN
    };

    /// Return an appropraite string representation of `powerState`.
    static std::string getPowerStateStr(PowerState powerState);

    PowerSubsystem() = delete;
    PowerSubsystem(PowerSubsystem const&) = delete;
    PowerSubsystem& operator=(PowerSubsystem const&) = delete;

    /// Try to make sure the power is off.
    ~PowerSubsystem();

    /// `sysType` must be `MOTOR` or `COMM`.
    PowerSubsystem(PowerSystemType sysType);

    /// Return the name of the class and systemType string.
    std::string getClassName() const { return "PowerSubsystem " + getPowerSystemTypeStr(_systemType); }

    /// Return `_systemType`.
    PowerSystemType getSystemType() const { return _systemType; }

    /// Return subsytem voltage in volts.
    double getVoltage() const;

    /// Return subsystem current in amps.
    double getCurrent() const;

    /// Take action to turn the power on, this may be called from nearly anywhere,
    /// but many things can interrupt the process.
    /// When the process is complete `getActualPowerState()` should return `ON`.
    /// See `_setPowerOn()`
    void setPowerOn();

    /// Take action to turn the power off, this may be called from nearly anywhere.
    /// @param note - should contain information about the source of
    ///         the `setPowerOff()` call.
    /// When the process is complete `getActualPowerState()` should return `OFF`.
    /// See `_setPowerOff()`
    void setPowerOff(std::string const& note);

    /// Use the information in `info` to control this power subsystem. `info`
    /// should be provided by `FpgaIo` and should contain the most recent
    /// data available. `processDaq` will try to attain the `_targPowerState`
    /// but will change the `_targPowerState` to `OFF` if problems occur.
    /// Based on PowerSubsystem->process_DAQ_telemetry.vi
    SysStatus processDaq(SysInfo const& info);

    /// Return the actual power state.
    PowerState getActualPowerState() const;

    /// Return the target power state.
    PowerState getTargPowerState() const;

private:
    /// If there are no related problems, turn power on by setting the
    /// `_targPowerState` to `ON` and setting the appropriate `OutputPortBits`.
    /// `processDaq` will monitor and complete the task of `TURNING_ON`.
    /// When the process is complete `getActualPowerState()` should return `ON`.
    void _setPowerOn();

    /// Turn off the power by setting the appropriate `OutputPortBits`.
    /// `processDaq` will monitor and complete the task of `TURNING_OFF`,
    /// When the process is complete `getActualPowerState()` should return `OFF`.
    void _setPowerOff(std::string const& note);

    /// Go through the sequence of events required when `_targPowerState` is ON,
    /// `_powerStateMtx` must be locked before calling.
    void _processPowerOn();

    /// Go through the sequence of events required when `_targPowerState` is OFF,
    /// `_powerStateMtx` must be locked before calling.
    void _processPowerOff();

    /// Return “Relay Control Output On”.
    bool _getRelayControlOutputOn() const;

    /// Return “cRIO Ready Output On”.
    bool _getCrioReadyOutputOn() const;

    /// Return ”Interlock Relay Control Output On”.
    bool _getInterlockRelayControlOutputOn() const;

    /// Return true if the OutputPort is has the correct bits to turn on this power system.
    /// Sets "interlock fault" if the interlock is preventing power on.
    bool _powerShouldBeOn();

    /// Return a log worth string of the power situation.
    std::string _getPowerShouldBeOnStr();

    /// Return true if there are any breaker faults. Breaker faults can only
    /// be reported if the voltage is over `_breakerOperatingVoltage`.
    /// Faults and warnings will be sent to the FaultMgr.
    /// Faults will result in `_setPowerOff()` being called.
    bool _checkForPowerOnBreakerFault(double voltage);

    /// Set power off and send the FaultMgr low voltage warnings and faults.
    void _sendBreakerVoltageFault();

    /// PLACEHOLDER to register an error with the fault manager.
    void _sendFaultMgrError();

    /// PLACEHOLDER to register an error with the fault manager.
    void _sendFaultMgrError(int errId, std::string note);

    /// PLACEHOLDER to register a warning with the fault manager.
    void _sendFaultMgrWarn();

    /// PLACEHOLDER to set a bit in the FaulMgr FaultStatusBits.
    void _sendFaultMgrSetBit(int bitPos);

    /// Return true if the FaultMgr has any faults that affect this PowerSubsystem.
    bool _checkForFaults();


    PowerSystemType _systemType; ///< indicates if this is the MOTOR or COMM system.

    PowerSubsystemConfig _psCfg; ///< Configuration values for this PowerSubsystem.

    SysInfo _sysInfo; ///< last value of SysInfo read by processDaq.

    util::TIMEPOINT _powerOnStart; ///< Time `_targPowerState` was set to ON.
    util::TIMEPOINT _powerOffStart; ///< Time `_targPowerState` was set to OFF.
    int _phase = 1; ///< Current phase of power up or power off. Initialize to powering off.
    util::TIMEPOINT _phaseStartTime; ///< Time the current `_phase` started.
    int _telemCounter = 0; ///< For unexplained reasons, power on `_phase` 1 lasts 10 telemetry reads.

    PowerState _targPowerState = OFF; ///< Target power state, valid values are ON, OFF, and RESET.
    PowerState _actualPowerState = UNKNOWN; ///< Actual state of this PowerSubsystem.
    mutable util::VMutex _powerStateMtx; ///< Protects `_targPowerState` and `_actualPowerState`.

    PowerState _targPowerStatePrev = UNKNOWN; ///< Previous value of `_targPowerState`
    PowerState _actualPowerStatePrev = UNKNOWN; ///< Previous value of `_actualPowerState`

    std::shared_ptr<FpgaIo> _fpgaIo; ///< pointer to the global FpgaIo instance.
};


}  // namespace control
}  // namespace m2cellcpp
}  // namespace LSST

#endif  // LSST_M2CELLCPP_CONTROL_POWERSUBSYSTEM_H
