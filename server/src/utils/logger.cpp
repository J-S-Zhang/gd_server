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
    std::ostringstream oss;
    oss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");
    return oss.str();
}

void Logger::info(const std::string& msg) {
    std::cout << "[" << timestamp() << "] [INFO] " << msg << std::endl;
}

void Logger::warn(const std::string& msg) {
    std::cout << "[" << timestamp() << "] [WARN] " << msg << std::endl;
}

void Logger::error(const std::string& msg) {
    std::cerr << "[" << timestamp() << "] [ERROR] " << msg << std::endl;
}

}  // namespace guandan
