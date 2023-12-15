/*
 * This file is part of LSST ts_m2cellcpp test suite.
 *
 * Developed for the Vera C. Rubin Observatory Telescope & Site Software Systems.
 * This product includes software developed by the Vera C.Rubin Observatory Project
 * (https://www.lsst.org). See the COPYRIGHT file at the top-level directory of
 * this distribution for details of code ownership.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation, either version 3 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program. If not, see <https://www.gnu.org/licenses/>.
 */

// Class header
#include "control/ControlMain.h"

// system headers
#include <ctime>

// project headers
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
#include "util/Bug.h"
#include "util/Log.h"

using namespace std;

namespace LSST {
namespace m2cellcpp {
namespace control {

ControlMain::Ptr ControlMain::_thisPtr;
std::mutex ControlMain::_thisPtrMtx;

void ControlMain::setup() {
    lock_guard<mutex> lock(_thisPtrMtx);
    if (_thisPtr) {
        LERROR("ControlMain already setup");
        return;
    }
    _thisPtr = Ptr(new ControlMain());
}

ControlMain::Ptr ControlMain::getPtr() {
    if (_thisPtr == nullptr) {
        throw system::ConfigException(ERR_LOC, "ControlMain has not been setup.");
    }
    return _thisPtr;
}

ControlMain::ControlMain() {}

ControlMain::~ControlMain() {}

void ControlMain::run(int argc, const char* argv[]) {
    thread t(&ControlMain::_cMain, this, argc, argv);
    _mainThrd = move(t);
}

void signalHandler(int sig) {
    if (sig == SIGPIPE) {
        LWARN("Ignoring SIGPIPE sig=", sig);
        return;
    }
    LCRITICAL("Unhandled signal caught sig=", sig);
    exit(sig);
}

void ControlMain::_cMain(int argc, const char* argv[]) {
    LINFO("starting main");
    util::Log& log = util::Log::getLog();

    // Get the configuration
    system::Config& sysCfg = system::Config::get();

    // Setup global items.
    system::Globals::setup(sysCfg);

    // Setup a simple signal handler to handle when clients closing connection results in SIGPIPE.
    signal(SIGPIPE, signalHandler);

    // Create the control system
    _simCore.reset(new LSST::m2cellcpp::simulator::SimCore());
    _simCore->start();
    faultmgr::FaultMgr::setup();
    control::FpgaIo::setup(_simCore);
    control::MotionEngine::setup();
    control::Context::setup();

    // Register the PowerSystem with FpgaIo so that it gets updates.
    auto powerSys = control::Context::get()->model.getPowerSystem();
    control::FpgaIo::get().registerPowerSys(powerSys);

    // Setup the control system
    auto context = control::Context::get();
    context->model.ctrlSetup();

    // At this point, MotionCtrl and FpgaCtrl should be configured. Start the control loops
    context->model.ctrlStart();

    // At this point, the LabView code seems to want to put the system into
    // ReadyIdle. We're NOT doing that. The system is going into StandbyState
    // until it gets an explicit command to do something else.

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
        exit(-1);  // change to powerdown and exit
    }

    // Wait for MotionCtrl to be ready
    context->model.waitForCtrlReady();

    // Start a ComControlServer
    LDEBUG("ComControlServer starting...");
    system::IoContextPtr ioContext = make_shared<boost::asio::io_context>();
    int port = system::Config::get().getControlServerPort();
    auto cmdFactory = control::NetCommandFactory::create();
    system::ComControl::setupNormalFactory(cmdFactory);
    _comServer = system::ComControlServer::create(ioContext, port, cmdFactory, true);
    LINFO("ComControlServer created port=", port);

    atomic<bool> comServerDone{false};
    auto comServState = _comServer->getState();
    LDEBUG("ComControlServer comServState=", _comServer->prettyState(comServState));

    auto cServ = _comServer;
    thread comControlServThrd([&cServ, &comServerDone, &log]() {
        LINFO("server run ", cServ->prettyState(cServ->getState()));
        log.flush();
        cServ->run();
        LINFO("server finish");
        comServerDone = true;
    });

    // wait for the server to be running
    {
        int j = 0;
        int const maxSeconds = 30;
        for (; (cServ->getState() != LSST::m2cellcpp::system::ComServer::RUNNING) && j < maxSeconds; ++j) {
            sleep(1);
        }
        if (j >= maxSeconds) {
            throw util::Bug(ERR_LOC,
                            "ControlMain server did not start within " + to_string(maxSeconds) + " seconds");
        }
        _running = true;
    }

    // Wait as long as the server is running. At some point,
    // something should call comServ->shutdown(). When the
    // server is shutdown, the rest of the program will try
    // to shutdown in an orderly manner.
    LINFO("ComControlServer is running, waiting for server shutdown");
    while (_comServer->getState() != system::ComServer::STOPPED) {
        sleep(1);
    }
    LINFO("ComControlServer has been shutdown");

    // This will terminate all TCP/IP communication.
    LINFO("ioContext->stop();");
    ioContext->stop();
    for (int j = 0; !comServerDone && j < 10; ++j) {
        sleep(1);
        bool d = comServerDone;
        LINFO("server wait ", d);
    }
    LINFO("server stopped");
    context->model.ctrlStop();

    LINFO("stopping model");
    context->model.ctrlJoin();

    LINFO("joining server");
    comControlServThrd.join();
    LINFO("server joined");
}

void ControlMain::stop() {
    // Stop the ComServer, which should cause `run(..)` to end.
    LINFO("ControlMain::stop() shutting down ComControlServer.");
    _comServer->shutdown();
}

void ControlMain::join() {
    LINFO("ControlMain joining the main thread.");
    if (_mainThrd.joinable()) {
        _mainThrd.join();
        LINFO("ControlMain main thread joined.");
    } else {
        LINFO("ControlMain main thread not joinable.");
    }
}

}  // namespace control
}  // namespace m2cellcpp
}  // namespace LSST
