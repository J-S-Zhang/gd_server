#pragma once

#include "auth/auth_service.h"
#include <functional>
#include <string>

namespace guandan {

class HttpServer {
public:
    using RequestHandler = std::function<void(const std::string& method, const std::string& path,
                                              const std::string& body)>;

    explicit HttpServer(int port);
    ~HttpServer();

    void setAuthService(AuthService* authService) { authService_ = authService; }
    void run();
    void stop();

private:
    int port_;
    AuthService* authService_ = nullptr;
    bool running_ = false;
};

}  // namespace guandan
