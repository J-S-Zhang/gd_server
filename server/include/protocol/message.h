#pragma once

#include "game/types.h"
#include <string>
#include <vector>

namespace guandan {

struct Message {
    int protocolVersion = 1;
    std::string type;
    uint64_t requestId = 0;
    std::string roomId;
    uint64_t turnId = 0;
    uint64_t stateVersion = 0;
    ErrorCode errorCode = ErrorCode::OK;
    std::string dataJson;
    std::vector<CardId> cards;
    std::string token;
};

std::string messageToJson(const Message& msg);
Message parseMessage(const std::string& json);
std::vector<CardId> parseCardsArray(const std::string& json);

}  // namespace guandan
