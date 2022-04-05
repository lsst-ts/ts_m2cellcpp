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
#ifndef LSST_M2CELLCPP_UTIL_ISSUE_H
#define LSST_M2CELLCPP_UTIL_ISSUE_H

// System headers
#include <exception>
#include <iosfwd>
#include <string>

// evil macro
#define ERR_LOC LSST::m2cellcpp::util::Issue::Context(__FILE__, __LINE__, __func__)

namespace LSST {
namespace m2cellcpp {
namespace util {

/// Base class for other error classes that includes location.
/// This class inherits from standard exception class and adds the facility
/// for tracking of where the exception originated.
/// @see util::Bug for an example;
/// Based on qserv util::Issue
/// unit test test_Bug.cpp
class Issue : public std::exception {
public:
    /// Context defines where exception has happened.
    class Context {
    public:
        // Constructor takes location of the context
        Context(char const* file, int line, char const* func);
        void print(std::ostream& out) const;

    private:
        std::string _file;
        std::string _func;
        int _line;
    };

    /// Constructor takes issue location and a message.
    Issue(Context const& ctx, std::string const& message);

    // Destructor
    ~Issue() throw() override = default;

    /// Implements std::exception::what(), returns message and
    /// context in one string.
    char const* what() const throw() override;

    /// Returns original message (without context).
    std::string const& message() const { return _message; }

private:
    // Data members
    std::string _message;
    std::string _fullMessage;  /// Message string plus context
};

}  // namespace util
}  // namespace m2cellcpp
}  // namespace LSST

#endif  // LSST_M2CELLCPP_UTIL_ISSUE_H
