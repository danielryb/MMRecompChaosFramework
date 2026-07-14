#include "global.h"

#include <random>

std::random_device rd;
std::mt19937 generator(rd());

void Rand_Seed(u32 seed) {
    // srand(seed);
}

f32 Rand_ZeroOne(void) {
    return (double)generator() / generator.max();
}