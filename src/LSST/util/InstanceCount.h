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

#ifndef LSST_M2CELLCPP_UTIL_INSTANCECOUNT_H
#define LSST_M2CELLCPP_UTIL_INSTANCECOUNT_H

// System headers
#include <map>
#include <mutex>
#include <string>

namespace LSST {
namespace m2cellcpp {
namespace util {

/// This a utility class to track the number of instances of any objects
/// or functions.
/// This class is useful for debugging. It can be used to track the
/// number of instances of a class and concurrent function calls. It
/// is very log intensive when being used to track more than a few
/// things at a time, so InstanceCount instantiations should be
/// removed when no longer needed.
///
/// To track the number of instances of a class, add the following
/// to its member variables `InstanceCount inst("<class_name>");`.
/// Where <class_name> is the name of the class.
/// NOTE: If the class being tracked has non-default copy and move
///       constructors, or non-default operator=, then those need to
///       be changed to include the instance of InstanceCounter.
///       Default copy, move , and operator= should be fine.
///
/// Similarly, add `InstanceCount inst("<function_name>");` to
/// track concurrent or recursive calls to a function.
///
/// It can also be used to track mutex contention/deadlock.
/// Before the mutex add `InstanceCount inst(string("<mutex_id>") + "_before");`
/// After mutes add `InstanceCount inst(string("<mutex_id>") + "_after");`
/// This will indacte how many threads are waiting on the mutex compared
/// to how many have passed it. <mutex_id> is an arbitray string to
/// uniquly define the mutex.
/// unit test: test_InstanceCount.cpp
class InstanceCount {
public:
    InstanceCount(std::string const& className);
    InstanceCount(InstanceCount const& other);
    InstanceCount(InstanceCount&& origin);
    ~InstanceCount();

    InstanceCount& operator=(InstanceCount const& o) = default;

    int getCount();  ///< Return the number of instances of _className.

    /// Put the contents of this instance of InstanceCount and the
    /// static elements into `os`. This includes a complete list of all
    /// identifiers being tracked and the number of instances of each.
    virtual std::ostream& dump(std::ostream& os) const;

    /// @return a string representation of this instance of InstanceCount and the
    /// static elements
    /// @see virtual std::ostream& dump(std::ostream& os)
    std::string dump() const;

    /// Put the contents of this instance of InstanceCount and the
    /// static elements into `os`.
    /// @see virtual std::ostream& dump(std::ostream& os)
    friend std::ostream& operator<<(std::ostream& out, InstanceCount const& instanceCount);

private:
    std::string _className;  ///< Names of the of which this is a member.
    static std::map<std::string, int>
            _instances;  ///< Map of instances per class name.
                         /// Protects _instances. recursive_mutex used to avoid issues with
                         /// operator<< in what should only be a debugging tool.
    static std::recursive_mutex _mx;

    /// Increment the count associated with this object.
    /// @param source - indicates the source of the new instance, "con" -constructor,
    ///                 "mov" - move constructor,  "cpy" - copy constructor.
    void _increment(std::string const& source);
};

}  // namespace util
}  // namespace m2cellcpp
}  // namespace LSST

#endif  // LSST_M2CELLCPP_UTIL_INSTANCECOUNT_H
