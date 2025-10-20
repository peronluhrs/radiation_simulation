#include "common.h"
#include <random>
#include <iostream>

// Implémentation des fonctions utilitaires mathématiques

// Générateurs aléatoires thread-local

// glm::vec3 RandomGenerator::randomDirection() {
//     // Méthode de Marsaglia pour génération uniforme sur la sphère
//     float z = 2.0f * random() - 1.0f;
//     float phi = TWO_PI * random();
//     float r = std::sqrt(1.0f - z * z);
//     return glm::vec3(r * std::cos(phi), r * std::sin(phi), z);
// }

// glm::vec3 RandomGenerator::randomHemisphere(const glm::vec3& normal) {
//     glm::vec3 dir = randomDirection();
//     return glm::dot(dir, normal) > 0.0f ? dir : -dir;
// }

// Implémentation des fonctions de logging
namespace Log
{
    void info(const std::string &message)
    {
        std::cout << "[INFO] " << message << std::endl;
    }

    void warning(const std::string &message)
    {
        std::cout << "[WARNING] " << message << std::endl;
    }

    void error(const std::string &message)
    {
        std::cerr << "[ERROR] " << message << std::endl;
    }
}