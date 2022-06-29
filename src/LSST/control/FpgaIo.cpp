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

    _ilcMotorCurrent = DaqIn::create("ILC_Motor_Current", &_mapDaqIn);
    _ilcCommCurrent = DaqIn::create("ILC_Comm_Current", &_mapDaqIn);
    _ilcMotorVoltage = DaqIn::create("ILC_Motor_Voltage", &_mapDaqIn);
    _ilcCommVoltage = DaqIn::create("ILC_Comm_Voltage", &_mapDaqIn);

    _ilcMotorPowerOnOut = DaqBoolOut::create("ILC_Motor_Power_On_out", &_mapDaqBoolOut);
    _ilcCommPowerOnOut = DaqBoolOut::create("ILC_Comm_Power_On_out", &_mapDaqBoolOut);
    _crioInterlockEnableOut = DaqBoolOut::create("cRIO_Interlock_Enable_out", &_mapDaqBoolOut);

    _ilcMotorPowerOnIn = DaqBoolIn::create("ILC_Motor_Power_On_in", &_mapDaqBoolIn);
    _ilcCommPowerOnIn = DaqBoolIn::create("ILC_Comm_Power_On_in", &_mapDaqBoolIn);
    _crioInterlockEnableIn = DaqBoolIn::create("cRIO_Interlock_Enable_in", &_mapDaqBoolIn);

    _ilcs = make_shared<AllIlcs>(useMocks);

    _testIlcMotorCurrent = DaqOut::create("ILC_Motor_Current_test", &_mapDaqOut);
    _testIlcCommCurrent = DaqOut::create("ILC_Comm_Current_test", &_mapDaqOut);
    _testIlcMotorVoltage = DaqOut::create("ILC_Motor_Voltage_test", &_mapDaqOut);
    _testIlcCommVoltage = DaqOut::create("ILC_Comm_Voltage_test", &_mapDaqOut);

    for (auto& elem : _mapDaqOut) {
        (elem.second)->finalSetup(_mapDaqIn);
    }

    for (auto& elem : _mapDaqBoolOut) {
        (elem.second)->finalSetup(_mapDaqBoolIn, _mapDaqOut);
    }
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

Ilc::Ilc(string const& name, int idNum) : _name(name), _idNum(idNum) {}

Ilc::Ptr& AllIlcs::_getIlcPtr(unsigned int idNum) {
    if (idNum < 1 || idNum > _ilcs.size()) {
        LERROR("AllIlcs::_getIlcPtr ", idNum, " throwing out of range");
        throw std::out_of_range("_getIlcPtr invalid val " + to_string(idNum));
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

DaqIn::Ptr DaqIn::create(string const& name, map<string, DaqIn::Ptr>* mapDaqIn) {
    Ptr ptr = shared_ptr<DaqIn>(new DaqIn(name));

    if (mapDaqIn != nullptr) {
        auto ret = mapDaqIn->insert(make_pair(name, ptr));
        if (ret.second == false) {
            string errMsg(name + " already in mapDaqIn");
            LERROR(errMsg);
            throw util::Bug(ERR_LOC, errMsg);
        }
    }
    return ptr;
}

DaqIn::DaqIn(string const& name) : _name(name) {
    // Look for "scale" in Config
    try {
        _scale = system::Config::get().getSectionKeyAsDouble(_name, "scale");
    } catch (system::ConfigException const& ex) {
        LWARN("No scale entry found for ", _name);
    }
    LINFO("DaqIn config ", _name, " scale=", _scale);
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

DaqOut::Ptr DaqOut::create(string const& name, map<string, DaqOut::Ptr>* mapDaqOut) {
    Ptr ptr = shared_ptr<DaqOut>(new DaqOut(name));

    if (mapDaqOut != nullptr) {
        auto ret = mapDaqOut->insert(make_pair(name, ptr));
        if (ret.second == false) {
            string errMsg(name + " already in mapDaqOut");
            LERROR(errMsg);
            throw util::Bug(ERR_LOC, errMsg);
        }
    }
    return ptr;
}

DaqOut::DaqOut(string const& name) : _name(name) {
    // Look for "scale" in Config
    try {
        _scale = system::Config::get().getSectionKeyAsDouble(_name, "scale");
    } catch (system::ConfigException const& ex) {
        LWARN("No scale entry found for ", _name);
    }
    LINFO("DaqOut config ", _name, " scale=", _scale);

    try {
        _linkStr = system::Config::get().getSectionKeyAsString(_name, "link");
    } catch (system::ConfigException const& ex) {
        LWARN("No link entry found for ", _name);
    }
    LINFO("DaqOut config ", _name, " scale=", _scale);
}

void DaqOut::finalSetup(map<string, DaqIn::Ptr>& mapDaqIn) {
    if (!_linkStr.empty()) {
        auto iter = mapDaqIn.find(_linkStr);
        if (iter != mapDaqIn.end()) {
            _link = iter->second;
            LINFO("Setting DaqOut ", _name, " link to ", _linkStr, " ", _link->getName());
        } else {
            string errMsg = string("DaqOut::finalSetup ") + _name + " couldn't find DaqIn " + _linkStr;
            LERROR(errMsg);
            throw util::Bug(ERR_LOC, errMsg);
        }
    }
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

void DaqOut::write() {
    /// FUTURE: possibly change name and actuall write the data to the FPGA.
    double outVal;
    {
        lock_guard<mutex> lg(_mtx);
        _data.lastWrite = FpgaClock::now();
        outVal = _data.outVal;
    }
    if (_link != nullptr) {
        _link->setRaw(outVal);
    }
}

DaqBoolIn::Ptr DaqBoolIn::create(string const& name, map<string, DaqBoolIn::Ptr>* mapDaqBoolIn) {
    Ptr ptr = shared_ptr<DaqBoolIn>(new DaqBoolIn(name));

    if (mapDaqBoolIn != nullptr) {
        auto ret = mapDaqBoolIn->insert(make_pair(name, ptr));
        if (ret.second == false) {
            string errMsg(name + " already in mapDaqBoolIn");
            LERROR(errMsg);
            throw util::Bug(ERR_LOC, errMsg);
        }
    }
    return ptr;
}

DaqBoolIn::DaqBoolIn(string const& name) : _name(name) {}

DaqBoolOut::Ptr DaqBoolOut::create(string const& name, map<string, DaqBoolOut::Ptr>* mapDaqBoolOut) {
    Ptr ptr = shared_ptr<DaqBoolOut>(new DaqBoolOut(name));

    if (mapDaqBoolOut != nullptr) {
        auto ret = mapDaqBoolOut->insert(make_pair(name, ptr));
        if (ret.second == false) {
            string errMsg(name + " already in mapDaqBoolOut");
            LERROR(errMsg);
            throw util::Bug(ERR_LOC, errMsg);
        }
    }
    return ptr;
}

DaqBoolOut::DaqBoolOut(string const& name) : _name(name) {
    try {
        _linkBoolInStr = system::Config::get().getSectionKeyAsString(_name, "linkBoolOut");
    } catch (system::ConfigException const& ex) {
        LWARN("No link entry found for ", _name);
    }
    LINFO("DaqBoolOut config ", _name, " linkBoolIn=", _linkBoolInStr);

    try {
        _linkCurrentOutStr = system::Config::get().getSectionKeyAsString(_name, "linkCurrentOut");
        try {
            _linkCurrentOutVal = system::Config::get().getSectionKeyAsDouble(_name, "linkCurrentOutVal");
        } catch (system::ConfigException const& ex) {
            throw util::Bug(ERR_LOC, string("If linkCurrentOut is defined for ") + _name +
                                             " linkCurrentOutVal must be defined");
        }
    } catch (system::ConfigException const& ex) {
        LWARN("No linkCurrent entry found for ", _name);
    }
    LINFO("DaqBoolOut config ", _name, " linkCurrentOut=", _linkCurrentOutStr, " val=", _linkCurrentOutVal);

    try {
        _linkVoltageOutStr = system::Config::get().getSectionKeyAsString(_name, "linkVoltageOut");
        try {
            _linkVoltageOutVal = system::Config::get().getSectionKeyAsDouble(_name, "linkVoltageOutVal");
        } catch (system::ConfigException const& ex) {
            throw util::Bug(ERR_LOC, string("If linkVoltageOut is defined for ") + _name +
                                             " linkVoltageOutVal must be defined");
        }
    } catch (system::ConfigException const& ex) {
        LWARN("No linkVoltage entry found for ", _name);
    }
    LINFO("DaqBoolOut config ", _name, " linkVoltageOut=", _linkVoltageOutStr, " val=", _linkVoltageOutVal);
}

void DaqBoolOut::finalSetup(map<string, DaqBoolIn::Ptr>& mapDaqBoolIn, map<string, DaqOut::Ptr>& mapDaqOut) {
    // Setup link to DaqBoolIn, if defined
    if (!_linkBoolInStr.empty()) {
        auto iter = mapDaqBoolIn.find(_linkBoolInStr);
        if (iter != mapDaqBoolIn.end()) {
            _linkBoolIn = iter->second;
            LINFO("Setting DaqBoolOut ", _name, " link to ", _linkBoolInStr, " ", _linkBoolIn->getName());
        } else {
            string errMsg =
                    string("DaqBoolOut::finalSetup ") + _name + " couldn't find DaqBoolIn " + _linkBoolInStr;
            LERROR(errMsg);
            throw util::Bug(ERR_LOC, errMsg);
        }
    }

    // Setup link to DaqOut for current
    if (!_linkCurrentOutStr.empty()) {
        auto iter = mapDaqOut.find(_linkCurrentOutStr);
        if (iter != mapDaqOut.end()) {
            _linkCurrentOut = iter->second;
            LINFO("Setting DaqBoolOut ", _name, " link to ", _linkCurrentOutStr, " ",
                  _linkCurrentOut->getName());
        } else {
            string errMsg =
                    string("DaqOut::finalSetup ") + _name + " couldn't find DaqOut " + _linkCurrentOutStr;
            LERROR(errMsg);
            throw util::Bug(ERR_LOC, errMsg);
        }
    }

    // Setup link to DaqOut for Voltage
    if (!_linkVoltageOutStr.empty()) {
        auto iter = mapDaqOut.find(_linkVoltageOutStr);
        if (iter != mapDaqOut.end()) {
            _linkVoltageOut = iter->second;
            LINFO("Setting DaqBoolOut ", _name, " link to ", _linkVoltageOutStr, " ",
                  _linkVoltageOut->getName());
        } else {
            string errMsg =
                    string("DaqOut::finalSetup ") + _name + " couldn't find DaqOut " + _linkVoltageOutStr;
            LERROR(errMsg);
            throw util::Bug(ERR_LOC, errMsg);
        }
    }
}

}  // namespace control
}  // namespace m2cellcpp
}  // namespace LSST
