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

    vector<int> motorBreakerInBits = {control::InputPortBits::J1_W9_1_MTR_PWR_BRKR_OK,
                                      control::InputPortBits::J1_W9_2_MTR_PWR_BRKR_OK,
                                      control::InputPortBits::J1_W9_3_MTR_PWR_BRKR_OK,
                                      control::InputPortBits::J2_W10_1_MTR_PWR_BRKR_OK,
                                      control::InputPortBits::J2_W10_2_MTR_PWR_BRKR_OK,
                                      control::InputPortBits::J2_W10_3_MTR_PWR_BRKR_OK,
                                      control::InputPortBits::J3_W11_1_MTR_PWR_BRKR_OK,
                                      control::InputPortBits::J3_W11_2_MTR_PWR_BRKR_OK,
                                      control::InputPortBits::J3_W11_3_MTR_PWR_BRKR_OK};

    _motorSub = SimPowerSubsystem::Ptr(new SimPowerSubsystem(
            control::MOTOR, _outputPort, control::OutputPortBits::MOTOR_POWER_ON,
            control::OutputPortBits::RESET_MOTOR_BREAKERS, _inputPort, motorBreakerInBits));

    vector<int> commBreakerInBits = {control::InputPortBits::J1_W12_1_COMM_PWR_BRKR_OK,
                                     control::InputPortBits::J1_W12_2_COMM_PWR_BRKR_OK,
                                     control::InputPortBits::J2_W13_1_COMM_PWR_BRKR_OK,
                                     control::InputPortBits::J2_W13_2_COMM_PWR_BRKR_OK,
                                     control::InputPortBits::J3_W14_1_COMM_PWR_BRKR_OK,
                                     control::InputPortBits::J3_W14_2_COMM_PWR_BRKR_OK};

    _commSub = SimPowerSubsystem::Ptr(new SimPowerSubsystem(
            control::COMM, _outputPort, control::OutputPortBits::ILC_COMM_POWER_ON,
            control::OutputPortBits::RESET_COMM_BREAKERS, _inputPort, commBreakerInBits));

    _newOutput = *_outputPort;

    // Set bits in the _inputPort that should normally be high.
    _inputPort->setBitAtPos(control::InputPortBits::REDUNDANCY_OK, true);
    _inputPort->setBitAtPos(control::InputPortBits::LOAD_DISTRIBUTION_OK, true);
    _inputPort->setBitAtPos(control::InputPortBits::POWER_SUPPLY_1_DC_OK, true);
    _inputPort->setBitAtPos(control::InputPortBits::POWER_SUPPLY_2_DC_OK, true);
    _inputPort->setBitAtPos(control::InputPortBits::POWER_SUPPLY_1_CURRENT_OK, true);
    _inputPort->setBitAtPos(control::InputPortBits::POWER_SUPPLY_2_CURRENT_OK, true);
}

SimCore::~SimCore() {
    stop();
    join();
}

void SimCore::_simRun() {
    control::OutputPortBits prevOutput = *_outputPort;
    while (_simLoop) {
        util::TIMEPOINT timestamp = util::CLOCK::now();
        // Read in new outputPorts set elsewhere. Keep the lock short.
        {
            lock_guard<mutex> lg(_mtx);
            *_outputPort = _newOutput;
        }
        control::OutputPortBits outputDiff(_outputPort->getBitmap() ^ prevOutput.getBitmap());
        if (outputDiff.getBitmap() != 0) {
            LINFO("Simcore output changed diff=", outputDiff.getAllSetBitEnums());
            LINFO("Simcore _outputPort=", _outputPort->getAllSetBitEnums());
        }
        double timeDiff = util::timePassedSec(_prevTimeStamp, timestamp);
        _motorSub->calcBreakers(timestamp);
        _motorSub->calcVoltageCurrent(timeDiff);
        _commSub->calcBreakers(timestamp);
        _commSub->calcVoltageCurrent(timeDiff);

        prevOutput = *_outputPort;
        {
            lock_guard<mutex> lg(_mtx);
            _simInfo.outputPort = *_outputPort;
            _simInfo.inputPort = *_inputPort;
            _simInfo.motorVoltage = _motorSub->getVoltage();
            _simInfo.motorCurrent = _motorSub->getCurrent();
            _simInfo.motorBreakerClosed = _motorSub->getBreakerClosed();
            _simInfo.commVoltage = _commSub->getVoltage();
            _simInfo.commCurrent = _commSub->getCurrent();
            _simInfo.commBreakerClosed = _commSub->getBreakerClosed();
            _simInfo.iterations = _iterations;
            _simInfo.timestamp = timestamp;
        }
        ++_iterations;
        _iterationCv.notify_all();
        _prevTimeStamp = timestamp;
        this_thread::sleep_for(chrono::duration<double, std::ratio<1, 1>>(1.0 / _frequencyHz));
    }
}

void SimCore::waitForUpdate(int count) const {
    LDEBUG("SimCore::waitForUpdate()");
    uint64_t iteration = _iterations;
    unique_lock<mutex> gLock(_mtx);
    _iterationCv.wait(gLock, [&]() {
        if (_iterations != iteration) {
            iteration = _iterations;
            --count;
            LDEBUG("SimCore::waitForUpdate() ", count);
        };
        return count <= 0;
    });
    LDEBUG("SimCore::waitForUpdate() end");
}

void SimCore::start() {
    _prevTimeStamp = util::CLOCK::now();
    thread thrd(&SimCore::_simRun, this);
    _simThread = std::move(thrd);
}

void SimCore::join() {
    if (_joined.exchange(true) == true) {
        return;
    }
    if (_simThread.joinable()) {
        _simThread.join();
    }
}

void SimCore::writeNewOutputPortBit(int pos, bool set) {
    lock_guard<mutex> lg(_mtx);
    _newOutput.writeBit(pos, set);
}

void SimCore::setNewOutputPort(control::OutputPortBits const& outputPort) {
    lock_guard<mutex> lg(_mtx);
    _newOutput.setBitmap(outputPort.getBitmap());
}

control::OutputPortBits SimCore::getNewOutputPort() {
    lock_guard<mutex> lg(_mtx);
    return _newOutput;
}

control::SysInfo SimCore::getSysInfo() const {
    lock_guard<mutex> lg(_mtx);
    return _simInfo;
}

void SimCore::writeInputPortBit(int pos, bool set) {
    lock_guard<mutex> lg(_mtx);
    _inputPort->writeBit(pos, set);
}

}  // namespace simulator
}  // namespace m2cellcpp
}  // namespace LSST
