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
#include "util/EventThread.h"

// System headers
#include <algorithm>

// Project headers
#include "util/Bug.h"
#include "util/Log.h"

using namespace std;

namespace LSST {
namespace m2cellcpp {
namespace util {

/* &&&
void CommandQueue::queCmd(Command::Ptr const& cmd) {
    {
        lock_guard<mutex> lock(_mx);
        _qu.push_back(cmd);
    }
    notify(false); // notify one thread
}
*/

void EventThread::queCmd(Command::Ptr cmd) {
    if (cmd == nullptr) {
        throw util::Bug(ERR_LOC, "EventThread::queCmd was nullptr");
    }
    _q->queCmd(cmd);
}

/// Handle commands as they arrive until queEnd() is called.
void EventThread::_handleCmds() {
    startup();
    while (_loop) {
        _cmd = _q->getCmd();
        _commandFinishCalled = false;
        _currentCommand = _cmd.get();
        if (_cmd != nullptr) {
            _q->commandStart(_cmd);
            specialActions(_cmd);
            _cmd->runAction(this);
            callCommandFinish(_cmd);
            // Reset _func in case it has a captured Command::Ptr,
            // which would keep it alive indefinitely.
            _cmd->resetFunc();
        }
        _cmd.reset();
        _currentCommand = nullptr;
    }
    finishup();
}

/// Ensure that commandFinish is only called once per loop.
void EventThread::callCommandFinish(Command::Ptr const& cmd) {
    if (_commandFinishCalled.exchange(true) == false) {
        _q->commandFinish(cmd);
    }
}

/// call this to start the thread
void EventThread::run() {
    thread t(&EventThread::_handleCmds, this);
    _t = move(t);
}

EventThreadJoiner::EventThreadJoiner() {
    thread t(&EventThreadJoiner::joinLoop, this);
    _tJoiner = move(t);
}

EventThreadJoiner::~EventThreadJoiner() {
    if (_continue) {
        LCRITICAL("~EventThreadJoiner() called without shutdownJoiner() being called");
    }
}

void EventThreadJoiner::shutdownJoin() {
    _continue = false;
    LDEBUG("Waiting for joiner thread to finish.");
    _tJoiner.join();
}

void EventThreadJoiner::joinLoop() {
    EventThread::Ptr evThrd;
    while (true) {
        unique_lock<mutex> ulock(_mtxJoiner);
        if (!_eventThreads.empty()) {
            evThrd = _eventThreads.front();
            _eventThreads.pop();
            ulock.unlock();
            evThrd->join();
            evThrd.reset();
            int c = --_count;
            LDEBUG("joined count=", c);
        } else {
            if (!_continue) break;
            ulock.unlock();
            this_thread::sleep_for(_sleepTime);
        }
    }
    LDEBUG("join loop exiting");
}

void EventThreadJoiner::addThread(EventThread::Ptr const& eventThread) {
    if (eventThread == nullptr) return;
    lock_guard<mutex> lg(_mtxJoiner);
    ++_count;
    _eventThreads.push(eventThread);
}

}  // namespace util
}  // namespace m2cellcpp
}  // namespace LSST
