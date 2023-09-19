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
#include "control/PowerSystem.h"

// system headers

// project headers
#include "util/Bug.h"
#include "util/Log.h"


using namespace std;

namespace LSST {
namespace m2cellcpp {
namespace control {

PowerSystem::PowerSystem(FpgaIo::Ptr const& fpgaIo) : _fpgaIo(fpgaIo), _motor(MOTOR), _comm(COMM) {
}

void PowerSystem::processDAQ(SysInfo info) {
    SysStatus motorStat = _motor.processDAQ(info);
    SysStatus commStat = _comm.processDAQ(info);

    if (_motorStatusPrev != motorStat || _commStatusPrev != commStat) {
        LINFO("Power status change motor:now=", motorStat, " prev=", _motorStatusPrev,
                " comm=", commStat, " prev=", _commStatusPrev);
        _motorStatusPrev = motorStat;
        _commStatusPrev = commStat;
    }
}



}  // namespace control
}  // namespace m2cellcpp
}  // namespace LSST

