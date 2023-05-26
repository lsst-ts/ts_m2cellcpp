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
#ifndef LSST_M2CELLCPP_SYSTEM_TELEMETRYCOM_H
#define LSST_M2CELLCPP_SYSTEM_TELEMETRYCOM_H

// System headers
#include <arpa/inet.h>
#include <atomic>
#include <map>
#include <memory>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <thread>
#include <unistd.h>

// Third party headers

// project headers
#include "system/TelemetryItem.h"

namespace LSST {
namespace m2cellcpp {
namespace system {

///  &&& doc
/// wait between messages is 0.05 seconds.
class TelemetryCom : public std::enable_shared_from_this<TelemetryCom> {
public:
    using Ptr = std::shared_ptr<TelemetryCom>;

    static const char* TERMINATOR() { return "\r\n"; }

    /// &&& doc
    static Ptr create() { return TelemetryCom::Ptr(new TelemetryCom()); }

    /// &&& doc - shutting down threads can be odd.
    ~TelemetryCom();

    /// &&& doc
    void server();

    ///  &&& doc
    int shutdownCom();

    /// &&& doc
    int client(int j);

    /// Wait upto `seconds` time for `_serverRunning` to be true.
    /// @return true if the server is running.
    bool waitForServerRunning(int seconds);

    //&&& static bool test();  // &&& move code to test_TelemetryCom.cpp

private:
    TelemetryCom();

    static std::atomic<uint32_t> _seqIdSource;

    /// &&& doc
    int _seqId = _seqIdSource++;

    /// &&& doc
    void _serverConnectionHandler(int sock);

    void _serverConnectionHandlerOld(int sock);  //&&&

    /// Return a socket file descriptor to the server.
    int _clientConnect();

    int _serverFd;
    int _port = 8080;                             // Port number //&&& set elsewhere
    std::atomic<bool> _loop{true};                ///< &&& doc
    std::atomic<bool> _shutdownComCalled{false};  ///< &&& doc
    std::atomic<bool> _serverRunning{false};      ///< &&& doc replace with bool and mtx.

    TelemetryMap::Ptr _telemMap;  ///< &&& doc
};

}  // namespace system
}  // namespace m2cellcpp
}  // namespace LSST

#endif  // LSST_M2CELLCPP_SYSTEM_TELEMETRYCOM_H
