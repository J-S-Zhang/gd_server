#pragma once

#include <cstdint>
#include <string>

namespace guandan {

using PlayerId = uint64_t;    ///< 玩家 id
using RoomId = std::string;   ///< 房间 id
using CardId = uint16_t;      ///< 牌唯一 id（整副牌内）
using RequestId = uint64_t;   ///< 请求 id

constexpr int kPlayerCount = 6;              ///< 标准六人局人数
constexpr int kDeckCount = 3;                ///< 副数
constexpr int kTotalCards = 162;             ///< 3 * 54
constexpr int kCardsPerPlayer = 27;          ///< 六人局每人张数
constexpr int kSoloTestCardsPerPlayer = 2;   ///< 单人测试模式每人张数
constexpr int kTeamCount = 2;                ///< 队伍数
constexpr int kPlayersPerTeam = 3;           ///< 每队人数（六人局）

/// 花色
enum class Suit : uint8_t {
    SPADE = 0,    ///< 黑桃
    HEART = 1,    ///< 红桃
    CLUB = 2,     ///< 梅花
    DIAMOND = 3,  ///< 方块
    JOKER = 4     ///< 王（无花色）
};

/// 点数
enum class Rank : uint8_t {
    R2 = 2, R3, R4, R5, R6, R7, R8, R9, R10,
    J = 11, Q, K, A,
    SMALL_JOKER = 15,  ///< 小王
    BIG_JOKER = 16     ///< 大王
};

/// 牌型
enum class CardType {
    INVALID,           ///< 非法
    SINGLE,            ///< 单张
    PAIR,              ///< 对子
    TRIPLE,            ///< 三不带
    TRIPLE_WITH_PAIR,  ///< 三带二
    STRAIGHT,          ///< 顺子
    THREE_PAIRS,       ///< 三连对
    TWO_TRIPLES,       ///< 钢板（两连续三张）
    STRAIGHT_FLUSH,    ///< 同花顺
    BOMB,              ///< 炸弹
    JOKER_BOMB         ///< 王炸
};

/// 对局阶段
enum class GamePhase {
    WAITING,          ///< 等待玩家
    READY,            ///< 准备中
    DEALING,          ///< 发牌
    TRIBUTE,          ///< 进贡
    RETURN_TRIBUTE,   ///< 还贡
    PLAYING,          ///< 出牌
    ROUND_END,        ///< 单局结束
    SETTLEMENT,       ///< 结算
    FINISHED          ///< 整场结束
};

/// 玩家在线状态
enum class PlayerStatus {
    ONLINE,   ///< 在线
    OFFLINE   ///< 离线
};

/// 业务与协议错误码
enum class ErrorCode : int {
    OK = 0,
    INVALID_TOKEN = 1001,      ///< token 无效
    UNAUTHORIZED = 1002,       ///< 未授权
    ROOM_NOT_FOUND = 2001,     ///< 房间不存在
    ROOM_FULL = 2002,          ///< 房间已满
    ALREADY_IN_ROOM = 2003,    ///< 已在其他房间
    NOT_IN_ROOM = 2004,        ///< 不在房间内
    NOT_ROOM_OWNER = 2005,     ///< 非房主
    NOT_YOUR_TURN = 3001,      ///< 非当前出牌者
    INVALID_CARDS = 3002,      ///< 牌不在手牌或重复
    INVALID_PATTERN = 3003,    ///< 牌型不合法
    CANNOT_BEAT = 3004,        ///< 压不过上家
    INVALID_STATE = 3005,      ///< 状态不允许该操作
    REQUEST_EXPIRED = 3006,    ///< turnId/stateVersion 过期
    DUPLICATE_REQUEST = 3007,  ///< 重复请求
    NOT_ALL_READY = 3008,      ///< 未全员准备
    INVALID_MESSAGE = 4001,    ///< 消息格式错误
    UNKNOWN_TYPE = 4002,         ///< 未知消息类型
    INTERNAL_ERROR = 5000        ///< 服务器内部错误
};

/// Rank 对应的比较用整数值
inline int rankValue(Rank r) {
    return static_cast<int>(r);
}

/// 是否为王
inline bool isJoker(Rank r) {
    return r == Rank::SMALL_JOKER || r == Rank::BIG_JOKER;
}

/// 花色缩写字符串
inline std::string suitToString(Suit s) {
    switch (s) {
        case Suit::SPADE: return "S";
        case Suit::HEART: return "H";
        case Suit::CLUB: return "C";
        case Suit::DIAMOND: return "D";
        default: return "J";
    }
}

/// 点数显示字符串
inline std::string rankToString(Rank r) {
    switch (r) {
        case Rank::R2: return "2";
        case Rank::R3: return "3";
        case Rank::R4: return "4";
        case Rank::R5: return "5";
        case Rank::R6: return "6";
        case Rank::R7: return "7";
        case Rank::R8: return "8";
        case Rank::R9: return "9";
        case Rank::R10: return "10";
        case Rank::J: return "J";
        case Rank::Q: return "Q";
        case Rank::K: return "K";
        case Rank::A: return "A";
        case Rank::SMALL_JOKER: return "SJ";
        case Rank::BIG_JOKER: return "BJ";
        default: return "?";
    }
}

}  // namespace guandan
