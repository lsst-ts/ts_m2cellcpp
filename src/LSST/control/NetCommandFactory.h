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
#ifndef LSST_M2CELLCPP_CONTROL_NETCOMMANDFACTORY_H
#define LSST_M2CELLCPP_CONTROL_NETCOMMANDFACTORY_H

// System headers
#include <memory>
#include <mutex>
#include <string>

// Third party headers

// Project headers
#include "control/NetCommand.h"

namespace LSST {
namespace m2cellcpp {
namespace control {

/// This class receives json strings and returns the appropriate NetCommand instance.
///
/// An instance of this class is meant to be created for each ComConnection. The
/// ComServer instance that creates the connection will supply a list of NetCommand
/// classes that the server is expected to handle using `addNetCommand`. Different
/// servers can then be used to handle different sets of commands.
///
/// The list of commands is represented by a list of `FactoryVersion` of NetCommand
/// child instances. The child instances are used to return a new instance of
/// the same class using the virtual `createNewNetCommand` member function.
///
/// unit test: test_NetCommand.cpp
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

    /// @return an instance of the `_defaultNoAck` command with seqId=0.
    NetCommand::Ptr getNoAck();

private:
    NetCommandFactory() = default;

    /// The command to use when there are lesser parsing problems
    /// or unknown commands.
    NetCommand::Ptr _defaultNoAck{NCmdNoAck::createFactoryVersion()};

    std::mutex _mtx;  ///< protects _cmdMap, _prevSeqId;
    std::map<std::string, std::shared_ptr<NetCommand>> _cmdMap;
    uint64_t _prevSeqId = 0;  ///< Value of the previous seqId.
};

}  // namespace control
}  // namespace m2cellcpp
}  // namespace LSST

#endif  // LSST_M2CELLCPP_CONTROL_NETCOMMANDFACTORY_H
