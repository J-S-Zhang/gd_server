#pragma once

#include "game/types.h"
#include <cstdint>
#include <string>

namespace guandan {

struct UserStats {
    uint32_t totalGames = 0;
    uint32_t wins = 0;
    uint32_t losses = 0;
};

struct UserRecord {
    PlayerId id = 0;
    std::string nickname;
    std::string password;
    std::string passwordHash;
    std::string salt;
    /// 对外可访问路径，如 /downloads/avatars/user_10001.jpg
    std::string avatar;
    /// 内置头像预设 id，如 avatar_1；与 avatar 一并同步给房间内其他玩家
    std::string avatarPreset;
    UserStats stats;
};

}  // namespace guandan
