#pragma once

#include <string>

namespace guandan {

/// 生成随机盐值
std::string generateSalt();
/// 使用盐对密码做哈希
std::string hashPassword(const std::string& password, const std::string& salt);
/// 校验密码是否与 stored hash 匹配
bool verifyPassword(const std::string& password, const std::string& salt,
                    const std::string& expectedHash);

}  // namespace guandan
