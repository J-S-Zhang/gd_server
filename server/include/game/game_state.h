#pragma once

#include "game/card.h"
#include "game/hand.h"
#include "game/types.h"
#include "game/card_pattern.h"
#include <array>
#include <optional>
#include <vector>

namespace guandan {

/// 单个玩家在对局中的状态
struct PlayerState {
    PlayerId id = 0;                        ///< 玩家 id
    int seatIndex = 0;                      ///< 座位 0..n-1
    int team = 0;                           ///< 0 = A 队, 1 = B 队
    Hand hand;                              ///< 手牌
    PlayerStatus status = PlayerStatus::ONLINE;  ///< 在线状态
    bool isReady = false;                   ///< 是否准备（局前）
    bool hasFinished = false;               ///< 本局是否已出完牌
    int finishRank = 0;                     ///< 0 未出完，1 头游等
};

/// 整桌对局状态（权威）
struct GameState {
    RoomId roomId;                          ///< 房间 id
    GamePhase phase = GamePhase::WAITING;   ///< 阶段
    std::array<PlayerState, kPlayerCount> players{};  ///< 各座位玩家
    int playerCount = 0;                    ///< 实际人数

    int currentPlayerIndex = 0;             ///< 当前出牌座位
    int currentLevel = 2;                   ///< 当前打几
    int attackingTeam = 0;                  ///< 攻方（打自己级牌的队伍）
    bool isPassARound = false;              ///< 本局是否为过 A 局
    int firstPlayerIndex = 0;               ///< 本局首出座位

    std::vector<CardId> lastPlayedCards;    ///< 上一手出的牌
    CardPattern lastPattern;                ///< 上一手牌型
    int lastPlayedPlayerIndex = -1;         ///< 上一手出牌座位
    int passCount = 0;                      ///< 本轮连续 pass 次数

    uint64_t turnId = 0;                    ///< 回合 id
    uint64_t stateVersion = 0;              ///< 状态版本
    int round = 0;                          ///< 局数计数

    std::vector<Card> allCards;             ///< 全部牌，按 id 索引

    /// 按 id 取牌
    const Card& getCardById(CardId id) const;
    /// 按玩家 id 取 mutable 状态
    PlayerState* getPlayerById(PlayerId id);
    /// 按玩家 id 取只读状态
    const PlayerState* getPlayerById(PlayerId id) const;
    /// 座位所属队伍（六人局 0/1 交替）
    int teamOf(int seatIndex) const { return seatIndex % 2; }
};

}  // namespace guandan
