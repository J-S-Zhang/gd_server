#pragma once

#include <string>

namespace guandan {

/// 简单日志输出
class Logger {
public:
    /// 信息级别
    static void info(const std::string& msg);
    /// 警告级别
    static void warn(const std::string& msg);
    /// 错误级别
    static void error(const std::string& msg);
};

}  // namespace guandan
