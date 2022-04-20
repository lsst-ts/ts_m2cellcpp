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

#ifndef LSST_M2CELLCPP_UTIL_COMMAND_H
#define LSST_M2CELLCPP_UTIL_COMMAND_H

// System headers
#include <condition_variable>
#include <functional>
#include <memory>
#include <mutex>

namespace LSST {
namespace m2cellcpp {
namespace util {

/// Tracker provides an interface for indicating an action is complete.
///
class Tracker {
public:
    enum class Status { INPROGRESS, COMPLETE };
    using Ptr = std::shared_ptr<Tracker>;

    Tracker() = default;
    virtual ~Tracker() = default;

    Tracker(Tracker const&) = delete;
    Tracker& operator=(Tracker const&) = delete;
    Tracker(Tracker&&) = delete;

    /// Set status to COMPLETE and notify everyone waiting for a status change.
    void setComplete();

    /// @return true if the action is complete without waiting.
    bool isFinished();

    /// Wait until this Tracker's action is complete.
    void waitComplete();

private:
    Status _trStatus{Status::INPROGRESS};  ///< current status
    std::mutex _trMutex;                   ///< protects status
    std::condition_variable _trCV;         ///< for signaling
};

/// Base class to allow arbitrary data to be passed to or returned from
/// Command::action.
struct CmdData {
    virtual ~CmdData(){};
};

/// Base class for commands. Can be used with functions as is or
/// as a base class when data is needed.
class Command {
public:
    using Ptr = std::shared_ptr<Command>;
    Command() = default;
    explicit Command(std::function<void(CmdData*)> func) : _func(func) {}
    virtual ~Command() = default;

    Command(Command const&) = delete;
    Command& operator=(Command const&) = delete;
    Command(Command&&) = delete;

    /// The action to take when running the `Command`.
    /// For simple cases, redefining `_func` with `setFunc` is a
    /// good option. More complex opperations or cases
    /// where `actionComplete` is required need child classes.
    virtual void action(CmdData* data) { _func(data); };

    /// Placeholder for child classes to take special action
    /// when the command is done running.
    virtual void actionComplete(CmdData*) {}

    /// Run `action` and call `actionComplete` when done.
    void runAction(CmdData* data) {
        action(data);
        actionComplete(data);
    }

    /// Change the function called when the Command is activated.
    /// nullptr is replaced with a nop function.
    void setFunc(std::function<void(CmdData*)> func);

    /// Set `_func` to nullptr.
    void resetFunc();

protected:
    /// Function to run when `action` is run. Child classes
    /// may alter `action(CmdData*)` so that this variable is irrelevant.
    std::function<void(CmdData*)> _func = [](CmdData*) { ; };
};

/// Extension of Command that can notify other threads when its
/// action is complete.
class CommandTracked : public Command, public Tracker {
public:
    using Ptr = std::shared_ptr<CommandTracked>;

    CommandTracked() = default;
    explicit CommandTracked(std::function<void(CmdData*)> func) : Command(func) {}
    ~CommandTracked() override = default;

    /// Overriden `Command::actionComplete` to call `Tracker::setComplete`
    void actionComplete(CmdData*) override { setComplete(); };
};

}  // namespace util
}  // namespace m2cellcpp
}  // namespace LSST

#endif  // LSST_M2CELLCPP_UTIL_COMMAND_H
