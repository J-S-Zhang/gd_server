#pragma once

#include "user/user.h"
#include <mutex>
#include <optional>
#include <string>
#include <vector>

namespace guandan {

/// 用户数据 JSON 文件存储
class UserStore {
public:
    explicit UserStore(std::string dbPath);

    /// 按昵称查找
    std::optional<UserRecord> findByNickname(const std::string& nickname) const;
    /// 按 id 查找
    std::optional<UserRecord> findById(PlayerId id) const;

    /// 注册新用户；成功返回空字符串，失败返回错误信息
    std::string registerUser(const std::string& nickname, const std::string& password,
                             const std::string& passwordHash, const std::string& salt);

    /// 更新战绩
    bool updateStats(PlayerId id, const UserStats& stats);

    /// 更新头像路径
    bool updateAvatar(PlayerId id, const std::string& avatarPath);

    /// 更新头像预设 id
    bool updateAvatarPreset(PlayerId id, const std::string& presetId);

private:
    std::string dbPath_;              ///< 数据库 JSON 文件路径
    mutable std::mutex mutex_;        ///< 读写锁
    PlayerId nextId_ = 10001;       ///< 下一个分配的用户 id
    std::vector<UserRecord> users_;  ///< 内存中的用户列表

    /// 从磁盘加载
    void load();
    /// 持久化到磁盘
    void save() const;
};

}  // namespace guandan
