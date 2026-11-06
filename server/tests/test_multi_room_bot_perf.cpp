#include "game/game_engine.h"
#include "game/room_config.h"
#include "room/room_executor.h"
#include "room/room_task_queue.h"
#include "test_framework.h"

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <iostream>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

using namespace guandan;

namespace {

/// 四人机器人局：使用 solo_test_v1（每人 2 张）以便 BotPlayer O(2^n) 可在合理时间内跑完。
/// 四人正式局每人 27 张时子集枚举不可用，不适合本性能测试。
GameRuleConfig fourBotRuleConfig() {
    GameRuleConfig cfg;
    cfg.playerCount = 4;
    cfg.deckCount = 2;
    cfg.playersPerTeam = 2;
    cfg.enableTribute = false;
    cfg.enableReturnTribute = false;
    cfg.enableAntiTribute = false;
    cfg.ruleVersion = "solo_test_v1";
    return cfg;
}

std::vector<PlayerId> fourBotIds(int roomIndex) {
    // 每房独立 bot id，避免跨房混淆
    const PlayerId base = kBotIdBase + 1 + static_cast<PlayerId>(roomIndex) * 10;
    return {base, base + 1, base + 2, base + 3};
}

struct BotMatchResult {
    bool ok = false;
    int turns = 0;
    int plays = 0;
    int passes = 0;
    std::string error;
};

/// 推进贡牌/还贡直至进入 PLAYING 或结束（本配置首局通常无贡牌）
void resolvePendingTribute(GameEngine& engine) {
    constexpr int kMaxTributeSteps = 64;
    for (int i = 0; i < kMaxTributeSteps && engine.hasPendingTributeAction(); ++i) {
        const auto& state = engine.getState();
        if (state.phase == GamePhase::TRIBUTE) {
            for (int seat : engine.pendingTributerSeats()) {
                if (seat < 0 || seat >= state.playerCount) continue;
                const PlayerId id = state.players[seat].id;
                auto card = engine.chooseBotTributeCard(id);
                if (!card) continue;
                engine.submitTribute(id, *card);
            }
        } else if (state.phase == GamePhase::RETURN_TRIBUTE) {
            for (int seat : engine.pendingReturnSeats()) {
                if (seat < 0 || seat >= state.playerCount) continue;
                const PlayerId id = state.players[seat].id;
                auto card = engine.chooseBotReturnCard(id);
                if (!card) continue;
                engine.submitReturn(id, *card);
            }
        } else {
            break;
        }
        if (!engine.hasPendingTributeAction() &&
            engine.getState().phase == GamePhase::PLAYING) {
            break;
        }
        if (!engine.hasPendingTributeAction() &&
            (engine.getState().phase == GamePhase::TRIBUTE ||
             engine.getState().phase == GamePhase::RETURN_TRIBUTE)) {
            engine.finalizeTributePhase();
        }
    }
}

/// 单房间：四机器人自动开局并按规则出牌（能压则压，否则过）
BotMatchResult playFourBotMatch(int roomIndex) {
    BotMatchResult out;
    GameEngine engine(fourBotRuleConfig());
    const auto players = fourBotIds(roomIndex);
    engine.init("perf_" + std::to_string(roomIndex), players);
    for (PlayerId id : players) {
        engine.setPlayerReady(id);
    }

    auto start = engine.startGame();
    if (start.code != ErrorCode::OK) {
        out.error = "startGame failed";
        return out;
    }

    resolvePendingTribute(engine);

    constexpr int kMaxTurns = 500;
    while (engine.getState().phase == GamePhase::PLAYING && out.turns < kMaxTurns) {
        ++out.turns;
        resolvePendingTribute(engine);
        if (engine.getState().phase != GamePhase::PLAYING) break;

        const auto& state = engine.getState();
        const int seat = state.currentPlayerIndex;
        if (seat < 0 || seat >= state.playerCount) {
            out.error = "invalid current seat";
            return out;
        }
        const PlayerId botId = state.players[seat].id;

        if (auto play = engine.chooseBotPlay(botId)) {
            auto result = engine.playCards(botId, *play);
            if (result.code != ErrorCode::OK) {
                out.error = "playCards failed code=" + std::to_string(static_cast<int>(result.code));
                return out;
            }
            ++out.plays;
            if (result.gameOver) break;
        } else {
            auto result = engine.pass(botId);
            if (result.code != ErrorCode::OK) {
                // 领出位不能过：强制打最小单张
                const auto& hand = engine.getState().players[seat].hand.cards();
                if (hand.empty()) {
                    out.error = "empty hand on forced play";
                    return out;
                }
                auto forced = engine.playCards(botId, {hand.front()});
                if (forced.code != ErrorCode::OK) {
                    out.error = "forced play failed";
                    return out;
                }
                ++out.plays;
                if (forced.gameOver) break;
            } else {
                ++out.passes;
                if (result.gameOver) break;
            }
        }
    }

    if (engine.getState().phase != GamePhase::FINISHED) {
        out.error = "match not finished after " + std::to_string(out.turns) + " turns";
        return out;
    }
    out.ok = true;
    return out;
}

/// 每个房间连续打 matchesPerRoom 局，汇总结果
BotMatchResult playRoomWorkload(int roomIndex, int matchesPerRoom) {
    BotMatchResult agg;
    agg.ok = true;
    for (int m = 0; m < matchesPerRoom; ++m) {
        // roomIndex 与局序号合成稳定 id，保证可复现
        auto one = playFourBotMatch(roomIndex * 1000 + m);
        if (!one.ok) {
            agg.ok = false;
            agg.error = "match " + std::to_string(m) + ": " + one.error;
            return agg;
        }
        agg.turns += one.turns;
        agg.plays += one.plays;
        agg.passes += one.passes;
    }
    return agg;
}

int64_t runWorkloadSequential(int roomCount, int matchesPerRoom,
                              std::vector<BotMatchResult>& results) {
    results.assign(roomCount, {});
    const auto t0 = std::chrono::steady_clock::now();
    for (int i = 0; i < roomCount; ++i) {
        results[i] = playRoomWorkload(i, matchesPerRoom);
    }
    const auto t1 = std::chrono::steady_clock::now();
    return std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count();
}

int64_t runWorkloadParallel(int roomCount, int matchesPerRoom, int workerCount,
                            std::vector<BotMatchResult>& results) {
    results.assign(roomCount, {});
    RoomExecutor executor(static_cast<std::size_t>(workerCount));
    std::vector<std::shared_ptr<RoomTaskQueue>> queues;
    queues.reserve(roomCount);

    std::mutex mutex;
    std::condition_variable cv;
    std::atomic<int> done{0};

    const auto t0 = std::chrono::steady_clock::now();
    for (int i = 0; i < roomCount; ++i) {
        auto queue = std::make_shared<RoomTaskQueue>(executor);
        queues.push_back(queue);
        queue->post([&, i]() {
            // 同房间多局串行；不同房间由线程池并行
            results[i] = playRoomWorkload(i, matchesPerRoom);
            if (done.fetch_add(1) + 1 == roomCount) {
                std::lock_guard lock(mutex);
                cv.notify_all();
            }
        });
    }

    {
        std::unique_lock lock(mutex);
        cv.wait(lock, [&] { return done.load() >= roomCount; });
    }
    const auto t1 = std::chrono::steady_clock::now();
    return std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count();
}

void printPerfSummary(const char* label, int roomCount, int matchesPerRoom, int64_t ms,
                      const std::vector<BotMatchResult>& results) {
    int ok = 0;
    int turns = 0;
    int plays = 0;
    int passes = 0;
    for (const auto& r : results) {
        if (r.ok) ++ok;
        turns += r.turns;
        plays += r.plays;
        passes += r.passes;
    }
    const int totalMatches = roomCount * matchesPerRoom;
    const double sec = ms / 1000.0;
    const double matchesPerSec = sec > 0 ? totalMatches / sec : 0;
    std::cout << "  [" << label << "] rooms=" << roomCount
              << " matches/room=" << matchesPerRoom
              << " total_matches=" << totalMatches
              << " ok_rooms=" << ok << "/" << roomCount
              << " elapsed_ms=" << ms
              << " matches/s=" << matchesPerSec
              << " turns=" << turns
              << " plays=" << plays
              << " passes=" << passes
              << std::endl;
    for (int i = 0; i < roomCount; ++i) {
        if (!results[i].ok) {
            std::cout << "    room " << i << " FAIL: " << results[i].error << std::endl;
        }
    }
}

}  // namespace

// 临时关闭：只跑多房间并发
// TEST(test_single_four_bot_match_finishes) {
//     auto r = playFourBotMatch(0);
//     ASSERT(r.ok);
//     ASSERT(r.plays > 0);
// }

TEST(test_multi_room_bot_parallel_perf) {
    // 工作量：16 房 × 每房 80 局，经 RoomExecutor 多房间并行
    constexpr int kRoomCount = 16;
    constexpr int kMatchesPerRoom = 80;
    constexpr int kWorkers = 4;

    // 串行基线（已关闭，需要对比加速比时再打开）
    // std::vector<BotMatchResult> seqResults;
    // const int64_t seqMs = runWorkloadSequential(kRoomCount, kMatchesPerRoom, seqResults);
    // printPerfSummary("sequential", kRoomCount, kMatchesPerRoom, seqMs, seqResults);
    // for (const auto& r : seqResults) {
    //     ASSERT(r.ok);
    // }

    std::vector<BotMatchResult> parResults;
    const int64_t parMs =
        runWorkloadParallel(kRoomCount, kMatchesPerRoom, kWorkers, parResults);
    printPerfSummary("parallel", kRoomCount, kMatchesPerRoom, parMs, parResults);
    for (const auto& r : parResults) {
        ASSERT(r.ok);
    }

    ASSERT(parMs >= 0);
    // ASSERT(seqMs > 0);
    // ASSERT(parMs <= seqMs + 20);
    // const double speedup = parMs > 0 ? static_cast<double>(seqMs) / parMs : 0;
    // std::cout << "  speedup=" << speedup << "x (workers=" << kWorkers << ")" << std::endl;
    // ASSERT(speedup >= 1.2 || seqMs < 30);
}
