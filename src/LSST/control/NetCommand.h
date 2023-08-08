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
#ifndef LSST_M2CELLCPP_CONTROL_NETCOMMAND_H
#define LSST_M2CELLCPP_CONTROL_NETCOMMAND_H

// System headers
#include <memory>
#include <mutex>
#include <string>

// Third party headers
#include <nlohmann/json.hpp>

// Project headers
#include "util/Issue.h"

namespace LSST {
namespace m2cellcpp {
namespace control {

class NetCommandFactory;

/// Exception specific to NetCommand and its ilk.
/// @see class NetCommand
///
/// unit test: test_NetCommand.cpp
class NetCommandException : public util::Issue {
public:
    NetCommandException(Context const& ctx, std::string const& msg) : util::Issue(ctx, msg) {}
};

/// The base class for all commands received over the network.
///
/// The children of this class can be created from a `json` object
/// where the "id" defines which child class will be used. "id"
/// and "sequence_id" are the only parameters expected in all `json`
/// objects. The child classes may require other keys be set.
/// They should set `ackJson["id"] = "ack";` after all expected
/// items have been parsed successfully, and throw
/// NetCommandException when required items are missing/invalid.
///
/// The NetCommandFactory requires `FactoryVersions` of all the
/// child classes it should understand. It uses the `FactoryVersion`
/// to identify the incoming "id" strings and then create the
/// correct child class.
/// While this (base class) does not implement it, all child classes
/// must have `static Ptr createFactoryVersion()` defined to
/// return a `FactoryVersion` instance of the child class.
/// See `NCmdAck::createFactoryVersion()` for an example.
///
/// Since basic communications errors should not crash the program,
/// this class and its ilk should throw NetCommandException
/// when problems arise so they can be caught before causing undue
/// harm. (This does not apply to segfaults or serious underlying
/// issues that should cause termination.)
///
/// unit test: test_NetCommand.cpp
class NetCommand : public std::enable_shared_from_this<NetCommand> {
public:
    using Ptr = std::shared_ptr<NetCommand>;
    using JsonPtr = std::shared_ptr<nlohmann::json>;

    NetCommand(NetCommand const&) = delete;
    NetCommand& operator=(NetCommand const&) = delete;
    virtual ~NetCommand() = default;

    /// Try to parse inStr and return a json object.
    /// @return a json object if `inStr` contains at least
    ///     a valid "id" and "sequence_id".
    /// @throw NetCommandException if there are issues with parsing
    ///     `inStr`
    static JsonPtr parse(std::string const& inStr);

    /// @return a new NetCommand child class object based on 'inJson'.
    /// This method is meant for use by NetCommandFactory. The child
    /// class implementations should return a new object of their
    /// class. The NCmdAck class should return a NCmdAck object,
    /// NCmdEcho should return a NCmdEcho object, etc.
    /// @throws NetCommandException if there are any problems.
    virtual Ptr createNewNetCommand(JsonPtr const& inJson) = 0;

    /// @return the name of the command this specific class handles.
    /// Each child class should have a unique value returned for this command
    /// that matches what is expected from clients.
    /// i.e. NCmdEcho::getCommandName() always returns "cmd_echo". If another
    /// NetCommand class returns that value, they cannot be registered in the
    /// same NetCommandFactory.
    virtual std::string getCommandName() const = 0;

    /// @return the command name that was parsed from `inJson`.
    std::string getName() const { return _name; }

    /// @return the sequence number.
    uint64_t getSeqId() const { return _seqId; }

    /// Set the user_info json field.
    void setAckUserInfo(std::string const& msg) { ackJson["user_info"] = msg; };

    /// Run the action function and set respJson["id"] to success or fail.
    /// If the action() was successful, set respJson["id"] to "success".
    /// Otherwise, set it to "fail"
    /// @return true if the action() was successful.
    /// This will probably need to run in its own thread.
    bool run();

    /// @return a json string version of the acknowledgment, aka 'ack'.
    std::string getAckJsonStr();

    /// @return a json string version of the final response
    std::string getRespJsonStr();

protected:
    /// Protected to ensure proper construction of enable_shared_From_this object.
    /// @param json must contain "id" and "sequence_id" fields.
    NetCommand(JsonPtr const& json);

    /// Execute the action this particular NetCommand needs to take.
    /// @return true if successful.
    virtual bool action() = 0;

    /// This consturctor is ONLY to be used in createFactoryVersion()/
    /// This constructor makes a dummy version of the object that is only
    /// used to create new instances of that class.
    /// The `action()` method of FactoryVersion instances should never be run.
    /// @see NetCommandFactory::getCommandFor(std::string const& jsonStr)
    /// @see createNewNetCommand(JsonPtr const& inJson)
    /// @see child class createFactoryVersion()
    NetCommand() = default;
    JsonPtr inJson;  ///< json representation of the received command.
    /// json used to create return ack.
    nlohmann::json ackJson = {{"id", "noack"}, {"sequence_id", 0}, {"user_info", ""}};
    /// json used to create the final response
    nlohmann::json respJson = {{"id", "fail"}, {"sequence_id", 0}, {"user_info", ""}};

private:
    std::string _name = "none";  ///< Name of the command as parsed from `inJson`.
    uint64_t _seqId = 0;         ///< Sequence number of command.
};

/// NetCommand to simply respond with an ack, and a success.
/// This class is meant to be useful for testing and diagnostics.
/// id and sequence_id are the only json parameters expected.
///
/// unit test: test_NetCommand.cpp
class NCmdAck : public NetCommand {
public:
    using Ptr = std::shared_ptr<NCmdAck>;

    /// @return a new NCmdAck object based on inJson.
    static Ptr create(JsonPtr const& inJson);

    virtual ~NCmdAck() = default;

    /// @return a version of NCmdAck to be used to generate commands.
    /// This returns basically a dummy version of the class that
    /// is only to be used to create new instances.
    static Ptr createFactoryVersion() { return Ptr(new NCmdAck()); }

    /// @return the name of the command this specific class handles.
    std::string getCommandName() const override { return "cmd_ack"; }

    /// @return a new NCmdAck object using the parameters in 'inJson'
    NetCommand::Ptr createNewNetCommand(JsonPtr const& inJson) override;

protected:
    /// NCmdAck always succeeds
    bool action() override { return true; }

private:
    NCmdAck(JsonPtr const& json);
    NCmdAck() : NetCommand() {}
};

/// NCmdNoAck is used to reply with a "noack" message.
/// This class is used to respond to NetCommand requests
/// that have an error in the initial request. This
/// includes unknown commands, missing parameters,
/// and sequence_id issues. This command can be added to
/// a NetCommandFactory, but it will always respond
/// with "noack" and then "fail".
///
/// unit test: test_NetCommand.cpp
class NCmdNoAck : public NetCommand {
public:
    using Ptr = std::shared_ptr<NCmdNoAck>;

    /// @return a new NCmdNoAck object based on inJson.
    static Ptr create(JsonPtr const& inJson);

    virtual ~NCmdNoAck() = default;

    /// @return a version of NCmdAck to be used to generate commands.
    static Ptr createFactoryVersion() { return Ptr(new NCmdNoAck()); }

    /// @return the name of the command this specific class handles.
    std::string getCommandName() const override { return "cmd_noack"; }

    /// @return a new NCmdNoAck object using the parameters in 'inJson'
    NetCommand::Ptr createNewNetCommand(JsonPtr const& inJson) override;

protected:
    // NCmdNoAck always fails
    bool action() override { return false; }

private:
    NCmdNoAck(JsonPtr const& json);
    NCmdNoAck() : NetCommand() {}
};

/// This class sends back the `inJson["msg"]` value in `respJson["msg"]`.
/// `inJson["msg"]` is a required field for this command.
///
/// unit test: test_NetCommand.cpp
class NCmdEcho : public NetCommand {
public:
    using Ptr = std::shared_ptr<NCmdEcho>;

    /// @return a new NCmdEcho object based on inJson.
    static Ptr create(JsonPtr const& inJson);

    virtual ~NCmdEcho() = default;

    /// @return a version of NCmdAck to be used to generate commands.
    static Ptr createFactoryVersion() { return Ptr(new NCmdEcho()); }

    /// @return the name of the command this specific class handles.
    std::string getCommandName() const override { return "cmd_echo"; }

    /// @return a new NCmdNoAck object using the parameters in 'inJson'
    NetCommand::Ptr createNewNetCommand(JsonPtr const& inJson) override;

protected:
    // NCmdNoAck always fails
    bool action() override;

private:
    NCmdEcho(JsonPtr const& json);
    NCmdEcho() : NetCommand() {}

    std::string _msg;  ///< The message to echo.
};

}  // namespace control
}  // namespace m2cellcpp
}  // namespace LSST

#endif  // LSST_M2CELLCPP_CONTROL_NETCOMMAND_H
