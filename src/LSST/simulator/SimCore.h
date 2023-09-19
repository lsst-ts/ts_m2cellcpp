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

#ifndef LSST_M2CELLCPP_SIMULATOR_SIMCORE_H
#define LSST_M2CELLCPP_SIMULATOR_SIMCORE_H

// System headers
#include <chrono>
#include <map>
#include <memory>
#include <mutex>
#include <stdint.h>
#include <string>
#include <thread>
#include <vector>


// Project headers
#include "control/InputPortBits.h"
#include "control/OutputPortBits.h"
#include "control/SysInfo.h"
#include "simulator/SimPowerSubsystem.h"
#include "util/clock_defs.h"
#include "util/Log.h"

namespace LSST {
namespace m2cellcpp {
namespace simulator {



/// A basic simulator for the hardware. At this point it is limited to power systems.
/// Breaker values can only be read while voltage is above a certain level, which
/// implies that breakers only have effect on the current. There are time delays
/// between outputs being set and other values changing.
/// output is what would be sent to the FPGA.
/// input is what would be read from the FPGA.
/// unit test: test_SimCore.cpp
class SimCore {
public:
    using Ptr = std::shared_ptr<SimCore>;

    SimCore();

    /// The values in `outputPort` will be read by the `_simThread` the
    /// next time through the loop, thread safe.
    void writeNewOutputPort(int pos, bool set);

    /// Get a copy of the `_newOutputPort`
    control::OutputPortBits getNewOutputPort();

    /// Return SysInfo from the most recent iteration.
    control::SysInfo getSysInfo();

    /// Start the simulation thread
    void start();

    /// Stop the simulation thread.
    void stop() {
        _simLoop = false;
    }

    /// Join the simulation thread.
    bool join();

    /// Write the value of `_newOutput` `bit` to 1 if `set` is true or 0
    /// if `set` is false.
    void writeNewOutputBit(int bit, bool set);

    /// Return `_iterations`.
    uint64_t getIterations() { return _iterations; }

private:
    double _frequencyHz = 40.0; ///< How many times loop should run per second.

    /// OutputPort value used for determine simulation actions.
    control::OutputPortBits::Ptr _outputPort;

    /// Contains system status as determined by the simulation.
    control::InputPortBits::Ptr _inputPort;

    SimPowerSubsystem::Ptr _motorSub; ///< Motor power simulation
    SimPowerSubsystem::Ptr _commSub; ///< Comm power simulation.

    void _simRun(); ///< Primary function run inside `_simThread`
    std::atomic<bool> _simLoop{true}; ///< _simThread will run until this is false.
    std::thread _simThread; ///< Generate hardware outputs at a specified rate.

    /// New value for `_outputPort` to be set on next `_simRun()` iteration.
    control::OutputPortBits _newOutput;

    control::SysInfo _simInfo; ///< simulation status information.

    std::mutex _mtx; ///< protects `_newOutput`

    std::atomic<uint64_t> _iterations{0}; ///< number of times through the `_simRun` loop.

    util::CLOCK::time_point _prevTimeStamp; ///< Last time through the `_simRun` loop.
};



}  // namespace simulator
}  // namespace m2cellcpp
}  // namespace LSST

#endif  // LSST_M2CELLCPP_SIMULATOR_SIMCORE_H
