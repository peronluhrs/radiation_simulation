#pragma once

#include "common.h"

// État d'une particule
enum class ParticleState {
    ACTIVE,     // Particule en transport
    ABSORBED,   // Absorbée par le matériau
    DETECTED,   // Détectée par un capteur
    ESCAPED,    // Sortie de la géométrie
    SCATTERED   // Diffusée
};

class Particle {
public:
    Particle() = default;
    Particle(RadiationType type, float energy, const glm::vec3& position, const glm::vec3& direction);
    
    // Propriétés de base
    RadiationType getType() const { return m_type; }
    void setType(RadiationType type) { m_type = type; }
    
    float getEnergy() const { return m_energy; }
    void setEnergy(float energy) { m_energy = std::max(0.0f, energy); }
    
    const glm::vec3& getPosition() const { return m_position; }
    void setPosition(const glm::vec3& position) { m_position = position; }
    
    const glm::vec3& getDirection() const { return m_direction; }
    void setDirection(const glm::vec3& direction) { m_direction = glm::normalize(direction); }
    
    ParticleState getState() const { return m_state; }
    void setState(ParticleState state) { m_state = state; }
    
    // Poids statistique (pour les techniques de réduction de variance)
    float getWeight() const { return m_weight; }
    void setWeight(float weight) { m_weight = std::max(0.0f, weight); }
    
    // Historique de transport
    uint32_t getGeneration() const { return m_generation; }
    void setGeneration(uint32_t generation) { m_generation = generation; }
    
    float getAge() const { return m_age; }
    void incrementAge(float dt) { m_age += dt; }
    
    float getTravelDistance() const { return m_travelDistance; }
    void incrementTravelDistance(float distance) { m_travelDistance += distance; }
    
    uint32_t getCollisionCount() const { return m_collisionCount; }
    void incrementCollisionCount() { ++m_collisionCount; }
    
    // Matériau actuel
    std::shared_ptr<Material> getCurrentMaterial() const { return m_currentMaterial; }
    void setCurrentMaterial(std::shared_ptr<Material> material) { m_currentMaterial = material; }
    
    // Transport
    void move(float distance);
    Ray getRay() const { return Ray(m_position, m_direction); }
    
    // Interactions
    void scatter(const glm::vec3& newDirection, float energyLoss = 0.0f);
    void absorb() { m_state = ParticleState::ABSORBED; m_energy = 0.0f; }
    void detect() { m_state = ParticleState::DETECTED; }
    void escape() { m_state = ParticleState::ESCAPED; }
    
    // États
    bool isActive() const { return m_state == ParticleState::ACTIVE && m_energy > 0.0f; }
    bool isAlive() const { return m_state != ParticleState::ABSORBED; }
    
    // Calculs dérivés
    float getVelocity() const; // m/s
    float getMomentum() const; // keV/c
    float getRestMass() const; // keV/c²
    
    // Propriétés spécifiques au type de particule
    int getCharge() const;
    float getMassNumber() const;
    
    // Sérialisation pour debugging
    std::string toString() const;

private:
    RadiationType m_type = RadiationType::GAMMA;
    float m_energy = 1000.0f; // keV
    glm::vec3 m_position{0.0f};
    glm::vec3 m_direction{0.0f, 0.0f, 1.0f};
    ParticleState m_state = ParticleState::ACTIVE;
    float m_weight = 1.0f;
    
    // Historique
    uint32_t m_generation = 0;
    float m_age = 0.0f; // ns
    float m_travelDistance = 0.0f; // cm
    uint32_t m_collisionCount = 0;
    
    // Contexte matériau
    std::shared_ptr<Material> m_currentMaterial;
};

// Factory pour création de particules
class ParticleFactory {
public:
    static Particle createGamma(float energy, const glm::vec3& position, const glm::vec3& direction);
    static Particle createNeutron(float energy, const glm::vec3& position, const glm::vec3& direction);
    static Particle createMuon(float energy, const glm::vec3& position, const glm::vec3& direction);
    static Particle createXRay(float energy, const glm::vec3& position, const glm::vec3& direction);
    static Particle createBeta(float energy, const glm::vec3& position, const glm::vec3& direction);
    static Particle createAlpha(float energy, const glm::vec3& position, const glm::vec3& direction);
    
    // Particules cosmiques
    static Particle createCosmicMuon(const glm::vec3& position);
    static Particle createCosmicGamma(const glm::vec3& position);
    
    // Particules de fond
    static Particle createBackgroundGamma(const glm::vec3& position);
    static Particle createRadonDecay(const glm::vec3& position);
};

// Pool de particules pour éviter les allocations
class ParticlePool {
public:
    static ParticlePool& getInstance();
    
    Particle* acquire();
    void release(Particle* particle);
    void clear();
    
    size_t getPoolSize() const { return m_pool.size(); }
    size_t getActiveCount() const { return m_activeCount; }

private:
    ParticlePool() = default;
    std::vector<std::unique_ptr<Particle>> m_pool;
    std::stack<Particle*> m_available;
    std::atomic<size_t> m_activeCount{0};
    std::mutex m_mutex;
};