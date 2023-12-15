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
#include "control/control_defs.h"
#include "control/NetCommand.h"

namespace LSST {
namespace m2cellcpp {
namespace control {

/// This class is used to handle the "cmd_switchCommandSource" command.
/// The "isRemote" item indicates that the GUI should be used when the value is `true`.
/// `false` would indicate the use of SAL.
/// For the purposes of m2cellcpp, "isRemote" should always be true.
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
    /// Set the value of `Globals::_commandableByDds` to the value of "isRemote",
    /// and broadcasts the new value to all clients.
    bool action() override;

private:
    NCmdSwitchCommandSource(JsonPtr const& json);
    NCmdSwitchCommandSource() : NetCommand() {}

    bool _isRemote = 1;  ///< Value of "isRemote" section of message.
};


/// This class handles the "cmd_power" message.
/// - "powerType" must be `1` (for MOTOR) or `2` (for COMM) (see PowerSystemType).
/// - "status" `true` indicates the "powerType" should be turned on, while `false` turns it off.
/// - MOTOR power should never be on if COMM power is not on.
/// Expected message form is: {'powerType': 2, 'status': true, 'id': 'cmd_power', 'sequence_id': 123}
/// This would turn on COMM power.
///
/// unit test: test_startup_shutdown.cpp
class NCmdPower : public NetCommand {
public:
    using Ptr = std::shared_ptr<NCmdPower>;

    /// @return a new NCmdPower object based on inJson.
    static Ptr create(JsonPtr const& inJson);

    virtual ~NCmdPower() = default;

    /// @return a version of NCmdPower to be used to generate commands.
    static Ptr createFactoryVersion() { return Ptr(new NCmdPower()); }

    /// @return the name of the command this specific class handles.
    std::string getCommandName() const override { return "cmd_power"; }

    /// @return a new NCmdPower object using the parameters in 'inJson'
    NetCommand::Ptr createNewNetCommand(JsonPtr const& inJson) override;

protected:
    /// Turn the `MOTOR` or `COMM` power on or off.
    bool action() override;

private:
    NCmdPower(JsonPtr const& json);
    NCmdPower() : NetCommand() {}

    /// Value of "powerType" section of message, where 1 is `MOTOR` and 2 is `COMM`.
    PowerSystemType _powerType = PowerSystemType::MOTOR;
    /// Value of "status" section of message, where `true` means turn on, and `false`
    /// means turn off.
    bool _status = false;
};


/// This class handles the "cmd_systemShutdown" message.
/// Expected message form is: {'id': 'cmd_systemShutdown', 'sequence_id': 123}
/// This would shutdown the entire system.
///
/// unit test: test_startup_shutdown.cpp
class NCmdSystemShutdown : public NetCommand {
public:
    using Ptr = std::shared_ptr<NCmdSystemShutdown>;

    /// @return a new NCmdSystemShutdown object based on inJson.
    static Ptr create(JsonPtr const& inJson);

    virtual ~NCmdSystemShutdown() = default;

    /// @return a version of NCmdSystemShutdown to be used to generate commands.
    static Ptr createFactoryVersion() { return Ptr(new NCmdSystemShutdown()); }

    /// @return the name of the command this specific class handles.
    std::string getCommandName() const override { return "cmd_systemShutdown"; }

    /// @return a new NCmdSystemShutdown object using the parameters in 'inJson'
    NetCommand::Ptr createNewNetCommand(JsonPtr const& inJson) override;

protected:
    /// Shutdown the entire system.
    bool action() override;

private:
    NCmdSystemShutdown(JsonPtr const& json);
    NCmdSystemShutdown() : NetCommand() {}
};


}  // namespace control
}  // namespace m2cellcpp
}  // namespace LSST

#endif  // LSST_M2CELLCPP_CONTROL_NETCOMMANDDEFS_H
