#pragma once

#include "room/room.h"
#include <memory>
#include <mutex>
#include <unordered_map>

namespace guandan {

class RoomManager {
public:
    std::shared_ptr<Room> createRoom(PlayerId ownerId, const std::string& nickname,
                                     const std::string& mode = "six");
    std::shared_ptr<Room> joinRoom(const RoomId& roomId, PlayerId playerId,
                                   const std::string& nickname);
    std::shared_ptr<Room> getRoom(const RoomId& roomId);
    void removeRoom(const RoomId& roomId);
    std::shared_ptr<Room> findRoomByPlayer(PlayerId playerId);
    void removePlayerFromRoom(PlayerId playerId);

private:
    std::mutex mutex_;
    std::unordered_map<RoomId, std::shared_ptr<Room>> rooms_;
    std::unordered_map<PlayerId, RoomId> playerRoomMap_;
};

}  // namespace guandan
