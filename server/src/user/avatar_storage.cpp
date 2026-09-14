#include "user/avatar_storage.h"
#include "utils/logger.h"

#include <cctype>
#include <filesystem>
#include <fstream>

namespace fs = std::filesystem;

namespace guandan {

namespace {

std::string normalizeFormat(const std::string& format) {
    std::string lower = format;
    for (char& c : lower) {
        c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    }
    if (lower == "jpg" || lower == "jpeg") return "jpg";
    if (lower == "png") return "png";
    if (lower == "webp") return "webp";
    return "";
}

std::string normalizePrefix(std::string prefix) {
    while (!prefix.empty() && prefix.back() == '/') {
        prefix.pop_back();
    }
    if (prefix.empty() || prefix.front() != '/') {
        prefix.insert(prefix.begin(), '/');
    }
    return prefix;
}

}  // namespace

std::optional<std::string> saveUserAvatar(const AvatarStorageConfig& config, PlayerId userId,
                                          const std::vector<uint8_t>& data,
                                          const std::string& format) {
    const std::string ext = normalizeFormat(format);
    if (ext.empty() || data.empty()) {
        return std::nullopt;
    }

    std::error_code ec;
    fs::create_directories(config.storageDir, ec);
    if (ec) {
        Logger::error("Avatar: failed to create directory " + config.storageDir + ": " + ec.message());
        return std::nullopt;
    }

    const std::string fileName = "user_" + std::to_string(userId) + "." + ext;
    const fs::path filePath = fs::path(config.storageDir) / fileName;
    std::ofstream out(filePath, std::ios::binary | std::ios::trunc);
    if (!out.is_open()) {
        Logger::error("Avatar: failed to open file for writing: " + filePath.string());
        return std::nullopt;
    }
    out.write(reinterpret_cast<const char*>(data.data()),
              static_cast<std::streamsize>(data.size()));
    if (!out.good()) {
        Logger::error("Avatar: failed to write file: " + filePath.string());
        return std::nullopt;
    }

    const std::string prefix = normalizePrefix(config.publicPathPrefix);
    return prefix + "/" + fileName;
}

}  // namespace guandan
