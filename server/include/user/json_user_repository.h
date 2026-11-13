#pragma once

#include "user/user_repository.h"
#include <mutex>
#include <string>
#include <vector>

namespace guandan {

/// 用户数据 JSON 文件存储（开发 / 无 libpq 时的回退）
class JsonUserRepository final : public UserRepository {
public:
    explicit JsonUserRepository(std::string dbPath);

    std::optional<UserRecord> findByNickname(const std::string& nickname) const override;
    std::optional<UserRecord> findById(PlayerId id) const override;
    std::string registerUser(const std::string& nickname, const std::string& password,
                             const std::string& passwordHash, const std::string& salt) override;
    bool updateStats(PlayerId id, const UserStats& stats) override;
    bool updateAvatar(PlayerId id, const std::string& avatarPath) override;
    bool updateAvatarPreset(PlayerId id, const std::string& presetId) override;

private:
    std::string dbPath_;
    mutable std::mutex mutex_;
    PlayerId nextId_ = 10001;
    std::vector<UserRecord> users_;

    void load();
    void save() const;
};

}  // namespace guandan
