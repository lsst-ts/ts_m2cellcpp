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
namespace control {


/// &&& doc
class ControlMain {
public:
    using Ptr = std::shared_ptr<ControlMain>;

    /// Create the global ControlMain object.
    static void setup();


    ControlMain(ControlMain const&) = delete;
    ControlMain& operator=(ControlMain const&) = delete;

    /// Verify threads are stopped and joined.
    ~ControlMain();

    /// Return a reference to the global ControlMain instance.
    /// @throws `ConfigException` if `setup` has not already been called.
    static ControlMain& get();

    /// Return a shared pointer to the global ControlMain instance.
    /// @throws `ConfigException` if `setup` has not already been called.
    static Ptr getPtr();

    /// &&& doc
    void startThrd();

    /// &&& doc
    void stopThrd();

    /// &&& doc
    void joinThrd();

private:
    static Ptr _thisPtr; ///< Pointer to the global instance of ControlMain.
    static std::mutex _thisPtrMtx; ///< Protects `_thisPtr`.

    /// &&& doc
    ControlMain();

    util::EventThread _evCThrd; /// Event thread, primariliy meant to wait for shutdown.
    std::atomic<bool> _evCThrdStarted{false}; /// Set to true when started.
    std::atomic<bool> _evCthrdJoinCalled{false}; /// Set to true when join is called.

};

}  // namespace control
}  // namespace m2cellcpp
}  // namespace LSST

#endif  // LSST_M2CELLCPP_CONTROL_CONTROLMAIN_H
