#pragma once

#include "auth/auth_service.h"
#include <functional>
#include <string>

namespace guandan {

struct AppVersionInfo {
    std::string version = "1.0.0";
    int versionCode = 1;
    std::string downloadUrl;
    bool forceUpdate = false;
    std::string changelog;
};

bool loadAppVersionInfo(const std::string& path, AppVersionInfo& out);

class HttpServer {
public:
    using RequestHandler = std::function<void(const std::string& method, const std::string& path,
                                              const std::string& body)>;

    explicit HttpServer(int port);
    ~HttpServer();

    void setAuthService(AuthService* authService) { authService_ = authService; }
    void setAppVersionInfo(const AppVersionInfo& info) { appVersionInfo_ = info; }
    void run();
    void stop();

private:
    int port_;
    AuthService* authService_ = nullptr;
    AppVersionInfo appVersionInfo_;
    bool running_ = false;
};

}  // namespace guandan
