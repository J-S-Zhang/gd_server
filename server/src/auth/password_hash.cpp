#include "auth/password_hash.h"
#include "utils/random.h"
#include "utils/sha1.h"
#include <iomanip>
#include <sstream>

namespace guandan {

namespace {

std::string bytesToHex(const std::string& bytes) {
    std::ostringstream oss;
    oss << std::hex << std::setfill('0');
    for (unsigned char c : bytes) {
        oss << std::setw(2) << static_cast<int>(c);
    }
    return oss.str();
}

}  // namespace

std::string generateSalt() {
    auto& rng = Random::instance().engine();
    std::uniform_int_distribution<int> dist(0, 255);
    std::string salt;
    salt.reserve(16);
    for (int i = 0; i < 16; ++i) {
        salt.push_back(static_cast<char>(dist(rng)));
    }
    return bytesToHex(salt);
}

std::string hashPassword(const std::string& password, const std::string& salt) {
    return bytesToHex(sha1Hex(salt + password));
}

bool verifyPassword(const std::string& password, const std::string& salt,
                    const std::string& expectedHash) {
    return hashPassword(password, salt) == expectedHash;
}

}  // namespace guandan
