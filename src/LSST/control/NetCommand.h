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
#ifndef LSST_M2CELLCPP_SYSTEM_COMMAND_H
#define LSST_M2CELLCPP_SYSTEM_COMMAND_H

// System headers
#include <memory>
#include <mutex>
#include <string>

// Third party headers
#include <nlohmann/json.hpp>

// Project headers

namespace LSST {
namespace m2cellcpp {
namespace control {

class NetCommandFactory;

/// Exception specific to NetCommand and its ilk.
class NetCommandException : public std::runtime_error {
public:
    NetCommandException(std::string const& msg) : std::runtime_error(msg) {}
};

/// The base class for all commands received over the network.
///
/// The children of this class can be created from a `json` object
/// where the "id" defines which child class will be used. "id"
/// and "seq_id" are the only parameters expected in all `json`
/// objects. The child classes may require other keys be set.
/// They should set `ackJson["id"] = "ack";` after all expected 
/// items have been parsed successfully, and throw 
/// NetCommandException when required items are missing/invalid.
///
/// The NetCommandFactory requires dummy versions of all the 
/// child classes it should understand. It uses the dummy classes
/// to identify the incoming "id" strings and then create the
/// correct child class.
///
/// While this (base class) does not implement it, all child classes
/// must have `static Ptr createFactoryVersion()` defined to
/// return a dummy instance of the child class. 
/// See `NCmdAck::createFactoryVersion()` for an example.
///
/// Since basic communications errors should not crash the program,
/// This class and its ilk should throw NetCommandException
/// when problems arise so they can be caught before causing undu 
/// harm. (This does not apply to segfaults or serious underlying 
/// issues that should cause termination.)
class NetCommand : public std::enable_shared_from_this<NetCommand> {
public:
    using Ptr = std::shared_ptr<NetCommand>;
    using JsonPtr = std::shared_ptr<nlohmann::json>;

    /// Create a NetCommand object from the received json object.
    /// Hidden constructor to ensure proper construction of
    /// enable_shared_From_this object.
    static Ptr create(JsonPtr const& inJson);

    NetCommand(NetCommand const&) = delete;
    NetCommand& operator=(NetCommand const&) = delete;
    virtual ~NetCommand() = default;

    /// Try to parse inStr and return a json object.
    /// @return a json object if the
    static JsonPtr parse(std::string const& inStr);
    //&&&static std::shared_ptr<nlohmann::json> parse(std::string const& inStr);

    /// @return the name of the command this specific class handles.
    virtual std::string getCommandName() const { return "cmd_"; } // &&& make pure virtual

    /// @ return a new NetCommand child class object based on 'inJson'.
    /// This method is meant for use by NetCommandFactory. The child
    /// class implementations should return a new object of their
    /// class. The NCmdAck class should return a NCmdAck object,
    /// NCmdEcho should return a NCmdEcho object, etc.
    /// @return A shared pointer to a child class of NetCommand.
    /// @throws NetCommandException if there are any problems.
    virtual Ptr createNewNetCommand(JsonPtr const& inJson) { return nullptr; } // &&& make pure virtual

    std::string getId() const { return _id; } //&&& change to getName()

    uint64_t getSeqId() const { return _seqId; }

    /// Set the user_info json field.
    void setAckUserInfo(std::string const& msg) {
        ackJson["user_info"] = msg;
    };

    /// Run the action function and set some respJson values.
    /// @return true if the action() was successful.
    bool run();

    /// @return a json string version of the acknowledgment, aka 'ack'.
    std::string getAckJsonStr();

    /// @return a json string version of the final response
    std::string getRespJsonStr();

protected:
    NetCommand(JsonPtr const& json);

    /// Execute the action this particular NetCommand needs to take.
    /// @return true if successful.
    virtual bool action() { return false; }; //&&& make pure virtual

    /// This consturctor is ONLY to be used in createFactoryVersion()/
    /// This constructor makes a dummy version of the object that is only
    /// used to create new instances of that class. 
    /// The `action()` method of dummy instances should never be run. 
    /// @see NetCommandFactory::getCommandFor(std::string const& jsonStr)
    /// @see createNewNetCommand(JsonPtr const& inJson)
    /// @see child class createFactoryVersion()
    NetCommand() = default;
    JsonPtr inJson; ///< json representation of the received command.
    /// json used to create return ack.
    nlohmann::json ackJson = {{"id","noack"},{"seq_id",0},{"user_info", ""}};
    /// json used to create the final response
    nlohmann::json respJson = {{"id","fail"},{"seq_id",0},{"user_info", ""}};
private:
    std::string _id = "none"; ///< Name of the command. //&&& rename this to _name, "id" changes meaning
    uint64_t _seqId = 0; ///< Sequence number of command.
};


/// NetCommand to simply respond with an ack, and a success.
/// This class is meant to be useful for testing and diagnostics.
/// id and seq_id are the only json parameters expected.
class NCmdAck : public NetCommand {
public:
    using Ptr = std::shared_ptr<NCmdAck>;

    /// &&& doc
    static Ptr create(JsonPtr const& inJson);

    virtual ~NCmdAck() = default;

    /// @return a version of NCmdAck to be used to generate commands.
    /// This returns basically a dummy version of the class that
    /// is only to be used
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


class NCmdNoAck : public NetCommand {
public:
    using Ptr = std::shared_ptr<NCmdNoAck>;

    /// &&& doc
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


class NCmdEcho : public NetCommand {
public:
    using Ptr = std::shared_ptr<NCmdEcho>;

    /// &&& doc
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

    std::string _msg; ///< The message to echo.
};


/// This class receives json strings and returns the appropriate NetCommand instance.
///&&& doc
class NetCommandFactory : public std::enable_shared_from_this<NetCommandFactory> {
public:
    using Ptr = std::shared_ptr<NetCommandFactory>;

    /// Private constructor to ensure proper enabled_shared_from_this construction.
    static Ptr create();

    /// Add a NetCommand to the map of know commands.
    /// @throws NetCommandException if the cmd is already in the map.
    void addNetCommand(NetCommand::Ptr const& cmd);

    /// Get the NetCommand appropriate for the jsonStr.
    /// @return Appropriate NetCommand. _defaultNoAck is returned on 
    ///         unknown commands that have a parsable id and seq_id.
    /// @throws NetCommandException if there are any problems.
    NetCommand::Ptr getCommandFor(std::string const& jsonStr);

    /// Used to change the value of _defaultNoAck if NCmdNoAck is incorrect.
    void setDefaultNoAck();

private:
    NetCommandFactory() = default;

    /// The command to use when there are lesser parsing problems
    /// or unknown commands.
    NetCommand::Ptr _defaultNoAck{NCmdNoAck::createFactoryVersion()};

    std::mutex _mtx; ///< protects _cmdMap, _prevSeqId;
    std::map<std::string, std::shared_ptr<NetCommand>> _cmdMap;
    uint64_t _prevSeqId = 0; ///< Value of the previous seqId.
};

}  // namespace control
}  // namespace m2cellcpp
}  // namespace LSST

#endif // LSST_M2CELLCPP_SYSTEM_COMMAND_H
