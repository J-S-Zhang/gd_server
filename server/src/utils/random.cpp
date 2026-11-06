#include "utils/random.h"

/// @file random.cpp
/// @brief 线程安全随机数引擎

namespace guandan {

/// 实现 instance 相关逻辑
Random& Random::instance() {
    static Random inst;
    return inst;
}

/// 构造 Random 实例
Random::Random() : engine_(rd_()) {}

/// 实现 engine 相关逻辑
std::mt19937_64& Random::engine() {
    return engine_;
}

}  // namespace guandan
