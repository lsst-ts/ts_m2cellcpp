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
#ifndef LSST_M2CELLCPP_SYSTEM_LOG_H
#define LSST_M2CELLCPP_SYSTEM_LOG_H

// System headers
#include <iostream>
#include <string>

// Project headers

// Third party headers

namespace LSST {
namespace m2cellcpp {
namespace system {

/// A placeholder class to be used until normal logging can be setup.
class Log {
public:
    enum LogLvl {
        DEBUG = 1,
        INFO,
        WARN,
        ERROR
    };

    Log() = default;
    Log(Log const&) = delete;
    Log& operator=(Log const&) = delete;

    ~Log() = default;

    static void log(LogLvl logLvl, std::string msg) {
        std::cout << "lvl=" << logLvl << " " << msg << std::endl;
    }
};


}}} // namespace LSST::m2cellcpp::system

#endif // LSST_M2CELLCPP_SYSTEM_COMCLIENT_H
