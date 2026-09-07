#pragma once

#include <cstdint>
#include <mutex>
#include <string>
#include <unordered_map>

#ifdef _WIN32
#include <winsock2.h>
using SocketHandle = SOCKET;
#else
using SocketHandle = int;
#endif

namespace guandan {

struct Connection {
    uint64_t sessionId = 0;
    SocketHandle socket = 0;
    bool isWebSocket = false;
};

class ConnectionManager {
public:
    void add(uint64_t sessionId, SocketHandle socket);
    void remove(uint64_t sessionId);
    Connection* get(uint64_t sessionId);
    bool send(uint64_t sessionId, const std::string& message);

private:
    std::mutex mutex_;
    std::unordered_map<uint64_t, Connection> connections_;
};

}  // namespace guandan
