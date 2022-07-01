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
#include <ctime>
#include <map>
#include <memory>
#include <mutex>
#include <stdint.h>
#include <string>
#include <vector>

// Project headers
#include "util/Log.h"

#ifndef LSST_M2CELLCPP_CONTROL_DAQBASE_H
#define LSST_M2CELLCPP_CONTROL_DAQBASE_H

namespace LSST {
namespace m2cellcpp {
namespace control {

/// TODO: Times should really be based on values from the
///       FPGA, uint64_t, but use these for now.
typedef std::chrono::system_clock FpgaClock;
typedef std::chrono::time_point<FpgaClock> FpgaTimePoint;

/// Return a reasonable string for `timePoint`.
inline std::string fpgaTimeStr(FpgaTimePoint timePoint) {
    std::time_t timePointC = std::chrono::system_clock::to_time_t(timePoint);
    // FUTURE: strftime or something is probably a better choice.
    std::string str(std::ctime(&timePointC));
    // remove the '\n' that ctime added to the end of str.
    return str.substr(0, str.length() - 1);
}

/// Class to contain basic DAQ information.
class DaqBase {
public:
    using Ptr = std::shared_ptr<DaqBase>;

    DaqBase(std::string const& name) : _name(name) {}

    DaqBase() = delete;
    DaqBase(DaqBase const&) = delete;
    DaqBase& operator=(DaqBase const&) = delete;

    ~DaqBase() = default;

    /// Returns the DAQ's name
    std::string getName() const { return _name; }

    /// Return a log worthy string describing the object.
    /// This calls `std::ostream& DaqBase::dump(std::ostream& os)`.
    std::string dump() {
        std::stringstream os;
        dump(os);
        return os.str();
    }

    /// Method for child classes to override for extended dump information.
    virtual std::ostream& dump(std::ostream& os) {
        os << "Daq:" << _name << ":";
        return os;
    }

private:
    std::string const _name;  ///< name of this DAQ
};

}  // namespace control
}  // namespace m2cellcpp
}  // namespace LSST

#endif  // LSST_M2CELLCPP_CONTROL_DAQBASE_H
