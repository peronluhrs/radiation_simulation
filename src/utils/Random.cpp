#include "utils/Random.h"
#include <chrono>
#include <cmath>

thread_local std::mt19937 RandomGenerator::generator{
    static_cast<uint32_t>(
        std::chrono::high_resolution_clock::now().time_since_epoch().count())};
thread_local std::uniform_real_distribution<float> RandomGenerator::uniform{0.0f, 1.0f};

glm::vec3 RandomGenerator::randomDirection()
{
    float u = random();                 // [0,1)
    float v = random();                 // [0,1)
    float z = 2.f * u - 1.f;            // cos(theta) in [-1,1]
    float phi = 6.283185307179586f * v; // 2*pi * v
    float t = std::sqrt(std::max(0.f, 1.f - z * z));
    float x = t * std::cos(phi);
    float y = t * std::sin(phi);
    return glm::vec3(x, y, z);
}
