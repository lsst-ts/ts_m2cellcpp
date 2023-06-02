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
#include <mutex>
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
    static Ptr create(TelemetryMap::Ptr const& telemMap) {
        return TelemetryCom::Ptr(new TelemetryCom(telemMap));
    }

    TelemetryCom() = delete;

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

    /// &&& doc
    TelemetryMap::Ptr getTMap() const { return _telemetryMap; }

    /// &&& doc
    class ServerConnectionHandler {
    public:
        using Ptr = std::shared_ptr<ServerConnectionHandler>;

        ServerConnectionHandler() = delete;
        ServerConnectionHandler(ServerConnectionHandler const&) = delete;

        /// &&& doc
        ServerConnectionHandler(int sock, TelemetryItemMap tItemMap, bool detach)
                : _servConnHSock(sock), _tItemMap(tItemMap), _detach(detach) {
            std::thread thrd(&ServerConnectionHandler::_servConnHandler, this);
            _servConnHThrd = std::move(thrd);
            if (_detach) {
                _servConnHThrd.detach();
            }
        }

        /// &&& doc
        ~ServerConnectionHandler() {
            if (!_detach) {
                std::lock_guard<std::mutex> lck(_joinMtx);
                _join();
            }
        }

        /// &&& doc
        void servConnHShutdown() { _connLoop = false; };

        /// If the thread has completed, try to join it
        bool checkJoin();

        /// Returns true if the thread has been joined.
        bool getJoined() { return _joined; }

        /// Lock `_joinMtx` and call _`join()`.
        void join();

    private:
        /// &&& doc
        void _servConnHandler();

        /// Join `_servConnHThrd`. `_joinMtx` shopuld be locked before calling
        void _join();

        const int _servConnHSock;  ///< &&& doc

        /// &&& doc
        TelemetryItemMap _tItemMap;  /// &&& change to TelemetryItemMap::Ptr
        const bool _detach;          ///< &&& doc
        std::thread _servConnHThrd;  ///< &&& doc

        std::atomic<bool> _connLoop{true};  ///< &&& doc

        std::atomic<bool> _readyToJoin{false};  ///< &&& doc
        std::atomic<bool> _joined{false};       ///< &&& doc
        std::mutex _joinMtx;                    ///< protects `_readyToJoin` and `_joined`;
    };

private:
    /// &&& doc
    TelemetryCom(TelemetryMap::Ptr const& telemMap);

    static std::atomic<uint32_t> _seqIdSource;

    /// &&& doc
    int _seqId = _seqIdSource++;

    /// &&& doc
    void _serverConnectionHandler(int sock);

    /// Return a socket file descriptor to the server.
    int _clientConnect();

    int _serverFd;
    int _port = 8080;                             // Port number //&&& set elsewhere
    std::atomic<bool> _acceptLoop{true};          ///< &&& doc
    std::atomic<bool> _shutdownComCalled{false};  ///< &&& doc
    std::atomic<bool> _serverRunning{false};      ///< &&& doc replace with bool and mtx.

    TelemetryMap::Ptr _telemetryMap;  ///< &&& doc

    std::vector<ServerConnectionHandler::Ptr> _handlerThreads;  ///< &&& doc
    std::mutex _handlerThreadsMtx;                              ///< protects _handlerThreads
};

}  // namespace system
}  // namespace m2cellcpp
}  // namespace LSST

#endif  // LSST_M2CELLCPP_SYSTEM_TELEMETRYCOM_H
