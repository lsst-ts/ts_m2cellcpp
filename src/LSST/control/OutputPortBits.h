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

#ifndef LSST_M2CELLCPP_CONTROL_OUTPUTPORTBITS_H
#define LSST_M2CELLCPP_CONTROL_OUTPUTPORTBITS_H

// System headers
#include <functional>
#include <map>
#include <memory>

// Project headers

namespace LSST {
namespace m2cellcpp {
namespace control {

/// Class representation of the output port bits as seen in create_output_port_bits_masks.vi.
/// This class contains enums for the position of all bits for the output port.
/// Unit tests in test_SimCore.cpp.
class OutputPortBits {
public:
    using Ptr = std::shared_ptr<OutputPortBits>;

    /// Create a OutputPortBits with a blank _bitmap
    OutputPortBits() = default;

    /// Create a OutputPortBits with a `_bitmap` set to `bitmap`.
    OutputPortBits(uint8_t bitmap) : _bitmap(bitmap) {}

    OutputPortBits(OutputPortBits const&) = default;
    OutputPortBits& operator=(OutputPortBits const&) = default;
    virtual ~OutputPortBits() = default;

    /// These enums should match those in create_output_port_bits_masks.vi
    /// Changing the values of these enums will likely cause hardware problems.
    enum {
        MOTOR_POWER_ON = 0,        ///< “ILC Motor Power On” = 0
        ILC_COMM_POWER_ON = 1,     ///< “ILC Comm Power On” = 1
        CRIO_INTERLOCK_ENABLE = 2, ///< “cRIO Interlock Enable” = 2
        RESET_MOTOR_BREAKERS = 3,  ///< “Reset Motor Power Breakers” = 3
        RESET_COMM_BREAKERS= 4,    ///< “Reset Comm Power Breakers” = 4
        SPARE_D05 = 5,             ///< “Spare D05” = 5
        SPARE_D06 = 6,             ///< “Spare D06” = 6
        SPARE_D07 = 7,             ///< “Spare D07” = 7
    };

    /// Returns the output port bit mask, which is all bits set. This
    /// is from the LabView code and kept as reference.
    static uint8_t getOuputPortMask() { return ~0; }

    /// Write the bit in `_bitmap` at `pos` to 1 if `set` is true, or 0
    /// if `set` is false.
    /// @throws range_error.
    void writeBit(int pos, bool set);

    /// To set multiple values atomically, `mask` has ones set at positions that
    /// will be set/unset, while `values` has 1's at set positions and 0's elsewhere.
    void writeMask(uint8_t mask, uint8_t values);

    /// Return a copy of `_bitmap`.
    uint8_t getBitmap() { return _bitmap; }

    /// Set `_bitmap` to `bitmap`.
    void setBitmap(uint8_t bitmap) { _bitmap = bitmap; }

    /// Return the string associated with `enumVal`.
    static std::string getEnumString(int enumVal);

    /// Return a string containing the string version of all set bit enums
    std::string getAllSetBitEnums();

    /// Return the bit in `_bitmap` at `pos`, `pos` out of range returns 0.
    bool getBit(int pos);

    /// Return all bits that are set in `_bitmap` and in `mask`.
    uint8_t getBitsSetInMask(uint8_t mask) {
        return _bitmap & mask;
    }

    /// Return all bits that are set in `_bitmap` and not in `mask`.
    uint8_t getBitsSetOutOfMask(uint8_t mask) {
        return _bitmap & ~mask;
    }

    /// Return a binary string representation of `_bitmap`.
    static std::string getBinaryStr(uint8_t);

private:
    uint8_t _bitmap = 0; ///< Bitmap of output port.
};

}  // namespace control
}  // namespace m2cellcpp
}  // namespace LSST


#endif  // LSST_M2CELLCPP_CONTROL_OUTPUTPORTBITS_H
