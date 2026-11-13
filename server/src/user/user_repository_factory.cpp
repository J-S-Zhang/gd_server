#include "user/user_repository.h"
#include "user/json_user_repository.h"
#include "user/postgres_user_repository.h"
#include "utils/logger.h"

#include <cctype>
#include <cstdlib>
#include <fstream>
#include <sstream>

/// @file user_repository_factory.cpp
/// @brief 用户存储配置加载与工厂

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
    const std::string needle = "\"" + key + "\":";
    auto pos = json.find(needle);
    if (pos == std::string::npos) return "";
    pos += needle.size();
    while (pos < json.size() && std::isspace(static_cast<unsigned char>(json[pos]))) ++pos;
    if (pos >= json.size() || json[pos] != '"') return "";
    ++pos;
    auto end = pos;
    while (end < json.size()) {
        if (json[end] == '\\' && end + 1 < json.size()) {
            end += 2;
            continue;
        }
        if (json[end] == '"') break;
        ++end;
    }
    if (end >= json.size()) return "";
    return json.substr(pos, end - pos);
}

std::string envOrEmpty(const char* name) {
    if (const char* v = std::getenv(name)) {
        return trim(v);
    }
    return "";
}

}  // namespace

bool loadUserRepositoryConfig(const std::string& configPath, UserRepositoryConfig& out) {
    out = UserRepositoryConfig{};

    const std::string fromEnv = envOrEmpty("DATABASE_URL");
    if (!fromEnv.empty()) {
        out.postgresUrl = fromEnv;
    }

    std::ifstream in(configPath);
    if (in.is_open()) {
        std::ostringstream oss;
        oss << in.rdbuf();
        const std::string json = oss.str();
        if (out.postgresUrl.empty()) {
            out.postgresUrl = extractJsonStringValue(json, "postgres_url");
        }
        const std::string jsonPath = extractJsonStringValue(json, "json_fallback_path");
        if (!jsonPath.empty()) {
            out.jsonFallbackPath = jsonPath;
        }
    } else if (configPath.find("example") == std::string::npos) {
        Logger::info("用户库配置未找到: " + configPath);
    }

    return !out.postgresUrl.empty() || !out.jsonFallbackPath.empty();
}

std::unique_ptr<UserRepository> createUserRepository(const UserRepositoryConfig& config) {
#ifdef GUANDAN_HAS_POSTGRES
    if (!config.postgresUrl.empty()) {
        try {
            auto repo = std::make_unique<PostgresUserRepository>(config.postgresUrl);
            Logger::info("用户持久化: PostgreSQL");
            return repo;
        } catch (const std::exception& e) {
            Logger::error(std::string("PostgreSQL 初始化失败: ") + e.what());
            return nullptr;
        }
    }
#else
    if (!config.postgresUrl.empty()) {
        Logger::error("已配置 postgres_url 但构建未启用 GUANDAN_HAS_POSTGRES（需 libpq）");
        return nullptr;
    }
#endif

    if (config.jsonFallbackPath.empty()) {
        Logger::error("未配置 postgres_url 且 json_fallback_path 为空");
        return nullptr;
    }

    Logger::info("用户持久化: JSON 文件 " + config.jsonFallbackPath);
    return std::make_unique<JsonUserRepository>(config.jsonFallbackPath);
}

}  // namespace guandan
