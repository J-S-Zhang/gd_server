#include "auth/token_manager.h"
#include "utils/uuid.h"

/// @file token_manager.cpp
/// @brief 登录令牌签发、校验与撤销

namespace guandan {

/// 签发新登录令牌
std::string TokenManager::issueToken(PlayerId userId) {
    std::lock_guard lock(mutex_);
    for (auto it = tokens_.begin(); it != tokens_.end();) {
        if (it->second.userId == userId) {
            it = tokens_.erase(it);
        } else {
            ++it;
        }
    }

    std::string token = generateUuid();
    TokenEntry entry;
    entry.userId = userId;
    entry.expiresAt = std::chrono::steady_clock::now() +
                      std::chrono::hours(24 * kTokenTtlDays);
    tokens_[token] = entry;
    return token;
}

/// 校验令牌并返回用户
std::optional<PlayerId> TokenManager::validateToken(const std::string& token) {
    if (token.empty()) return std::nullopt;
    std::lock_guard lock(mutex_);
    auto it = tokens_.find(token);
    if (it == tokens_.end()) return std::nullopt;
    if (std::chrono::steady_clock::now() > it->second.expiresAt) {
        tokens_.erase(it);
        return std::nullopt;
    }
    return it->second.userId;
}

/// 撤销指定令牌
void TokenManager::revokeToken(const std::string& token) {
    std::lock_guard lock(mutex_);
    tokens_.erase(token);
}

/// 撤销某用户全部令牌
void TokenManager::revokeAllForUser(PlayerId userId) {
    for (auto it = tokens_.begin(); it != tokens_.end();) {
        if (it->second.userId == userId) {
            it = tokens_.erase(it);
        } else {
            ++it;
        }
    }
}

}  // namespace guandan
