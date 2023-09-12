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
#include "util/clock_defs.h"
#include "util/Log.h"


namespace LSST {
namespace m2cellcpp {
namespace control {

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
    //&&&std::tuple<util::SysStatus, std::string> checkBreakers(InputPortBits const& input);
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

    BreakerFeedGroup::Ptr _breakerFeedGroup;

};


/// doc &&&  Class for MOTOR and COMM power systems.
/// unit tests: &&&
class PowerSubsystem {
public:




    /// &&& doc     Based on PowerSubsystem->process_DAQ_telemetry.vi
    SysStatus processDAQ(SysInfo const& info);

private:
    PowerSystemType _systemType; ///< indicates if this is the MOTOR or COMM system.

    PowerSubsystemConfig _psCfg; ///< Configuration values for this PowerSubsystem.

    BreakerFeedGroup _breakerFeeds; ///< Contains all breaker feeds for this power susbsystem.

    bool _targPowerOn = false; ///< true when the user desires to have the power on.
};


}  // namespace control
}  // namespace m2cellcpp
}  // namespace LSST

#endif  // LSST_M2CELLCPP_CONTROL_POWERSUBSYSTEM_H
