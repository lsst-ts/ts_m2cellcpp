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
#ifndef LSST_M2CELLCPP_CONTROL_NETCOMMANDDEFS_H
#define LSST_M2CELLCPP_CONTROL_NETCOMMANDDEFS_H

// System headers

// Third party headers

// Project headers
#include "control/NetCommand.h"

namespace LSST {
namespace m2cellcpp {
namespace control {

/// This class sends back the `inJson["msg"]` value in `respJson["msg"]`.
/// `inJson["msg"]` is a required field for this command.
/// Expected message form is: {"isRemote": true, "id": "cmd_switchCommandSource", "sequence_id": 123}
///
/// unit test: test_NetCommand.cpp
class NCmdSwitchCommandSource : public NetCommand {
public:
    using Ptr = std::shared_ptr<NCmdSwitchCommandSource>;

    /// @return a new NCmdSwitchCommandSource object based on inJson.
    static Ptr create(JsonPtr const& inJson);

    virtual ~NCmdSwitchCommandSource() = default;

    /// @return a version of NCmdAck to be used to generate commands.
    static Ptr createFactoryVersion() { return Ptr(new NCmdSwitchCommandSource()); }

    /// @return the name of the command this specific class handles.
    std::string getCommandName() const override { return "cmd_switchCommandSource"; }

    /// @return a new NCmdNoAck object using the parameters in 'inJson'
    NetCommand::Ptr createNewNetCommand(JsonPtr const& inJson) override;

protected:
    // NCmdNoAck always fails
    bool action() override;

private:
    NCmdSwitchCommandSource(JsonPtr const& json);
    NCmdSwitchCommandSource() : NetCommand() {}

    bool _isRemote = 1;  ///< Value of "isRemote" section of message.
};

}  // namespace control
}  // namespace m2cellcpp
}  // namespace LSST

#endif  // LSST_M2CELLCPP_CONTROL_NETCOMMANDDEFS_H
