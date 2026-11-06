#include "livekit/livekit_config.h"
#include "utils/logger.h"
#include <cctype>
#include <cstdint>
#include <fstream>
#include <sstream>

/// @file livekit_config.cpp
/// @brief 从 JSON 加载 LiveKit 配置

namespace guandan {

namespace {

/// 从 JSON 文本解析字符串字段
std::string extractJsonStringValue(const std::string& json, const std::string& key) {
    const std::string needle = "\"" + key + "\":";
    auto pos = json.find(needle);
    if (pos == std::string::npos) return "";
    pos += needle.size();
    while (pos < json.size() && std::isspace(static_cast<unsigned char>(json[pos]))) {
        ++pos;
    }
    if (pos >= json.size() || json[pos] != '"') return "";
    ++pos;
    auto end = pos;
    while (end < json.size()) {
        if (json[end] == '\\' && end + 1 < json.size()) {
            end += 2;
            continue;
        }
        if (json[end] == '"') break;
        ++end;
    }
    if (end >= json.size()) return "";
    return json.substr(pos, end - pos);
}

/// extractJsonIntValue 辅助函数
int64_t extractJsonIntValue(const std::string& json, const std::string& key, int64_t defaultValue) {
    const std::string needle = "\"" + key + "\":";
    auto pos = json.find(needle);
    if (pos == std::string::npos) return defaultValue;
    pos += needle.size();
    size_t end = pos;
    while (end < json.size() && (std::isdigit(json[end]) || json[end] == '-')) ++end;
    if (end == pos) return defaultValue;
    try {
        return std::stoll(json.substr(pos, end - pos));
    } catch (...) {
        return defaultValue;
    }
}

/// extractJsonBoolValue 辅助函数
bool extractJsonBoolValue(const std::string& json, const std::string& key, bool defaultValue) {
    const std::string needle = "\"" + key + "\":";
    auto pos = json.find(needle);
    if (pos == std::string::npos) return defaultValue;
    pos += needle.size();
    while (pos < json.size() && std::isspace(static_cast<unsigned char>(json[pos]))) {
        ++pos;
    }
    if (json.compare(pos, 4, "true") == 0) return true;
    if (json.compare(pos, 5, "false") == 0) return false;
    return defaultValue;
}

}  // namespace

bool loadLiveKitConfig(const std::string& path, LiveKitConfig& out) {
    std::ifstream file(path);
    if (!file.is_open()) {
        Logger::info("LiveKit config not found: " + path + " (voice_join disabled)");
        return false;
    }

    std::ostringstream buffer;
    buffer << file.rdbuf();
    const std::string json = buffer.str();

    out.enabled = extractJsonBoolValue(json, "enabled", out.enabled);
    const std::string apiKey = extractJsonStringValue(json, "api_key");
    if (!apiKey.empty()) {
        out.apiKey = apiKey;
    }
    const std::string apiSecret = extractJsonStringValue(json, "api_secret");
    if (!apiSecret.empty()) {
        out.apiSecret = apiSecret;
    }
    const std::string url = extractJsonStringValue(json, "url");
    if (!url.empty()) {
        out.url = url;
    }
    out.tokenTtlSeconds =
        static_cast<int>(extractJsonIntValue(json, "token_ttl_seconds", out.tokenTtlSeconds));

    if (out.enabled && (out.apiKey.empty() || out.apiSecret.empty())) {
        Logger::error("LiveKit enabled but api_key or api_secret missing in " + path);
        out.enabled = false;
        return false;
    }

    /// 实现 info 相关逻辑
    Logger::info("LiveKit config loaded: enabled=" + std::string(out.enabled ? "true" : "false") +
                 (out.url.empty() ? " (client resolves LiveKit host from login IP)"
                                  : " url=" + out.url));
    return true;
}

}  // namespace guandan
