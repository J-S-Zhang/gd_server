#pragma once

#include "livekit/livekit_config.h"
#include <string>

namespace guandan {

/// 生成 LiveKit 访问 token（JWT，HS256）；失败返回空字符串
std::string createLiveKitAccessToken(const LiveKitConfig& config, const std::string& roomName,
                                     const std::string& identity, const std::string& displayName);

}  // namespace guandan
