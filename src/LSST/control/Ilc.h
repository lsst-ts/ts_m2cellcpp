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
#include "util/Log.h"

#ifndef LSST_M2CELLCPP_CONTROL_ILC_H
#define LSST_M2CELLCPP_CONTROL_ILC_H

namespace LSST {
namespace m2cellcpp {
namespace control {

/// This class contains information about one ILC.
/// FUTURE: Possibly use this as a base class for Axial and Tangential ILC classes.
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

/// Class to hold and access all Ilc instances.
/// Unit test in test_FpgaIo.cpp.
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

}  // namespace control
}  // namespace m2cellcpp
}  // namespace LSST

#endif  // LSST_M2CELLCPP_CONTROL_ILC_H
