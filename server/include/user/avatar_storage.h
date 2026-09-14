#pragma once

#include "game/types.h"
#include <optional>
#include <string>
#include <vector>

namespace guandan {

struct AvatarStorageConfig {
    /// 磁盘目录，如 downloads/avatars
    std::string storageDir = "downloads/avatars";
    /// 对外 URL 路径前缀，如 /downloads/avatars
    std::string publicPathPrefix = "/downloads/avatars";
};

/// 保存用户头像，返回对外访问路径（如 /downloads/avatars/user_10001.jpg）。
std::optional<std::string> saveUserAvatar(const AvatarStorageConfig& config, PlayerId userId,
                                            const std::vector<uint8_t>& data,
                                            const std::string& format);

}  // namespace guandan
