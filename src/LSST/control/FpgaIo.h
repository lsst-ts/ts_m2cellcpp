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

// System headers
#include <chrono>
#include <map>
#include <memory>
#include <mutex>
#include <stdint.h>
#include <string>
#include <vector>

// Project headers
#include "control/DaqInMock.h"
#include "control/Ilc.h"
#include "util/Log.h"

#ifndef LSST_M2CELLCPP_CONTROL_FPGAIO_H
#define LSST_M2CELLCPP_CONTROL_FPGAIO_H

namespace LSST {
namespace m2cellcpp {
namespace control {

/// This class is currently a place holder for FPGA I/O.
/// Since the rest of the system isn't setup for I/O with
/// the FPGA, this class will be a placeholder for now,
/// and likely used as a mock in the future.
/// Unit test in test_FpgaIo.cpp.
class FpgaIo {
public:
    /// Construct the FpgaIo instance, using mocks if `useMocks` is true.
    /// Mocks are meant for unit testing where no hardware is available.
    FpgaIo(bool useMocks = false);

    /// Wrtie all outputs, starting with `DaqBoolOutMock`. ILC's not included.
    void writeAllOutputs();

    /// Returns `_ilcMotorCurrent`
    DaqInMock::Ptr getIlcMotorCurrent() const { return _ilcMotorCurrent; }
    /// Returns `_ilcCommCurrent`
    DaqInMock::Ptr getIlcCommCurrent() const { return _ilcCommCurrent; }
    /// Returns `_ilcMotorVoltage`
    DaqInMock::Ptr getIlcMotorVoltage() const { return _ilcMotorVoltage; }
    /// Returns `_ilcCommVoltage`
    DaqInMock::Ptr getIlcCommVoltage() const { return _ilcCommVoltage; }

    /// Returns `_ilcMotorPowerOnOut`
    DaqBoolOutMock::Ptr getIlcMotorPowerOnOut() const { return _ilcMotorPowerOnOut; }
    /// Returns `_ilcCommPowerOnOut`
    DaqBoolOutMock::Ptr getIlcCommPowerOnOut() const { return _ilcCommPowerOnOut; }
    /// Returns `_crioInterlockEnableOut`
    DaqBoolOutMock::Ptr getCrioInterlockEnableOut() const { return _crioInterlockEnableOut; }

    /// Returns `_ilcMotorPowerOnIn`
    DaqBoolInMock::Ptr getIlcMotorPowerOnIn() const { return _ilcMotorPowerOnIn; }
    /// Returns `_ilcCommPowerOnIn`
    DaqBoolInMock::Ptr getIlcCommPowerOnIn() const { return _ilcCommPowerOnIn; }
    /// Returns `_crioInterlockEnableIn`
    DaqBoolInMock::Ptr getCrioInterlockEnableIn() const { return _crioInterlockEnableIn; }

    /// Returns pointer container for all ILC instances.
    AllIlcs::Ptr getAllIlcs() const { return _ilcs; }

    /// Returns `_testIlcMotorCurrent`
    DaqOutMock::Ptr getTestIlcMotorCurrent() const { return _testIlcMotorCurrent; }
    /// Returns `_testIlcCommCurrent`
    DaqOutMock::Ptr getTestIlcCommCurrent() const { return _testIlcCommCurrent; }
    /// Returns `_testIlcMotorVoltage`
    DaqOutMock::Ptr getTestIlcMotorVoltage() const { return _testIlcMotorVoltage; }
    /// Returns `_testIlcCommVoltage`
    DaqOutMock::Ptr getTestIlcCommVoltage() const { return _testIlcCommVoltage; }

    /// Return a log string describing the contents of this FpgaIo object.
    std::string dump();

private:
    std::map<std::string, DaqInMock::Ptr> _mapDaqIn;            ///< map of all DaqInMock instances.
    std::map<std::string, DaqOutMock::Ptr> _mapDaqOut;          ///< map of all DaqOutMock instances.
    std::map<std::string, DaqBoolInMock::Ptr> _mapDaqBoolIn;    ///< map of all DaqBoolInMock instances.
    std::map<std::string, DaqBoolOutMock::Ptr> _mapDaqBoolOut;  ///< map of all DaqBoolOutMock instances.

    DaqInMock::Ptr _ilcMotorCurrent;  ///< ILC motor current input
    DaqInMock::Ptr _ilcCommCurrent;   ///< ILC communication current input
    DaqInMock::Ptr _ilcMotorVoltage;  ///< ILC motor voltage input
    DaqInMock::Ptr _ilcCommVoltage;   ///< ILC communication voltage input

    DaqBoolOutMock::Ptr _ilcMotorPowerOnOut;      ///< ILC motor power on output
    DaqBoolOutMock::Ptr _ilcCommPowerOnOut;       //< ILomm power on output
    DaqBoolOutMock::Ptr _crioInterlockEnableOut;  ///< cRIO interlock enable output

    DaqBoolInMock::Ptr _ilcMotorPowerOnIn;      ///< ILC motor power on input
    DaqBoolInMock::Ptr _ilcCommPowerOnIn;       ///< ILC communication power on input
    DaqBoolInMock::Ptr _crioInterlockEnableIn;  ///< cRIO interlock enable input

    AllIlcs::Ptr _ilcs;  ///< Vector of all ILC

    // The following are only expected to be found on the test hardware.
    DaqOutMock::Ptr _testIlcMotorCurrent;  ///< ILC motor current test output
    DaqOutMock::Ptr _testIlcCommCurrent;   ///< ILC communication current test output
    DaqOutMock::Ptr _testIlcMotorVoltage;  ///< ILC motor voltage test output
    DaqOutMock::Ptr _testIlcCommVoltage;   ///< ILC communication voltage test output
};

}  // namespace control
}  // namespace m2cellcpp
}  // namespace LSST

#endif  // LSST_M2CELLCPP_CONTROL_FPGAIO_H
