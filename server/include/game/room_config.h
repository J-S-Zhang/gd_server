#pragma once

#include "game/card.h"
#include <string>

namespace guandan {

constexpr PlayerId kBotIdBase = 9000000000ULL;  ///< 机器人 id 起始值

/// 房间/对局模式
enum class GameMode {
    SIX,   ///< 六人掼蛋
    FOUR,  ///< 四人
    SOLO   ///< 单人测试
};

/// 创建房间时的配置
struct RoomConfig {
    GameMode mode = GameMode::SIX;   ///< 模式
    int maxPlayers = 6;              ///< 最大人数
    int deckCount = 3;               ///< 副数
    int playersPerTeam = 3;          ///< 每队人数
    std::string modeName = "six";    ///< 模式名字符串
    bool enableTribute = true;       ///< 是否开启进贡

    /// 从 mode 字符串解析配置
    static RoomConfig fromModeName(const std::string& mode);
};

/// 是否为机器人 id
inline bool isBotPlayer(PlayerId id) {
    return id >= kBotIdBase;
}

/// RoomConfig 转为 GameRuleConfig
inline GameRuleConfig toGameRuleConfig(const RoomConfig& config) {
    GameRuleConfig cfg;
    cfg.playerCount = config.maxPlayers;
    cfg.deckCount = config.deckCount;
    cfg.playersPerTeam = config.playersPerTeam;
    cfg.enableTribute = config.enableTribute;
    switch (config.mode) {
        case GameMode::FOUR:
            cfg.ruleVersion = "four_player_guandan_v1";
            break;
        case GameMode::SOLO:
            cfg.ruleVersion = "solo_test_v1";
            break;
        default:
            cfg.ruleVersion = "six_player_guandan_v1";
            break;
    }
    return cfg;
}

}  // namespace guandan
