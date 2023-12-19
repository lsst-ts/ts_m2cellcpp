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
#include "control/Context.h"

// System headers

// Project headers
#include "control/PowerSystem.h"
#include "util/Log.h"

using namespace std;

namespace LSST {
namespace m2cellcpp {
namespace control {

// static Context members
Context::Ptr Context::_thisPtr;
std::mutex Context::_thisMtx;

void Context::setup() {
    {
        lock_guard<mutex> lock(_thisMtx);
        if (_thisPtr) {
            LERROR("Context already setup");
            return;
        }
        _thisPtr = Ptr(new Context());
    }
    _thisPtr->model.getPowerSystem()->setContext(_thisPtr);
}

Context::Ptr Context::get() {
    if (_thisPtr == nullptr) {
        throw util::Bug(ERR_LOC, "Context has not been setup.");
    }
    return _thisPtr;
}

}  // namespace control
}  // namespace m2cellcpp
}  // namespace LSST
