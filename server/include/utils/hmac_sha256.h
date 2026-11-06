#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace guandan {

/// 计算 SHA-256 摘要
std::vector<uint8_t> sha256(const std::string& data);
/// 计算 HMAC-SHA256
std::vector<uint8_t> hmacSha256(const std::string& key, const std::string& data);
/// Base64 URL 安全编码（无填充）
std::string base64UrlEncode(const std::vector<uint8_t>& data);

}  // namespace guandan
