#include "utils/uuid.h"
#include "utils/random.h"
#include <sstream>
#include <iomanip>

namespace guandan {

std::string generateRoomId() {
    auto& rng = Random::instance().engine();
    std::uniform_int_distribution<int> dist(100000, 999999);
    return std::to_string(dist(rng));
}

std::string generateUuid() {
    auto& rng = Random::instance().engine();
    std::uniform_int_distribution<uint64_t> dist;
    std::ostringstream oss;
    oss << std::hex << std::setfill('0');
    for (int i = 0; i < 4; ++i) {
        oss << std::setw(8) << dist(rng);
        if (i < 3) oss << '-';
    }
    return oss.str();
}

}  // namespace guandan
