#pragma once

#include <string>

namespace guandan {

std::string generateSalt();
std::string hashPassword(const std::string& password, const std::string& salt);
bool verifyPassword(const std::string& password, const std::string& salt,
                    const std::string& expectedHash);

}  // namespace guandan
