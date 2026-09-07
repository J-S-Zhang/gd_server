#include "network/websocket_server.h"
#include "network/connection_manager.h"
#include "utils/logger.h"
#include "utils/sha1.h"

#include <atomic>
#include <cstring>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

#ifdef _WIN32
    #ifndef WIN32_LEAN_AND_MEAN
    #define WIN32_LEAN_AND_MEAN
    #endif
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #pragma comment(lib, "ws2_32.lib")
    using SocketHandle = SOCKET;
    const SocketHandle kInvalidSocket = INVALID_SOCKET;
#else
    #include <arpa/inet.h>
    #include <netinet/in.h>
    #include <sys/select.h>
    #include <sys/socket.h>
    #include <unistd.h>
    using SocketHandle = int;
    const SocketHandle kInvalidSocket = -1;
#endif

namespace guandan {

static ConnectionManager g_connections;

static bool initNetwork() {
#ifdef _WIN32
    WSADATA wsa;
    return WSAStartup(MAKEWORD(2, 2), &wsa) == 0;
#else
    return true;
#endif
}

static void cleanupNetwork() {
#ifdef _WIN32
    WSACleanup();
#endif
}

static void closeSocket(SocketHandle sock) {
#ifdef _WIN32
    closesocket(sock);
#else
    close(sock);
#endif
}

static bool recvLine(SocketHandle sock, std::string& line) {
    char c;
    line.clear();
    while (true) {
        int n = recv(sock, &c, 1, 0);
        if (n <= 0) return false;
        line += c;
        if (line.size() >= 2 && line[line.size()-2] == '\r' && line[line.size()-1] == '\n') {
            line.resize(line.size() - 2);
            return true;
        }
    }
}

static std::string computeAcceptKey(const std::string& clientKey) {
    static const char* magic = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
    std::string combined = clientKey + magic;
    std::string hash = sha1Hex(combined);
    std::vector<uint8_t> bytes(hash.begin(), hash.end());
    return base64Encode(bytes);
}

static bool handleWebSocketHandshake(SocketHandle sock) {
    std::string requestLine;
    if (!recvLine(sock, requestLine)) return false;

    std::string wsKey;
    std::string line;
    while (recvLine(sock, line) && !line.empty()) {
        if (line.find("Sec-WebSocket-Key:") == 0) {
            wsKey = line.substr(19);
            while (!wsKey.empty() && wsKey[0] == ' ') wsKey.erase(0, 1);
        }
    }

    std::string acceptKey = computeAcceptKey(wsKey);
    std::ostringstream response;
    response << "HTTP/1.1 101 Switching Protocols\r\n";
    response << "Upgrade: websocket\r\n";
    response << "Connection: Upgrade\r\n";
    response << "Sec-WebSocket-Accept: " << acceptKey << "\r\n";
    response << "\r\n";
    std::string resp = response.str();
    return ::send(sock, resp.c_str(), static_cast<int>(resp.size()), 0) > 0;
}

static bool readWebSocketMessage(SocketHandle sock, std::string& message) {
    uint8_t header[2];
    int n = recv(sock, reinterpret_cast<char*>(header), 2, 0);
    if (n != 2) return false;

    bool masked = (header[1] & 0x80) != 0;
    uint64_t payloadLen = header[1] & 0x7F;

    if (payloadLen == 126) {
        uint8_t ext[2];
        if (recv(sock, reinterpret_cast<char*>(ext), 2, 0) != 2) return false;
        payloadLen = (ext[0] << 8) | ext[1];
    } else if (payloadLen == 127) {
        uint8_t ext[8];
        if (recv(sock, reinterpret_cast<char*>(ext), 8, 0) != 8) return false;
        payloadLen = 0;
        for (int i = 0; i < 8; ++i) payloadLen = (payloadLen << 8) | ext[i];
    }

    uint8_t mask[4] = {0};
    if (masked) {
        if (recv(sock, reinterpret_cast<char*>(mask), 4, 0) != 4) return false;
    }

    message.resize(payloadLen);
    uint64_t received = 0;
    while (received < payloadLen) {
        int r = recv(sock, &message[received], static_cast<int>(payloadLen - received), 0);
        if (r <= 0) return false;
        received += r;
    }

    if (masked) {
        for (uint64_t i = 0; i < payloadLen; ++i) {
            message[i] ^= mask[i % 4];
        }
    }
    return true;
}

WebSocketServer::WebSocketServer(int port) : port_(port) {}

WebSocketServer::~WebSocketServer() {
    stop();
}

void WebSocketServer::setMessageHandler(MessageHandler handler) {
    messageHandler_ = std::move(handler);
}

void WebSocketServer::setConnectHandler(ConnectHandler handler) {
    connectHandler_ = std::move(handler);
}

void WebSocketServer::setDisconnectHandler(DisconnectHandler handler) {
    disconnectHandler_ = std::move(handler);
}

void WebSocketServer::sendTo(uint64_t sessionId, const std::string& message) {
    g_connections.send(sessionId, message);
}

void WebSocketServer::run() {
    if (!initNetwork()) {
        Logger::error("Failed to initialize network");
        return;
    }

    SocketHandle serverSock = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSock == kInvalidSocket) {
        Logger::error("Failed to create socket");
        return;
    }

    int opt = 1;
    setsockopt(serverSock, SOL_SOCKET, SO_REUSEADDR,
               reinterpret_cast<const char*>(&opt), sizeof(opt));

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(static_cast<uint16_t>(port_));

    if (bind(serverSock, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) != 0) {
        Logger::error("Failed to bind port " + std::to_string(port_));
        closeSocket(serverSock);
        return;
    }

    if (listen(serverSock, 64) != 0) {
        Logger::error("Failed to listen");
        closeSocket(serverSock);
        return;
    }

    running_ = true;
    Logger::info("WebSocket server listening on port " + std::to_string(port_));

    while (running_) {
        fd_set readSet;
        FD_ZERO(&readSet);
        FD_SET(serverSock, &readSet);
        timeval tv{1, 0};
        int sel = select(static_cast<int>(serverSock) + 1, &readSet, nullptr, nullptr, &tv);
        if (sel <= 0) continue;

        sockaddr_in clientAddr{};
#ifdef _WIN32
        int addrLen = sizeof(clientAddr);
#else
        socklen_t addrLen = sizeof(clientAddr);
#endif
        SocketHandle clientSock = accept(serverSock,
            reinterpret_cast<sockaddr*>(&clientAddr), &addrLen);
        if (clientSock == kInvalidSocket) continue;

        std::thread([this, clientSock]() {
            static std::atomic<uint64_t> nextSessionId{1};
            uint64_t sessionId = nextSessionId++;

            if (!handleWebSocketHandshake(clientSock)) {
                closeSocket(clientSock);
                return;
            }

            g_connections.add(sessionId, clientSock);
            if (connectHandler_) connectHandler_(sessionId);

            while (running_) {
                std::string msg;
                if (!readWebSocketMessage(clientSock, msg)) break;
                if (messageHandler_) messageHandler_(sessionId, msg);
            }

            if (disconnectHandler_) disconnectHandler_(sessionId);
            g_connections.remove(sessionId);
            closeSocket(clientSock);
        }).detach();
    }

    closeSocket(serverSock);
    cleanupNetwork();
}

void WebSocketServer::stop() {
    running_ = false;
}

}  // namespace guandan
