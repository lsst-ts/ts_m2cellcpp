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
#include "system/TelemetryMap.h"

namespace LSST {
namespace m2cellcpp {
namespace system {

/// This class is used to manage telemetry communication socket connections.
/// It sends out json descriptions of all items in the `_telemetryMap` on
/// each client connection, sleeps for 0.05 seconds, and then repeats.
/// There is no set limit on the number of client connections.
/// The class listens on a separate thread, and creates a
/// `TelemetryCom::ServerConnectionHandler` for each client connection,
/// which also starts it's own thread.
/// The `TelemetryMap` is expected to be immutable except for element
/// values (element `id` cannot change). The values of the elements
/// must be variable and thread safe. They must be set elsewhere whenever
/// their values change so that the telemetry information is up to date.
/// Unit tests in tests/test_TelemetryCom
class TelemetryCom : public std::enable_shared_from_this<TelemetryCom> {
public:
    using Ptr = std::shared_ptr<TelemetryCom>;

    static const char* TERMINATOR() { return "\r\n"; }

    /// Return a new `TelemetryCom` object using `telemMap` for its data and `port` for its port.
    static Ptr create(TelemetryMap::Ptr const& telemMap, int port) {
        return TelemetryCom::Ptr(new TelemetryCom(telemMap, port));
    }

    TelemetryCom() = delete;

    /// Ensure shutdown is called and thread are joined.
    ~TelemetryCom();

    /// Start the thread to listen for client connections. See `_server`.
    void startServer();

    /// Shutdown the server socket and join all server related threads.
    /// Clients are primarily used for testing and not expected to have
    /// threads that need stopping on the server.
    int shutdownCom();

    /// Start a client connection in this thread with `idNum`. This is primarily meant
    /// for testing.
    int client(int idNum);

    /// Wait up to `seconds` time for `_serverRunning` to be true.
    /// @return true if the server is running.
    bool waitForServerRunning(int seconds);

    /// Get a pointer to the `TelemetryMap` in use by this instance.
    TelemetryMap::Ptr getTMap() const { return _telemetryMap; }

    /// This class is used to handle a unique client connection made to this
    /// `TelemetryCom` server. This handler will send out messages
    /// for all items in the `_telemetryMap`, briefly sleep and then
    /// repeat until `servConnHShutdown` is called.
    class ServerConnectionHandler {
    public:
        using Ptr = std::shared_ptr<ServerConnectionHandler>;

        ServerConnectionHandler() = delete;
        ServerConnectionHandler(ServerConnectionHandler const&) = delete;

        /// Create a new `ServerConnectionHandler` and start its threads to handle `sock`.
        ServerConnectionHandler(int sock, TelemetryMap::Ptr const& tItemMap)
                : _servConnHSock(sock), _tItemMap(tItemMap) {
            std::thread thrdH(&ServerConnectionHandler::_servConnHandler, this);
            _servConnHThrd = std::move(thrdH);
            std::thread thrdR(&ServerConnectionHandler::_servConnReader, this);
            _servConnReadThrd = std::move(thrdR);
        }

        /// Join associated threads.
        ~ServerConnectionHandler() { joinAll(); }

        /// Stop this instance's threads.
        void servConnHShutdown();

        /// If the threads have completed, try joining them.
        bool checkJoinAll();

        /// Returns true if both the handler and read threads have been joined.
        bool getJoinedAll() { return _joinedHandler && _joinedReader; }

        /// Lock `_joinMtx` and join all of our threads.
        void joinAll();

    private:
        /// This function writes all TelemetryItem's over the connection.
        void _servConnHandler();

        /// This function reads json messages from the connection.
        void _servConnReader();

        /// Join `_servConnHThrd`. `_joinMtx` must be locked before calling
        void _joinHandler();

        /// Join `_servConnReadThrd`. `_joinMtx` must be locked before calling
        void _joinReader();

        /// Check if the reader thread is ready to join, and try to join it if it is.
        /// @return - Returns true if the Reader thread has been joined.
        bool _checkJoinReader();

        const int _servConnHSock;  ///< Socket used by this class.

        /// A map of all the telemetry values that need to be sent to the client.
        TelemetryMap::Ptr _tItemMap;
        std::thread _servConnHThrd;     ///< The thread running the handler.
        std::thread _servConnReadThrd;  ///< The thread running the read thread.

        std::atomic<bool> _connLoop{true};  ///< Setting this to false stops the threads.

        std::atomic<bool> _readyToJoinHandler{false};  ///< True when the handler thread has reached its end.
        std::atomic<bool> _joinedHandler{false};       ///< True when the handler thread has joined.
        std::atomic<bool> _readyToJoinReader{false};   ///< True when the reader thread has reached its end.
        std::atomic<bool> _joinedReader{false};        ///< True when the reader thread has joined.
        std::mutex _joinMtx;  ///< protects `_readyToJoinHandler`, `_joinedHandler`, `_readyToJoinReader`, and
                              ///< `_joinedReader`
    };

private:
    /// Private constructor to force creation as shared pointer object.
    TelemetryCom(TelemetryMap::Ptr const& telemMap, int port);

    /// Bind a socket listen to `_port`, creating new `ServerConnectionHandler` objects
    /// to handle clients until `shutdownCom` is called.
    void _server();

    static std::atomic<uint32_t> _seqIdSource;

    /// A unique identifier for each `TelemetryCom` instance.
    unsigned int _seqId = _seqIdSource++;

    /// Return a socket file descriptor to the server.
    int _clientConnect();

    /// A pointer to the `TelemetryMap` instance that is used as the source of
    /// telemetry data to transmit.
    TelemetryMap::Ptr _telemetryMap;

    int _serverFd = -1;                           ///< File descriptor for server socket.
    int _port = -1;                               ///< Port number
    std::atomic<bool> _acceptLoop{true};          ///< Set to false to stop the accept loop.
    std::atomic<bool> _shutdownComCalled{false};  ///< Set to true when `shutdownCom` has been called.
    std::atomic<bool> _serverRunning{false};      ///< Set to true once the server has started.

    std::vector<ServerConnectionHandler::Ptr> _handlerThreads;  ///< List of all heandler threads.
    std::mutex _handlerThreadsMtx;                              ///< Protects `_handlerThreads`

    std::thread _serverThread;  ///< Thread for accepting socket connections.
};

}  // namespace system
}  // namespace m2cellcpp
}  // namespace LSST

#endif  // LSST_M2CELLCPP_SYSTEM_TELEMETRYCOM_H
