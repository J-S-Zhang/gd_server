#include "network/connection_manager.h"
#include <vector>

#ifdef _WIN32
    #include <winsock2.h>
#else
    #include <sys/socket.h>
    #include <sys/types.h>
#endif

namespace guandan {

static bool sendWebSocketFrame(SocketHandle sock, const std::string& message) {
    std::vector<uint8_t> frame;
    frame.push_back(0x81);  // FIN + text frame
    size_t len = message.size();
    if (len <= 125) {
        frame.push_back(static_cast<uint8_t>(len));
    } else if (len <= 65535) {
        frame.push_back(126);
        frame.push_back(static_cast<uint8_t>((len >> 8) & 0xFF));
        frame.push_back(static_cast<uint8_t>(len & 0xFF));
    } else {
        frame.push_back(127);
        for (int i = 7; i >= 0; --i) {
            frame.push_back(static_cast<uint8_t>((len >> (i * 8)) & 0xFF));
        }
    }
    frame.insert(frame.end(), message.begin(), message.end());

#ifdef _WIN32
    int sent = ::send(sock, reinterpret_cast<const char*>(frame.data()),
                    static_cast<int>(frame.size()), 0);
    return sent > 0;
#else
    ssize_t sent = ::send(sock, frame.data(), frame.size(), 0);
    return sent > 0;
#endif
}

void ConnectionManager::add(uint64_t sessionId, SocketHandle socket) {
    std::lock_guard lock(mutex_);
    connections_[sessionId] = Connection{sessionId, socket, true};
}

void ConnectionManager::remove(uint64_t sessionId) {
    std::lock_guard lock(mutex_);
    connections_.erase(sessionId);
}

Connection* ConnectionManager::get(uint64_t sessionId) {
    std::lock_guard lock(mutex_);
    auto it = connections_.find(sessionId);
    return it != connections_.end() ? &it->second : nullptr;
}

bool ConnectionManager::send(uint64_t sessionId, const std::string& message) {
    Connection* conn = nullptr;
    {
        std::lock_guard lock(mutex_);
        auto it = connections_.find(sessionId);
        if (it == connections_.end()) return false;
        conn = &it->second;
    }
    return sendWebSocketFrame(conn->socket, message);
}

}  // namespace guandan
