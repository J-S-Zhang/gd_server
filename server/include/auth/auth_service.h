#pragma once

#include "auth/token_manager.h"
#include "user/user.h"
#include "user/user_store.h"
#include <optional>
#include <string>

namespace guandan {

struct AuthResult {
    bool success = false;
    int httpStatus = 400;
    std::string errorCode;
    std::string message;
    UserRecord user;
    std::string token;
};

class AuthService {
public:
    AuthService(UserStore& userStore, TokenManager& tokenManager);

    AuthResult registerUser(const std::string& nickname, const std::string& password);
    AuthResult login(const std::string& nickname, const std::string& password);
    std::optional<UserRecord> validateToken(const std::string& token);

private:
    UserStore& userStore_;
    TokenManager& tokenManager_;

    static std::string validatePassword(const std::string& password);
};

}  // namespace guandan
