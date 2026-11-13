#pragma once

#include "user/user_repository.h"
#include <mutex>
#include <string>

namespace guandan {

/// PostgreSQL 用户表读写（libpq）
class PostgresUserRepository final : public UserRepository {
public:
    explicit PostgresUserRepository(std::string connectionUrl);
    ~PostgresUserRepository() override;

    PostgresUserRepository(const PostgresUserRepository&) = delete;
    PostgresUserRepository& operator=(const PostgresUserRepository&) = delete;

    std::optional<UserRecord> findByNickname(const std::string& nickname) const override;
    std::optional<UserRecord> findById(PlayerId id) const override;
    std::string registerUser(const std::string& nickname, const std::string& password,
                             const std::string& passwordHash, const std::string& salt) override;
    bool updateStats(PlayerId id, const UserStats& stats) override;
    bool updateAvatar(PlayerId id, const std::string& avatarPath) override;
    bool updateAvatarPreset(PlayerId id, const std::string& presetId) override;

private:
    std::string connectionUrl_;
    mutable std::mutex mutex_;
    /// PGconn*，头文件不依赖 libpq
    mutable void* conn_ = nullptr;

    bool ensureConnected() const;
    void disconnect() const;
    static std::string trimNickname(const std::string& nickname);
};

}  // namespace guandan
