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
#include "FpgaIoOld.h"
#include "system/Config.h"
#include "util/Bug.h"
#include "util/Log.h"

using namespace std;

namespace LSST {
namespace m2cellcpp {
namespace control {

DaqInMock::Ptr DaqInMock::create(string const& name, map<string, DaqInMock::Ptr>* mapDaqIn) {
    Ptr ptr = shared_ptr<DaqInMock>(new DaqInMock(name));

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

DaqInMock::DaqInMock(string const& name) : DaqBase(name) {
    // Look for "scale" in Config
    try {
        _scale = system::Config::get().getSectionKeyAsDouble(getName(), "scale");
    } catch (system::ConfigException const& ex) {
        LWARN("No scale entry found for ", getName());
    }
    LINFO("DaqInMock config ", getName(), " scale=", _scale);
}

void DaqInMock::setRaw(double val) {
    lock_guard<mutex> lg(_mtx);
    _data.raw = val;
    _data.lastRead = FpgaClock::now();
    _data.adjusted = _data.raw * _scale;
    LDEBUG("DaqInMock::setRaw ", getName(), " raw=", _data.raw, " adjusted=", _data.adjusted,
           " scale=", _scale);
}

DaqInMock::Data DaqInMock::getData() {
    lock_guard<mutex> lg(_mtx);
    return _data;
}

std::ostream& DaqInMock::dump(std::ostream& os) {
    DaqBase::dump(os);
    os << "DaqInMock scale=" << _scale;
    os << " lastRead=" << fpgaTimeStr(_data.lastRead) << " raw=" << _data.raw
       << " adjusted=" << _data.adjusted;
    return os;
}

DaqOutMock::Ptr DaqOutMock::create(string const& name, map<string, DaqOutMock::Ptr>* mapDaqOut) {
    Ptr ptr = shared_ptr<DaqOutMock>(new DaqOutMock(name));

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

DaqOutMock::DaqOutMock(string const& name) : DaqBase(name) {
    // Look for "scale" in Config
    try {
        _scale = system::Config::get().getSectionKeyAsDouble(getName(), "scale");
    } catch (system::ConfigException const& ex) {
        LWARN("No scale entry found for ", getName());
    }
    LINFO("DaqOutMock config ", getName(), " scale=", _scale);

    try {
        _linkStr = system::Config::get().getSectionKeyAsString(getName(), "link");
    } catch (system::ConfigException const& ex) {
        LWARN("No link entry found for ", getName());
    }
    LINFO("DaqOutMock config ", getName(), " scale=", _scale);
}

void DaqOutMock::finalSetup(map<string, DaqInMock::Ptr>& mapDaqIn) {
    if (!_linkStr.empty()) {
        auto iter = mapDaqIn.find(_linkStr);
        if (iter != mapDaqIn.end()) {
            _link = iter->second;
            LINFO("Setting DaqOutMock ", getName(), " link to ", _linkStr, " ", _link->getName());
        } else {
            string errMsg =
                    string("DaqOutMock::finalSetup ") + getName() + " couldn't find DaqInMock " + _linkStr;
            LERROR(errMsg);
            throw util::Bug(ERR_LOC, errMsg);
        }
    }
}

void DaqOutMock::setSource(double val) {
    lock_guard<mutex> lg(_mtx);
    _data.source = val;
    _data.outVal = _data.source / _scale;
    LDEBUG("DaqOutMock::setSource ", getName(), " source=", _data.source, " _outVal=", _data.outVal,
           " scale=", _scale);
}

DaqOutMock::Data DaqOutMock::getData() {
    lock_guard<mutex> lg(_mtx);
    return _data;
}

std::ostream& DaqOutMock::dump(std::ostream& os) {
    DaqBase::dump(os);
    os << "DaqOutMock scale=" << _scale;
    os << " lastWrite=" << fpgaTimeStr(_data.lastWrite) << " source=" << _data.source
       << " outVal=" << _data.outVal;
    return os;
}

void DaqOutMock::write() {
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

DaqBoolInMock::Ptr DaqBoolInMock::create(string const& name, map<string, DaqBoolInMock::Ptr>* mapDaqBoolIn) {
    Ptr ptr = shared_ptr<DaqBoolInMock>(new DaqBoolInMock(name));

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

DaqBoolInMock::DaqBoolInMock(string const& name) : DaqBase(name) {}

std::ostream& DaqBoolInMock::dump(std::ostream& os) {
    DaqBase::dump(os);
    os << "DaqBoolInMock lastRead=" << fpgaTimeStr(_lastRead) << " val = " << _val;
    return os;
}

DaqBoolOutMock::Ptr DaqBoolOutMock::create(string const& name,
                                           map<string, DaqBoolOutMock::Ptr>* mapDaqBoolOut) {
    Ptr ptr = shared_ptr<DaqBoolOutMock>(new DaqBoolOutMock(name));

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

DaqBoolOutMock::DaqBoolOutMock(string const& name) : DaqBase(name) {
    try {
        _linkBoolInStr = system::Config::get().getSectionKeyAsString(getName(), "linkBoolIn");
    } catch (system::ConfigException const& ex) {
        LWARN("No link entry found for ", getName());
    }
    LINFO("DaqBoolOutMock config ", getName(), " linkBoolIn=", _linkBoolInStr);

    try {
        _linkCurrentOutStr = system::Config::get().getSectionKeyAsString(getName(), "linkCurrentOut");
        try {
            _linkCurrentOutVal = system::Config::get().getSectionKeyAsDouble(getName(), "linkCurrentOutVal");
        } catch (system::ConfigException const& ex) {
            throw util::Bug(ERR_LOC, string("If linkCurrentOut is defined for ") + getName() +
                                             " linkCurrentOutVal must be defined");
        }
    } catch (system::ConfigException const& ex) {
        LWARN("No linkCurrent entry found for ", getName());
    }
    LINFO("DaqBoolOutMock config ", getName(), " linkCurrentOut=", _linkCurrentOutStr,
          " val=", _linkCurrentOutVal);

    try {
        _linkVoltageOutStr = system::Config::get().getSectionKeyAsString(getName(), "linkVoltageOut");
        try {
            _linkVoltageOutVal = system::Config::get().getSectionKeyAsDouble(getName(), "linkVoltageOutVal");
        } catch (system::ConfigException const& ex) {
            throw util::Bug(ERR_LOC, string("If linkVoltageOut is defined for ") + getName() +
                                             " linkVoltageOutVal must be defined");
        }
    } catch (system::ConfigException const& ex) {
        LWARN("No linkVoltage entry found for ", getName());
    }
    LINFO("DaqBoolOutMock config ", getName(), " linkVoltageOut=", _linkVoltageOutStr,
          " val=", _linkVoltageOutVal);
}

void DaqBoolOutMock::finalSetup(map<string, DaqBoolInMock::Ptr>& mapDaqBoolIn,
                                map<string, DaqOutMock::Ptr>& mapDaqOut) {
    // Setup link to DaqBoolInMock, if defined
    if (!_linkBoolInStr.empty()) {
        auto iter = mapDaqBoolIn.find(_linkBoolInStr);
        if (iter != mapDaqBoolIn.end()) {
            _linkBoolIn = iter->second;
            LINFO("Setting DaqBoolOutMock ", getName(), " link to ", _linkBoolInStr, " ",
                  _linkBoolIn->getName());
        } else {
            string errMsg = string("DaqBoolOutMock::finalSetup ") + getName() +
                            " couldn't find DaqBoolInMock " + _linkBoolInStr;
            LERROR(errMsg);
            throw util::Bug(ERR_LOC, errMsg);
        }
    }

    // Setup link to DaqOutMock for current
    if (!_linkCurrentOutStr.empty()) {
        auto iter = mapDaqOut.find(_linkCurrentOutStr);
        if (iter != mapDaqOut.end()) {
            _linkCurrentOut = iter->second;
            LINFO("Setting DaqBoolOutMock ", getName(), " link to ", _linkCurrentOutStr, " ",
                  _linkCurrentOut->getName());
        } else {
            string errMsg = string("DaqOutMock::finalSetup ") + getName() + " couldn't find DaqOutMock " +
                            _linkCurrentOutStr;
            LERROR(errMsg);
            throw util::Bug(ERR_LOC, errMsg);
        }
    }

    // Setup link to DaqOutMock for Voltage
    if (!_linkVoltageOutStr.empty()) {
        auto iter = mapDaqOut.find(_linkVoltageOutStr);
        if (iter != mapDaqOut.end()) {
            _linkVoltageOut = iter->second;
            LINFO("Setting DaqBoolOutMock ", getName(), " link to ", _linkVoltageOutStr, " ",
                  _linkVoltageOut->getName());
        } else {
            string errMsg = string("DaqOutMock::finalSetup ") + getName() + " couldn't find DaqOutMock " +
                            _linkVoltageOutStr;
            LERROR(errMsg);
            throw util::Bug(ERR_LOC, errMsg);
        }
    }
}

void DaqBoolOutMock::write() {
    _lastWrite = FpgaClock::now();

    LDEBUG(getName(), " DaqBoolOutMock::write val=", _val);
    // Forward this output to the linked input.
    if (_linkBoolIn != nullptr) {
        LDEBUG(getName(), " DaqBoolOutMock::write bool val=", _val, " to ", _linkBoolIn);
        _linkBoolIn->setVal(_val);
    }

    // Set current and voltage DaqOutMock items if specified
    if (_linkCurrentOut != nullptr) {
        double currentVal = _val ? _linkCurrentOutVal : 0.0;
        LDEBUG(getName(), " DaqBoolOutMock::write val=", _val, " current ", _linkCurrentOut->getName(), " ",
               currentVal);
        _linkCurrentOut->setSource(currentVal);
    }
    if (_linkVoltageOut != nullptr) {
        double voltageVal = _val ? _linkVoltageOutVal : 0.0;
        LDEBUG(getName(), " DaqBoolOutMock::write val=", _val, " voltage ", _linkVoltageOut->getName(), " ",
               voltageVal);
        _linkVoltageOut->setSource(voltageVal);
    }
}

std::ostream& DaqBoolOutMock::dump(std::ostream& os) {
    DaqBase::dump(os);
    os << "DaqBoolOutMock linkBoolInStr=" << _linkBoolInStr;
    os << " (CurrentOutStr=" << _linkCurrentOutStr << ":" << _linkCurrentOutVal << ")";
    os << " (VoltageOutStr=" << _linkVoltageOutStr << ":" << _linkVoltageOutVal << ")";
    os << " lastWrite=" << fpgaTimeStr(_lastWrite) << " val=" << _val;
    return os;
}

}  // namespace control
}  // namespace m2cellcpp
}  // namespace LSST
