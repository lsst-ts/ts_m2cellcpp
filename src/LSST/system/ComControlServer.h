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
#ifndef LSST_M2CELLCPP_SYSTEM_COMCONTROLSERVER_H
#define LSST_M2CELLCPP_SYSTEM_COMCONTROLSERVER_H

// System headers

// Third party headers
#include <boost/asio.hpp>

// project headers
#include "control/NetCommandFactory.h"
#include "system/ComConnection.h"
#include "system/ComControl.h"
#include "system/ComServer.h"

namespace LSST {
namespace m2cellcpp {
namespace system {

class ComControlServer : public ComServer {
public:
    using Ptr = std::shared_ptr<ComControlServer>;

    // parent class non-copyable, non-movable.
    virtual ~ComControlServer() = default;

    /// A factory method to prevent issues with enable_shared_from_this.
    /// @param ioContext - asio io context.
    /// @param port - port number.
    /// @param cmdFactory - pointer to the NetCommandFactory that can handle
    ///             requests expected of this server
    /// @param makeGlobal - set to true if the created ComControlServer should
    ///             be the global instance (default is false).
    /// @return A pointer to the created ComControlServer object.
    static Ptr create(IoContextPtr const& ioContext, int port,
                      control::NetCommandFactory::Ptr const& cmdFactory,
                      bool makeGlobal = false);

    /// Return a weak pointer to `_globalComControlServer`.
    static std::weak_ptr<ComControlServer> get() { return _globalComControlServer; }

    /// @return a new ComControl object.
    ComConnection::Ptr newComConnection(IoContextPtr const& ioContext, uint64_t connId,
                                        std::shared_ptr<ComServer> const& server) override;

protected:
    /// Protected constructor to force use of create().
    ComControlServer(IoContextPtr const& ioContext, int port,
                     control::NetCommandFactory::Ptr const& cmdFactory)
            : ComServer(ioContext, port), _cmdFactory(cmdFactory) {}

private:
    /// NetCommandFactory to decipher messages and provide NetCommands.
    control::NetCommandFactory::Ptr _cmdFactory;

    /// Pointer to the global ComControlServer instance, if there is one.
    static std::weak_ptr<ComControlServer> _globalComControlServer;
};

}  // namespace system
}  // namespace m2cellcpp
}  // namespace LSST

#endif  // LSST_M2CELLCPP_SYSTEM_COMCONTROLSERVER_H
