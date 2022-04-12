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
#ifndef LSST_M2CELLCPP_SYSTEM_COMCONTROL_H
#define LSST_M2CELLCPP_SYSTEM_COMCONTROL_H

// System headers

// Project headers
#include "system/ComConnection.h"

namespace LSST {
namespace m2cellcpp {
namespace system {

/// &&& doc
class ComControl : public ComConnection {
public:
    using Ptr = std::shared_ptr<ComControl>;

    /// Factory method used to prevent issues with enable_shared_from_this.
    /// @param ioContext asio object for the network I/O operations
    static Ptr create(IoContextPtr const& ioContext, uint64_t connId,
                      std::shared_ptr<ComServer> const& server);

private:
    /// @see ComControl::create()
    ComControl(IoContextPtr const& ioContext, uint64_t connId, std::shared_ptr<ComServer> const& server);
};

}  // namespace system
}  // namespace m2cellcpp
}  // namespace LSST

#endif  // LSST_M2CELLCPP_SYSTEM_COMCONTROL_H