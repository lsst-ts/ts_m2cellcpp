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

#ifndef LSST_M2CELLCPP_CONTROL_CONTEXT_H
#define LSST_M2CELLCPP_CONTROL_CONTEXT_H

// System headers
#include <memory>
#include <mutex>


// Project headers
#include "state/Model.h"


namespace LSST {
namespace m2cellcpp {
namespace control {

/// This class contains majority of the system's data, including the Model.
/// unit test: test_Context.cpp
class Context {
public:
    using Ptr = std::shared_ptr<Context>;

    Context(Context const&) = delete;
    Context& operator=(Context const&) = delete;

    /// Create the global Context object
    /// Config needs to exist before this is called.
    static void setup();

    /// Get a copy of the global context pointer.
    static Ptr get();

    virtual ~Context() = default;

    /// Reset the global Ptr to the Context.
    /// This should only be called at termination or unit testing.
    void reset() { _thisPtr.reset(); }

    /// The Model, cRIO control.
    state::Model model;

private:
    /// Context requires the Config is setup first.
    Context() = default;

    static Ptr _thisPtr;         ///< Pointer to the global instance of Context
    static std::mutex _thisMtx;  ///< Protects _thisPtr.
};

}  // namespace control
}  // namespace m2cellcpp
}  // namespace LSST

#endif  // LSST_M2CELLCPP_CONTROL_CONTEXT_H
