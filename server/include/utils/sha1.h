#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace guandan {

/// SHA-1 十六进制字符串
std::string sha1Hex(const std::string& input);
/// 标准 Base64 编码
std::string base64Encode(const std::vector<uint8_t>& data);
/// 标准 Base64 解码
std::vector<uint8_t> base64Decode(const std::string& encoded);

}  // namespace guandan
