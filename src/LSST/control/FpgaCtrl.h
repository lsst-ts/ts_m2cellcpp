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

#ifndef LSST_M2CELLCPP_CONTROL_FPGACTRL_H
#define LSST_M2CELLCPP_CONTROL_FPGACTRL_H

// System headers
#include <memory>

// Project headers
#include "control/FpgaIo.h"

namespace LSST {
namespace m2cellcpp {
namespace control {

/// This class is responsible for controlling and communicating with the FPGA.
/// &&& more doc.
class FpgaCtrl {
public:
    using Ptr = std::shared_ptr<FpgaCtrl>;
    // constructors &&&;

    /// &&& doc
    void fCtrlStart();

private:

    /// This is the function that is run by the `_fCtrlThread`.
    void _fCtrlAction() {
        // FUTURE: What this does will likely be a result of how we communicate with the FPGA
        //         It may be useful for simulation.
    }

    FpgaIo::Ptr _fpgaIo; ///< The FpgaIo instance.

    std::thread _fCtrlThread; ///< contains _fCtrlAction

    // &&&I     (See CellCommunications.vi, which appears to be a major control loop that seems to do a LOT more than its name would imply ???)
    bool _feedDorward{true}; ///< &&&I     - FeedForward = true
    bool _feedBack{true}; ///< &&&I     - FeedBack = true
    bool _axialDeadband{false}; ///< &&&I     - AxialDeadband = false
    bool _outputEnabled{false}; ///< &&&I     - OutputEnabled = false
    bool _inPosition{false}; ///< &&&I     - In Position = false
    bool _tuningLogOff{false}; ///< &&&I     - Tuning Log (off) = false  Note: awful name
    bool _beginSoftStart{false}; ///< &&&I     - Begin Soft Start = false
    bool _clearReset{true}; ///< &&&I     - Clear, Reset = true Note: awful name

};

}  // namespace control
}  // namespace m2cellcpp
}  // namespace LSST

#endif  // LSST_M2CELLCPP_CONTROL_FPGACTRL_H
