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

// Project headers
#include "util/Bug.h"
#include "util/Log.h"

using namespace std;

namespace LSST {
namespace m2cellcpp {
namespace simulator {

SimPowerSubsystem::SimPowerSubsystem(control::PowerSubsystemConfig::SystemType systemType,
        control::OutputPortBits::Ptr const& outputPort, int powerOnBitPos, int breakerResetPos,
        control::InputPortBits::Ptr const& inputPort,  std::vector<int> const& breakerBitPositions)
        : _systemType(systemType) ,_outputPort(outputPort),  _powerOnBitPos(powerOnBitPos), _breakerResetPos(breakerResetPos),
          _inputPort(inputPort), _breakerBitPositions(breakerBitPositions) {
    _setup();
}

void SimPowerSubsystem::_setup() {
    control::PowerSubsystemConfig psc(_systemType);

    _voltageNominal = psc.getNominalVoltage();

     /// Once powered on, voltage should reach an acceptable level
     /// before outputOnMaxDelay() time has past. See PowerSubsystemCommonConfig.vi
     _voltageChangeRateOn = (_voltageNominal/psc.outputOnMaxDelay())*1.3;
     /// Similar to on change rate
     _voltageChangeRateOff = (_voltageNominal/psc.outputOffMaxDelay())*1.3;

     _currentMax = psc.getMaxCurrentFault(); ///< max current, amps. "maximum output current" 20A

     /// Current based on `_voltage`, amp/volt. 0.75 as the system shouldn't normally be running at
     /// maximum current levels.
     _currentGain = 0.75 * (_currentMax/_voltageNominal);

     /// Assuming "breaker on time" is related to how long it takes for the breaker to close.
     /// 0.75 as closing the breaker shouldn't normally take the maximum amount of time.
     _breakerCloseTimeSec = psc.getBreakerOnTime() * 0.75;
}


void SimPowerSubsystem::calcBreakers(util::CLOCK::time_point ts) {
    // breaker reset opens the breaker.
    bool newbreakerClosedTarg = !(_outputPort->getBit(_breakerResetPos));
    if (newbreakerClosedTarg != _breakerClosedTarg) {
        _breakerClosedTargTs = ts;
        _breakerClosedTarg = newbreakerClosedTarg;
    }

    if (!_breakerClosed) {
        if (_breakerClosedTarg) {
            double timeDiff = chrono::duration<double, std::ratio<1,1>>(ts - _breakerClosedTargTs).count();
            if (timeDiff > _breakerCloseTimeSec) {
                _breakerClosed = true;
            }
        }
    } else {
        if (!_breakerClosedTarg) {
            _breakerClosed = false; // assuming breaker opens extremely quickly
        }
    }

    for (auto const& bPos : _breakerBitPositions) {
        /// Bits indicate breaker ok with active high.
        _inputPort->writeBit(bPos, _breakerClosed);
    }
}


void SimPowerSubsystem::calcVoltageCurrent(double timeDiff) {
    if (getPowerOn()) {
        if (_voltage < _voltageNominal) {
            _voltage += _voltageChangeRateOn*timeDiff;
            if (_voltage > _voltageNominal) {
                _voltage = _voltageNominal;
            }
        }
        if (_voltage > _voltageNominal) {
            _voltage = _voltageNominal;
        }
    } else {
        double voltageMin = 0.0;
        if (_voltage > voltageMin) {
            _voltage -= _voltageChangeRateOff*timeDiff;
        }
        if (_voltage < voltageMin) {
            _voltage = voltageMin;
        }
    }
    if (_breakerClosed) {
        _current = _voltage*_currentGain;
    } else {
        _current = 0.0;
    }

    LTRACE(control::PowerSubsystemConfig::getPrettyType(_systemType),
           " _current=", _current, " _voltage=", _voltage);
}



}  // namespace simulator
}  // namespace m2cellcpp
}  // namespace LSST
