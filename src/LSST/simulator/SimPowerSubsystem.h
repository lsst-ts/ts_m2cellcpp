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

// Project headers
#include "control/InputPortBits.h"
#include "control/OutputPortBits.h"
#include "system/clock_defs.h"
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
    SimPowerSubsystem(control::OutputPortBits::Ptr const& outputPort, int powerOnBitPos,
            control::InputPortBits::Ptr const& inputPort,  std::vector<int> const& breakerBitPositions);

    /// &&& doc
    bool getPowerOn() { return _outputPort->getBit(_powerOnBitPos); }

    /// &&& doc
    void setPowerOn(bool on) {
        if (on) {
            _outputPort->setBit(_powerOnBitPos);
        } else {
            _outputPort->unsetBit(_powerOnBitPos);
        }
    }

    /// &&& doc
    void calcBreakers(system::CLOCK::time_point ts);

    /// &&& doc
    void calcVoltageCurrent(system::CLOCK::time_point ts);

private:
    double _voltage = 0.0; ///< current voltage, volts
    double _current = 0.0; ///< current current, amps

    /// Nominal voltage, volts "output voltage nominal level" 24V
    /// Both systems use the same value.
    double _voltageNominal = 24.0;

    /// voltage change rate, volts/sec. Once powered on, voltage should reach an acceptable level
    /// in about 0.5 seconds. See PowerSubsystemCommonConfig.vi
    double _voltageChangeRate = _voltageNominal/0.40;

    double _currentMax = 20.0; ///< max current, amps. "maximum output current" 20A
    double _currentGain = 0.75; ///< Current based on `_voltage`, amp/volt.

    bool _breakerClosed = true; ///< &&& doc
    bool _breakerClosedTarg = true; ///< Desired state of `_breakerClosed`
    system::CLOCK::time_point _breakerClosedTargTs; ///< Time stamp when `_breakerClosedTarg` was set.
    double _breakerCloseTimeSec = 0.1; ///< Time for the breaker to go from open to close.

    system::CLOCK::time_point _lastRead{system::CLOCK::now()}; ///< &&& doc

    control::OutputPortBits::Ptr _outputPort; ///< &&& doc
    int _powerOnBitPos; ///< &&& doc

    control::InputPortBits::Ptr _inputPort; ///< &&& doc
    std::vector<int>  _breakerBitPositions; ///< &&& doc
};


}  // namespace simulator
}  // namespace m2cellcpp
}  // namespace LSST

#endif  // LSST_M2CELLCPP_SIMULATOR_SIMPOWERSUBSYSTEM_H
