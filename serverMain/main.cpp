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

#include "control/Context.h"
#include "system/ComControl.h"
#include "system/ComControlServer.h"
#include "system/Config.h"
#include "util/Log.h"

using namespace std;
using namespace LSST::m2cellcpp;

int main(int argc, char* argv[]) {
    // Store log messages and send to cout until the logfile is setup.
    // If environment LOGLVL is undefined, defaults to `trace`.
    LSST::m2cellcpp::util::Log::getLog().useEnvironmentLogLvl();
    util::Log& lg = util::Log::getLog();
    lg.setOutputDest(util::Log::MIRRORED);
    LINFO("starting main");

    // Read the configuration
    string cfgPath = system::Config::getEnvironmentCfgPath("./configs");
    system::Config::setup(cfgPath + "m2cellCfg.yaml");
    system::Config& sysCfg = system::Config::get();

    // Setup logging
    string logFileName = sysCfg.getLogFileName();
    int logFileSizeMB = sysCfg.getLogFileSizeMB();
    int logMaxFiles = sysCfg.getLogMaxFiles();
    LINFO("Starting logger name=", logFileName, " sizeMB=", logFileSizeMB, " maxFiles=", logMaxFiles);
    if (!lg.setupFileRotation(logFileName, 1024 * 1024 * logFileSizeMB, logMaxFiles)) {
        LCRITICAL("FAILED to setup logging name=", logFileName, " sizeMB=", logFileSizeMB,
                  " maxFiles=", logMaxFiles);
        exit(-1);
    }
    lg.setOutputDest(util::Log::SPEEDLOG);
    lg.flush();

    // Start the control system
    control::Context::setup();

    // Start the telemetry server

    // Start a ComControlServer
    LDEBUG("ComControlServer starting...");
    lg.flush();
    system::IoContextPtr ioContext = make_shared<boost::asio::io_context>();
    int port = system::Config::get().getControlServerPort();
    auto cmdFactory = control::NetCommandFactory::create();
    system::ComControl::setupNormalFactory(cmdFactory);
    auto serv = system::ComControlServer::create(ioContext, port, cmdFactory);

    atomic<bool> comServerDone{false};
    LDEBUG("ComControlServer started");

    thread comControlServThrd([&serv, &comServerDone]() {
        LINFO("server run ", serv->prettyState(serv->getState()));
        serv->run();
        LINFO("server finish");
        comServerDone = true;
    });

    // Wait as long as the server is running. At some point,
    // something should call comServ->shutdown().
    while (serv->getState() != system::ComServer::STOPPED) {
        sleep(5);
    }

    // This will terminate all TCP/IP communication.
    ioContext->stop();
    for (int j = 0; !comServerDone && j < 10; ++j) {
        sleep(1);
        bool d = comServerDone;
        LINFO("server wait ", d);
    }
    LINFO("server stopped");
    comControlServThrd.join();
    return 0;
}
