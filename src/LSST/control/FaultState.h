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

#ifndef LSST_M2CELLCPP_CONTROL_FAULTSTATE_H
#define LSST_M2CELLCPP_CONTROL_FAULTSTATE_H

// System headers
#include <functional>
#include <map>
#include <memory>

// Project headers
#include "control/State.h"

namespace LSST {
namespace m2cellcpp {
namespace control {

/// Class representation of the "FaultState", this state appears to be unused in LabView.
class FaultState : public State {
public:
    using Ptr = std::shared_ptr<FaultState>;

    /// Create an instance and insert it into `stateMap`.
    /// @throws Bug if there's already an instance of this class in `stateMap`.
    static Ptr create(StateMap& stateMap);

    FaultState(FaultState const&) = delete;
    FaultState& operator=(FaultState const&) = delete;
    virtual ~FaultState() = default;
    // VI-PH  clearErrorVI   // calls Model::resetErrorCodeVI then Model::changeStateVI(StandbyState)
    // VI-PH  goToStandbyVI  // calls Model::resetErrorCodeVI then Model::stopVI then
    // Model::changeStateVI(StandbyState)

private:
    FaultState() : State("FaultState") {}
};

}  // namespace control
}  // namespace m2cellcpp
}  // namespace LSST

#endif  // LSST_M2CELLCPP_CONTROL_FAULTSTATE_H
