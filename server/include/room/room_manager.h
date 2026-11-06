#pragma once

#include "room/room.h"
#include <memory>
#include <mutex>
#include <unordered_map>

namespace guandan {

class RoomExecutor;

/// 全局房间 registry：创建、加入、按玩家查找
class RoomManager {
public:
    /// 注入房间串行执行器（创建 Room 时使用）
    void setRoomExecutor(RoomExecutor* executor) { executor_ = executor; }
    /// 创建房间并令房主加入
    std::shared_ptr<Room> createRoom(PlayerId ownerId, const PlayerJoinInfo& profile,
                                     const std::string& mode = "six");
    /// 加入已有房间
    std::shared_ptr<Room> joinRoom(const RoomId& roomId, PlayerId playerId,
                                   const PlayerJoinInfo& profile);
    /// 按 id 获取房间，不存在返回 nullptr
    std::shared_ptr<Room> getRoom(const RoomId& roomId);
    /// 移除并销毁房间
    void removeRoom(const RoomId& roomId);
    /// 查找玩家当前所在房间
    std::shared_ptr<Room> findRoomByPlayer(PlayerId playerId);
    /// 将玩家从所在房间移除（不销毁空房逻辑由调用方处理）
    void removePlayerFromRoom(PlayerId playerId);

private:
    RoomExecutor* executor_ = nullptr;                              ///< 房间执行器
    std::mutex mutex_;                                              ///< 保护 rooms_ 与 playerRoomMap_
    std::unordered_map<RoomId, std::shared_ptr<Room>> rooms_;       ///< 房间 id -> 房间
    std::unordered_map<PlayerId, RoomId> playerRoomMap_;            ///< 玩家 -> 当前房间 id
};

}  // namespace guandan
