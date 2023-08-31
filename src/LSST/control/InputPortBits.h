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

#ifndef LSST_M2CELLCPP_CONTROL_INPUTPORTBITS_H
#define LSST_M2CELLCPP_CONTROL_INPUTPORTBITS_H

// System headers
#include <functional>
#include <map>
#include <memory>

// Project headers

namespace LSST {
namespace m2cellcpp {
namespace control {

/// Class representation of the Input port bits as seen in create_Input_port_bits_masks.vi.
/// Unit tests in test_FpgaIo.cpp. // need unit tests &&&
class InputPortBits {
public:
    using Ptr = std::shared_ptr<InputPortBits>;

    /// Create a InputPortBits with a blank _bitmap
    InputPortBits() = default;

    /// Create a InputPortBits with a `_bitmap` set to `bitmap`.
    InputPortBits(uint32_t bitmap) : _bitmap(bitmap) {}

    InputPortBits(InputPortBits const&) = default;
    InputPortBits& operator=(InputPortBits const&) = default;
    virtual ~InputPortBits() = default;

    /// These enums should match those in create_input_port_bits_masks.vi
    /// Changing the values of these enums will likely cause hardware problems.
    enum {
        REDUNDANCY_OK = 0,              ///< “Redundancy OK” = 0
        LOAD_DISTRIBUTION_OK = 1,       ///< “Load Distribution OK” = 1
        POWER_SUPPLY_2_DC_OK = 2,       ///< “Power Suppy #2 DC OK” = 2  <sic>
        POWER_SUPPLY_1_DC_OK = 3,       ///< “Power Supply #1 DC OK” = 3
        POWER_SUPPLY_2_CURRENT_OK = 4,  ///< “Power Supply #2 Current OK” = 4
        POWER_SUPPLY_1_CURRENT_OK = 5,  ///< “Power Supply #1 Current OK” = 5
        J1_W9_1_MTR_PWR_BRKR_OK = 6,    ///< “J1-W9-1-MtrPwrBrkr OK” = 6
        J1_W9_2_MTR_PWR_BRKR_OK = 7,    ///< “J1-W9-2-MtrPwrBrkr OK” = 7
        J1_W9_3_MTR_PWR_BRKR_OK = 8,    ///< “J1-W9-3-MtrPwrBrkr OK” = 8
        J2_W10_1_MTR_PWR_BRKR_OK = 9,   ///< “J2-W10-1-MtrPwrBrkr OK” = 9
        J2_W10_2_MTR_PWR_BRKR_OK = 10,  ///< “J2-W10-2-MtrPwrBrkr OK” = 10
        J2_W10_3_MTR_PWR_BRKR_OK = 11,  ///< “J2-W10-3-MtrPwrBrkr OK” = 11
        J3_W11_1_MTR_PWR_BRKR_OK = 12,  ///< “J3-W11-1-MtrPwrBrkr OK” = 12
        J3_W11_2_MTR_PWR_BRKR_OK = 13,  ///< “J3-W11-2-MtrPwrBrkr OK” = 13
        J3_W11_3_MTR_PWR_BRKR_OK = 14,  ///< “J3-W11-3-MtrPwrBrkr OK” = 14
        J1_W12_1_COMM_PWR_BRKR_OK = 15, ///< “J1-W12-1-CommPwrBrkr OK” = 15
        SPARE_D16 = 16,                 ///< “Spare_D16” = 16
        SPARE_D17 = 17,                 ///< “Spare_D16” = 16
        SPARE_D18 = 18,                 ///< “Spare_D16” = 16
        SPARE_D19 = 19,                 ///< “Spare_D16” = 16
        SPARE_D20 = 20,                 ///< “Spare_D16” = 16
        SPARE_D21 = 21,                 ///< “Spare_D16” = 16
        SPARE_D22 = 22,                 ///< “Spare_D16” = 16
        SPARE_D23 = 23,                 ///< “Spare_D23” = 23
        J1_W12_2_COMM_PWR_BRKR_OK = 24, ///< “J1-W12-2-CommPwrBrkr OK” = 24
        J2_W13_1_COMM_PWR_BRKR_OK = 25, ///< “J2-W13-1-CommPwrBrkr OK” = 25
        J2_W13_2_COMM_PWR_BRKR_OK = 26, ///< “J2-W13-2-CommPwrBrkr OK” = 26
        J3_W14_1_COMM_PWR_BRKR_OK = 27, ///< “J3-W14-1-CommPwrBrkr OK” = 27
        J3_W14_2_COMM_PWR_BRKR_OK = 28, ///< “J3-W14-2-CommPwrBrkr OK” = 28
        SPARE_D29 = 29,                 ///< “Spare_D29” = 29
        SPARE_D30 = 30,                 ///< “Spare_D30” = 30
        INTERLOCK_POWER_RELAY = 31, ///< “Interlock Power Relay On” = 31
    };

    /// Returns the input port bit mask, which, in the vi, is all bits.
    static uint32_t getInputPortMask() { return ~0; }

    /// Set the bit in `_bitmap` at `pos`.
    /// @throws range_error.
    void setBit(int pos);

    /// Unset the bit in `_bitmap` at `pos`.
    /// @throws range_error.
    void unsetBit(int pos);

    /// Return a copy of `_bitmap`.
    uint32_t getBitmap() { return _bitmap; }

    /// Set `_bitmap` to `bitmap`.
    void setBitmap(uint32_t bitmap) { _bitmap = bitmap; }

    /// Return the string associated with `enumVal`.
    static std::string getEnumString(int enumVal);

    /// Return a string containing the string version of all set bit enums
    std::string getAllSetBitEnums();

    /// Return the bit in `_bitmap` at `pos`, `pos` out of range returns 0.
    uint32_t getBit(int pos);

    /// Return all bits that are set in `_bitmap` and in `mask`.
    uint32_t getBitsSetInMask(uint32_t mask) {
        return _bitmap & mask;
    }

    /// Return all bits that are set in `_bitmap` and not in `mask`.
    uint32_t getBitsSetOutOfMask(uint32_t mask) {
        return _bitmap & ~mask;
    }

    /// Return a binary string representation of `_bitmap`.
    static std::string getBinaryStr(uint32_t);

private:
    uint32_t _bitmap = 0; ///< Bitmap of input port.
};

}  // namespace control
}  // namespace m2cellcpp
}  // namespace LSST

/* &&&
create_input_port_bits_masks.vi
Input Port Bit Masks
“Redundancy OK” = 0
“Load Distribution OK” = 1
“Power Suppy #2 DC OK” = 2  <sic>
“Power Supply #1 DC OK” = 3
“Power Supply #2 Current OK” = 4
“Power Supply #1 Current OK” = 5
“J1-W9-1-MtrPwrBrkr OK” = 6
“J1-W9-2-MtrPwrBrkr OK” = 7
“J1-W9-3-MtrPwrBrkr OK” = 8
“J2-W10-1-MtrPwrBrkr OK” = 9
“J2-W10-2-MtrPwrBrkr OK” = 10
“J2-W10-3-MtrPwrBrkr OK” = 11
“J3-W11-1-MtrPwrBrkr OK” = 12
“J3-W11-2-MtrPwrBrkr OK” = 13
“J3-W11-3-MtrPwrBrkr OK” = 14
“J1-W12-1-CommPwrBrkr OK” = 15
“Spare_D16” = 16
…
“Spare_D23” = 23
“J1-W12-2-CommPwrBrkr OK” = 24
“J2-W13-1-CommPwrBrkr OK” = 25
“J2-W13-2-CommPwrBrkr OK” = 26
“J3-W14-1-CommPwrBrkr OK” = 27
“J3-W14-2-CommPwrBrkr OK” = 28
“Spare_D29” = 29
“Spare_D30” = 30
“Interlock Power Relay On” = 31


create_output_port_bits_masks.vi
“ILC Motor Power On” = 0
“ILC Comm Power On” = 1
“cRIO Interlock Enable” = 2
“Reset Motor Power Breakers” = 3
“Reset Comm Power Breakers” = 4
“Spare D05” = 5
“Spare D06” = 6
“Spare D07” = 7
&&& */

#endif  // LSST_M2CELLCPP_CONTROL_INPUTPORTBITS_H