#include "auth/auth_service.h"
#include "auth/password_hash.h"
#include "user/avatar_storage.h"
#include "utils/sha1.h"

namespace guandan {

AuthService::AuthService(UserStore& userStore, TokenManager& tokenManager)
    : userStore_(userStore), tokenManager_(tokenManager) {}

std::string AuthService::validatePassword(const std::string& password) {
    if (password.size() < 6) {
        return "密码长度至少 6 位";
    }
    if (password.size() > 64) {
        return "密码长度不能超过 64 位";
    }
    return "";
}

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
    const std::string regErr = userStore_.registerUser(nickname, password, hash, salt);
    if (!regErr.empty()) {
        result.httpStatus = 409;
        result.errorCode = "nickname_taken";
        result.message = regErr;
        return result;
    }

    auto user = userStore_.findByNickname(nickname);
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

AuthResult AuthService::login(const std::string& nickname, const std::string& password) {
    AuthResult result;
    auto user = userStore_.findByNickname(nickname);
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

std::optional<UserRecord> AuthService::validateToken(const std::string& token) {
    auto userId = tokenManager_.validateToken(token);
    if (!userId) return std::nullopt;
    return userStore_.findById(*userId);
}

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

    if (!userStore_.updateAvatar(user->id, *avatarPath)) {
        result.httpStatus = 500;
        result.errorCode = "save_failed";
        result.message = "头像保存失败，请稍后重试";
        return result;
    }

    auto updated = userStore_.findById(user->id);
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

}  // namespace guandan
