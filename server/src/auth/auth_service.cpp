#include "auth/auth_service.h"
#include "auth/password_hash.h"

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
    const std::string regErr = userStore_.registerUser(nickname, hash, salt);
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

}  // namespace guandan
