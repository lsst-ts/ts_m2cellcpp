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


#include "control/ControlMain.h"
#include "system/Config.h"
#include "util/Log.h"

using namespace std;
using namespace LSST::m2cellcpp;


int main(int argc, const char* argv[]) {
    // Store log messages and send to cout until the logfile is setup.
    // If environment LOGLVL is undefined, defaults to `trace`.
    LSST::m2cellcpp::util::Log::getLog().useEnvironmentLogLvl();
    util::Log& log = util::Log::getLog();
    log.setOutputDest(util::Log::MIRRORED);

    // Read the configuration
    LINFO("Reading Config");
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
    log.setAlwaysFlush(true);  // spdlog is highly prone to waiting a long time before writing to disk.
    LINFO("Logging ready");

    /// Start the main thread
    control::ControlMain::setup();
    control::ControlMain::Ptr ctMain = control::ControlMain::getPtr();
    ctMain->run(argc, argv);
    LINFO("ctrlMain started");

    ctMain->join();
    LINFO("ctrlMain joined");

    return 0;
}
