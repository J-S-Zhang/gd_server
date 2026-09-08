#include "room/room_manager.h"
#include "game/room_config.h"
#include "utils/uuid.h"

namespace guandan {

std::shared_ptr<Room> RoomManager::createRoom(PlayerId ownerId,
                                              const std::string& nickname,
                                              const std::string& mode) {
    std::lock_guard lock(mutex_);
    if (playerRoomMap_.count(ownerId)) return nullptr;

    auto roomId = generateRoomId();
    while (rooms_.count(roomId)) {
        roomId = generateRoomId();
    }

    auto config = RoomConfig::fromModeName(mode);
    auto room = std::make_shared<Room>(roomId, ownerId, config);
    room->join(ownerId, nickname);
    if (config.mode == GameMode::SOLO) {
        room->fillBots();
    }
    rooms_[roomId] = room;
    playerRoomMap_[ownerId] = roomId;
    return room;
}

std::shared_ptr<Room> RoomManager::joinRoom(const RoomId& roomId, PlayerId playerId,
                                            const std::string& nickname) {
    std::lock_guard lock(mutex_);
    if (playerRoomMap_.count(playerId)) return nullptr;

    auto it = rooms_.find(roomId);
    if (it == rooms_.end()) return nullptr;

    if (!it->second->join(playerId, nickname)) return nullptr;
    playerRoomMap_[playerId] = roomId;
    return it->second;
}

std::shared_ptr<Room> RoomManager::getRoom(const RoomId& roomId) {
    std::lock_guard lock(mutex_);
    auto it = rooms_.find(roomId);
    return it != rooms_.end() ? it->second : nullptr;
}

void RoomManager::removeRoom(const RoomId& roomId) {
    std::lock_guard lock(mutex_);
    auto it = rooms_.find(roomId);
    if (it == rooms_.end()) return;
    for (const auto& p : it->second->players()) {
        if (!isBotPlayer(p.id)) {
            playerRoomMap_.erase(p.id);
        }
    }
    rooms_.erase(it);
}

std::shared_ptr<Room> RoomManager::findRoomByPlayer(PlayerId playerId) {
    std::lock_guard lock(mutex_);
    auto it = playerRoomMap_.find(playerId);
    if (it == playerRoomMap_.end()) return nullptr;
    auto roomIt = rooms_.find(it->second);
    return roomIt != rooms_.end() ? roomIt->second : nullptr;
}

void RoomManager::removePlayerFromRoom(PlayerId playerId) {
    std::lock_guard lock(mutex_);
    if (isBotPlayer(playerId)) return;
    auto it = playerRoomMap_.find(playerId);
    if (it == playerRoomMap_.end()) return;
    auto roomIt = rooms_.find(it->second);
    if (roomIt != rooms_.end() && roomIt->second->isEmpty()) {
        rooms_.erase(roomIt);
    }
    playerRoomMap_.erase(it);
}

}  // namespace guandan
