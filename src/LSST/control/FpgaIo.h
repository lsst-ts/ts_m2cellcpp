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

    /// Uses Config to set values using `name` and adds it to `mapDaqIn`.
    /// `mapDaqIn` can be nullptr.
    static Ptr create(std::string const& name, std::map<std::string, DaqIn::Ptr>* mapDaqIn);

    DaqIn() = delete;
    DaqIn(DaqIn const&) = delete;
    DaqIn& operator=(DaqIn const&) = delete;

    ~DaqIn() = default;

    /// Return `_name`.
    std::string getName() const { return _name; }

    /// Set `_raw` to `val` and `_lastRead` to now.
    /// FUTURE: type for `val` possibly incorrect.
    void setRaw(double val);

    /// Use value of `_data.raw to set `_data.adjusted`
    /// FUTURE: What values need to be setup to make the `adjust()` call?
    void adjust();

    /// Returns a copy of `_data`.
    Data getData();

private:
    /// Uses Config to set values using `name`.
    /// FUTURE: Likely to need defines from the LaBView C API
    DaqIn(std::string const& name);

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

    /// Uses Config to set values using `name` and adds it to `mapDaqOut`.
    /// `mapDaqOut` can be nullptr.
    static Ptr create(std::string const& name, std::map<std::string, DaqOut::Ptr>* mapDaqOut);

    DaqOut() = delete;
    DaqOut(DaqOut const&) = delete;
    DaqOut& operator=(DaqOut const&) = delete;

    ~DaqOut() = default;

    /// Return `_name`.
    std::string getName() const { return _name; }

    /// Setup items that require all other FpgaIo members are defined.
    void finalSetup(std::map<std::string, DaqIn::Ptr>& mapDaqIn);

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
    void write();

private:
    /// Uses Config to set values using `name`.
    /// FUTURE: Likely to need defines from the LaBView C API
    DaqOut(std::string const& name);

    std::string const _name;  ///< The name of this DAQ.
    double _scale = 1.0;      ///< scale between source and outVal.
    Data _data;               ///< Data read from the DAQ for this analog value.
    std::mutex _mtx;          ///< protects _data
    DaqIn::Ptr _link;         ///< If not nullptr, forward outVal to this DaqIn.
    std::string _linkStr;     ///< Name of DaqIn for _link
};

/// Class to store a boolean value from the FPGA.
class DaqBoolIn {
public:
    using Ptr = std::shared_ptr<DaqBoolIn>;

    /// Uses Config to set values using `name` and add the object to `mapDaqBoolIn`.
    static Ptr create(std::string const& name, std::map<std::string, DaqBoolIn::Ptr>* mapDaqBoolIn);

    DaqBoolIn() = delete;
    DaqBoolIn(DaqBoolIn const&) = delete;
    DaqBoolIn& operator=(DaqBoolIn const&) = delete;

    ~DaqBoolIn() = default;

    /// Return `_name`.
    std::string getName() const { return _name; }

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
    /// Uses Config to set values using `name`.
    /// FUTURE: Likely to need defines from the LaBView C API
    DaqBoolIn(std::string const& name);

    std::string _name;        ///< Name of this input
    FpgaTimePoint _lastRead;  ///< Last time this input was read.
    bool _val = false;        ///< value read for this input.
    std::mutex _mtx;          ///< protects _val and _lastRead.
};

/// Class to store a boolean value to write to the FPGA.
class DaqBoolOut {
public:
    using Ptr = std::shared_ptr<DaqBoolOut>;

    /// Uses Config to set values using `name`, and adds it to `mapDaqBoolOut`.
    static Ptr create(std::string const& name, std::map<std::string, DaqBoolOut::Ptr>* mapDaqBoolOut);

    DaqBoolOut() = delete;
    DaqBoolOut(DaqBoolOut const&) = delete;
    DaqBoolOut& operator=(DaqBoolOut const&) = delete;

    ~DaqBoolOut() = default;

    /// Return `_name`.
    std::string getName() const { return _name; }

    /// Setup items that require all other FpgaIo members are defined.
    void finalSetup(std::map<std::string, DaqBoolIn::Ptr>& mapDaqBoolIn,
                    std::map<std::string, DaqOut::Ptr>& mapDaqOut);

    /// Set `_val` to `v`.
    void setVal(bool v) { _val = v; }

    /// Return the last time this value was read.
    FpgaTimePoint getLastWrite() { return _lastWrite; }

    /// Return the boolean value.
    bool getVal() { return _val; }

    /// Set _lastWrite to now and write the data.
    /// FUTURE: possibly change name and actually write the data to the FPGA.
    void write() { _lastWrite = FpgaClock::now(); }

private:
    /// Uses Config to set values using `name`.
    /// FUTURE: Likely to need defines from the LaBView C API
    DaqBoolOut(std::string const& name);

    std::string _name;         ///< Name of this input
    FpgaTimePoint _lastWrite;  ///< Last time this input was written.
    bool _val;                 ///< value read for this input.

    DaqBoolIn::Ptr _linkBoolIn;  ///< If not nullptr, forward _val to this DaqBoolIn.
    std::string _linkBoolInStr;  ///< Name of DaqIn for _link

    DaqOut::Ptr _linkCurrentOut;     ///< Pointer to DaqOut for current.
    std::string _linkCurrentOutStr;  ///< Name of the DaqOut for current.
    double _linkCurrentOutVal;       ///< The value for DaqOut current when `_val` is true.
    DaqOut::Ptr _linkVoltageOut;     ///< Pointer to DaqOut for voltage.
    std::string _linkVoltageOutStr;  ///< Name of the DaqOut for voltage.
    double _linkVoltageOutVal;       ///< The value for DaqOut voltage when `_val` is true.
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
    FpgaIo(bool useMocks = false);

    /// Returns `_ilcMotorCurrent`
    DaqIn::Ptr getIlcMotorCurrent() const { return _ilcMotorCurrent; }
    /// Returns `_ilcCommCurrent`
    DaqIn::Ptr getIlcCommCurrent() const { return _ilcCommCurrent; }
    /// Returns `_ilcMotorVoltage`
    DaqIn::Ptr getIlcMotorVoltage() const { return _ilcMotorVoltage; }
    /// Returns `_ilcCommVoltage`
    DaqIn::Ptr getIlcCommVoltage() const { return _ilcCommVoltage; }

    /// Returns `_ilcMotorPowerOnOut`
    DaqBoolOut::Ptr getIlcMotorPowerOnOut() const { return _ilcMotorPowerOnOut; }
    /// Returns `_ilcCommPowerOnOut`
    DaqBoolOut::Ptr getIlcCommPowerOnOut() const { return _ilcCommPowerOnOut; }
    /// Returns `_crioInterlockEnableOut`
    DaqBoolOut::Ptr getCrioInterlockEnableOut() const { return _crioInterlockEnableOut; }

    /// Returns `_ilcMotorPowerOnIn`
    DaqBoolIn::Ptr getIlcMotorPowerOnIn() const { return _ilcMotorPowerOnIn; }
    /// Returns `_ilcCommPowerOnIn`
    DaqBoolIn::Ptr getIlcCommPowerOnIn() const { return _ilcCommPowerOnIn; }
    /// Returns `_crioInterlockEnableIn`
    DaqBoolIn::Ptr getCrioInterlockEnableIn() const { return _crioInterlockEnableIn; }

    /// Returns pointer container for all ILC instances.
    AllIlcs::Ptr getAllIlcs() const { return _ilcs; }

    /// Returns `_testIlcMotorCurrent`
    DaqOut::Ptr getTestIlcMotorCurrent() const { return _testIlcMotorCurrent; }
    /// Returns `_testIlcCommCurrent`
    DaqOut::Ptr getTestIlcCommCurrent() const { return _testIlcCommCurrent; }
    /// Returns `_testIlcMotorVoltage`
    DaqOut::Ptr getTestIlcMotorVoltage() const { return _testIlcMotorVoltage; }
    /// Returns `_testIlcCommVoltage`
    DaqOut::Ptr getTestIlcCommVoltage() const { return _testIlcCommVoltage; }

private:
    std::map<std::string, DaqIn::Ptr> _mapDaqIn;            ///< map of all DaqIn instances.
    std::map<std::string, DaqOut::Ptr> _mapDaqOut;          ///< map of all DaqOut instances.
    std::map<std::string, DaqBoolIn::Ptr> _mapDaqBoolIn;    ///< map of all DaqBoolIn instances.
    std::map<std::string, DaqBoolOut::Ptr> _mapDaqBoolOut;  ///< map of all DaqBoolOut instances.

    DaqIn::Ptr _ilcMotorCurrent;  ///< ILC motor current input
    DaqIn::Ptr _ilcCommCurrent;   ///< ILC comm current input
    DaqIn::Ptr _ilcMotorVoltage;  ///< ILC motor voltage input
    DaqIn::Ptr _ilcCommVoltage;   ///< ILC comm voltage input

    DaqBoolOut::Ptr _ilcMotorPowerOnOut;      ///< ILC mototr power on output
    DaqBoolOut::Ptr _ilcCommPowerOnOut;       //< ILomm power on output
    DaqBoolOut::Ptr _crioInterlockEnableOut;  ///< cRIO interlock enable output

    DaqBoolIn::Ptr _ilcMotorPowerOnIn;      ///< ILC mototr power on input
    DaqBoolIn::Ptr _ilcCommPowerOnIn;       ///< ILC comm power on input
    DaqBoolIn::Ptr _crioInterlockEnableIn;  ///< cRIO interlock enable input

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
