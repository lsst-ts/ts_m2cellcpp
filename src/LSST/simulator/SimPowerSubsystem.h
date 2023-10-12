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

#ifndef LSST_M2CELLCPP_SIMULATOR_SIMPOWERSUBSYTEM_H
#define LSST_M2CELLCPP_SIMULATOR_SIMPOWERSUBSYTEM_H

// System headers
#include <memory>
#include <stdint.h>
#include <string>
#include <vector>


// Project headers
#include "control/control_defs.h"
#include "control/InputPortBits.h"
#include "control/OutputPortBits.h"
#include "control/PowerSubsystem.h"
#include "util/clock_defs.h"
#include "util/Log.h"


namespace LSST {
namespace m2cellcpp {
namespace simulator {


/// The purpose for this class is make a reasonable facsimile of power system
/// to get the software off the ground. If there is time, it would be good to
/// make a more accurate representation of the hardware.
/// unit test: test_SimCore.cpp
class SimPowerSubsystem {
public:
    using Ptr = std::shared_ptr<SimPowerSubsystem>;

    SimPowerSubsystem() = delete;

    /// Class constructor
    /// @param `systemType` MOTOR or COMM
    /// @param `outputPort` pointer to the output port (output to FPGA)
    /// @param `powerOnBitPos` bit representing power on.
    /// @param `breakerResetPos` bit representing breaker reset.
    /// @param `inputPort` pointer to the input port (input from the FPGA)
    /// @param `breakerBitPositions` list of bits affected by breaker status.
    /// It's unclear which InputPortBits each power system should be concerned
    /// with at this point. Hopefully that will become clearer.
    SimPowerSubsystem(control::PowerSystemType systemType,
            control::OutputPortBits::Ptr const& outputPort, int powerOnBitPos, int breakerResetPos,
            control::InputPortBits::Ptr const& inputPort,  std::vector<int> const& breakerBitPositions);

    /// Return true if outputPort has set the power on.
    bool getPowerOn() { return _outputPort->getBitAtPos(_powerOnBitPos); }

    /// Return the simulated `_voltage`.
    double getVoltage() { return _voltage; }

    /// Return the simulated `_current`.
    double getCurrent() { return _current; }

    /// The output bit for power on.
    void setPowerOn(bool on) {
        _outputPort->writeBit(_powerOnBitPos, on);
    }

    /// Return `breakerClosed`
    bool getBreakerClosed() { return _breakerClosed; }

    /// Determine the values related to breakers in the simulation.
    void calcBreakers(util::CLOCK::time_point ts);

    /// Determine the voltage and current related values in the simulation.
    void calcVoltageCurrent(double timeDiff);

    /// Return a string version of the class name and `_systemType`.
    std::string getClassName() const;

    /// Change the voltage so it goes over the maximum allowable voltage
    /// and should trip a fault, for testing.
    /// @param `overVoltage` - when `true`, allow the voltage to go too high,
    ///                when 'false`, use normal voltage.
    void forceOverVoltage(bool overVoltage);

    /// Change the current so it goes over the maximum allowable current
    /// and should trip a fault, for testing.
    /// @param `overCurrent` - when `true`, allow the current to go too high,
    ///                when 'false`, use normal current.
    void forceOverCurrent(bool overCurrent);

private:
    void _setup(); ///< Setup according to `_systemType`.

    control::PowerSystemType _systemType; ///< MOTOR or COMM.

    double _voltage = 0.0; ///< current voltage, volts
    double _current = 0.0; ///< current current, amps

    /// Nominal voltage, volts "output voltage nominal level"
    double _voltageNominal;

    /// voltage change rate powering on, volts/sec. See PowerSubsystemCommonConfig.vi
    double _voltageChangeRateOn;

    /// voltage change rate powering off, volts/sec. See PowerSubsystemCommonConfig.vi
    double _voltageChangeRateOff;

    double _currentMax; ///< max current, amps. "maximum output current"
    double _currentGain; ///< Current based on `_voltage`, amp/volt.


    bool _breakerClosed = true; ///< true when the simulated breaker is closed.
    bool _breakerClosedPrev = !_breakerClosed; ///< Set to the previous value of `_breakerClosed`.
    bool _breakerClosedTarg = true; ///< Target state of `_breakerClosed`.

    util::CLOCK::time_point _breakerClosedTargTs; ///< Time stamp when `_breakerClosedTarg` was set.
    double _breakerCloseTimeSec; ///< Time for the breaker to go from open to close.

    control::OutputPortBits::Ptr _outputPort; ///< Integer value to be written to the ouput port.
    int _powerOnBitPos; ///< Bit position indicating power on (active high).
    int _breakerResetPos; ///< Bit position indicating breaker reset (active low).

    control::InputPortBits::Ptr _inputPort; ///< Integer value representing simulator status.
    std::vector<int> _breakerBitPositions; ///< Positions indicating breaker ok.

    std::atomic<bool> _overVoltage{false}; ///< When true, allow voltage to run much too high.
    std::atomic<bool> _overCurrent{false}; ///< When true, allow current to run much too high.

};


}  // namespace simulator
}  // namespace m2cellcpp
}  // namespace LSST

#endif  // LSST_M2CELLCPP_SIMULATOR_SIMPOWERSUBSYSTEM_H
