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
#include "faultmgr/FaultMgr.h"

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
namespace faultmgr {

FaultMgr::Ptr FaultMgr::_thisPtr;
std::mutex FaultMgr::_thisPtrMtx;

void FaultMgr::setup() {
    lock_guard<mutex> lock(_thisPtrMtx);
    if (_thisPtr) {
        LERROR("FaultMgr already setup");
        return;
    }
    _thisPtr = Ptr(new FaultMgr());
}


FaultMgr::Ptr FaultMgr::getPtr() {
    if (_thisPtr == nullptr) {
        throw system::ConfigException(ERR_LOC, "FaultMgr has not been setup.");
    }
    return _thisPtr;
}

FaultMgr& FaultMgr::get() {
    if (_thisPtr == nullptr) {
        throw system::ConfigException(ERR_LOC, "FaultMgr has not been setup.");
    }
    return *_thisPtr;
}



bool FaultMgr::checkForPowerSubsystemFaults(FaultStatusBits const& subsystemMask) {
    /// &&& see PowerSubsystem->set_motor_power.vi and BasicFaultManager->read_faults.vi
    FaultStatusBits faultBitmap(subsystemMask.getBitmap()
            & _faultEnableMask.getBitmap() & _currentFaults.getBitmap());
    if (faultBitmap.getBitmap() != 0) {
        LERROR("checkForPowerSubsystemFaults has faults for ", faultBitmap.getAllSetBitEnums());
    }
    return false;
}

FaultMgr::FaultMgr()   {
}

}  // namespace faultmgr
}  // namespace m2cellcpp
}  // namespace LSST

