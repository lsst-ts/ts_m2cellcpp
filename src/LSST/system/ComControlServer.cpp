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

// Class header
#include "system/ComControlServer.h"

// System headers
#include <functional>
#include <thread>

// Project headers
#include "system/Config.h"
#include "util/Log.h"

// LSST headers

using namespace std;
using namespace std::placeholders;

namespace LSST {
namespace m2cellcpp {
namespace system {

std::weak_ptr<ComControlServer> ComControlServer::_globalComControlServer;

ComControlServer::Ptr ComControlServer::create(IoContextPtr const& ioContext, int port,
                                               control::NetCommandFactory::Ptr const& cmdFactory,
                                               bool makeGlobal) {
    Ptr comControlServ = Ptr(new ComControlServer(ioContext, port, cmdFactory));
    if (makeGlobal) {
        if (_globalComControlServer.use_count() > 0) {
            LWARN("Reseting _globalComControlServer while existing is still in use.");
        }
        _globalComControlServer = comControlServ;
    }
    return comControlServ;
}

ComConnection::Ptr ComControlServer::newComConnection(IoContextPtr const& ioContext, uint64_t connId,
                                                      std::shared_ptr<ComServer> const& server) {
    ComConnection::Ptr ptr = ComControl::create(ioContext, connId, server, _cmdFactory);
    ptr->setDoSendWelcomeMsg(getDoSendWelcomeMsgServ());
    return ptr;
}

}  // namespace system
}  // namespace m2cellcpp
}  // namespace LSST
