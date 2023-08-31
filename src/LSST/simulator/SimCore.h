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
#include <chrono>
#include <map>
#include <memory>
#include <mutex>
#include <stdint.h>
#include <string>
#include <thread>
#include <vector>

// Project headers
#include "control/FpgaIo.h"
#include "control/InputPortBits.h"
#include "control/OutputPortBits.h"
#include "simulator/SimPowerSubsystem.h"
#include "system/clock_defs.h"
#include "util/Log.h"

#ifndef LSST_M2CELLCPP_SIMULATOR_SIMCORE_H
#define LSST_M2CELLCPP_SIMULATOR_SIMCORE_H

namespace LSST {
namespace m2cellcpp {
namespace simulator {

/* &&& what does this need to do???  (Start this thing up with everything wrong and have system init fix it.)
 * 2 power systems, very similar Motor and Comm
 * Motor has interlocks, comm does not
 * Both have breakers
 * Output we need to worry about:
 *  - read/write control - "ILC_Motor_Power_On" - set to true during powering on
 *  - read/write control - "ILC_Comm_Power_On" - set to true during powering on
 *      - All instances:
 *                BasePowerSubsystem.lvclass:power_on.vi - true-on
 *                PS_Init.lvclass:PS_start.vi -  false-off
 *                PS_Powered_On.lvclass:PS_turn_power_off.vi -  false-off
 *                PS_Powering_On.lvclass:PS_turn_power_off.vi - false-off
 *                PS_Resettting_Breakers.lvclass:PS_turn_power_off.vi - - false-off
 *                PSS_State.lvclass:goto_powering_off.vi - false-off
 *                PSS_State.lvclass:restart.vi - false-off
 *  - read/write control - "Reset_Motor_Power_Breakers"  - set to true during powering on
 *  - read/write control - "Reset_Comm_Power_Breakers"   - set to true during powering on
 *      - All instances:
 *                BasePowerSubsystem.lvclass:power_on.vi - true-on  AFTER power set true-on -> state "powering on"
 *                PS_Init.lvclass:PS_start.vi - true-on AFTER power set false-off   -> state "powered off"
 *                PS_Resettting_Breakers.lvclass:PS_process_telemery.vi - true-on        -> state "powered on"   (so, set to true once done with "powering on")
 *                PS_Resettting_Breakers.lvclass:PS_turn_power_off.vi - true-on AFTER power set false-off -> state "powering off"  (set "telem counter"=0)
 *                PSS_State.lvclass:goto_powering_off.vi - true-on AFTER power set false-off -> state "powering off"   (but don't touch "telem counter")
 *                PSS_State.lvclass:PS_restart.vi - true-on AFTER power set false-off -> state "init" (but don't touch "telem counter")
 *                PSS_State.lvclass:reset_breakers.vi - false-off  -> state "reseting breakers"  (don't touch power or or "telem counter")
 *             The last item, reset_breakers.vi is the only thing to set breakers to false-off.
 *
 * Model->Read SystemController.vi
 */
/// &&&
/// unit test
class SimCore {
public:
    // &&&M2CellCtrlrTopVI.vi  DiscreteSimulation.vi   search top level vi for text CellSimulation
    // &&& CellCommSupport->getTelemetry.vi  LTS-346

    SimCore();

    /// The values in `outputPort` will be read by the `_simThread` the
    /// next time through the loop, thread safe.
    void writeToOutputPort(control::OutputPortBits outputPort);

    /// Get a copy of the `_readOutputPort`
    control::OutputPortBits getOutputPort();

    /// &&& doc
    void start();

    /// &&& doc
    void stop() {
        _simLoop = false;
    }

    /// &&& doc
    bool join();

    /// Write the value of `_newOutput` `bit` to 1 if `set` is true or 0
    /// if `set` is false.
    void writeNewOutputBit(int bit, bool set);

    /// Return a copy of `_newOutputPort
    control::OutputPortBits getNewOutputPort();

private:
    control::FpgaIo _fpgaIo; ///< &&&
    double _frequencyHz = 40.0; ///< How many times loop should run per second.

    control::OutputPortBits _outputPort; ///< doc &&&
    control::InputPortBits _inputPort; ///< doc &&&

    SimPowerSubsystem::Ptr _motorSub; ///< doc &&&
    SimPowerSubsystem::Ptr _commSub; ///< doc &&&

    void _simRun(); ///< Primary function run inside `_simThread`
    std::atomic<bool> _simLoop{true}; ///< _simThread will run until this is false.
    std::thread _simThread; ///< Generate hardware outputs at a specified rate.

    /// New value for `_outputPort` to be set on next `_simRun()` iteration.
    control::OutputPortBits _newOutput;

    std::mutex _mtx; ///< protects `_newOutput`
};



}  // namespace simulator
}  // namespace m2cellcpp
}  // namespace LSST

#endif  // LSST_M2CELLCPP_SIMULATOR_SIMCORE_H
