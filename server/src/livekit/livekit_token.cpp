#include "livekit/livekit_token.h"
#include "utils/hmac_sha256.h"
#include <chrono>
#include <sstream>

/// @file livekit_token.cpp
/// @brief 生成 LiveKit 房间访问 JWT

namespace guandan {

namespace {

/// jsonEscapeMinimal 辅助函数
std::string jsonEscapeMinimal(const std::string& s) {
    std::string out;
    out.reserve(s.size() + 8);
    for (char c : s) {
        switch (c) {
            case '"': out += "\\\""; break;
            case '\\': out += "\\\\"; break;
            default: out += c; break;
        }
    }
    return out;
}

}  // namespace

std::string createLiveKitAccessToken(const LiveKitConfig& config, const std::string& roomName,
                                     const std::string& identity, const std::string& displayName) {
    if (!config.enabled || config.apiKey.empty() || config.apiSecret.empty()) {
        return {};
    }

    const auto now = std::chrono::system_clock::now();
    const auto nowSec =
        std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch()).count();
    const int64_t iat = nowSec;
    const int64_t exp = nowSec + config.tokenTtlSeconds;

    const std::string header = R"({"alg":"HS256","typ":"JWT"})";
    std::ostringstream payload;
    payload << "{"
            << "\"iss\":\"" << jsonEscapeMinimal(config.apiKey) << "\","
            << "\"sub\":\"" << jsonEscapeMinimal(identity) << "\","
            << "\"iat\":" << iat << ","
            << "\"nbf\":" << iat << ","
            << "\"exp\":" << exp << ","
            << "\"name\":\"" << jsonEscapeMinimal(displayName) << "\","
            << "\"video\":{"
            << "\"room\":\"" << jsonEscapeMinimal(roomName) << "\","
            << "\"roomJoin\":true,"
            << "\"canPublish\":true,"
            << "\"canSubscribe\":true,"
            << "\"canPublishData\":true"
            << "}"
            << "}";

    const std::string headerB64 = base64UrlEncode(std::vector<uint8_t>(header.begin(), header.end()));
    const std::string payloadStr = payload.str();
    const std::string payloadB64 =
        base64UrlEncode(std::vector<uint8_t>(payloadStr.begin(), payloadStr.end()));

    const std::string signingInput = headerB64 + "." + payloadB64;
    const auto sig = hmacSha256(config.apiSecret, signingInput);
    const std::string sigB64 = base64UrlEncode(sig);

    return signingInput + "." + sigB64;
}

}  // namespace guandan
