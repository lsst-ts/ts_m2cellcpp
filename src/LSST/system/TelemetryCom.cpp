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
//&&& using json = nlohmann::json;

namespace LSST {
namespace m2cellcpp {
namespace system {

bool TelemetryCom::test() {
    {
        LINFO("Creating serv + client");
        auto serv = TelemetryCom::create();
        auto client = TelemetryCom::create();

        auto servFunc = [serv]() {
            LINFO("&&& servFunc start");
            serv->server();
            LINFO("&&& servFunc end");
        };
        LINFO("&&& Running serv");
        thread servThrd(servFunc);
        serv->waitForServerRunning(3);
        // &&& wait and verify server started.

        LINFO("&&& Running clients");
        std::vector<thread> clientThreads;
        auto clientFunc = [](TelemetryCom::Ptr client, int j) {
            LINFO("&&& clientFunc start j=", j);
            client->client();
            LINFO("&&& clientFunc end j=", j);
        };

        for (int j = 0; j < 10; ++j) {
            clientThreads.emplace_back(clientFunc, client, j);
        }
        sleep(5);  //&&& replace with wait for client complete flag
        LINFO("&&& Stopping server");
        serv->shutdownCom();
        client->shutdownCom();
        LINFO("&&& waiting for joins");
        //&&& clientThrd1.join();
        for (auto& thrd : clientThreads) {
            thrd.join();
        }
        LINFO("&&& client joined");
        servThrd.join();
        LINFO("&&& serv joined");
    }
    return true;
}

TelemetryCom::~TelemetryCom() {
    shutdownCom();
    // &&& join threads, if needed.
}

int TelemetryCom::shutdownCom() {
    LDEBUG("TelemetryCom::shutdownCom()");
    _loop = false;
    if (_shutdownComCalled.exchange(true)) {
        return 0;
    }
    if (_serverRunning) {  // &&& need mutex
        LINFO("TelemetryCom::shutdownCom() connecting to serversocket");
        int clientFd = _clientConnect();

        // closing the connected socket
        close(clientFd);
    }
    LINFO("TelemetryCom::shutdownCom() end &&&");
    return 0;
}

int TelemetryCom::_clientConnect() {
    LINFO("&&& client a");
    int clientFd = socket(AF_INET, SOCK_STREAM, 0);
    if (clientFd < 0) {
        LERROR("TelemetryCom::_clientConnect() socket failed clientFd=", clientFd);
        return -1;
    }

    struct sockaddr_in servAddr;
    servAddr.sin_family = AF_INET;
    servAddr.sin_port = htons(_port);

    string hostId = "127.0.0.1";  //&&& need to replace "127.0.0.1"
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
    sleep(seconds);  // actually wait &&&;
    return true;
}

void TelemetryCom::server() {
    // Listen for new connections to accept
    LINFO("&&& server a");
    if ((_serverFd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        LINFO("&&& server a1");
        throw util::Bug(ERR_LOC,
                        "TelemetryCom::sever() failed to create listening socket");  //&&& change type
    }
    LINFO("&&& server b");
    int opt = 1;
    if (setsockopt(_serverFd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
        LINFO("&&& server b1");
        throw util::Bug(ERR_LOC, "TelemetryCom::sever() failed to setsockopt");  //&&& change type
    }

    struct sockaddr_in address;
    int addrlen = sizeof(address);
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(_port);

    LINFO("&&& server c");
    if (bind(_serverFd, (struct sockaddr*)&address, sizeof(address)) < 0) {
        throw util::Bug(ERR_LOC,
                        "TelemetryCom::sever() failed to bind " + to_string(_port));  //&&& change type
    }
    LINFO("&&& server d");
    if (listen(_serverFd, 3) < 0) {
        LINFO("&&& server d1");
        throw util::Bug(ERR_LOC, "TelemetryCom::sever() failed to listen " + to_string(_port));
    }
    _serverRunning = true;  // &&& needs unique_lock to be thread safe
    LINFO("&&& server e");
    while (_loop) {
        LINFO("&&& server f");
        int sock = accept(_serverFd, (struct sockaddr*)&address, (socklen_t*)&addrlen);
        LINFO("&&& server f1");
        if (!_loop) {
            continue;
        }
        if (sock < 0) {
            LERROR("TelemetryCom::sever() failed to accept on ", _port, " sock=", sock);
            _loop = false;
            continue;
        }
        // &&& move to separate thread.
        _serverConnectionHandler(sock);
        /* &&&
        char buffer[1024] = {0};
        const char* hello = "Hello from server";
        int valread = read(sock, buffer, 1024);
        LINFO("&&& server f2");
        printf("%s\n  %d\n", buffer, valread);
        send(sock, hello, strlen(hello), 0);
        LINFO("&&& server f3");
        printf("server sent message\n"); //&&&
        close(sock);
        LINFO("&&& server f4");
        */
    }

    LINFO("&&& server shutdown");
    // closing the listening socket
    shutdown(_serverFd, SHUT_RDWR);
    LINFO("&&& server shutdown done");
}

void TelemetryCom::_serverConnectionHandler(int sock) {
    char buffer[1024] = {0};
    const char* hello = "Hello from server";
    int valread = read(sock, buffer, 1024);
    LINFO("&&& server f2");
    printf("%s\n  %d\n", buffer, valread);
    send(sock, hello, strlen(hello), 0);
    LINFO("&&& server f3");
    printf("server sent message\n");  //&&&
    close(sock);
    LINFO("&&& server f4");
}

int TelemetryCom::client() {
    const char* hello = "Hello from client";
    char buffer[1024] = {0};
    int client_fd = _clientConnect();
    LINFO("&&& client d");
    send(client_fd, hello, strlen(hello), 0);
    printf("Hello message sent\n");
    int valread = read(client_fd, buffer, 1024);
    printf("%s\n", buffer);

    // closing the connected socket
    LINFO("&&& client e");
    close(client_fd);
    LINFO("&&& client end");
    return 0;
}

}  // namespace system
}  // namespace m2cellcpp
}  // namespace LSST
