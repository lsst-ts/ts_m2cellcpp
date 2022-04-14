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
#include "system/ComControl.h"

// System headers
#include <cerrno>
#include <cstring>
#include <ctime>
#include <functional>
#include <iostream>
#include <stdexcept>

// Third party headers

// Project headers
#include "util/Log.h"

using namespace std;

namespace LSST {
namespace m2cellcpp {
namespace system {

void ComControl::setupNormalFactory(control::NetCommandFactory::Ptr const& cmdFactory) {
    cmdFactory->addNetCommand(control::NCmdAck::createFactoryVersion());
    cmdFactory->addNetCommand(control::NCmdNoAck::createFactoryVersion());
    cmdFactory->addNetCommand(control::NCmdEcho::createFactoryVersion());
}

std::tuple<std::string, util::Command::Ptr> ComControl::interpretCommand(std::string const& commandStr) {
    control::NetCommand::Ptr netCmd;
    try {
        netCmd = _cmdFactory->getCommandFor(commandStr);
    } catch (control::NetCommandException const& nex) {
        LWARN("ComControl::interpretCommand(", commandStr, " excecption thrown ", nex.what());
        netCmd = _cmdFactory->getNoAck();
    }
    auto ackMsg = netCmd->getAckJsonStr();
    // This lambda function will be run when `cmd->runAction()` is called.
    // It needs a shared_ptr to this to prevent segfaults if ComConnection was closed.
    auto thisPtr = shared_from_this();
    auto func = [thisPtr, netCmd](util::CmdData*) {
        LDEBUG("Running func netCmd ", netCmd->getName(), " seqId=", netCmd->getSeqId());
        netCmd->run();
        string finalMsg = netCmd->getRespJsonStr();
        thisPtr->asyncWrite(finalMsg);
    };
    auto cmd = make_shared<util::Command>(func);
    return {ackMsg, cmd};
}

}  // namespace system
}  // namespace m2cellcpp
}  // namespace LSST
