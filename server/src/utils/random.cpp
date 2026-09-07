#include "utils/random.h"

namespace guandan {

Random& Random::instance() {
    static Random inst;
    return inst;
}

Random::Random() : engine_(rd_()) {}

std::mt19937_64& Random::engine() {
    return engine_;
}

}  // namespace guandan
