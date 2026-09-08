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
    std::string passwordHash;
    std::string salt;
    UserStats stats;
};

}  // namespace guandan
