#include "user/user_store.h"
#include "utils/logger.h"
#include <algorithm>
#include <cctype>
#include <fstream>
#include <mutex>
#include <sstream>

namespace guandan {

namespace {

std::string trim(const std::string& s) {
    size_t start = 0;
    while (start < s.size() && std::isspace(static_cast<unsigned char>(s[start]))) ++start;
    size_t end = s.size();
    while (end > start && std::isspace(static_cast<unsigned char>(s[end - 1]))) --end;
    return s.substr(start, end - start);
}

std::string extractJsonStringValue(const std::string& json, const std::string& key) {
    const std::string needle = "\"" + key + "\":\"";
    auto pos = json.find(needle);
    if (pos == std::string::npos) return "";
    pos += needle.size();
    auto end = json.find('"', pos);
    if (end == std::string::npos) return "";
    return json.substr(pos, end - pos);
}

uint64_t extractJsonUintValue(const std::string& json, const std::string& key) {
    const std::string needle = "\"" + key + "\":";
    auto pos = json.find(needle);
    if (pos == std::string::npos) return 0;
    pos += needle.size();
    size_t end = pos;
    while (end < json.size() && (std::isdigit(json[end]) || json[end] == '-')) ++end;
    if (end == pos) return 0;
    try {
        return std::stoull(json.substr(pos, end - pos));
    } catch (...) {
        return 0;
    }
}

std::string escapeJson(const std::string& s) {
    std::string out;
    out.reserve(s.size());
    for (char c : s) {
        switch (c) {
            case '"': out += "\\\""; break;
            case '\\': out += "\\\\"; break;
            case '\n': out += "\\n"; break;
            case '\r': out += "\\r"; break;
            case '\t': out += "\\t"; break;
            default: out += c; break;
        }
    }
    return out;
}

}  // namespace

UserStore::UserStore(std::string dbPath) : dbPath_(std::move(dbPath)) {
    load();
}

void UserStore::load() {
    std::lock_guard lock(mutex_);
    users_.clear();
    nextId_ = 10001;

    std::ifstream in(dbPath_);
    if (!in.is_open()) {
        Logger::info("User database not found, creating new: " + dbPath_);
        save();
        return;
    }

    std::ostringstream oss;
    oss << in.rdbuf();
    const std::string json = oss.str();
    if (json.empty()) return;

    nextId_ = static_cast<PlayerId>(extractJsonUintValue(json, "next_id"));
    if (nextId_ < 10001) nextId_ = 10001;

    size_t pos = 0;
    while ((pos = json.find("\"id\":", pos)) != std::string::npos) {
        UserRecord user;
        user.id = static_cast<PlayerId>(extractJsonUintValue(json.substr(pos), "id"));
        pos += 5;

        auto nickPos = json.find("\"nickname\":\"", pos);
        if (nickPos == std::string::npos) break;
        nickPos += 12;
        auto nickEnd = json.find('"', nickPos);
        if (nickEnd == std::string::npos) break;
        user.nickname = json.substr(nickPos, nickEnd - nickPos);

        user.password = extractJsonStringValue(json.substr(pos), "password");
        user.passwordHash = extractJsonStringValue(json.substr(pos), "password_hash");
        user.salt = extractJsonStringValue(json.substr(pos), "salt");
        user.stats.totalGames = static_cast<uint32_t>(
            extractJsonUintValue(json.substr(pos), "total_games"));
        user.stats.wins = static_cast<uint32_t>(extractJsonUintValue(json.substr(pos), "wins"));
        user.stats.losses = static_cast<uint32_t>(extractJsonUintValue(json.substr(pos), "losses"));

        if (user.id > 0 && !user.nickname.empty()) {
            users_.push_back(std::move(user));
        }
    }

    Logger::info("Loaded " + std::to_string(users_.size()) + " users from database");
}

void UserStore::save() const {
    std::ostringstream oss;
    oss << "{\n";
    oss << "  \"next_id\":" << nextId_ << ",\n";
    oss << "  \"users\":[\n";
    for (size_t i = 0; i < users_.size(); ++i) {
        const auto& u = users_[i];
        oss << "    {";
        oss << "\"id\":" << u.id;
        oss << ",\"nickname\":\"" << escapeJson(u.nickname) << "\"";
        oss << ",\"password\":\"" << escapeJson(u.password) << "\"";
        oss << ",\"password_hash\":\"" << escapeJson(u.passwordHash) << "\"";
        oss << ",\"salt\":\"" << escapeJson(u.salt) << "\"";
        oss << ",\"total_games\":" << u.stats.totalGames;
        oss << ",\"wins\":" << u.stats.wins;
        oss << ",\"losses\":" << u.stats.losses;
        oss << "}";
        if (i + 1 < users_.size()) oss << ",";
        oss << "\n";
    }
    oss << "  ]\n";
    oss << "}\n";

    std::ofstream out(dbPath_, std::ios::trunc);
    if (!out.is_open()) {
        Logger::error("Failed to save user database: " + dbPath_);
        return;
    }
    out << oss.str();
}

std::optional<UserRecord> UserStore::findByNickname(const std::string& nickname) const {
    std::lock_guard lock(mutex_);
    const std::string key = trim(nickname);
    for (const auto& u : users_) {
        if (u.nickname == key) return u;
    }
    return std::nullopt;
}

std::optional<UserRecord> UserStore::findById(PlayerId id) const {
    std::lock_guard lock(mutex_);
    for (const auto& u : users_) {
        if (u.id == id) return u;
    }
    return std::nullopt;
}

std::string UserStore::registerUser(const std::string& nickname, const std::string& password,
                                    const std::string& passwordHash, const std::string& salt) {
    std::lock_guard lock(mutex_);
    const std::string nick = trim(nickname);
    if (nick.size() < 2 || nick.size() > 16) {
        return "昵称长度需在 2-16 个字符之间";
    }
    for (const auto& u : users_) {
        if (u.nickname == nick) {
            return "昵称已被使用，请更换昵称";
        }
    }

    UserRecord user;
    user.id = nextId_++;
    user.nickname = nick;
    user.password = password;
    user.passwordHash = passwordHash;
    user.salt = salt;
    users_.push_back(std::move(user));
    save();
    return "";
}

bool UserStore::updateStats(PlayerId id, const UserStats& stats) {
    std::lock_guard lock(mutex_);
    for (auto& u : users_) {
        if (u.id == id) {
            u.stats = stats;
            save();
            return true;
        }
    }
    return false;
}

}  // namespace guandan
