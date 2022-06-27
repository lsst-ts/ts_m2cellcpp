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
#include <memory>
#include <mutex>
#include <stdint.h>
#include <string>
#include <vector>

// Project headers

#ifndef LSST_M2CELLCPP_CONTROL_FPGAIO_H
#define LSST_M2CELLCPP_CONTROL_FPGAIO_H

namespace LSST {
namespace m2cellcpp {
namespace control {

/// TODO: Times should really be based on values from the
///       FPGA, uint64_t, but use these for now.
typedef std::chrono::system_clock FpgaClock;
typedef std::chrono::time_point<FpgaClock> FpgaTimePoint;

/// Values from the DAQ on the cRIO, for single analog input.
class DaqIn {
public:
    using Ptr = std::shared_ptr<DaqIn>;

    /// DaqIn data values, must be copyable.
    struct Data {
        double raw = 0;          ///< Value as read from FPGA
        FpgaTimePoint lastRead;  ///< Last time _raw was read from Fpga.
        double adjusted = 0.0;   ///< Adjusted value, for internal use.
        bool upToDate = false;   ///< True if _adjusted is based on latest _raw value.
    };

    /// Uses Config to set values using `name`.
    /// FUTURE: Likely to need defines from the LaBView C API
    DaqIn(std::string const& name);

    DaqIn() = delete;
    DaqIn(DaqIn const&) = delete;
    DaqIn& operator=(DaqIn const&) = delete;

    ~DaqIn() = default;

    /// Set `_raw` to `val` and `_lastRead` to now.
    /// FUTURE: type for `val` possibly incorrect.
    void setRaw(double val);

    /// Use value of `_data.raw to set `_data.adjusted`
    /// FUTURE: What values need to be setup to make the `adjust()` call?
    void adjust();

    /// Returns a copy of `_data`.
    Data getData();

private:
    std::string const _name;  ///< The name of this DAQ.
    double _scale = 1.0;      ///< scale between raw and adjusted.
    Data _data;               ///< Data read from the DAQ for this analog value.
    std::mutex _mtx;          ///< protects _data
};

/// Values to the DAQ on the cRIO, for single analog output.
/// FUTURE: DaqIn and DaqOut may have enough commonalities to have a shared base class.
class DaqOut {
public:
    using Ptr = std::shared_ptr<DaqOut>;

    /// Data elements for DaqOut, must be copyable.
    struct Data {
        double outVal = 0;        ///< Value to write to FPGA
        FpgaTimePoint lastWrite;  ///< Last time `outVal` was written to Fpga.
        double source = 0.0;      ///< Internal value.
        bool upToDate = false;    ///< True if _adjusted is based on latest _raw value.
    };

    /// Uses Config to set values using `name`.
    /// FUTURE: Likely to need defines from the LaBView C API
    DaqOut(std::string const& name);

    DaqOut() = delete;
    DaqOut(DaqOut const&) = delete;
    DaqOut& operator=(DaqOut const&) = delete;

    ~DaqOut() = default;

    /// Set `_raw` to `val` and `_lastRead` to now.
    /// FUTURE: type for `val` possibly incorrect.
    /// Set `_data.source` to `val`
    void setSource(double val);

    /// Use `_data.source` to set`data.outVal` and set `upToDate` = true.
    /// FUTURE: What values need to be setup to make the `adjust()` call?
    void adjust();

    /// Return a copy of `_data`
    Data getData();

    /// Just set _data.lastWrite to now.
    /// FUTURE: possibly change name and actuall write the data to the FPGA.
    void written();

private:
    std::string const _name;  ///< The name of this DAQ.
    double _scale = 1.0;      ///< scale between source and outVal.
    Data _data;               ///< Data read from the DAQ for this analog value.
    std::mutex _mtx;          ///< protects _data
};

/// Class to store a boolean value from the FPGA.
class DaqBoolIn {
public:
    using Ptr = std::shared_ptr<DaqBoolIn>;
    /// Uses Config to set values using `name`.
    /// FUTURE: Likely to need defines from the LaBView C API
    DaqBoolIn(std::string const& name) : _name(name) {}

    DaqBoolIn() = delete;
    DaqBoolIn(DaqBoolIn const&) = delete;
    DaqBoolIn& operator=(DaqBoolIn const&) = delete;

    ~DaqBoolIn() = default;

    void setVal(bool v) {
        std::lock_guard<std::mutex> lg(_mtx);
        _lastRead = FpgaClock::now();
        _val = v;
    }

    /// Return the last time this value was read.
    FpgaTimePoint getLastRead() {
        std::lock_guard<std::mutex> lg(_mtx);
        return _lastRead;
    }

    /// Return the boolean value.
    bool getVal() {
        std::lock_guard<std::mutex> lg(_mtx);
        return _val;
    }

private:
    std::string _name;        ///< Name of this input
    FpgaTimePoint _lastRead;  ///< Last time this input was read.
    bool _val = false;        ///< value read for this input.
    std::mutex _mtx;          ///< protects _val and _lastRead.
};

/// Class to store a boolean value to write to the FPGA.
class DaqBoolOut {
public:
    using Ptr = std::shared_ptr<DaqBoolOut>;

    /// Uses Config to set values using `name`.
    /// FUTURE: Likely to need defines from the LaBView C API
    DaqBoolOut(std::string const& name) : _name(name) {}

    DaqBoolOut() = delete;
    DaqBoolOut(DaqBoolOut const&) = delete;
    DaqBoolOut& operator=(DaqBoolOut const&) = delete;

    ~DaqBoolOut() = default;

    /// Sewt `_val` to `v`.
    void setVal(bool v) { _val = v; }

    /// Return the last time this value was read.
    FpgaTimePoint getLastWrite() { return _lastWrite; }

    /// Return the boolean value.
    bool getVal() { return _val; }

    /// Just set _lastWrite to now.
    /// FUTURE: possibly change name and actually write the data to the FPGA.
    void written() { _lastWrite = FpgaClock::now(); }

private:
    std::string _name;         ///< Name of this input
    FpgaTimePoint _lastWrite;  ///< Last time this input was written.
    bool _val;                 ///< value read for this input.
};

/// This class contains information about one ILC.
/// FUTURE: Use this as a base class for Axial and Tangential ILC classes.
/// Unit tests in test_FpgaIo.cpp
class Ilc {
public:
    using Ptr = std::shared_ptr<Ilc>;

    /// Create a new Ilc
    Ilc(std::string const& name, int idNum);

    Ilc() = delete;
    Ilc(Ilc const&) = delete;
    Ilc& operator=(Ilc const&) = delete;

    ~Ilc() = default;

    /// Return the `_name` of the ILC.
    std::string getName() { return _name; }

    /// Return the `_idNum` of the ILC.
    int getIdNum() { return _idNum; }

    /// Return value of `bit` from `byt`.
    /// `bit` 0 would be the left/least most bit.
    /// `bit` 7 would be the right most bit.
    static bool getBit(int bit, uint8_t byt);

    /// Return true if ILC Fault
    bool getFault();

    /// Return true if clockwise limit tripped
    bool getCWLimit();

    /// Return true if clockwise limit tripped
    bool getCCWLimit();

    /// Return Broadcast communication counter
    uint16_t getBroadcastCommCount();

    /// Set `_rawStatus` to `val`.
    void setStatus(uint8_t val);

private:
    std::string _name;  ///< Name of the ILC
    int _idNum;         ///< Id number.
    /// Raw status byte.
    /// bit0: ILC Fault
    /// bit1: not used (always 0)
    /// bit2: limit switch CW (0=open, 1=closed)
    /// bit3: limit switch CCW (0=open, 1=closed)
    /// bit4..7: Broadcast communication counter (0..15)
    uint8_t _rawStatus = 0;
    int32_t _rawPosition = 0;  ///< encoder position
    float _rawForce = 0.0;     ///< Actuator force (Float32 scaled in Newtons from ILC)
};

class AllIlcs {
public:
    using Ptr = std::shared_ptr<AllIlcs>;

    /// Constructor requires Config has been setup
    AllIlcs(bool useMocks);

    AllIlcs() = delete;
    AllIlcs(AllIlcs const&) = delete;
    AllIlcs& operator=(AllIlcs const&) = delete;

    ~AllIlcs() = default;

    /// Return a pointer to the ILC.
    /// throws: out_of_range if idNum < 1 or > 78
    Ilc::Ptr getIlc(int idNum) { return _getIlcPtr(idNum); }

private:
    /// Returns a reference to the Ptr for the ILC.
    /// throws: out_of_range if idNum < 1 or > 78
    Ilc::Ptr& _getIlcPtr(unsigned int idNum);

    std::vector<Ilc::Ptr> _ilcs;  ///< vector of all ILC's.
};

/// This class is currently a place holder for FPGA I/O.
/// Since the rest of the system isn't setup for I/O with
/// the FPGA, this class will be a placeholder for now,
/// and likely used as a mock in the future.
class FpgaIo {
public:
    /// Construct the FpgaIo instance, using mocks if `useMocks` is true.
    /// Mocks are meant for unit testing where no hardware is available.
    FpgaIo(bool useMocks);

private:
    DaqIn::Ptr _IlcMotorCurrent;  ///< ILC motor current input
    DaqIn::Ptr _IlcCommCurrent;   ///< ILC comm current input
    DaqIn::Ptr _IlcMotorVoltage;  ///< ILC motor voltage input
    DaqIn::Ptr _IlcCommVoltage;   ///< ILC comm voltage input

    DaqBoolOut::Ptr _IlcMotorPowerOnOut;      ///< ILC mototr power on output
    DaqBoolOut::Ptr _IlcCommPowerOnOut;       //< ILomm power on output
    DaqBoolOut::Ptr _CrioInterlockEnableOut;  ///< cRIO interlock enable output

    DaqBoolIn::Ptr _IlcMotorPowerOnIn;      ///< ILC mototr power on input
    DaqBoolIn::Ptr _IlcCommPowerOnIn;       ///< ILC comm power on input
    DaqBoolIn::Ptr _CrioInterlockEnableIn;  ///< cRIO interlock enable input

    AllIlcs::Ptr _ilcs;  ///< Vector of all ILC

    // The following are only expected to be found on the test hardware.
    DaqOut::Ptr _testIlcMotorCurrent;  ///< ILC motor current test output
    DaqOut::Ptr _testIlcCommCurrent;   ///< ILC comm current test output
    DaqOut::Ptr _testIlcMotorVoltage;  ///< ILC motor voltage test output
    DaqOut::Ptr _testIlcCommVoltage;   ///< ILC comm voltage test output
};

}  // namespace control
}  // namespace m2cellcpp
}  // namespace LSST

#endif  // LSST_M2CELLCPP_CONTROL_FPGAIO_H
