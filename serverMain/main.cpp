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
#include "system/Globals.h"
#include "system/TelemetryCom.h"
#include "system/TelemetryMap.h"
#include "util/Log.h"

using namespace std;
using namespace LSST::m2cellcpp;

void signalHandler(int sig) {
    if (sig == SIGPIPE) {
        LWARN("Ignoring SIGPIPE sig=", sig);
        return;
    }
    LCRITICAL("Unhandled signal caught sig=", sig);
    exit(sig);
}

int main(int argc, char* argv[]) {
    // Store log messages and send to cout until the logfile is setup.
    // If environment LOGLVL is undefined, defaults to `trace`.
    LSST::m2cellcpp::util::Log::getLog().useEnvironmentLogLvl();
    util::Log& log = util::Log::getLog();
    log.setOutputDest(util::Log::MIRRORED);
    LINFO("starting main");

    // Read the configuration
    string cfgPath = system::Config::getEnvironmentCfgPath("./configs");
    system::Config::setup(cfgPath + "m2cellCfg.yaml");
    system::Config& sysCfg = system::Config::get();

    // Setup global items.
    system::Globals::setup();

    // Setup logging
    string logFileName = sysCfg.getLogFileName();
    int logFileSizeMB = sysCfg.getLogFileSizeMB();
    int logMaxFiles = sysCfg.getLogMaxFiles();
    LINFO("Starting logger name=", logFileName, " sizeMB=", logFileSizeMB, " maxFiles=", logMaxFiles);
    if (!log.setupFileRotation(logFileName, 1024 * 1024 * logFileSizeMB, logMaxFiles)) {
        LCRITICAL("FAILED to setup logging name=", logFileName, " sizeMB=", logFileSizeMB,
                  " maxFiles=", logMaxFiles);
        exit(-1);
    }
    log.setOutputDest(util::Log::SPEEDLOG);
    // FUTURE: DM-39974 add command line argument to turn `Log::_alwaysFlush` off.
    log.setAlwaysFlush(true); // spdlog is highly prone to waiting a long time before writing to disk.

    // Setup a simple signal handler to handle when clients closing connection results in SIGPIPE.
    signal(SIGPIPE, signalHandler);

    // Start the telemetry server
    LINFO("Starting Telemetry Server");
    // FUTURE: get the correct entries into `system::Config`.
    int const telemPort = 50001;
    auto servTelemetryMap = system::TelemetryMap::Ptr(new system::TelemetryMap());
    auto telemetryServ = system::TelemetryCom::create(servTelemetryMap, telemPort);

    telemetryServ->startServer();
    if (!telemetryServ->waitForServerRunning(5)) {
        LCRITICAL("Telemetry server failed to start.");
        exit(-1);
    }

    // Start the control system
    control::Context::setup();

    // Start a ComControlServer
    LDEBUG("ComControlServer starting...");
    system::IoContextPtr ioContext = make_shared<boost::asio::io_context>();
    int port = system::Config::get().getControlServerPort();
    auto cmdFactory = control::NetCommandFactory::create();
    system::ComControl::setupNormalFactory(cmdFactory);
    auto serv = system::ComControlServer::create(ioContext, port, cmdFactory);
    LINFO("ComControlServer created port=", port);

    atomic<bool> comServerDone{false};
    auto comServState = serv->getState();
    LDEBUG("ComControlServer comServState=", serv->prettyState(comServState));

    thread comControlServThrd([&serv, &comServerDone, &log]() {
        LINFO("server run ", serv->prettyState(serv->getState()));
        log.flush();
        serv->run();
        LINFO("server finish");
        comServerDone = true;
    });

    // Wait as long as the server is running. At some point,
    // something should call comServ->shutdown().
    LDEBUG("ComControlServer waiting for server shutdown");
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
