#include "protocol/message.h"
#include <sstream>
#include <stdexcept>

namespace guandan {

static std::string extractJsonString(const std::string& json, const std::string& key) {
    auto pos = json.find("\"" + key + "\":\"");
    if (pos == std::string::npos) return "";
    pos += key.size() + 4;
    auto end = json.find('"', pos);
    if (end == std::string::npos) return "";
    return json.substr(pos, end - pos);
}

static uint64_t extractJsonUint(const std::string& json, const std::string& key) {
    auto pos = json.find("\"" + key + "\":");
    if (pos == std::string::npos) return 0;
    pos += key.size() + 3;
    size_t end = pos;
    while (end < json.size() && (isdigit(json[end]) || json[end] == '-')) ++end;
    if (end == pos) return 0;
    try {
        return std::stoull(json.substr(pos, end - pos));
    } catch (...) {
        return 0;
    }
}

static std::string extractDataObject(const std::string& json) {
    auto pos = json.find("\"data\":");
    if (pos == std::string::npos) return "{}";
    pos += 7;
    while (pos < json.size() && json[pos] == ' ') ++pos;
    if (pos >= json.size() || json[pos] != '{') return "{}";
    int depth = 0;
    size_t start = pos;
    for (; pos < json.size(); ++pos) {
        if (json[pos] == '{') ++depth;
        else if (json[pos] == '}') {
            --depth;
            if (depth == 0) return json.substr(start, pos - start + 1);
        }
    }
    return "{}";
}

std::vector<CardId> parseCardsArray(const std::string& json) {
    std::vector<CardId> cards;
    auto pos = json.find("\"cards\":");
    if (pos == std::string::npos) return cards;
    pos = json.find('[', pos);
    if (pos == std::string::npos) return cards;
    ++pos;
    while (pos < json.size() && json[pos] != ']') {
        while (pos < json.size() && (json[pos] == ' ' || json[pos] == ',')) ++pos;
        if (pos >= json.size() || json[pos] == ']') break;
        size_t end = pos;
        while (end < json.size() && isdigit(json[end])) ++end;
        if (end > pos) {
            cards.push_back(static_cast<CardId>(std::stoul(json.substr(pos, end - pos))));
        }
        pos = end;
    }
    return cards;
}

std::string messageToJson(const Message& msg) {
    std::ostringstream oss;
    oss << "{";
    oss << "\"protocol_version\":" << msg.protocolVersion;
    oss << ",\"type\":\"" << msg.type << "\"";
    if (msg.requestId > 0) oss << ",\"request_id\":" << msg.requestId;
    if (!msg.roomId.empty()) oss << ",\"room_id\":\"" << msg.roomId << "\"";
    if (msg.turnId > 0) oss << ",\"turn_id\":" << msg.turnId;
    if (msg.stateVersion > 0) oss << ",\"state_version\":" << msg.stateVersion;
    if (msg.errorCode != ErrorCode::OK) {
        oss << ",\"error_code\":" << static_cast<int>(msg.errorCode);
    }
    if (!msg.dataJson.empty()) {
        oss << ",\"data\":" << msg.dataJson;
    } else {
        oss << ",\"data\":{}";
    }
    oss << "}";
    return oss.str();
}

Message parseMessage(const std::string& json) {
    Message msg;
    msg.type = extractJsonString(json, "type");
    msg.roomId = extractJsonString(json, "room_id");
    if (msg.roomId.empty()) {
        msg.roomId = extractJsonString(extractDataObject(json), "room_id");
    }
    msg.requestId = extractJsonUint(json, "request_id");
    msg.turnId = extractJsonUint(json, "turn_id");
    msg.stateVersion = extractJsonUint(json, "state_version");
    msg.token = extractJsonString(json, "token");
    if (msg.token.empty()) {
        msg.token = extractJsonString(extractDataObject(json), "token");
    }
    msg.dataJson = extractDataObject(json);
    msg.cards = parseCardsArray(json);
    return msg;
}

}  // namespace guandan
