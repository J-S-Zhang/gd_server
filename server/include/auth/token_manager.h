#pragma once

#include "game/types.h"
#include <chrono>
#include <mutex>
#include <optional>
#include <string>
#include <unordered_map>

namespace guandan {

/// 登录 token 签发与校验（内存存储）
class TokenManager {
public:
    /// 为用户签发新 token 字符串
    std::string issueToken(PlayerId userId);
    /// 校验 token，有效返回 userId
    std::optional<PlayerId> validateToken(const std::string& token);

    /// 撤销单个 token
    void revokeToken(const std::string& token);
    /// 撤销某用户全部 token
    void revokeAllForUser(PlayerId userId);

private:
    struct TokenEntry {
        PlayerId userId = 0;                              ///< 所属用户
        std::chrono::steady_clock::time_point expiresAt;  ///< 过期时间点
    };

    std::mutex mutex_;                                      ///< 保护 tokens_
    std::unordered_map<std::string, TokenEntry> tokens_;    ///< token -> 条目
    static constexpr int kTokenTtlDays = 30;              ///< 默认有效期（天）
};

}  // namespace guandan
