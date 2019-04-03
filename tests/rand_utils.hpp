#pragma ide diagnostic ignored "readability-magic-numbers"

#pragma once

#include <chrono>
#include <random>

inline std::chrono::milliseconds randomDelay() {
    static std::random_device rd;
    static std::mt19937 generator(rd());
    static std::uniform_int_distribution<> distribution(1, 10);
    return std::chrono::milliseconds{distribution(generator)};
}

inline bool randomBool() {
    static std::random_device rd;
    static std::mt19937 generator(rd());
    static std::uniform_int_distribution<> distribution(0, 1);
    return distribution(generator) == 1;
}
