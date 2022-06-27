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

    _IlcMotorCurrent = std::make_shared<DaqIn>("ILC_Motor_Current");
    _IlcCommCurrent = std::make_shared<DaqIn>("ILC_Comm_Current");
    _IlcMotorVoltage = std::make_shared<DaqIn>("ILC_Motor_Voltage");
    _IlcCommVoltage = std::make_shared<DaqIn>("ILC_Comm_Voltage");

    _IlcMotorPowerOnOut = std::make_shared<DaqBoolOut>("ILC_Motor_Power_On_out");
    _IlcCommPowerOnOut = std::make_shared<DaqBoolOut>("ILC_Comm_Power_On_out");
    _CrioInterlockEnableOut = std::make_shared<DaqBoolOut>("cRIO_Interlock_Enable_out");

    _IlcMotorPowerOnIn = std::make_shared<DaqBoolIn>("ILC_Motor_Power_On_in");
    _IlcCommPowerOnIn = std::make_shared<DaqBoolIn>("ILC_Comm_Power_On_in");
    _CrioInterlockEnableIn = std::make_shared<DaqBoolIn>("cRIO_Interlock_Enable_in");

    _ilcs = std::make_shared<AllIlcs>(useMocks);

    _testIlcMotorCurrent = std::make_shared<DaqOut>("ILC_Motor_Current_test");
    _testIlcCommCurrent = std::make_shared<DaqOut>("ILC_Comm_Current_test");
    _testIlcMotorVoltage = std::make_shared<DaqOut>("ILC_Motor_Voltage_test");
    _testIlcCommVoltage = std::make_shared<DaqOut>("ILC_Comm_Voltage_test");
}

AllIlcs::AllIlcs(bool useMocks) {
    // FUTURE: Once C API is available, create instances capable of communicating
    //         with the FPGA.
    if (!useMocks) {
        throw util::Bug(ERR_LOC, "Only Mock instances available.");
    }

    // The first 6 ILC's are Tangential
    // 7-78 are Axial.
    for (int j = 1; j <= 78; ++j) {
        string name;
        if (j <= 6) {
            name = "Tangent_";
        } else {
            name = "Axial_";
        }
        name += to_string(j);
        _ilcs.emplace_back(make_shared<Ilc>(name, j));
    }
}

Ilc::Ilc(std::string const& name, int idNum) : _name(name), _idNum(idNum) {}

Ilc::Ptr& AllIlcs::_getIlcPtr(unsigned int idNum) {
    if (idNum < 1 || idNum > _ilcs.size()) {
        LERROR("AllIlcs::_getIlcPtr ", idNum, " throwing out of range");
        throw std::out_of_range("_getIlcPtr invalid val " + std::to_string(idNum));
    }
    return _ilcs[idNum - 1];
}

bool Ilc::getBit(int bit, uint8_t byt) {
    uint8_t j = 1;
    j = j << bit;
    return (j & byt) != 0;
}

bool Ilc::getFault() { return getBit(0, _rawStatus); }

bool Ilc::getCWLimit() { return getBit(2, _rawStatus); }

bool Ilc::getCCWLimit() { return getBit(3, _rawStatus); }

void Ilc::setStatus(uint8_t val) { _rawStatus = val; }

uint16_t Ilc::getBroadcastCommCount() {
    /// bit4..7: Broadcast communication counter (0..15)
    uint8_t mask = 0xf0;
    uint8_t out = (mask & _rawStatus) >> 4;
    return out;
}

DaqIn::DaqIn(string const& name) : _name(name) {
    // FUTURE: Probably set _scale from config file.
}

void DaqIn::setRaw(double val) {
    lock_guard<mutex> lg(_mtx);
    _data.raw = val;
    _data.lastRead = FpgaClock::now();
    _data.upToDate = false;
}

void DaqIn::adjust() {
    lock_guard<mutex> lg(_mtx);
    // FUTURE: This is probably seriously over simplified.
    _data.adjusted = _data.raw * _scale;
    _data.upToDate = true;
}

DaqIn::Data DaqIn::getData() {
    lock_guard<mutex> lg(_mtx);
    return _data;
}

DaqOut::DaqOut(string const& name) : _name(name) {
    // FUTURE: Probably set _scale from config file.
}

void DaqOut::setSource(double val) {
    lock_guard<mutex> lg(_mtx);
    _data.source = val;
    _data.upToDate = false;
}

void DaqOut::adjust() {
    lock_guard<mutex> lg(_mtx);
    _data.outVal = _data.source / _scale;
    _data.upToDate = true;
}

DaqOut::Data DaqOut::getData() {
    lock_guard<mutex> lg(_mtx);
    return _data;
}

void DaqOut::written() {
    /// FUTURE: possibly change name and actuall write the data to the FPGA.
    lock_guard<mutex> lg(_mtx);
    _data.lastWrite = FpgaClock::now();
}

}  // namespace control
}  // namespace m2cellcpp
}  // namespace LSST
