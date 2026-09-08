#pragma once

#include "game/types.h"
#include <chrono>
#include <mutex>
#include <optional>
#include <string>
#include <unordered_map>

namespace guandan {

class TokenManager {
public:
    std::string issueToken(PlayerId userId);
    std::optional<PlayerId> validateToken(const std::string& token);

    void revokeToken(const std::string& token);
    void revokeAllForUser(PlayerId userId);

private:
    struct TokenEntry {
        PlayerId userId = 0;
        std::chrono::steady_clock::time_point expiresAt;
    };

    std::mutex mutex_;
    std::unordered_map<std::string, TokenEntry> tokens_;
    static constexpr int kTokenTtlDays = 30;
};

}  // namespace guandan
