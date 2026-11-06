#pragma once

#include "game/types.h"
#include <cstdint>
#include <string>

namespace guandan {

/// 用户战绩统计
struct UserStats {
    uint32_t totalGames = 0;  ///< 总对局数
    uint32_t wins = 0;        ///< 胜场
    uint32_t losses = 0;      ///< 负场
};

/// 用户持久化记录
struct UserRecord {
    PlayerId id = 0;              ///< 用户 id
    std::string nickname;         ///< 昵称
    std::string password;         ///< 明文密码（仅迁移/调试，正常应使用 hash）
    std::string passwordHash;     ///< 密码哈希
    std::string salt;             ///< 密码盐
    /// 对外可访问路径，如 /downloads/avatars/user_10001.jpg
    std::string avatar;
    /// 内置头像预设 id，如 avatar_1；与 avatar 一并同步给房间内其他玩家
    std::string avatarPreset;
    UserStats stats;              ///< 战绩
};

}  // namespace guandan
