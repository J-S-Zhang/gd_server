#pragma once

#include <random>

namespace guandan {

class Random {
public:
    static Random& instance();

    std::mt19937_64& engine();

private:
    Random();
    std::random_device rd_;
    std::mt19937_64 engine_;
};

}  // namespace guandan
