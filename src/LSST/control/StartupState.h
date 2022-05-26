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

#ifndef LSST_M2CELLCPP_CONTROL_STARTUPSTATE_H
#define LSST_M2CELLCPP_CONTROL_STARTUPSTATE_H

// System headers
#include <functional>
#include <map>
#include <memory>

// Project headers
#include "control/State.h"

namespace LSST {
namespace m2cellcpp {
namespace control {

/// This class represents the state when the system is started.
class StartupState : public State {
public:
    using Ptr = std::shared_ptr<StartupState>;

    /// Create an instance and insert it into `stateMap`.
    /// @throws Bug if there's already an instance of this class in `stateMap`.
    static Ptr create(StateMap& stateMap);

    StartupState(StartupState const&) = delete;
    StartupState& operator=(StartupState const&) = delete;
    virtual ~StartupState() = default;

private:
    StartupState() : State("StartupState") {}
};

}  // namespace control
}  // namespace m2cellcpp
}  // namespace LSST

#endif  // LSST_M2CELLCPP_CONTROL_STARTUPSTATE_H
