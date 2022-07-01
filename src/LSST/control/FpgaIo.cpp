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
#include "control/FpgaIo.h"

// System headers

// Project headers
#include "system/Config.h"
#include "util/Bug.h"
#include "util/Log.h"

using namespace std;

namespace LSST {
namespace m2cellcpp {
namespace control {

FpgaIo::FpgaIo(bool useMocks) {
    // FUTURE: Once C API is available, create instances capable of communicating
    //         with the FPGA.
    if (!useMocks) {
        throw util::Bug(ERR_LOC, "Only Mock instances available.");
    }

    _ilcMotorCurrent = DaqInMock::create("ILC_Motor_Current", &_mapDaqIn);
    _ilcCommCurrent = DaqInMock::create("ILC_Comm_Current", &_mapDaqIn);
    _ilcMotorVoltage = DaqInMock::create("ILC_Motor_Voltage", &_mapDaqIn);
    _ilcCommVoltage = DaqInMock::create("ILC_Comm_Voltage", &_mapDaqIn);

    _ilcMotorPowerOnOut = DaqBoolOutMock::create("ILC_Motor_Power_On_out", &_mapDaqBoolOut);
    _ilcCommPowerOnOut = DaqBoolOutMock::create("ILC_Comm_Power_On_out", &_mapDaqBoolOut);
    _crioInterlockEnableOut = DaqBoolOutMock::create("cRIO_Interlock_Enable_out", &_mapDaqBoolOut);

    _ilcMotorPowerOnIn = DaqBoolInMock::create("ILC_Motor_Power_On_in", &_mapDaqBoolIn);
    _ilcCommPowerOnIn = DaqBoolInMock::create("ILC_Comm_Power_On_in", &_mapDaqBoolIn);
    _crioInterlockEnableIn = DaqBoolInMock::create("cRIO_Interlock_Enable_in", &_mapDaqBoolIn);

    _ilcs = make_shared<AllIlcs>(useMocks);

    _testIlcMotorCurrent = DaqOutMock::create("ILC_Motor_Current_test", &_mapDaqOut);
    _testIlcCommCurrent = DaqOutMock::create("ILC_Comm_Current_test", &_mapDaqOut);
    _testIlcMotorVoltage = DaqOutMock::create("ILC_Motor_Voltage_test", &_mapDaqOut);
    _testIlcCommVoltage = DaqOutMock::create("ILC_Comm_Voltage_test", &_mapDaqOut);

    for (auto& elem : _mapDaqOut) {
        (elem.second)->finalSetup(_mapDaqIn);
    }

    for (auto& elem : _mapDaqBoolOut) {
        (elem.second)->finalSetup(_mapDaqBoolIn, _mapDaqOut);
    }
}

void FpgaIo::writeAllOutputs() {
    // start with booleans and then DAQ.
    for (auto& elem : _mapDaqBoolOut) {
        (elem.second)->write();
    }

    for (auto& elem : _mapDaqOut) {
        (elem.second)->write();
    }
}

std::string FpgaIo::dump() {
    stringstream os;
    os << "FpgaIo:" << endl;
    for (auto const& elem : _mapDaqOut) {
        elem.second->dump(os);
        os << endl;
    }
    os << endl;
    for (auto const& elem : _mapDaqBoolOut) {
        elem.second->dump(os);
        os << endl;
    }
    os << endl;
    for (auto const& elem : _mapDaqIn) {
        elem.second->dump(os);
        os << endl;
    }
    os << endl;
    for (auto const& elem : _mapDaqBoolIn) {
        elem.second->dump(os);
        os << endl;
    }
    os << endl;

    return os.str();
}

}  // namespace control
}  // namespace m2cellcpp
}  // namespace LSST
