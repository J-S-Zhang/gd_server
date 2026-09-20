#include "voice/voice_room_manager.h"

#include <algorithm>

namespace guandan::voice {

namespace {

bool sameAddr(const VoiceSocketAddr& a, const VoiceSocketAddr& b) {
    return a.sin_addr.s_addr == b.sin_addr.s_addr && a.sin_port == b.sin_port;
}

void removeFromRoom(std::unordered_map<std::string, std::vector<uint32_t>>& rooms,
                    const std::string& roomId,
                    uint32_t userId) {
    auto it = rooms.find(roomId);
    if (it == rooms.end()) return;
    auto& members = it->second;
    members.erase(std::remove(members.begin(), members.end(), userId), members.end());
    if (members.empty()) {
        rooms.erase(it);
    }
}

}  // namespace

void VoiceRoomManager::join(uint32_t userId, const std::string& roomId, const VoiceSocketAddr& addr) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto existing = clients_.find(userId);
    if (existing != clients_.end() && existing->second.roomId != roomId) {
        removeFromRoom(roomMembers_, existing->second.roomId, userId);
    }

    VoiceClient client;
    client.userId = userId;
    client.roomId = roomId;
    client.addr = addr;
    client.lastSeen = std::chrono::steady_clock::now();
    clients_[userId] = client;

    auto& members = roomMembers_[roomId];
    if (std::find(members.begin(), members.end(), userId) == members.end()) {
        members.push_back(userId);
    }
}

void VoiceRoomManager::leave(uint32_t userId) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = clients_.find(userId);
    if (it == clients_.end()) return;
    removeFromRoom(roomMembers_, it->second.roomId, userId);
    clients_.erase(it);
}

void VoiceRoomManager::touch(uint32_t userId, const VoiceSocketAddr& addr) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = clients_.find(userId);
    if (it == clients_.end()) return;
    it->second.addr = addr;
    it->second.lastSeen = std::chrono::steady_clock::now();
}

std::vector<VoiceSocketAddr> VoiceRoomManager::peersExcept(uint32_t userId,
                                                           const std::string& roomId) const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<VoiceSocketAddr> peers;
    auto roomIt = roomMembers_.find(roomId);
    if (roomIt == roomMembers_.end()) return peers;

    for (const auto memberId : roomIt->second) {
        if (memberId == userId) continue;
        auto clientIt = clients_.find(memberId);
        if (clientIt != clients_.end()) {
            peers.push_back(clientIt->second.addr);
        }
    }
    return peers;
}

bool VoiceRoomManager::roomIdForUser(uint32_t userId, std::string& roomIdOut) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = clients_.find(userId);
    if (it == clients_.end()) return false;
    roomIdOut = it->second.roomId;
    return true;
}

void VoiceRoomManager::pruneIdle(std::chrono::seconds maxIdle) {
    std::lock_guard<std::mutex> lock(mutex_);
    const auto now = std::chrono::steady_clock::now();
    for (auto it = clients_.begin(); it != clients_.end();) {
        if (now - it->second.lastSeen > maxIdle) {
            removeFromRoom(roomMembers_, it->second.roomId, it->first);
            it = clients_.erase(it);
        } else {
            ++it;
        }
    }
}

}  // namespace guandan::voice
