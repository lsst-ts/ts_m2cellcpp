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
}

void FpgaIo::writeOutputPortBitPos(int pos, bool set) {
    lock_guard<util::VMutex> lg(_portMtx);
    _outputPort.writeBit(pos, set);
}

}  // namespace control
}  // namespace m2cellcpp
}  // namespace LSST


