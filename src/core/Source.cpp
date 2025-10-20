#include "core/Source.h"
#include "simulation/Particle.h"

// EnergySpectrum implementation
float EnergySpectrum::sampleEnergy() const {
    switch (type) {
        case MONOENERGETIC:
            return energy;
            
        case CONTINUOUS:
        case DISCRETE: {
            if (spectrum.empty()) return energy;
            
            // Échantillonnage par méthode de rejet simple
            float maxIntensity = 0.0f;
            float minEnergy = spectrum.front().first;
            float maxEnergy = spectrum.back().first;
            
            for (const auto& point : spectrum) {
                maxIntensity = std::max(maxIntensity, point.second);
            }
            
            // Méthode de rejet
            for (int attempt = 0; attempt < 1000; ++attempt) {
                float e = RandomGenerator::randomRange(minEnergy, maxEnergy);
                float intensity = interpolateIntensity(e);
                float r = RandomGenerator::random() * maxIntensity;
                
                if (r <= intensity) {
                    return e;
                }
            }
            
            // Fallback : énergie moyenne
            return (minEnergy + maxEnergy) * 0.5f;
        }
    }
    
    return energy;
}

float EnergySpectrum::interpolateIntensity(float e) const {
    if (spectrum.empty()) return 1.0f;
    if (spectrum.size() == 1) return spectrum[0].second;
    
    // Extrapolation aux bords
    if (e <= spectrum.front().first) return spectrum.front().second;
    if (e >= spectrum.back().first) return spectrum.back().second;
    
    // Recherche de l'intervalle
    for (size_t i = 0; i < spectrum.size() - 1; ++i) {
        if (e >= spectrum[i].first && e <= spectrum[i + 1].first) {
            float t = (e - spectrum[i].first) / (spectrum[i + 1].first - spectrum[i].first);
            return spectrum[i].second + t * (spectrum[i + 1].second - spectrum[i].second);
        }
    }
    
    return 1.0f;
}

// Source implementation
Source::Source(const std::string& name, SourceType type, RadiationType radiationType)
    : m_name(name), m_sourceType(type), m_radiationType(radiationType) {
}

glm::vec3 Source::sampleDirection() const {
    // Direction par défaut (sera surchargée par les classes dérivées)
    return m_direction;
}

glm::vec3 Source::samplePosition() const {
    // Position par défaut
    return m_position;
}

// IsotropicSource implementation
IsotropicSource::IsotropicSource(const std::string& name, RadiationType radiationType)
    : Source(name, SourceType::ISOTROPIC, radiationType) {
}

Particle IsotropicSource::emitParticle() const {
    glm::vec3 pos = samplePosition();
    glm::vec3 dir = sampleDirection();
    float energy = m_spectrum.sampleEnergy();
    
    Particle particle(m_radiationType, energy, pos, dir);
    particle.setWeight(1.0f);
    
    incrementEmitted();
    return particle;
}

glm::vec3 IsotropicSource::sampleDirection() const {
    // Émission isotrope dans toutes les directions
    return RandomGenerator::randomDirection();
}

// DirectionalSource implementation
DirectionalSource::DirectionalSource(const std::string& name, RadiationType radiationType)
    : Source(name, SourceType::DIRECTIONAL, radiationType) {
}

Particle DirectionalSource::emitParticle() const {
    glm::vec3 pos = samplePosition();
    glm::vec3 dir = sampleDirection();
    float energy = m_spectrum.sampleEnergy();
    
    Particle particle(m_radiationType, energy, pos, dir);
    particle.setWeight(1.0f);
    
    incrementEmitted();
    return particle;
}

glm::vec3 DirectionalSource::sampleDirection() const {
    // Faisceau avec ouverture angulaire
    if (m_beamAngle <= 0.0f) {
        return m_direction; // Faisceau parfaitement collimaté
    }
    
    // Échantillonnage dans un cône
    float cosTheta = std::cos(m_beamAngle);
    float z = RandomGenerator::randomRange(cosTheta, 1.0f);
    float phi = RandomGenerator::randomRange(0.0f, TWO_PI);
    
    float sinTheta = std::sqrt(1.0f - z * z);
    
    // Direction dans le système local du faisceau
    glm::vec3 localDir(sinTheta * std::cos(phi), sinTheta * std::sin(phi), z);
    
    // Construction du système de coordonnées local
    glm::vec3 w = m_direction;
    glm::vec3 u = std::abs(w.x) > 0.1f ? 
                  glm::vec3(0.0f, 1.0f, 0.0f) : glm::vec3(1.0f, 0.0f, 0.0f);
    u = glm::normalize(glm::cross(u, w));
    glm::vec3 v = glm::cross(w, u);
    
    // Transformation vers l'espace monde
    return localDir.x * u + localDir.y * v + localDir.z * w;
}

// AmbientSource implementation
AmbientSource::AmbientSource(const std::string& name, RadiationType radiationType)
    : Source(name, SourceType::AMBIENT, radiationType) {
}

Particle AmbientSource::emitParticle() const {
    glm::vec3 pos = samplePosition();
    glm::vec3 dir = sampleDirection();
    float energy = m_spectrum.sampleEnergy();
    
    Particle particle(m_radiationType, energy, pos, dir);
    particle.setWeight(1.0f);
    
    incrementEmitted();
    return particle;
}

glm::vec3 AmbientSource::sampleDirection() const {
    // Direction aléatoire pour source ambiante
    return RandomGenerator::randomDirection();
}

glm::vec3 AmbientSource::samplePosition() const {
    // Position aléatoire dans les limites définies
    return glm::vec3(
        RandomGenerator::randomRange(m_minBounds.x, m_maxBounds.x),
        RandomGenerator::randomRange(m_minBounds.y, m_maxBounds.y),
        RandomGenerator::randomRange(m_minBounds.z, m_maxBounds.z)
    );
}

// SourceManager implementation
void SourceManager::addSource(std::shared_ptr<Source> source) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_sources[source->getName()] = source;
}

void SourceManager::removeSource(const std::string& name) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_sources.erase(name);
}

std::shared_ptr<Source> SourceManager::getSource(const std::string& name) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_sources.find(name);
    return it != m_sources.end() ? it->second : nullptr;
}

std::vector<std::shared_ptr<Source>> SourceManager::getAllSources() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    std::vector<std::shared_ptr<Source>> result;
    result.reserve(m_sources.size());
    
    for (const auto& pair : m_sources) {
        result.push_back(pair.second);
    }
    
    return result;
}

std::vector<std::shared_ptr<Source>> SourceManager::getEnabledSources() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    std::vector<std::shared_ptr<Source>> result;
    
    for (const auto& pair : m_sources) {
        if (pair.second->isEnabled()) {
            result.push_back(pair.second);
        }
    }
    
    return result;
}

void SourceManager::clearAllStats() {
    std::lock_guard<std::mutex> lock(m_mutex);
    for (auto& pair : m_sources) {
        pair.second->resetStats();
    }
}

uint64_t SourceManager::getTotalEmitted() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    uint64_t total = 0;
    
    for (const auto& pair : m_sources) {
        total += pair.second->getEmittedCount();
    }
    
    return total;
}

std::shared_ptr<Source> SourceManager::createCosmicBackground() {
    auto source = std::make_shared<AmbientSource>("Fond_Cosmique", RadiationType::MUON);
    
    // Configuration typique du fond cosmique
    source->setBounds(glm::vec3(-50.0f, 10.0f, -50.0f), 
                     glm::vec3(50.0f, 20.0f, 50.0f));
    source->setIntensity(170.0f); // muons/m²/s au niveau de la mer
    
    // Spectre énergétique des muons cosmiques
    EnergySpectrum spectrum;
    spectrum.type = EnergySpectrum::CONTINUOUS;
    spectrum.spectrum = {
        {1000.0f, 0.1f},      // 1 MeV
        {10000.0f, 0.5f},     // 10 MeV
        {100000.0f, 1.0f},    // 100 MeV
        {1000000.0f, 0.8f},   // 1 GeV
        {10000000.0f, 0.3f}   // 10 GeV
    };
    source->setSpectrum(spectrum);
    
    return source;
}

std::shared_ptr<Source> SourceManager::createGammaPoint(const std::string& name, 
                                                       float energy, float activity) {
    auto source = std::make_shared<IsotropicSource>(name, RadiationType::GAMMA);
    
    source->setIntensity(activity); // Bq
    
    EnergySpectrum spectrum;
    spectrum.type = EnergySpectrum::MONOENERGETIC;
    spectrum.energy = energy;
    source->setSpectrum(spectrum);
    
    return source;
}

std::shared_ptr<Source> SourceManager::createNeutronBeam(const std::string& name, 
                                                        float energy, float flux) {
    auto source = std::make_shared<DirectionalSource>(name, RadiationType::NEUTRON);
    
    source->setIntensity(flux); // neutrons/s
    
    EnergySpectrum spectrum;
    spectrum.type = EnergySpectrum::MONOENERGETIC;
    spectrum.energy = energy;
    source->setSpectrum(spectrum);
    
    return source;
}