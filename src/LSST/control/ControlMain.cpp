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
#include "control/ControlMain.h"

// system headers
#include <ctime>

// project headers
#include "faultmgr/FaultMgr.h"
#include "system/Config.h"
#include "util/Bug.h"
#include "util/Log.h"

using namespace std;

namespace LSST {
namespace m2cellcpp {
namespace control {

ControlMain::Ptr ControlMain::_thisPtr;
std::mutex ControlMain::_thisPtrMtx;

void ControlMain::setup() {
    lock_guard<mutex> lock(_thisPtrMtx);
    if (_thisPtr) {
        LERROR("ControlMain already setup");
        return;
    }
    _thisPtr = Ptr(new ControlMain());
}

ControlMain::Ptr ControlMain::getPtr() {
    if (_thisPtr == nullptr) {
        throw system::ConfigException(ERR_LOC, "ControlMain has not been setup.");
    }
    return _thisPtr;
}

ControlMain& ControlMain::get() {
    if (_thisPtr == nullptr) {
        throw system::ConfigException(ERR_LOC, "ControlMain has not been setup.");
    }
    return *_thisPtr;
}

ControlMain::ControlMain() {}

ControlMain::~ControlMain() {}

}  // namespace control
}  // namespace m2cellcpp
}  // namespace LSST
