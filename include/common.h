#pragma once

#include <memory>
#include <vector>
#include <string>
#include <map>
#include <unordered_map>
#include <functional>
#include <random>
#include <atomic>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <array>
#include <cmath>
#include <limits>
#include <stack>
#include <iomanip>

#include "glm_simple.h"
#include "utils/Random.h" // API unique pour le hasard (random(), randomRange(), randomDirection())

// ----------------------------
// Types de radiation supportés
// ----------------------------
enum class RadiationType
{
    GAMMA,
    NEUTRON,
    MUON,
    X_RAY,
    BETA,
    ALPHA
};

// -----------------
// Types d'interaction
// -----------------
enum class InteractionType
{
    ABSORPTION,
    SCATTERING,
    TRANSMISSION,
    CAPTURE
};

// -----------------
// Forward declarations
// -----------------
class Object3D;
class Material;
class Sensor;
class Source;
class Particle;
class Scene;
class MonteCarloEngine;

// -----------------
// Constantes physiques
// -----------------
namespace Physics
{
    constexpr double AVOGADRO = 6.02214076e23;
    constexpr double SPEED_OF_LIGHT = 2.99792458e8;     // m/s
    constexpr double PLANCK = 6.62607015e-34;           // J·s
    constexpr double ELECTRON_CHARGE = 1.602176634e-19; // C
}

// -------------------------------------
// Structure pour les résultats d'intersection
// -------------------------------------
struct IntersectionResult
{
    bool hit = false;
    float distance = std::numeric_limits<float>::max();
    glm::vec3 point{0.0f};
    glm::vec3 normal{0.0f};
    std::shared_ptr<Object3D> object = nullptr;
    std::shared_ptr<Material> material = nullptr;
};

// -----------------
// Structure pour les rayons
// -----------------
struct Ray
{
    glm::vec3 origin{0.0f};
    glm::vec3 direction{0.0f, 0.0f, 1.0f};
    float tMin = 0.001f;
    float tMax = std::numeric_limits<float>::max();

    Ray() = default;
    Ray(const glm::vec3 &o, const glm::vec3 &d)
        : origin(o), direction(glm::normalize(d)) {}

    glm::vec3 at(float t) const { return origin + t * direction; }
};

// --------------
// Macros utiles
// --------------
#define EPSILON 1e-6f
#define PI 3.14159265359f
#define TWO_PI 6.28318530718f
#define HALF_PI 1.57079632679f

// --------------
// Logging simple
// --------------
namespace Log
{
    void info(const std::string &message);
    void warning(const std::string &message);
    void error(const std::string &message);
}
