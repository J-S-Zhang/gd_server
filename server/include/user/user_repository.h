#pragma once

#include "user/user.h"
#include <memory>
#include <optional>
#include <string>

namespace guandan {

/// 用户档案持久化抽象（JSON 或 PostgreSQL 实现）
class UserRepository {
public:
    virtual ~UserRepository() = default;

    virtual std::optional<UserRecord> findByNickname(const std::string& nickname) const = 0;
    virtual std::optional<UserRecord> findById(PlayerId id) const = 0;

    /// 成功返回空字符串，失败返回用户可见错误信息
    virtual std::string registerUser(const std::string& nickname, const std::string& password,
                                     const std::string& passwordHash, const std::string& salt) = 0;

    virtual bool updateStats(PlayerId id, const UserStats& stats) = 0;
    virtual bool updateAvatar(PlayerId id, const std::string& avatarPath) = 0;
    virtual bool updateAvatarPreset(PlayerId id, const std::string& presetId) = 0;
};

struct UserRepositoryConfig {
    /// 非空则优先使用 PostgreSQL（需编译期 GUANDAN_HAS_POSTGRES）
    std::string postgresUrl;
    /// 未配置或未编译 PG 时的 JSON 文件路径
    std::string jsonFallbackPath = "users.json";
};

/// 从环境变量与配置文件解析连接信息
bool loadUserRepositoryConfig(const std::string& configPath, UserRepositoryConfig& out);

/// 按配置创建实现；失败返回 nullptr
std::unique_ptr<UserRepository> createUserRepository(const UserRepositoryConfig& config);

}  // namespace guandan
