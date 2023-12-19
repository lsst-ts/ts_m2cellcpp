/*
 * This file is part of LSST ts_m2cellcpp test suite.
 *
 * Developed for the Vera C. Rubin Observatory Telescope & Site Software Systems.
 * This product includes software developed by the Vera C.Rubin Observatory Project
 * (https://www.lsst.org). See the COPYRIGHT file at the top-level directory of
 * this distribution for details of code ownership.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation, either version 3 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program. If not, see <https://www.gnu.org/licenses/>.
 */

// Class header
#include "control/FpgaIo.h"

#include <bitset>
#include <sstream>
#include <mutex>
#include <stdexcept>
#include <string>

// Project headers
#include "control/PowerSystem.h"
#include "simulator/SimCore.h"
#include "system/Config.h"
#include "util/Bug.h"
#include "util/Log.h"

using namespace std;

namespace LSST {
namespace m2cellcpp {
namespace control {

FpgaIo::Ptr FpgaIo::_thisPtr;
std::mutex FpgaIo::_thisPtrMtx;

void FpgaIo::setup(std::shared_ptr<simulator::SimCore> const& simCore) {
    lock_guard<mutex> lock(_thisPtrMtx);
    if (_thisPtr) {
        LERROR("FpgaIo already setup");
        return;
    }
    _thisPtr = Ptr(new FpgaIo(simCore));
}

FpgaIo::Ptr FpgaIo::getPtr() {
    // No mutex needed as once set by `setup()`, the value of _thisPtr
    // cannot change.
    if (_thisPtr == nullptr) {
        throw system::ConfigException(ERR_LOC, "FpgaIo has not been setup.");
    }
    return _thisPtr;
}

FpgaIo& FpgaIo::get() {
    if (_thisPtr == nullptr) {
        throw system::ConfigException(ERR_LOC, "FpgaIo has not been setup.");
    }
    return *_thisPtr;
}

FpgaIo::FpgaIo(std::shared_ptr<simulator::SimCore> const& simCore) : _simCore(simCore) {
    std::thread thrd(&FpgaIo::_readWriteFpga, this);
    _fpgaIoThrd = move(thrd);
}

FpgaIo::~FpgaIo() {
    LTRACE("FpgaIo::~FpgaIo");
    _loop = false;
    if (_fpgaIoThrd.joinable()) {
        _fpgaIoThrd.join();
    }
}

void FpgaIo::writeOutputPortBitPos(int pos, bool set) {
    VMUTEX_NOT_HELD(_portMtx);
    lock_guard<util::VMutex> lg(_portMtx);
    _outputPort.writeBit(pos, set);
}

/// Return a copy of the current system information.
SysInfo FpgaIo::getSysInfo() const {
    VMUTEX_NOT_HELD(_portMtx);
    lock_guard<util::VMutex> lg(_portMtx);
    return _sysInfo;
}

void FpgaIo::registerPowerSys(std::shared_ptr<PowerSystem> const& powerSys) {
    if (powerSys != nullptr) {
        LINFO("FpgaIo::registerPowerSys power system registered _daq");
    } else {
        LERROR("FpgaIo::registerPowerSys got nullptr _daq");
    }
    _powerSys = powerSys;
}

void FpgaIo::_emergencyTurnOffAllPower() {
    VMUTEX_NOT_HELD(_portMtx);  // Following function calls need to lock it.
    LWARN("FpgaIo::_emergencyTurnOffAllPower()");
    writeOutputPortBitPos(OutputPortBits::MOTOR_POWER_ON, false);
    writeOutputPortBitPos(OutputPortBits::ILC_COMM_POWER_ON, false);
}

void FpgaIo::_readWriteFpga() {
    while (_loop) {
        auto powerSys = _powerSys.lock();
        if (powerSys == nullptr) {
            LERROR("FpgaIo::_readWriteFpga() No PowerSystemRegistered");
            // With no power system registered, breakers are not being checked, etc,
            // so turn everything off.
            _emergencyTurnOffAllPower();
        }

        {
            lock_guard<util::VMutex> lg(_portMtx);
            if (_simCore == nullptr) {
                throw util::Bug(ERR_LOC, "FpgaIo::getSysInfo() hardware mode unavailable");
            } else {
                _simCore->setNewOutputPort(_outputPort);
                _sysInfo = _simCore->getSysInfo();
            }
        }

        if (powerSys != nullptr) {
            powerSys->queueDaqInfoRead();
        }
        std::this_thread::sleep_for(std::chrono::duration<double>(_loopSleepSecs));
    }
}

/// Return `_outputPort`
OutputPortBits FpgaIo::getOutputPort() const { return _outputPort; }

}  // namespace control
}  // namespace m2cellcpp
}  // namespace LSST
