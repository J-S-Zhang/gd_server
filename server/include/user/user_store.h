#pragma once

#include "user/user.h"
#include <mutex>
#include <optional>
#include <string>
#include <vector>

namespace guandan {

class UserStore {
public:
    explicit UserStore(std::string dbPath);

    std::optional<UserRecord> findByNickname(const std::string& nickname) const;
    std::optional<UserRecord> findById(PlayerId id) const;

    /// @return empty string on success, error message on failure
    std::string registerUser(const std::string& nickname, const std::string& passwordHash,
                             const std::string& salt);

    bool updateStats(PlayerId id, const UserStats& stats);

private:
    std::string dbPath_;
    mutable std::mutex mutex_;
    PlayerId nextId_ = 10001;
    std::vector<UserRecord> users_;

    void load();
    void save() const;
};

}  // namespace guandan
