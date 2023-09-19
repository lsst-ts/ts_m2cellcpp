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
#include "control/OutputPortBits.h"
#include "control/InputPortBits.h"
#include "util/Log.h"
#include "util/VMutex.h"

#ifndef LSST_M2CELLCPP_CONTROL_FPGAIO_H
#define LSST_M2CELLCPP_CONTROL_FPGAIO_H

namespace LSST {
namespace m2cellcpp {

namespace simulator {
class SimCore;
}

namespace control {

// &&& doc
class FpgaIo {
public:
    using Ptr = std::shared_ptr<FpgaIo>;

    /// Create the global FaultMgr object. If `simCore` is not nullptr,
    /// this is a simulation.
    static void setup(std::shared_ptr<simulator::SimCore> const& simCore);

    /// Return a reference to the global FpgaIo instance.
    /// @throws `ConfigException` if `setup` has not already been called.
    static FpgaIo& get();

    /// Return a shared pointer to the global FpgaIo instance.
    /// @throws `ConfigException` if `setup` has not already been called.
    static Ptr getPtr();

    /// Return `_outputPort`
    OutputPortBits getOutputPort() const { return _outputPort; }

    /// &&& doc
    void writeOutputPortBitPos(int pos, bool set);

private:
    static Ptr _thisPtr; ///< Pointer to the global instance of FpgaIo.
    static std::mutex _thisPtrMtx; ///< Protects `_thisPtr`.

    /// Private constructor to force call to `setup`.
    FpgaIo(std::shared_ptr<simulator::SimCore> const& simCore);
    // &&& constructors etc.


    OutputPortBits _outputPort; ///< Output to be written to the output port.
    InputPortBits _inputPort; ///< Input to be read from to the input port.
    util::VMutex _portMtx; ///< Protects `_outputPort` and `_inputPort`.

    std::shared_ptr<simulator::SimCore> _simCore; ///< simulator
};


}  // namespace control
}  // namespace m2cellcpp
}  // namespace LSST

#endif  // LSST_M2CELLCPP_CONTROL_FPGAIO_H
