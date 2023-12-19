/**+
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
#include "control/MotionEngine.h"

// Project headers
#include "faultmgr/FaultMgr.h"
#include "system/Config.h"
#include "util/Bug.h"
#include "util/Log.h"

using namespace std;

namespace LSST {
namespace m2cellcpp {
namespace control {

MotionEngine::Ptr MotionEngine::_thisPtr;
mutex MotionEngine::_thisPtrMtx;

void MotionEngine::setup() {
    lock_guard<mutex> lock(_thisPtrMtx);
    if (_thisPtr) {
        LERROR("FpgaIo already setup");
        return;
    }
    _thisPtr = Ptr(new MotionEngine());
}

MotionEngine::~MotionEngine() {
    engineStop();
    engineJoin();
}

MotionEngine::Ptr MotionEngine::getPtr() {
    // No mutex needed as once set by `setup()`, the value of _thisPtr
    // cannot change.
    if (_thisPtr == nullptr) {
        throw system::ConfigException(ERR_LOC, "MotionEngine has not been setup.");
    }
    return _thisPtr;
}

MotionEngine& MotionEngine::get() {
    if (_thisPtr == nullptr) {
        throw system::ConfigException(ERR_LOC, "MotionEngine has not been setup.");
    }
    return *_thisPtr;
}

MotionEngine::MotionEngine() {}

void MotionEngine::engineStart() {
    LINFO("MotionEngine::mCtrlStart() running threads");
    if ((_eStarted.exchange(true) == true) || _eStopCalled == true) {
        throw util::Bug(ERR_LOC, "MotionEngine::engineStart() can only be called once eStarted=" +
                                         to_string(_eStarted) + " eEnded=" + to_string(_eStopCalled));
    }

    _comReadTime = util::CLOCK::now();
    _eThrd.run();

    // This function will be run in a separate thread until the destructor is called.
    // The destructor sets _timeoutLoop false and then joins this thread.
    auto func = [this]() {
        while (_timeoutLoop) {
            queueTimeoutCheck();
            this_thread::sleep_for(_timeoutSleep);
        }
    };
    thread tThrd(func);
    _timeoutThread = move(tThrd);
}

void MotionEngine::waitForEngine() const {
    // If _eStarted is true, the event queue exists, so messages won't be lost,
    // and the thread should actually be running very soon.
    while (!_eStarted) {
        this_thread::sleep_for(50ms);
    }
}

bool MotionEngine::engineStop() {
    LDEBUG("MotionEngine::engineStop _eStopCalled=", to_string(_eStopCalled));
    if (_eStopCalled.exchange(true) == true) {
        LWARN("MotionEngine::engineStop() has already been called");
        return false;
    }

    _timeoutLoop = false;
    _eThrd.queEnd();
    return true;
}

void MotionEngine::engineJoin() {
    if (_eJoinCalled.exchange(true) == true) {
        LWARN("MotionEngine::engineJoin() has already been called");
        return;
    }
    if (_eStarted) {
        _eThrd.join();
    }
    if (_timeoutThread.joinable()) {
        _timeoutThread.join();
    }
}

void MotionEngine::queueTimeoutCheck() {
    auto cmdTimeoutCheck = std::make_shared<util::Command>([&](util::CmdData*) { _comTimeoutCheck(); });
    _eThrd.queCmd(cmdTimeoutCheck);
}

void MotionEngine::_comTimeoutCheck() {
    auto now = util::CLOCK::now();
    auto diff = (util::timePassedSec(_comReadTime, now));
    bool timedOut = _checkTimeout(diff);
    if (timedOut) {
        stringstream os;
        time_t tm = util::steadyToTimeT(_comReadTime);
        os << "MotionEngine::_comTimeoutCheck timedOut last read=" << ctime(&tm)
           << " seconds since last read=" << diff;
        LDEBUG(os.str());
    }
}

bool MotionEngine::_checkTimeout(double diffInSeconds) {
    bool timedOutErr = diffInSeconds > _comTimeoutErrorSecs;
    if (timedOutErr) {
        string eMsg = string("MotionEngine") + __func__ + " _daq timed out ERROR" + to_string(timedOutErr);
        LDEBUG(eMsg);
        faultmgr::FaultMgr::get().reportMotionEngineTimeout(true, eMsg);
        return true;
    }
    bool timedOutWarn = diffInSeconds > _comTimeoutWarnSecs;
    if (timedOutWarn) {
        string eMsg = string("MotionEngine") + __func__ + " _daq timed out WARN" + to_string(timedOutWarn);
        LDEBUG(eMsg);
        faultmgr::FaultMgr::get().reportMotionEngineTimeout(false, eMsg);
    }
    return false;
}

}  // namespace control
}  // namespace m2cellcpp
}  // namespace LSST
