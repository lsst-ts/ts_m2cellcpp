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

// Class header
#include "simulator/SimCore.h"

// System headers

// Project headers
#include "util/Log.h"

using namespace std;

namespace LSST {
namespace m2cellcpp {
namespace simulator {

SimCore::SimCore() {
    _outputPort = control::OutputPortBits::Ptr(new control::OutputPortBits());
    _inputPort = control::InputPortBits::Ptr(new control::InputPortBits());

    /// &&&double frequencyHz = 40.0;
    vector<int> motorInBits = {control::InputPortBits::J1_W9_1_MTR_PWR_BRKR_OK,
            control::InputPortBits::J1_W9_2_MTR_PWR_BRKR_OK,
            control::InputPortBits::J1_W9_3_MTR_PWR_BRKR_OK,
            control::InputPortBits::J2_W10_1_MTR_PWR_BRKR_OK,
            control::InputPortBits::J2_W10_2_MTR_PWR_BRKR_OK,
            control::InputPortBits::J2_W10_3_MTR_PWR_BRKR_OK,
            control::InputPortBits::J3_W11_1_MTR_PWR_BRKR_OK,
            control::InputPortBits::J3_W11_2_MTR_PWR_BRKR_OK,
            control::InputPortBits::J3_W11_3_MTR_PWR_BRKR_OK};
    _motorSub = SimPowerSubsystem::Ptr(new SimPowerSubsystem(
            _outputPort, control::OutputPortBits::ILC_MOTOR_POWER_ON,
            _inputPort, motorInBits));

    _commSub = SimPowerSubsystem::Ptr(new SimPowerSubsystem(
            _outputPort, control::OutputPortBits::ILC_COMM_POWER_ON,
            _inputPort,
            {control::InputPortBits::J1_W12_1_COMM_PWR_BRKR_OK,
            control::InputPortBits::J1_W12_2_COMM_PWR_BRKR_OK,
            control::InputPortBits::J2_W13_1_COMM_PWR_BRKR_OK,
            control::InputPortBits::J2_W13_2_COMM_PWR_BRKR_OK,
            control::InputPortBits::J3_W14_1_COMM_PWR_BRKR_OK,
            control::InputPortBits::J3_W14_2_COMM_PWR_BRKR_OK}));
}


void SimCore::_simRun() {

    while(_simLoop) {
        system::CLOCK::time_point timestamp = system::CLOCK::now();
        // &&& read in new outputPorts set by system.
        _motorSub->calcBreakers(timestamp);
        _motorSub->calcVoltageCurrent(timestamp);
        _commSub->calcBreakers(timestamp);
        _commSub->calcVoltageCurrent(timestamp);

        // &&& send message to system with current input, voltages, currents, and ouputs.
        this_thread::sleep_for(chrono::duration<double, std::ratio<1,1>>(1.0/_frequencyHz));
    }
}

void SimCore::start() {
    thread thrd(&SimCore::_simRun, this);
    _simThread = std::move(thrd);
}

bool SimCore::join() {
    if (_simThread.joinable()) {
        _simThread.join();
        return true;
    }
    return false;
}

}  // namespace simulator
}  // namespace m2cellcpp
}  // namespace LSST
