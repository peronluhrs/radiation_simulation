#pragma once
#include <random>
#include <cstdint>
#include "glm_simple.h" // garantit glm::vec3

struct RandomGenerator
{
    static thread_local std::mt19937 generator;
    static thread_local std::uniform_real_distribution<float> uniform;

    static void seed(std::uint64_t s) { generator.seed(static_cast<uint32_t>(s)); }
    static float random() { return uniform(generator); } // [0,1)
    static float randomRange(float a, float b) { return a + (b - a) * random(); }
    static glm::vec3 randomDirection(); // direction isotrope
};
