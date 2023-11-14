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
#include "control/FpgaIo.h"
#include "control/MotionEngine.h"
#include "faultmgr/FaultMgr.h"
#include "simulator/SimCore.h"
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
    LINFO("Main - logging ready");

    // Setup global items.
    system::Globals::setup(sysCfg);

    // Setup a simple signal handler to handle when clients closing connection results in SIGPIPE.
    signal(SIGPIPE, signalHandler);


    // Create the control system
    LSST::m2cellcpp::simulator::SimCore::Ptr simCore(new LSST::m2cellcpp::simulator::SimCore());
    simCore->start();
    faultmgr::FaultMgr::setup();
    control::FpgaIo::setup(simCore);
    control::MotionEngine::setup();
    control::Context::setup();

    // Register the PowerSystem with FpgaIo so that it gets updates.
    auto powerSys = control::Context::get()->model.getPowerSystem();
    control::FpgaIo::get().registerPowerSys(powerSys);

    // Setup the control system
    auto context = control::Context::get();
    context->model.ctrlSetup();


    // &&& At this point, MotionCtrl and FpgaCtrl should be configured. Start the control loops
    context->model.ctrlStart();

    // &&& At this point, the LabView code seems to want to put the system into
    // &&& ReadyIdle. We're NOT doing that. The system is going into StandbyState
    // &&& until it gets an explicit command to do something else.



    // Start the telemetry server
    // FUTURE: Telemetry for the GUI requires the ComControlServer->ComConnection::welcomeMsg to
    //         work properly. Those elements should be in the telemetry so there's no need for a
    //         ComControlServer connection for Telemetry. This has to wait until the existing
    //         controller is no longer used.
    LINFO("Starting Telemetry Server");
    // FUTURE: get the correct entries into `system::Config`.
    int const telemPort = 50001;
    auto servTelemetryMap = system::TelemetryMap::Ptr(new system::TelemetryMap());
    auto telemetryServ = system::TelemetryCom::create(servTelemetryMap, telemPort);

    telemetryServ->startServer();
    if (!telemetryServ->waitForServerRunning(5)) {
        LCRITICAL("Telemetry server failed to start.");
        exit(-1); // &&& change to powerdown and exit
    }

    // &&& wait for MotionCtrl to be ready
    context->model.waitForCtrlReady();

    // Start a ComControlServer
    LDEBUG("ComControlServer starting...");
    system::IoContextPtr ioContext = make_shared<boost::asio::io_context>();
    int port = system::Config::get().getControlServerPort();
    auto cmdFactory = control::NetCommandFactory::create();
    system::ComControl::setupNormalFactory(cmdFactory);
    auto serv = system::ComControlServer::create(ioContext, port, cmdFactory, true);
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

    // &&& send shutdown  to ComServer.
    // &&& send shutdown  to TelemetryServer

    // Wait as long as the server is running. At some point,
    // something should call comServ->shutdown().
    LDEBUG("ComControlServer waiting for server shutdown");
    while (serv->getState() != system::ComServer::STOPPED) {
        sleep(1);
    }

    // This will terminate all TCP/IP communication.
    ioContext->stop();
    for (int j = 0; !comServerDone && j < 10; ++j) {
        sleep(1);
        bool d = comServerDone;
        LINFO("server wait ", d);
    }
    LINFO("server stopped");

    context->model.ctrlStop();
    LINFO("stopping model");

    comControlServThrd.join();
    LINFO("server joined");

    context->model.ctrlJoin();
    LINFO("Model threads joined");

    return 0;
}
