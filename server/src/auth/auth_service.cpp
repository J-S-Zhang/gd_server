#include "auth/auth_service.h"
#include "auth/password_hash.h"
#include "user/avatar_storage.h"
#include "utils/sha1.h"

/// @file auth_service.cpp
/// @brief 认证服务：注册、登录、令牌校验与头像更新

namespace guandan {

/// 构造并注入用户存储与令牌管理器
AuthService::AuthService(UserRepository& userRepository, TokenManager& tokenManager)
    : userRepository_(userRepository), tokenManager_(tokenManager) {}

/// 校验密码长度等规则
std::string AuthService::validatePassword(const std::string& password) {
    if (password.size() < 6) {
        return "密码长度至少 6 位";
    }
    if (password.size() > 64) {
        return "密码长度不能超过 64 位";
    }
    return "";
}

/// 注册新用户
AuthResult AuthService::registerUser(const std::string& nickname,
                                     const std::string& password) {
    AuthResult result;
    const std::string pwdErr = validatePassword(password);
    if (!pwdErr.empty()) {
        result.httpStatus = 400;
        result.errorCode = "invalid_password";
        result.message = pwdErr;
        return result;
    }

    const std::string salt = generateSalt();
    const std::string hash = hashPassword(password, salt);
    const std::string regErr = userRepository_.registerUser(nickname, password, hash, salt);
    if (!regErr.empty()) {
        result.httpStatus = 409;
        result.errorCode = "nickname_taken";
        result.message = regErr;
        return result;
    }

    auto user = userRepository_.findByNickname(nickname);
    if (!user) {
        result.httpStatus = 500;
        result.errorCode = "internal_error";
        result.message = "注册失败，请稍后重试";
        return result;
    }

    result.success = true;
    result.httpStatus = 200;
    result.user = *user;
    result.token = tokenManager_.issueToken(user->id);
    return result;
}

/// 用户登录
AuthResult AuthService::login(const std::string& nickname, const std::string& password) {
    AuthResult result;
    auto user = userRepository_.findByNickname(nickname);
    if (!user || !verifyPassword(password, user->salt, user->passwordHash)) {
        result.httpStatus = 401;
        result.errorCode = "invalid_credentials";
        result.message = "昵称或密码错误";
        return result;
    }

    result.success = true;
    result.httpStatus = 200;
    result.user = *user;
    result.token = tokenManager_.issueToken(user->id);
    return result;
}

/// 校验令牌并返回用户
std::optional<UserRecord> AuthService::validateToken(const std::string& token) {
    auto userId = tokenManager_.validateToken(token);
    if (!userId) return std::nullopt;
    return userRepository_.findById(*userId);
}

/// 更新用户自定义头像
AvatarUpdateResult AuthService::updateAvatar(const std::string& token,
                                             const std::string& imageBase64,
                                             const std::string& format,
                                             const AvatarStorageConfig& config) {
    AvatarUpdateResult result;
    auto user = validateToken(token);
    if (!user) {
        result.httpStatus = 401;
        result.errorCode = "unauthorized";
        result.message = "请先登录";
        return result;
    }

    if (imageBase64.empty()) {
        result.httpStatus = 400;
        result.errorCode = "invalid_image";
        result.message = "头像数据无效";
        return result;
    }

    const std::vector<uint8_t> bytes = base64Decode(imageBase64);
    constexpr size_t kMaxAvatarBytes = 512 * 1024;
    if (bytes.empty() || bytes.size() > kMaxAvatarBytes) {
        result.httpStatus = 400;
        result.errorCode = "invalid_image";
        result.message = "头像大小需在 512KB 以内";
        return result;
    }

    const auto avatarPath = saveUserAvatar(config, user->id, bytes, format);
    if (!avatarPath) {
        result.httpStatus = 500;
        result.errorCode = "save_failed";
        result.message = "头像保存失败，请稍后重试";
        return result;
    }

    if (!userRepository_.updateAvatar(user->id, *avatarPath)) {
        result.httpStatus = 500;
        result.errorCode = "save_failed";
        result.message = "头像保存失败，请稍后重试";
        return result;
    }

    auto updated = userRepository_.findById(user->id);
    if (!updated) {
        result.httpStatus = 500;
        result.errorCode = "internal_error";
        result.message = "头像更新失败";
        return result;
    }

    result.success = true;
    result.httpStatus = 200;
    result.user = *updated;
    return result;
}

namespace {

/// 判断头像预设 ID 是否合法
bool isAllowedAvatarPreset(const std::string& presetId) {
    return presetId == "avatar_1" || presetId == "avatar_2" ||
           presetId == "avatar_3" || presetId == "avatar_4" ||
           presetId == "avatar_5";
}

}  // namespace

/// 更新用户预设头像
AvatarPresetUpdateResult AuthService::updateAvatarPreset(const std::string& token,
                                                       const std::string& presetId) {
    AvatarPresetUpdateResult result;
    auto user = validateToken(token);
    if (!user) {
        result.httpStatus = 401;
        result.errorCode = "unauthorized";
        result.message = "请先登录";
        return result;
    }

    if (!presetId.empty() && !isAllowedAvatarPreset(presetId)) {
        result.httpStatus = 400;
        result.errorCode = "invalid_preset";
        result.message = "头像预设无效";
        return result;
    }

    if (!userRepository_.updateAvatarPreset(user->id, presetId)) {
        result.httpStatus = 500;
        result.errorCode = "save_failed";
        result.message = "头像预设保存失败";
        return result;
    }

    auto updated = userRepository_.findById(user->id);
    if (!updated) {
        result.httpStatus = 500;
        result.errorCode = "internal_error";
        result.message = "头像预设更新失败";
        return result;
    }

    result.success = true;
    result.httpStatus = 200;
    result.user = *updated;
    return result;
}

}  // namespace guandan
