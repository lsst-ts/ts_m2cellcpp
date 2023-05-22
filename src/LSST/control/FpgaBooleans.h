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

#ifndef LSST_M2CELLCPP_CONTROL_FPGABOOLEANS_H
#define LSST_M2CELLCPP_CONTROL_FPGABOOLEANS_H

// System headers
#include <cstddef>

// Project headers

namespace LSST {
namespace m2cellcpp {
namespace control {

/// Class for storing boolean values meant for the FPGA
/// Current functionality is limited to setting and getting values.
class FpgaBooleans {
public:
    enum { BIT_MIN = 0, BIT_MAX = 7 };

    FpgaBooleans() = default;

    /// Returns the value of the bit at `pos` in `byt`.
    /// @throws range_error if pos it out of range.
    static bool getBit(std::byte byt, int pos);

    /// Sets the value of the bit at `pos` in `byt` to `val`.
    /// @throws range_error if pos it out of range.
    static void setBit(std::byte& byt, int pos, bool val);
};

}  // namespace control
}  // namespace m2cellcpp
}  // namespace LSST

#endif  // LSST_M2CELLCPP_CONTROL_FPGABOOLEANS_H
