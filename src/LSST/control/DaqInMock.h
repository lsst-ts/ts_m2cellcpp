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
#include "control/DaqBase.h"
#include "util/Log.h"

#ifndef LSST_M2CELLCPP_CONTROL_DAQINMOCK_H
#define LSST_M2CELLCPP_CONTROL_DAQINMOCK_H

namespace LSST {
namespace m2cellcpp {
namespace control {

/// Mockup for values from the DAQ on the cRIO, for single analog input.
/// Unit test in test_FpgaIo.cpp.
class DaqInMock : public DaqBase {
public:
    using Ptr = std::shared_ptr<DaqInMock>;

    /// DaqInMock data values, must be copyable.
    struct Data {
        double raw = 0;          ///< Value as read from FPGA
        FpgaTimePoint lastRead;  ///< Last time _raw was read from Fpga.
        double adjusted = 0.0;   ///< Adjusted value, for internal use.
    };

    /// Uses Config to set values using `name` and adds it to `mapDaqIn`.
    /// `mapDaqIn` can be nullptr.
    static Ptr create(std::string const& name, std::map<std::string, DaqInMock::Ptr>* mapDaqIn);

    DaqInMock() = delete;
    DaqInMock(DaqInMock const&) = delete;
    DaqInMock& operator=(DaqInMock const&) = delete;

    ~DaqInMock() = default;

    /// Set `_raw` to `val` and `_lastRead` to now.
    /// FUTURE: type for `val` possibly incorrect.
    void setRaw(double val);

    /// Returns a copy of `_data`.
    Data getData();

    /// Dump information for the log.
    std::ostream& dump(std::ostream& os) override;

private:
    /// Uses Config to set values using `name`.
    /// FUTURE: Likely to need defines from the LaBView C API
    DaqInMock(std::string const& name);

    double _scale = 1.0;  ///< scale between raw and adjusted.
    Data _data;           ///< Data read from the DAQ for this analog value.
    std::mutex _mtx;      ///< protects _data
};

/// Mockup for values to the DAQ on the cRIO, for single analog output.
/// This class can be setup to write its `outVal` to a DaqInMock.
/// Unit test in test_FpgaIo.cpp.
class DaqOutMock : public DaqBase {
public:
    using Ptr = std::shared_ptr<DaqOutMock>;

    /// Data elements for DaqOutMock, must be copyable.
    struct Data {
        double outVal = 0;        ///< Value to write to FPGA
        FpgaTimePoint lastWrite;  ///< Last time `outVal` was written to Fpga.
        double source = 0.0;      ///< Internal value.
    };

    /// Uses Config to set values using `name` and adds it to `mapDaqOut`.
    /// `mapDaqOut` can be nullptr.
    static Ptr create(std::string const& name, std::map<std::string, DaqOutMock::Ptr>* mapDaqOut);

    DaqOutMock() = delete;
    DaqOutMock(DaqOutMock const&) = delete;
    DaqOutMock& operator=(DaqOutMock const&) = delete;

    ~DaqOutMock() = default;

    /// Setup items that require all other FpgaIo members are defined.
    void finalSetup(std::map<std::string, DaqInMock::Ptr>& mapDaqIn);

    /// Set `_raw` to `val` and `_lastRead` to now.
    /// FUTURE: type for `val` possibly incorrect.
    /// Set `_data.source` to `val`
    void setSource(double val);

    /// Return a copy of `_data`
    Data getData();

    /// Just set _data.lastWrite to now.
    /// FUTURE: possibly change name and actuall write the data to the FPGA.
    void write();

    /// Dump information for the log.
    std::ostream& dump(std::ostream& os) override;

private:
    /// Uses Config to set values using `name`.
    /// FUTURE: Likely to need defines from the LaBView C API
    DaqOutMock(std::string const& name);

    double _scale = 1.0;   ///< scale between source and outVal.
    Data _data;            ///< Data read from the DAQ for this analog value.
    std::mutex _mtx;       ///< protects _data
    DaqInMock::Ptr _link;  ///< If not nullptr, forward outVal to this DaqInMock.
    std::string _linkStr;  ///< Name of DaqInMock for _link
};

/// Mock class to store a boolean value from the FPGA.
/// Unit test in test_FpgaIo.cpp.
class DaqBoolInMock : public DaqBase {
public:
    using Ptr = std::shared_ptr<DaqBoolInMock>;

    /// Uses Config to set values using `name` and add the object to `mapDaqBoolIn`.
    static Ptr create(std::string const& name, std::map<std::string, DaqBoolInMock::Ptr>* mapDaqBoolIn);

    DaqBoolInMock() = delete;
    DaqBoolInMock(DaqBoolInMock const&) = delete;
    DaqBoolInMock& operator=(DaqBoolInMock const&) = delete;

    ~DaqBoolInMock() = default;

    void setVal(bool v) {
        std::lock_guard<std::mutex> lg(_mtx);
        LDEBUG(getName(), " setVal val=", v);
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

    /// Dump information for the log.
    std::ostream& dump(std::ostream& os) override;

private:
    /// Uses Config to set values using `name`.
    /// FUTURE: Likely to need defines from the LaBView C API
    DaqBoolInMock(std::string const& name);

    FpgaTimePoint _lastRead;  ///< Last time this input was read.
    bool _val = false;        ///< value read for this input.
    std::mutex _mtx;          ///< protects _val and _lastRead.
};

/// Mock class to store a boolean value to write to the FPGA.
/// This class can be setup to write its value to other Daq Mock objects,
/// includeing DaqBoolInMock and DaqOutMock.
/// Unit test in test_FpgaIo.cpp.
class DaqBoolOutMock : public DaqBase {
public:
    using Ptr = std::shared_ptr<DaqBoolOutMock>;

    /// Uses Config to set values using `name`, and adds it to `mapDaqBoolOut`.
    static Ptr create(std::string const& name, std::map<std::string, DaqBoolOutMock::Ptr>* mapDaqBoolOut);

    DaqBoolOutMock() = delete;
    DaqBoolOutMock(DaqBoolOutMock const&) = delete;
    DaqBoolOutMock& operator=(DaqBoolOutMock const&) = delete;

    ~DaqBoolOutMock() = default;

    /// Setup items that require all other FpgaIo members are defined.
    void finalSetup(std::map<std::string, DaqBoolInMock::Ptr>& mapDaqBoolIn,
                    std::map<std::string, DaqOutMock::Ptr>& mapDaqOut);

    /// Set `_val` to `v`.
    void setVal(bool v) { _val = v; }

    /// Return the last time this value was read.
    FpgaTimePoint getLastWrite() { return _lastWrite; }

    /// Return the boolean value.
    bool getVal() { return _val; }

    /// Set _lastWrite to now and write the data.
    /// FUTURE: possibly change name and actually write the data to the FPGA.
    void write();

    /// Dump information for the log.
    std::ostream& dump(std::ostream& os) override;

private:
    /// Uses Config to set values using `name`.
    /// FUTURE: Likely to need defines from the LaBView C API
    DaqBoolOutMock(std::string const& name);

    FpgaTimePoint _lastWrite;  ///< Last time this input was written.
    bool _val = false;         ///< value read for this input.

    DaqBoolInMock::Ptr _linkBoolIn;  ///< If not nullptr, forward _val to this DaqBoolInMock.
    std::string _linkBoolInStr;      ///< Name of DaqInMock for _link

    DaqOutMock::Ptr _linkCurrentOut;  ///< Pointer to DaqOutMock for current.
    std::string _linkCurrentOutStr;   ///< Name of the DaqOutMock for current.
    double _linkCurrentOutVal = 0.0;  ///< The value for DaqOutMock current when `_val` is true.
    DaqOutMock::Ptr _linkVoltageOut;  ///< Pointer to DaqOutMock for voltage.
    std::string _linkVoltageOutStr;   ///< Name of the DaqOutMock for voltage.
    double _linkVoltageOutVal = 0.0;  ///< The value for DaqOutMock voltage when `_val` is true.
};

}  // namespace control
}  // namespace m2cellcpp
}  // namespace LSST

#endif  // LSST_M2CELLCPP_CONTROL_DAQINMOCK_H
