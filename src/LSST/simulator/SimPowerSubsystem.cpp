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
#include "simulator/SimPowerSubsystem.h"

// System headers
#include <bitset>

// Project headers
#include "util/Bug.h"
#include "util/Log.h"

using namespace std;

namespace LSST {
namespace m2cellcpp {
namespace simulator {

SimPowerSubsystem::SimPowerSubsystem(control::PowerSystemType systemType,
                                     control::OutputPortBits::Ptr const& outputPort, int powerOnBitPos,
                                     int breakerResetPos, control::InputPortBits::Ptr const& inputPort,
                                     std::vector<int> const& breakerBitPositions)
        : _systemType(systemType),
          _outputPort(outputPort),
          _powerOnBitPos(powerOnBitPos),
          _breakerResetPos(breakerResetPos),
          _inputPort(inputPort),
          _breakerBitPositions(breakerBitPositions) {
    _setup();
}

string SimPowerSubsystem::getClassName() const {
    string str = string("SimPowerSubsystem ") + getPowerSystemTypeStr(_systemType);
    return str;
}

void SimPowerSubsystem::_setup() {
    control::PowerSubsystemConfig psc(_systemType);

    _voltageNominal = psc.getNominalVoltage();

    /// The voltage in the simulator needs to rise fast enough so that
    /// it doesn't trip the safety checks. This value should be large
    /// enough to prevent problems.
    const double rateIncrease = 1.3;

    /// Once powered on, voltage should reach an acceptable level
    /// before outputOnMaxDelay() time has past. See PowerSubsystemCommonConfig.vi
    _voltageChangeRateOn = (_voltageNominal / psc.outputOnMaxDelay()) * rateIncrease;
    /// Similar to on change rate
    _voltageChangeRateOff = (_voltageNominal / psc.outputOffMaxDelay()) * rateIncrease;

    _currentMax = psc.getMaxCurrentFault();  ///< max current, amps. "maximum output current" 20A

    /// Current based on `_voltage`, amp/volt. 0.75 as the system shouldn't normally be running at
    /// maximum current levels.
    _currentGain = 0.75 * (_currentMax / _voltageNominal);

    /// Assuming "breaker on time" is related to how long it takes for the breaker to close.
    /// 0.5 as closing the breaker shouldn't normally take the maximum amount of time.
    _breakerCloseTimeSec = psc.getBreakerOnTime() * 0.5;
}

void SimPowerSubsystem::calcBreakers(util::CLOCK::time_point ts) {
    // breaker reset opens the breaker.
    bool newbreakerClosedTarg = _outputPort->getBitAtPos(_breakerResetPos);
    if (newbreakerClosedTarg != _breakerClosedTarg) {
        _breakerClosedTargTs = ts;
        _breakerClosedTarg = newbreakerClosedTarg;
        LINFO(getClassName(), " calcBreakers _breakerClosedTarg changed to ", _breakerClosedTarg);
    }

    if (!_breakerClosed) {
        if (_breakerClosedTarg) {
            double timeDiff = chrono::duration<double, std::ratio<1, 1>>(ts - _breakerClosedTargTs).count();
            LDEBUG(getClassName(),
                   "SimPowerSubsystem::calcBreakers not _breakerClosed Targ=", _breakerClosedTarg,
                   " timeDiff=", timeDiff, " timeTarg=", _breakerCloseTimeSec);
            if (timeDiff > _breakerCloseTimeSec) {
                _breakerClosed = true;
                LDEBUG(getClassName(),
                       "SimPowerSubsystem::calcBreakers not _breakerClosed changed to true Targ=",
                       _breakerClosedTarg, " timeDiff=", timeDiff, " timeTarg=", _breakerCloseTimeSec);
            }
        }
    } else {
        if (!_breakerClosedTarg) {
            _breakerClosed = false;  // assuming breaker opens extremely quickly
            LDEBUG(getClassName(), "SimPowerSubsystem::calcBreakers _breakerClosed changed to false");
        }
    }

    /// Only set the bits once per _breakerClosed change. This allows so bits to be
    // changed individually elsewhere for testing.
    if (_breakerClosed != _breakerClosedPrev) {
        _breakerClosedPrev = _breakerClosed;
        // This is more for testing purposes so that something affects the `_inputPort`.
        // there's no evidence this logic exists in the hardware. The next ticket
        // will probably use `_inputPort` to trigger PowerSubsystem faults.
        for (auto const& bPos : _breakerBitPositions) {
            /// Bits indicate breaker ok with active high.
            _inputPort->writeBit(bPos, _breakerClosed);
            LDEBUG(getClassName(), " calcBreakers wrote bPos=", bPos, " _breakerClosed=", _breakerClosed);
        }
        bitset<32> inp(_inputPort->getBitmap());
        LDEBUG("SimPowerSubsystem::calcBreakers _inputPort=", inp);
    }
}

void SimPowerSubsystem::calcVoltageCurrent(double timeDiff) {
    double startingVoltage = _voltage;
    if (getPowerOn()) {
        // Allow voltage to go very high if _overVoltage is true.
        double voltNom = (_overVoltage) ? (_voltageNominal * 10.0) : _voltageNominal;
        if (_voltage < voltNom) {
            _voltage += _voltageChangeRateOn * timeDiff;
            if (_voltage > voltNom) {
                _voltage = voltNom;
            }
        }
        if (_voltage > voltNom) {
            _voltage = voltNom;
        }
    } else {
        double voltageMin = 0.0;
        if (_voltage > voltageMin) {
            _voltage -= _voltageChangeRateOff * timeDiff;
        }
        if (_voltage < voltageMin) {
            _voltage = voltageMin;
        }
    }
    if (_breakerClosed) {
        _current = _voltage * _currentGain;
        if (_overCurrent) {
            _current *= 10.0;
        }
    } else {
        _current = 0.0;
    }

    if (_voltage != startingVoltage) {
        LINFO(getClassName(), " _current=", _current, " _voltage=", _voltage);
    }
}

void SimPowerSubsystem::forceOverVoltage(bool overVoltage) {
    _overVoltage = overVoltage;
    LWARN("_overVoltage set to ", to_string(_overVoltage));
}

void SimPowerSubsystem::forceOverCurrent(bool overCurrent) {
    _overCurrent = overCurrent;
    LWARN("_overCurrent set to ", to_string(_overCurrent));
}

}  // namespace simulator
}  // namespace m2cellcpp
}  // namespace LSST
