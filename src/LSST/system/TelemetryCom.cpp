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

// Class header
#include "system/TelemetryCom.h"

// System headers

// Third party headers

// Project headers
#include "util/Bug.h"
#include "util/Log.h"

using namespace std;
using json = nlohmann::json;

namespace LSST {
namespace m2cellcpp {
namespace system {

atomic<uint32_t> TelemetryCom::_seqIdSource{0};

TelemetryCom::TelemetryCom(TelemetryMap::Ptr const& telemMap, int port)
        : _telemetryMap(telemMap), _port(port) {
    LDEBUG("TelemetryCom::TelemetryCom() _seqId=", _seqId);
}

TelemetryCom::~TelemetryCom() {
    LDEBUG("TelemetryCom::~TelemetryCom() _seqId=", _seqId);
    shutdownCom();

    // Shutdown and join client threads
    lock_guard<mutex> htLock();
    for (auto&& ht : _handlerThreads) {
        ht->servConnHShutdown();
        ht->join();
    }
}

int TelemetryCom::shutdownCom() {
    LDEBUG("TelemetryCom::shutdownCom()");
    _acceptLoop = false;
    if (_shutdownComCalled.exchange(true)) {
        return 0;
    }
    if (_serverRunning) {
        LINFO("TelemetryCom::shutdownCom() connecting to serversocket");
        int clientFd = _clientConnect();

        // closing the connected socket
        if (clientFd >= 0) {
            close(clientFd);
        }
        LINFO("TelemetryCom::shutdownCom() joining serverThread");
        _serverThread.join();
    }
    for (auto& hThrd : _handlerThreads) {
        hThrd->servConnHShutdown();
    }
    return 0;
}

int TelemetryCom::_clientConnect() {
    int clientFd = socket(AF_INET, SOCK_STREAM, 0);
    if (clientFd < 0) {
        LERROR("TelemetryCom::_clientConnect() socket failed clientFd=", clientFd);
        return -1;
    }

    struct sockaddr_in servAddr;
    servAddr.sin_family = AF_INET;
    servAddr.sin_port = htons(_port);

    string hostId = "127.0.0.1";  // at this time, only ever need to connect to the local host.
    int inetRes = inet_pton(AF_INET, hostId.c_str(), &servAddr.sin_addr);
    if (inetRes <= 0) {
        return -1;
    }

    int status = connect(clientFd, (struct sockaddr*)&servAddr, sizeof(servAddr));
    if (status < 0) {
        LERROR("TelemetryCom::_clientConnect() connect failed host=", hostId, " port=", _port,
               " status=", status);
        return -1;
    }
    return clientFd;
}

bool TelemetryCom::waitForServerRunning(int seconds) {
    for (int j = 0; j < seconds && !_serverRunning; ++j) {
        sleep(1);
    }
    return _serverRunning;
}

void TelemetryCom::startServer() {
    thread servThrd(&TelemetryCom::_server, this);
    _serverThread = std::move(servThrd);
}

void TelemetryCom::_server() {
    if (_shutdownComCalled) {
        LERROR("TelemetryCom::_server() attempt to start after shutdown has been called");
        return;
    }

    // Listen for new connections to accept
    if ((_serverFd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        throw util::Bug(ERR_LOC, "TelemetryCom::server() failed to create listening socket");
    }
    int opt = 1;
    if (setsockopt(_serverFd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
        throw util::Bug(ERR_LOC, "TelemetryCom::server() failed to setsockopt");
    }

    struct sockaddr_in address;
    int addrlen = sizeof(address);
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(_port);

    if (bind(_serverFd, (struct sockaddr*)&address, sizeof(address)) < 0) {
        throw util::Bug(ERR_LOC, "TelemetryCom::server() failed to bind " + to_string(_port));
    }
    if (listen(_serverFd, 3) < 0) {
        throw util::Bug(ERR_LOC, "TelemetryCom::server() failed to listen " + to_string(_port));
    }
    LINFO("TelemetryCom::server() listening");

    _serverRunning = true;
    while (_acceptLoop) {
        int sock = accept(_serverFd, (struct sockaddr*)&address, (socklen_t*)&addrlen);
        if (!_acceptLoop) {
            continue;
        }
        if (sock < 0) {
            LERROR("TelemetryCom::server() failed to accept on ", _port, " sock=", sock);
            _acceptLoop = false;
            continue;
        }
        // Create an object to handle the new connection.
        LINFO("TelemetryCom::server() accepting new client");
        {
            auto handlerThrd =
                    ServerConnectionHandler::Ptr(new ServerConnectionHandler(sock, _telemetryMap->copyMap()));
            lock_guard<mutex> htLock();
            _handlerThreads.push_back(handlerThrd);
            // Check if any of the threads should be joined and removed.
            auto iter = _handlerThreads.begin();
            while (iter != _handlerThreads.end()) {
                auto oldIter = iter++;
                auto needToErase = (*oldIter)->checkJoin();
                if (needToErase || (*oldIter)->getJoined()) {
                    LINFO("TelemetryCom::server() removing old client");
                    _handlerThreads.erase(oldIter);
                }
            }

        }
    }

    LINFO("TelemetryCom::server() shuting down");
    // closing the listening socket
    shutdown(_serverFd, SHUT_RDWR);
    LINFO("TelemetryCom::server() done");
}

void TelemetryCom::ServerConnectionHandler::_servConnHandler() {
    LDEBUG("TelemetryCom::ServerConnectionHandler::_servConnHandler starting sock=", _servConnHSock);
    while (_connLoop) {
        for (auto const& elem : _tItemMap) {
            auto js = elem.second->getJson();
            string msg = to_string(js) + TelemetryCom::TERMINATOR();
            LTRACE("server sending msg=", msg);
            ssize_t status = send(_servConnHSock, msg.c_str(), msg.length(), 0);
            if (status < 0) {
                LWARN("TelemetryCom::ServerConnectionHandler::_servConnHandler failure status=", status);
                break;
            }
        }
        this_thread::sleep_for(50ms);
    }
    LDEBUG("TelemetryCom::ServerConnectionHandler::_servConnHandler close sock=", _servConnHSock);
    close(_servConnHSock);
    {
        lock_guard<mutex> lck(_joinMtx);
        _readyToJoin = true;
    }
    LINFO("TelemetryCom::ServerConnectionHandler::_servConnHandler done sock=", _servConnHSock);
}

bool TelemetryCom::ServerConnectionHandler::checkJoin() {
    lock_guard<mutex> lck(_joinMtx);
    if (_joined) {
        LDEBUG("TelemetryCom::ServerConnectionHandler::checkJoin already joined");
        return true;
    }
    if (!_servConnHThrd.joinable() || !_readyToJoin) {
        LDEBUG("TelemetryCom::ServerConnectionHandler::checkJoin not ready to join");
        return false;
    }
    LINFO("TelemetryCom::ServerConnectionHandler::checkJoin joining");
    // The join should happen quickly as the thread should be finished, or very nearly so.
    _join();
    LINFO("TelemetryCom::ServerConnectionHandler::checkJoin joined");
    return true;
}

void TelemetryCom::ServerConnectionHandler::join() {
    lock_guard<mutex> lck(_joinMtx);
    _join();
}

void TelemetryCom::ServerConnectionHandler::_join() {
    if (_joined) {
        return;
    }
    _servConnHThrd.join();
    _joined = true;
}

int TelemetryCom::client(int idNum) {
    int clientFd = _clientConnect();
    LINFO("TelemetryCom::client() start clientFd=", clientFd, " idNum=", idNum, "_seqId=", _seqId);
    bool serverOk = true;
    string inMsg;
    while (_acceptLoop && serverOk) {
        char buffer[3];
        // Read one byte at a time to check every byte for terminator.
        ssize_t status = read(clientFd, buffer, 1);
        if (status <= 0) {
            LINFO("TelemetryCom::client() idNum=", idNum, " recv failed with status=", status);
            serverOk = false;
            break;
        }
        char inChar = buffer[0];
        if (inChar == '\n' && inMsg.back() == '\r') {
            inMsg.pop_back();
            LDEBUG("client idNUm=", idNum, " seq=", _seqId, " fd=", clientFd, " got message ", inMsg);
            _telemetryMap->setItemFromJsonStr(inMsg);
            inMsg.clear();
        } else {
            inMsg += inChar;
        }
    }
    LINFO("TelemetryCom::client() closing jidNum=", idNum, " seq=", _seqId, " inMsg=", inMsg);
    close(clientFd);
    return 0;
}

}  // namespace system
}  // namespace m2cellcpp
}  // namespace LSST
