#include "Random.h"

#include <algorithm>
#include <chrono>
#include <random>

namespace {

// 函数内静态对象：random_device 在部分 MinGW 实现里每次进程启动可能返回相同值，
// 因此再混入高精度时钟，保证每次启动的随机序列不同。
std::mt19937& engine() {
    static std::mt19937 generator = [] {
        std::random_device device;
        const auto now = std::chrono::high_resolution_clock::now().time_since_epoch().count();
        std::seed_seq seed{
            device(),
            device(),
            static_cast<unsigned int>(now & 0xFFFFFFFFu),
            static_cast<unsigned int>((now >> 32) & 0xFFFFFFFFu)
        };
        return std::mt19937(seed);
    }();
    return generator;
}

}  // namespace

namespace Random {

int integer(int minValue, int maxValue) {
    if (minValue > maxValue) {
        std::swap(minValue, maxValue);
    }
    std::uniform_int_distribution<int> distribution(minValue, maxValue);
    return distribution(engine());
}

double real(double minValue, double maxValue) {
    if (minValue > maxValue) {
        std::swap(minValue, maxValue);
    }
    std::uniform_real_distribution<double> distribution(minValue, maxValue);
    return distribution(engine());
}

bool chance(double probability) {
    if (probability <= 0.0) {
        return false;
    }
    if (probability >= 1.0) {
        return true;
    }
    std::bernoulli_distribution distribution(probability);
    return distribution(engine());
}

}  // namespace Random
