#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace guandan {

std::string sha1Hex(const std::string& input);
std::string base64Encode(const std::vector<uint8_t>& data);

}  // namespace guandan
