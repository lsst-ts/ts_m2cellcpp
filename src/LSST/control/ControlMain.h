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

#ifndef LSST_M2CELLCPP_CONTROL_CONTROLMAIN_H
#define LSST_M2CELLCPP_CONTROL_CONTROLMAIN_H

// System headers
#include <memory>
#include <string>

// Project headers
#include "util/EventThread.h"

namespace LSST {
namespace m2cellcpp {

namespace simulator {
class SimCore;
}

namespace system {
class ComControlServer;
}

namespace control {

/// PLACEHOLDER This class will contain a thread running the main instance of the program.
class ControlMain {
public:
    using Ptr = std::shared_ptr<ControlMain>;

    /// Create the global ControlMain object.
    static void setup();

    ControlMain(ControlMain const&) = delete;
    ControlMain& operator=(ControlMain const&) = delete;

    /// Verify threads are stopped and joined.
    ~ControlMain();

    /// Return a shared pointer to the global ControlMain instance.
    /// @throws `ConfigException` if `setup` has not already been called.
    static Ptr getPtr();

    /// &&& doc
    void run(int argc, const char* argv[]);

    /// Returns true once the server is running.
    bool getRunning() { return _running; }

    /// Return a pointer to `_simCore`.
    std::shared_ptr<simulator::SimCore> getSimCore() { return _simCore; }


    /// &&& doc
    void stop();

    /// &&& doc
    void join();

    /// &&& doc
    std::shared_ptr<system::ComControlServer> getComServer() {
        return _comServer;
    }

private:
    static Ptr _thisPtr;            ///< Pointer to the global instance of ControlMain.
    static std::mutex _thisPtrMtx;  ///< Protects `_thisPtr`.

    /// Private constructor to force the use of `setup()`.
    ControlMain();

    /// &&& doc
    void _cMain(int argc, const char* argv[]);

    std::thread _mainThrd; ///< &&& doc

    /// Pointer to the system ComControllServer.
    std::shared_ptr<system::ComControlServer> _comServer;

    /// Point to the simulator instance, if there is one.
    /// This is only used for testing.
    std::shared_ptr<simulator::SimCore> _simCore;

    std::atomic<bool> _running{false}; ///< Set to true once the server is running.

};

}  // namespace control
}  // namespace m2cellcpp
}  // namespace LSST

#endif  // LSST_M2CELLCPP_CONTROL_CONTROLMAIN_H
