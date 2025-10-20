#include "utils/Random.h"
#include "simulation/Particle.h"

Particle::Particle(RadiationType type, float energy, const glm::vec3& position, const glm::vec3& direction)
    : m_type(type), m_energy(energy), m_position(position), m_direction(glm::normalize(direction)) {
}

void Particle::move(float distance) {
    m_position += m_direction * distance;
    m_travelDistance += distance;
    
    // Mise à jour de l'âge (approximation simple)
    float velocity = getVelocity();
    if (velocity > 0.0f) {
        m_age += distance / velocity * 1e9f; // conversion en ns
    }
}

void Particle::scatter(const glm::vec3& newDirection, float energyLoss) {
    m_direction = glm::normalize(newDirection);
    m_energy = std::max(0.0f, m_energy - energyLoss);
    m_collisionCount++;
    m_state = ParticleState::SCATTERED;
    
    // Redevient active si elle a encore de l'énergie
    if (m_energy > 0.0f) {
        m_state = ParticleState::ACTIVE;
    }
}

float Particle::getVelocity() const {
    // Calcul simplifié de la vitesse selon le type de particule
    switch (m_type) {
        case RadiationType::GAMMA:
        case RadiationType::X_RAY:
            return Physics::SPEED_OF_LIGHT; // Photons à c
            
        case RadiationType::NEUTRON: {
            // Neutrons : E = 1/2 * m * v²
            // v = sqrt(2E/m), avec m = 939.6 MeV/c²
            float restMass = 939600.0f; // keV/c²
            return std::sqrt(2.0f * m_energy / restMass) * Physics::SPEED_OF_LIGHT;
        }
        
        case RadiationType::MUON: {
            // Muons relativistes
            float restMass = 105700.0f; // keV/c²
            float gamma = (m_energy + restMass) / restMass;
            float beta = std::sqrt(1.0f - 1.0f / (gamma * gamma));
            return beta * Physics::SPEED_OF_LIGHT;
        }
        
        case RadiationType::BETA: {
            // Électrons/positrons
            float restMass = 511.0f; // keV/c²
            float gamma = (m_energy + restMass) / restMass;
            float beta = std::sqrt(1.0f - 1.0f / (gamma * gamma));
            return beta * Physics::SPEED_OF_LIGHT;
        }
        
        case RadiationType::ALPHA: {
            // Particules alpha (He-4)
            float restMass = 3728000.0f; // keV/c²
            return std::sqrt(2.0f * m_energy / restMass) * Physics::SPEED_OF_LIGHT;
        }
        
        default:
            return 0.1f * Physics::SPEED_OF_LIGHT; // Valeur par défaut
    }
}

float Particle::getMomentum() const {
    // p = E/c pour les photons, calcul relativiste pour les autres
    switch (m_type) {
        case RadiationType::GAMMA:
        case RadiationType::X_RAY:
            return m_energy; // keV/c
            
        default: {
            float restMass = getRestMass();
            float totalEnergy = m_energy + restMass;
            return std::sqrt(totalEnergy * totalEnergy - restMass * restMass);
        }
    }
}

float Particle::getRestMass() const {
    switch (m_type) {
        case RadiationType::GAMMA:
        case RadiationType::X_RAY:
            return 0.0f; // Sans masse
            
        case RadiationType::NEUTRON:
            return 939600.0f; // keV/c²
            
        case RadiationType::MUON:
            return 105700.0f; // keV/c²
            
        case RadiationType::BETA:
            return 511.0f; // keV/c²
            
        case RadiationType::ALPHA:
            return 3728000.0f; // keV/c²
            
        default:
            return 1.0f;
    }
}

int Particle::getCharge() const {
    switch (m_type) {
        case RadiationType::GAMMA:
        case RadiationType::X_RAY:
        case RadiationType::NEUTRON:
            return 0; // Neutres
            
        case RadiationType::MUON:
            return -1; // Muon négatif (généralement)
            
        case RadiationType::BETA:
            return -1; // Électron (beta-)
            
        case RadiationType::ALPHA:
            return 2; // He++
            
        default:
            return 0;
    }
}

float Particle::getMassNumber() const {
    switch (m_type) {
        case RadiationType::NEUTRON:
            return 1.0f;
            
        case RadiationType::ALPHA:
            return 4.0f; // He-4
            
        default:
            return 0.0f; // Leptons ou photons
    }
}

std::string Particle::toString() const {
    std::string typeStr;
    switch (m_type) {
        case RadiationType::GAMMA: typeStr = "Gamma"; break;
        case RadiationType::NEUTRON: typeStr = "Neutron"; break;
        case RadiationType::MUON: typeStr = "Muon"; break;
        case RadiationType::X_RAY: typeStr = "X-Ray"; break;
        case RadiationType::BETA: typeStr = "Beta"; break;
        case RadiationType::ALPHA: typeStr = "Alpha"; break;
    }
    
    std::string stateStr;
    switch (m_state) {
        case ParticleState::ACTIVE: stateStr = "Active"; break;
        case ParticleState::ABSORBED: stateStr = "Absorbed"; break;
        case ParticleState::DETECTED: stateStr = "Detected"; break;
        case ParticleState::ESCAPED: stateStr = "Escaped"; break;
        case ParticleState::SCATTERED: stateStr = "Scattered"; break;
    }
    
    return typeStr + " (" + stateStr + ", " + std::to_string(m_energy) + " keV)";
}

// ParticleFactory implémentations
Particle ParticleFactory::createGamma(float energy, const glm::vec3& position, const glm::vec3& direction) {
    return Particle(RadiationType::GAMMA, energy, position, direction);
}

Particle ParticleFactory::createNeutron(float energy, const glm::vec3& position, const glm::vec3& direction) {
    return Particle(RadiationType::NEUTRON, energy, position, direction);
}

Particle ParticleFactory::createMuon(float energy, const glm::vec3& position, const glm::vec3& direction) {
    return Particle(RadiationType::MUON, energy, position, direction);
}

Particle ParticleFactory::createXRay(float energy, const glm::vec3& position, const glm::vec3& direction) {
    return Particle(RadiationType::X_RAY, energy, position, direction);
}

Particle ParticleFactory::createBeta(float energy, const glm::vec3& position, const glm::vec3& direction) {
    return Particle(RadiationType::BETA, energy, position, direction);
}

Particle ParticleFactory::createAlpha(float energy, const glm::vec3& position, const glm::vec3& direction) {
    return Particle(RadiationType::ALPHA, energy, position, direction);
}

Particle ParticleFactory::createCosmicMuon(const glm::vec3& position) {
    // Muon cosmique typique : énergie élevée, direction vers le bas
    float energy = RandomGenerator::randomRange(1000.0f, 1000000.0f); // 1 MeV - 1 GeV
    glm::vec3 direction = glm::vec3(
        RandomGenerator::randomRange(-0.2f, 0.2f),  // Légère déviation
        -1.0f,  // Principalement vers le bas
        RandomGenerator::randomRange(-0.2f, 0.2f)
    );
    
    return createMuon(energy, position, glm::normalize(direction));
}

Particle ParticleFactory::createCosmicGamma(const glm::vec3& position) {
    // Gamma cosmique
    float energy = RandomGenerator::randomRange(100.0f, 10000.0f); // 100 keV - 10 MeV
    glm::vec3 direction = RandomGenerator::randomDirection();
    
    return createGamma(energy, position, direction);
}

Particle ParticleFactory::createBackgroundGamma(const glm::vec3& position) {
    // Gamma de fond naturel (radon, etc.)
    float energy = RandomGenerator::randomRange(50.0f, 3000.0f); // 50 keV - 3 MeV
    glm::vec3 direction = RandomGenerator::randomDirection();
    
    return createGamma(energy, position, direction);
}

Particle ParticleFactory::createRadonDecay(const glm::vec3& position) {
    // Produits de décroissance du radon (alpha principalement)
    if (RandomGenerator::random() < 0.8f) {
        // Alpha du Rn-222 → Po-218
        float energy = 5490.0f; // keV
        glm::vec3 direction = RandomGenerator::randomDirection();
        return createAlpha(energy, position, direction);
    } else {
        // Gamma associés
        float energy = RandomGenerator::randomRange(100.0f, 1000.0f);
        glm::vec3 direction = RandomGenerator::randomDirection();
        return createGamma(energy, position, direction);
    }
}