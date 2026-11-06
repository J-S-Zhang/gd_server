#include "room/room_manager.h"
#include "game/room_config.h"
#include "room/room_executor.h"
#include "utils/uuid.h"

/// @file room_manager.cpp
/// @brief 房间创建、加入、查询与销毁

namespace guandan {

/// 创建新房间
std::shared_ptr<Room> RoomManager::createRoom(PlayerId ownerId,
                                              const PlayerJoinInfo& profile,
                                              const std::string& mode) {
    std::lock_guard lock(mutex_);
    if (playerRoomMap_.count(ownerId)) return nullptr;

    auto roomId = generateRoomId();
    while (rooms_.count(roomId)) {
        roomId = generateRoomId();
    }

    if (!executor_) return nullptr;

    auto config = RoomConfig::fromModeName(mode);
    auto room = std::make_shared<Room>(roomId, ownerId, config, *executor_);
    room->join(ownerId, profile);
    if (config.mode == GameMode::SOLO) {
        room->fillBots();
    }
    rooms_[roomId] = room;
    playerRoomMap_[ownerId] = roomId;
    return room;
}

/// 加入指定房间
std::shared_ptr<Room> RoomManager::joinRoom(const RoomId& roomId, PlayerId playerId,
                                            const PlayerJoinInfo& profile) {
    std::lock_guard lock(mutex_);
    if (playerRoomMap_.count(playerId)) return nullptr;

    auto it = rooms_.find(roomId);
    if (it == rooms_.end()) return nullptr;

    if (!it->second->join(playerId, profile)) return nullptr;
    playerRoomMap_[playerId] = roomId;
    return it->second;
}

/// 获取房间实例
std::shared_ptr<Room> RoomManager::getRoom(const RoomId& roomId) {
    std::lock_guard lock(mutex_);
    auto it = rooms_.find(roomId);
    return it != rooms_.end() ? it->second : nullptr;
}

/// 移除并销毁房间
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

/// 按玩家查找所在房间
std::shared_ptr<Room> RoomManager::findRoomByPlayer(PlayerId playerId) {
    std::lock_guard lock(mutex_);
    auto it = playerRoomMap_.find(playerId);
    if (it == playerRoomMap_.end()) return nullptr;
    auto roomIt = rooms_.find(it->second);
    return roomIt != rooms_.end() ? roomIt->second : nullptr;
}

/// 将玩家从房间映射中移除
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
