// -*- LSST-C++ -*-
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

#ifndef LSST_M2CELLCPP_UTIL_BUG_H
#define LSST_M2CELLCPP_UTIL_BUG_H

// Project headers
#include "util/Issue.h"

namespace LSST {
namespace m2cellcpp {
namespace util {

/// Bug is a generic Qserv exception that indicates a probable bug or fatal issue.
/// This class is not intended to be caught. It's an indication that something
/// unexpected and/or dangerous has happened that needs to be fixed.
/// Based on qserv util::Bug
class Bug : public util::Issue {
public:
    explicit Bug(util::Issue::Context const& ctx, std::string const& msg);
};

}  // namespace util
}  // namespace m2cellcpp
}  // namespace LSST

#endif  // LSST_M2CELLCPP_UTIL_BUG_H
