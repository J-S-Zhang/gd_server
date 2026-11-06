#pragma once

#include <random>

namespace guandan {

/// 全局随机数引擎（单例）
class Random {
public:
    /// 获取单例
    static Random& instance();

    /// 64 位 Mersenne Twister 引擎引用
    std::mt19937_64& engine();

private:
    Random();
    std::random_device rd_;       ///< 种子源
    std::mt19937_64 engine_;      ///< 随机引擎
};

}  // namespace guandan
