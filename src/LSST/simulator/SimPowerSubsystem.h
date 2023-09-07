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

// System headers
#include <memory>
#include <stdint.h>
#include <string>
#include <vector>

#include "../util/clock_defs.h"
// Project headers
#include "control/InputPortBits.h"
#include "control/OutputPortBits.h"
#include "control/PowerSubsystem.h"
#include "util/Log.h"

#ifndef LSST_M2CELLCPP_SIMULATOR_SIMPOWERSUBSYTEM_H
#define LSST_M2CELLCPP_SIMULATOR_SIMPOWERSUBSYTEM_H

namespace LSST {
namespace m2cellcpp {
namespace simulator {


/// The purposed for this class is make a reasonable facsimile of power system
/// to get the software off the ground. If there is time, it would be good to
/// make a more accurate representation of the hardware.
/// unit test // need unit tests &&&
class SimPowerSubsystem {
public:
    using Ptr = std::shared_ptr<SimPowerSubsystem>;

    SimPowerSubsystem() = delete;

    /// &&& doc
    /// It's unclear which InputPortBits each power system should be concerned with at this point. Hopefully that will become clearer.
    SimPowerSubsystem(control::PowerSubsystemConfig::SystemType systemType,
            control::OutputPortBits::Ptr const& outputPort, int powerOnBitPos, int breakerResetPos,
            control::InputPortBits::Ptr const& inputPort,  std::vector<int> const& breakerBitPositions);

    /// &&& doc
    bool getPowerOn() { return _outputPort->getBit(_powerOnBitPos); }

    /// &&& doc
    double getVoltage() { return _voltage; }

    /// &&& doc
    double getCurrent() { return _current; }

    /// &&& doc
    void setPowerOn(bool on) {
        _outputPort->writeBit(_powerOnBitPos, on);
    }

    /// &&& doc
    bool getBreakerClosed() { return _breakerClosed; }


    /// &&& doc
    void calcBreakers(util::CLOCK::time_point ts);

    /// &&& doc
    void calcVoltageCurrent(double timeDiff);


private:
    void _setup(); ///< Setup according to `_systemType`.

    control::PowerSubsystemConfig::SystemType _systemType; ///< MOTOR or COMM.

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

    bool _breakerClosed = true; ///< &&& doc
    bool _breakerClosedTarg = true; ///< Target state of `_breakerClosed`
    util::CLOCK::time_point _breakerClosedTargTs; ///< Time stamp when `_breakerClosedTarg` was set.
    double _breakerCloseTimeSec; ///< Time for the breaker to go from open to close.

    util::CLOCK::time_point _lastRead{util::CLOCK::now()}; ///< &&& doc

    control::OutputPortBits::Ptr _outputPort; ///< &&& doc
    int _powerOnBitPos; ///< &&& doc
    int _breakerResetPos; ///< &&& doc

    control::InputPortBits::Ptr _inputPort; ///< &&& doc
    std::vector<int>  _breakerBitPositions; ///< &&& doc
};


}  // namespace simulator
}  // namespace m2cellcpp
}  // namespace LSST

#endif  // LSST_M2CELLCPP_SIMULATOR_SIMPOWERSUBSYSTEM_H
