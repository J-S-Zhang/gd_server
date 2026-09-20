#include "voice/voice_server.h"

#include "utils/logger.h"
#include "voice/voice_packet.h"
#include "voice/voice_room_manager.h"

#include <atomic>
#include <chrono>
#include <cstring>
#include <string>
#include <thread>
#include <vector>

#ifdef _WIN32
    #ifndef WIN32_LEAN_AND_MEAN
    #define WIN32_LEAN_AND_MEAN
    #endif
    #include <winsock2.h>
    #include <ws2tcpip.h>
    using SocketHandle = SOCKET;
    const SocketHandle kInvalidSocket = INVALID_SOCKET;
#else
    #include <arpa/inet.h>
    #include <netinet/in.h>
    #include <sys/socket.h>
    #include <unistd.h>
    using SocketHandle = int;
    const SocketHandle kInvalidSocket = -1;
#endif

namespace guandan::voice {

namespace {

bool initNetwork() {
#ifdef _WIN32
    WSADATA wsa;
    return WSAStartup(MAKEWORD(2, 2), &wsa) == 0;
#else
    return true;
#endif
}

void cleanupNetwork() {
#ifdef _WIN32
    WSACleanup();
#endif
}

void closeSocket(SocketHandle sock) {
#ifdef _WIN32
    closesocket(sock);
#else
    close(sock);
#endif
}

std::string payloadToString(const uint8_t* data, uint16_t len) {
    if (data == nullptr || len == 0) return {};
    return std::string(reinterpret_cast<const char*>(data), len);
}

}  // namespace

VoiceServer::VoiceServer(int port) : port_(port) {}

VoiceServer::~VoiceServer() {
    stop();
}

void VoiceServer::stop() {
    running_ = false;
}

void VoiceServer::run() {
    if (!initNetwork()) {
        Logger::error("Voice server network init failed");
        return;
    }

    SocketHandle sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sock == kInvalidSocket) {
        Logger::error("Voice server socket create failed");
        cleanupNetwork();
        return;
    }

    sockaddr_in addr {};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(static_cast<uint16_t>(port_));

    if (bind(sock, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) != 0) {
        Logger::error("Voice server bind failed on port " + std::to_string(port_));
        closeSocket(sock);
        cleanupNetwork();
        return;
    }

    running_ = true;
    Logger::info("Voice server listening on UDP " + std::to_string(port_));

    VoiceRoomManager rooms;
    auto lastPrune = std::chrono::steady_clock::now();

    std::vector<uint8_t> buffer(kHeaderSize + kMaxPayload);
    while (running_) {
        sockaddr_in from {};
        socklen_t fromLen = sizeof(from);
        const int received = recvfrom(sock,
                                      reinterpret_cast<char*>(buffer.data()),
                                      static_cast<int>(buffer.size()),
                                      0,
                                      reinterpret_cast<sockaddr*>(&from),
                                      &fromLen);
        if (received < 0) {
            continue;
        }

        PacketHeader header;
        if (!readHeader(buffer.data(), received, header)) {
            continue;
        }
        if (static_cast<size_t>(received) < kHeaderSize + header.payloadLen) {
            continue;
        }

        const uint8_t* payload = buffer.data() + kHeaderSize;
        const auto type = static_cast<PacketType>(header.type);

        switch (type) {
            case PacketType::Join: {
                const std::string roomId = payloadToString(payload, header.payloadLen);
                if (roomId.empty() || header.userId == 0) break;
                rooms.join(header.userId, roomId, from);
                Logger::info("Voice join user=" + std::to_string(header.userId) +
                             " room=" + roomId);
                break;
            }
            case PacketType::Leave:
                rooms.leave(header.userId);
                break;
            case PacketType::Heartbeat:
                rooms.touch(header.userId, from);
                break;
            case PacketType::Audio: {
                rooms.touch(header.userId, from);
                std::string roomId;
                if (!rooms.roomIdForUser(header.userId, roomId)) break;

                const auto peers = rooms.peersExcept(header.userId, roomId);
                for (const auto& peer : peers) {
                    if (peer.sin_addr.s_addr == from.sin_addr.s_addr &&
                        peer.sin_port == from.sin_port) {
                        continue;
                    }
                    sendto(sock,
                           reinterpret_cast<const char*>(buffer.data()),
                           received,
                           0,
                           reinterpret_cast<const sockaddr*>(&peer),
                           sizeof(peer));
                }
                break;
            }
            default:
                break;
        }

        const auto now = std::chrono::steady_clock::now();
        if (now - lastPrune > std::chrono::seconds(5)) {
            rooms.pruneIdle(std::chrono::seconds(30));
            lastPrune = now;
        }
    }

    closeSocket(sock);
    cleanupNetwork();
}

}  // namespace guandan::voice
