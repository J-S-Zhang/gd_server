#pragma once

#include <string>

namespace guandan {

/// LiveKit 实时音视频配置
struct LiveKitConfig {
    bool enabled = false;           ///< 是否启用 LiveKit
    std::string apiKey;             ///< API Key
    std::string apiSecret;          ///< API Secret
    /// 可选；客户端按登录公网 IP + LIVEKIT_PORT 拼接 WS 地址，一般无需配置。
    std::string url;
    int tokenTtlSeconds = 3600;     ///< 访问 token 有效期（秒）
};

/// 从配置文件加载 LiveKitConfig
bool loadLiveKitConfig(const std::string& path, LiveKitConfig& out);

}  // namespace guandan
