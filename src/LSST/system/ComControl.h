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
#ifndef LSST_M2CELLCPP_SYSTEM_COMCONTROL_H
#define LSST_M2CELLCPP_SYSTEM_COMCONTROL_H

// System headers

// Project headers
#include "control/NetCommandFactory.h"
#include "system/ComConnection.h"

namespace LSST {
namespace m2cellcpp {
namespace system {

/// This class is used to handle a connection to a client that controls
/// the mirror, as opposed to an event monitor. It is based on
/// ComConnection, and multiple clients can connect simultaneously.
/// The commands it is expected to handle should be put in
/// `setupNormalFactory` and the NetCommandFactory should be created
/// before this class and passed to `ComControlServer`. One factory
/// can be shared by multiple `ComControl` and `ComControlServer`
/// objects. Separate servers, on different ports, can be given
/// factories with different sets of `NetCommands`.
class ComControl : public ComConnection {
public:
    using Ptr = std::shared_ptr<ComControl>;

    /// Factory method used to prevent issues with enable_shared_from_this.
    /// @param ioContext asio object for the network I/O operations
    static Ptr create(IoContextPtr const& ioContext, uint64_t connId,
                      std::shared_ptr<ComServer> const& server,
                      control::NetCommandFactory::Ptr const& cmdFactory);

    /// Put NetCommands needed for a normal ComControl interface into `cmdFactory`.
    static void setupNormalFactory(control::NetCommandFactory::Ptr const& cmdFactory);

    /// Use `commandStr` to get a runnable command and ack string from `_cmdFactory`.
    /// @return a json string to use for the 'ack'
    /// @return a Command object that when run will do what the `commandStr` indicated should be done.
    ///    If the facotry doesn't recognize `commandStr` it return a `Noack` or similar message
    ///    and the returned command will be a noop. This is depends on the provided
    ///    `NetCommandFactory`, `_cmdFactory`.
    std::tuple<std::string, util::Command::Ptr> interpretCommand(std::string const& commandStr) override;

protected:
    /// @see ComControl::create()
    ComControl(IoContextPtr const& ioContext, uint64_t connId, std::shared_ptr<ComServer> const& server,
               control::NetCommandFactory::Ptr const& cmdFactory)
            : ComConnection(ioContext, connId, server), _cmdFactory(cmdFactory) {}

private:
    /// Factory for producing commands to run.
    std::shared_ptr<control::NetCommandFactory> _cmdFactory;
};

}  // namespace system
}  // namespace m2cellcpp
}  // namespace LSST

#endif  // LSST_M2CELLCPP_SYSTEM_COMCONTROL_H
