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

#define CATCH_CONFIG_MAIN

// System headers
#include <exception>
#include <thread>

// 3rd party headers
#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_session.hpp>

// Project headers
#include "control/Context.h"
#include "control/FpgaIo.h"
#include "control/MotionEngine.h"
#include "control/PowerSubsystem.h"
#include "control/PowerSystem.h"
#include "faultmgr/FaultMgr.h"
#include "simulator/SimCore.h"
#include "system/Config.h"
#include "system/Globals.h"
#include "util/Log.h"

using namespace std;
using namespace LSST::m2cellcpp::control;
using namespace LSST::m2cellcpp;

TEST_CASE("Test MotionEngine", "[MotionEngine]") {
    util::Log::getLog().useEnvironmentLogLvl();
    string cfgPath = system::Config::getEnvironmentCfgPath("../configs");
    system::Config::setup(cfgPath + "unitTestCfg.yaml");
    auto& config = LSST::m2cellcpp::system::Config::get();
    system::Globals::setup(config);

    simulator::SimCore::Ptr simCore(new simulator::SimCore());
    faultmgr::FaultMgr::setup();
    control::FpgaIo::setup(simCore);
    control::MotionEngine::setup();
    control::Context::setup();

    LDEBUG("test_SimCore start");
    simCore->start();

    PowerSystem::Ptr powerSys = control::Context::get()->model.getPowerSystem();
    FpgaIo::get().registerPowerSys(powerSys);

    control::Context::Ptr context = control::Context::get();
    context->model.ctrlSetup();

    while (simCore->getIterations() < 1) {
        this_thread::sleep_for(0.1s);
    }

    {
        MotionEngine::Ptr motionEng = MotionEngine::getPtr();
        REQUIRE(motionEng != nullptr);

        // Test the timeout.
        motionEng->engineStart();
        motionEng->waitForEngine();

        // Require the stale data bits are not set.
        auto faultMgr = faultmgr::FaultMgr::getPtr();
        REQUIRE(faultMgr != nullptr);
        faultmgr::FaultStatusBits summary = faultMgr->getSummaryFaults();
        LDEBUG("summary=", summary.getBitmap(), " bit = ", faultmgr::FaultStatusBits::STALE_DATA_WARN);
        faultmgr::FaultStatusBits wMask;
        wMask.setBitAt(faultmgr::FaultStatusBits::STALE_DATA_WARN);
        faultmgr::FaultStatusBits eMask;
        eMask.setBitAt(faultmgr::FaultStatusBits::STALE_DATA_FAULT);
        REQUIRE((summary.getBitmap() & wMask.getBitmap()) == 0);
        REQUIRE((summary.getBitmap() & eMask.getBitmap()) == 0);

        // STALE_DATA_WARN and STALE_DATA_FAULT are not normally enabled
        faultmgr::FaultStatusBits faultBits(eMask.getBitmap() | wMask.getBitmap());
        auto changed = faultMgr->enableFaultsInMask(faultBits);
        LDEBUG("changed=", changed.getAllSetBitEnums());
        LDEBUG("start faultMgr=", faultMgr->dump());

        double tErrSecs = motionEng->comTimeoutError();
        double tWarnSecs = motionEng->comTimeoutError();
        REQUIRE(tErrSecs >= tWarnSecs);
        chrono::duration errDur = chrono::duration<double>(tErrSecs) + 0.5s;
        this_thread::sleep_for(errDur);

        // Require the stale data bits have been set.
        summary = faultMgr->getSummaryFaults();
        LDEBUG("summary=", summary.getAllSetBitEnums(), " bit = ", faultmgr::FaultStatusBits::STALE_DATA_WARN,
               " mask=", wMask.getAllSetBitEnums());
        REQUIRE(summary.getBitmap() & wMask.getBitmap());
        LDEBUG("summary=", summary.getAllSetBitEnums(),
               " bit = ", faultmgr::FaultStatusBits::STALE_DATA_FAULT, " mask=", eMask.getAllSetBitEnums());
        REQUIRE(summary.getBitmap() & eMask.getBitmap());

        LDEBUG("end faultMgr=", faultMgr->dump());

        // Stop and join MotionEngine
        motionEng->engineStop();
        LDEBUG("motionEng Stop");
        motionEng->engineJoin();
        LDEBUG("motionEng Joined");
    }

    LDEBUG("simCore stop");
    simCore->stop();

    LDEBUG("simCore joining");
    simCore->join();
    LINFO("done");
}
