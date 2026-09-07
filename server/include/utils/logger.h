#pragma once

#include <string>

namespace guandan {

class Logger {
public:
    static void info(const std::string& msg);
    static void warn(const std::string& msg);
    static void error(const std::string& msg);
};

}  // namespace guandan
