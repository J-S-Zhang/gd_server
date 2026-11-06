#pragma once

#include "auth/token_manager.h"
#include "user/avatar_storage.h"
#include "user/user.h"
#include "user/user_store.h"
#include <optional>
#include <string>

namespace guandan {

/// 上传自定义头像结果
struct AvatarUpdateResult {
    bool success = false;     ///< 是否成功
    int httpStatus = 400;     ///< 建议 HTTP 状态码
    std::string errorCode;    ///< 业务错误码
    std::string message;       ///< 说明信息
    UserRecord user;          ///< 更新后的用户记录
};

/// 设置预设头像结果
struct AvatarPresetUpdateResult {
    bool success = false;
    int httpStatus = 400;
    std::string errorCode;
    std::string message;
    UserRecord user;
};

/// 注册/登录统一结果
struct AuthResult {
    bool success = false;
    int httpStatus = 400;
    std::string errorCode;
    std::string message;
    UserRecord user;
    std::string token;  ///< 成功时颁发的 token
};

/// 用户注册、登录、token 校验与头像更新
class AuthService {
public:
    AuthService(UserStore& userStore, TokenManager& tokenManager);

    /// 注册新用户
    AuthResult registerUser(const std::string& nickname, const std::string& password);
    /// 登录
    AuthResult login(const std::string& nickname, const std::string& password);
    /// 校验 token 并返回用户，无效返回 nullopt
    std::optional<UserRecord> validateToken(const std::string& token);
    /// 更新 Base64 头像
    AvatarUpdateResult updateAvatar(const std::string& token, const std::string& imageBase64,
                                    const std::string& format, const AvatarStorageConfig& config);
    /// 更新预设头像 id
    AvatarPresetUpdateResult updateAvatarPreset(const std::string& token,
                                                const std::string& presetId);

private:
    UserStore& userStore_;       ///< 用户持久化
    TokenManager& tokenManager_; ///< token 管理

    /// 校验密码强度/格式，通过返回空字符串，否则返回错误说明
    static std::string validatePassword(const std::string& password);
};

}  // namespace guandan
