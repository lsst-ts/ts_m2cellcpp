// -*- LSST-C++ -*-
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
#include "util/Command.h"

using namespace std;

namespace LSST {
namespace m2cellcpp {
namespace util {

/// Set status to COMPLETE and notify everyone waiting for a status change.
void Tracker::setComplete() {
    {
        lock_guard<mutex> lock(_trMutex);
        _trStatus = Status::COMPLETE;
    }
    _trCV.notify_all();
}

/// Check if the action is complete without waiting.
bool Tracker::isFinished() {
    lock_guard<mutex> lock(_trMutex);
    return _trStatus == Status::COMPLETE;
}

/// Wait until this Tracker's action is complete.
void Tracker::waitComplete() {
    unique_lock<mutex> lock(_trMutex);
    _trCV.wait(lock, [this]() { return _trStatus == Status::COMPLETE; });
}

/// Change the function called when the Command is activated.
/// nullptr is replaced with a nop function.
void Command::setFunc(function<void(CmdData*)> func) {
    if (func == nullptr) {
        _func = [](CmdData*) { ; };
    } else {
        _func = func;
    }
}

/// If _func is a lambda with a captured shared_ptr to this command,
/// this function must be called or the lambda will keep this object alive.
void Command::resetFunc() { setFunc(nullptr); }

}  // namespace util
}  // namespace m2cellcpp
}  // namespace LSST
