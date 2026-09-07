#pragma once

#include <iostream>
#include <cstdlib>

#define TEST(name) void name(); \
    struct name##_runner { name##_runner() { std::cout << "  " #name "... "; name(); std::cout << "OK\n"; } } name##_instance; \
    void name()

#define ASSERT(cond) do { if (!(cond)) { std::cerr << "\n  FAIL: " #cond << " at " << __FILE__ << ":" << __LINE__ << std::endl; std::exit(1); } } while(0)
