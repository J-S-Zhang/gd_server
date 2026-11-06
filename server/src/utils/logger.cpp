#include "utils/logger.h"
#include <chrono>
#include <ctime>
#include <iomanip>
#include <iostream>
#include <sstream>

namespace guandan {

static std::string timestamp() {
    auto now = std::chrono::system_clock::now();
    auto t = std::chrono::system_clock::to_time_t(now);
    std::tm tm{};
#ifdef _WIN32
    localtime_s(&tm, &t);
#else
    localtime_r(&t, &tm);
#endif
/// @file logger.cpp
/// @brief 分级日志输出

    std::ostringstream oss;
    oss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");
    return oss.str();
}

/// 实现 info 相关逻辑
void Logger::info(const std::string& msg) {
    std::cout << "[" << timestamp() << "] [INFO] " << msg << std::endl;
}

/// 实现 warn 相关逻辑
void Logger::warn(const std::string& msg) {
    std::cout << "[" << timestamp() << "] [WARN] " << msg << std::endl;
}

/// 实现 error 相关逻辑
void Logger::error(const std::string& msg) {
    std::cerr << "[" << timestamp() << "] [ERROR] " << msg << std::endl;
}

}  // namespace guandan
