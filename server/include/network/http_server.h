#pragma once

#include "auth/auth_service.h"
#include "user/avatar_storage.h"
#include <functional>
#include <string>

namespace guandan {

/// 客户端版本检查信息（/api/version 等）
struct AppVersionInfo {
    std::string version = "1.0.0";  ///< 版本号字符串
    int versionCode = 1;              ///< 数字版本码
    std::string downloadUrl;          ///< 下载地址
    bool forceUpdate = false;         ///< 是否强制更新
    std::string changelog;            ///< 更新说明
};

/// 从 JSON 文件加载 AppVersionInfo
bool loadAppVersionInfo(const std::string& path, AppVersionInfo& out);

/// HTTP 服务：注册/登录、头像、版本等 REST 接口
class HttpServer {
public:
    using RequestHandler = std::function<void(const std::string& method, const std::string& path,
                                              const std::string& body)>;  ///< 通用请求处理（若扩展用）

    explicit HttpServer(int port);
    ~HttpServer();

    /// 注入认证服务
    void setAuthService(AuthService* authService) { authService_ = authService; }
    /// 设置应用版本信息
    void setAppVersionInfo(const AppVersionInfo& info) { appVersionInfo_ = info; }
    /// 设置头像存储配置
    void setAvatarStorageConfig(const AvatarStorageConfig& config) { avatarConfig_ = config; }
    /// 阻塞运行 HTTP 服务
    void run();
    /// 停止服务
    void stop();

private:
    int port_;                              ///< 监听端口
    AuthService* authService_ = nullptr;    ///< 认证服务
    AppVersionInfo appVersionInfo_;         ///< 版本信息
    AvatarStorageConfig avatarConfig_;      ///< 头像存储
    bool running_ = false;                  ///< 运行标志
};

}  // namespace guandan
