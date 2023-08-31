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


/// &&&
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
    double _voltage = 0.0; ///< &&& doc
    double _current = 0.0; ///< &&& doc

    double _voltageMax = 0.0; ///< &&& doc

    double _currentMax = 0.0; ///< &&& doc

    bool _breakerClosed = true; ///< &&& doc

    system::CLOCK::time_point _lastRead{system::CLOCK::now()}; ///< &&& doc

    control::OutputPortBits::Ptr _outputPort; ///< &&& doc
    int _powerOnBitPos; ///< &&& doc

    control::InputPortBits::Ptr _inputPort; ///< &&& doc
    std::vector<int>  _breakerBitPositions; ///< &&& doc
};


/* &&&
#include <chrono>

system::CLOCK::time_point begin = system::CLOCK::now();
std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();

std::cout << "Time difference = " << std::chrono::duration_cast<std::chrono::microseconds>(end - begin).count() << "[µs]" << std::endl;
std::cout << "Time difference = " << std::chrono::duration_cast<std::chrono::nanoseconds> (end - begin).count() << "[ns]" << std::endl;
&&& */

}  // namespace simulator
}  // namespace m2cellcpp
}  // namespace LSST

#endif  // LSST_M2CELLCPP_SIMULATOR_SIMCORE_H
