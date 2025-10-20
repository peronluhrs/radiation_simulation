#include "core/Sensor.h"
#include "simulation/Particle.h"
#include <chrono>
#include <cmath>

Sensor::Sensor(const std::string& name, SensorType type, const glm::vec3& position)
    : m_name(name), m_type(type), m_position(position) {
    m_startTime = std::chrono::steady_clock::now();
}

bool Sensor::detectsParticle(const Particle& particle) const {
    if (!m_enabled) return false;
    
    // Filtre par type de radiation
    if (!m_radiationFilter.empty()) {
        bool typeMatch = false;
        for (RadiationType type : m_radiationFilter) {
            if (particle.getType() == type) {
                typeMatch = true;
                break;
            }
        }
        if (!typeMatch) return false;
    }
    
    // Filtre énergétique
    float energy = particle.getEnergy();
    if (energy < m_minEnergy || energy > m_maxEnergy) {
        return false;
    }
    
    // Test géométrique selon le type de capteur
    switch (m_type) {
        case SensorType::POINT: {
            // Capteur ponctuel : détection si la particule passe "près"
            float distance = glm::length(particle.getPosition() - m_position);
            return distance < 0.01f; // 1 cm de rayon effectif
        }
        
        case SensorType::VOLUME: {
            // Capteur volumique
            return pointInSensor(particle.getPosition());
        }
        
        case SensorType::SURFACE: {
            // Capteur surfacique : intersection avec la surface
            Ray particleRay = particle.getRay();
            float t;
            return rayIntersectsSensor(particleRay, t);
        }
    }
    
    return false;
}

void Sensor::recordDetection(const Particle& particle) {
    if (!detectsParticle(particle)) return;
    
    // Enregistrement des statistiques
    m_stats.totalCounts.fetch_add(1);
    
    // Par type de radiation
    switch (particle.getType()) {
        case RadiationType::GAMMA:
        case RadiationType::X_RAY:
            m_stats.gammaCounts.fetch_add(1);
            break;
        case RadiationType::NEUTRON:
            m_stats.neutronCounts.fetch_add(1);
            break;
        case RadiationType::MUON:
            m_stats.muonCounts.fetch_add(1);
            break;
        default:
            break;
    }
    
    // Énergie déposée (simplifiée : toute l'énergie est déposée)
    double energy = static_cast<double>(particle.getEnergy());
    m_stats.totalEnergy.fetch_add(energy);
    
    // Calcul de dose simplifié (approximation)
    double dose = energy * 1.6e-16; // Conversion keV -> J, puis facteur de dose
    m_stats.totalDose.fetch_add(dose);
}

double Sensor::getCountRate() const {
    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - m_startTime);
    double timeSeconds = elapsed.count();
    
    if (timeSeconds > 0.0) {
        return m_stats.totalCounts.load() / timeSeconds;
    }
    return 0.0;
}

double Sensor::getDoseRate() const {
    // Calcul simplifié du débit de dose en μSv/h
    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - m_startTime);
    double timeHours = elapsed.count() / 3600.0;
    
    if (timeHours > 0.0) {
        double totalDoseJoules = m_stats.totalDose.load();
        double doseSieverts = totalDoseJoules; // Approximation (facteur de qualité = 1)
        return doseSieverts * 1e6 / timeHours; // μSv/h
    }
    return 0.0;
}

double Sensor::getEfficiency() const {
    // Efficacité simplifiée basée sur le type de radiation et l'énergie
    // En réalité, cela dépendrait du type de détecteur
    return 1.0; // 100% d'efficacité pour la simulation
}

double Sensor::getAttenuationFactor(double incidentIntensity) const {
    double detectedIntensity = getCountRate();
    if (incidentIntensity > 0.0) {
        return detectedIntensity / incidentIntensity;
    }
    return 0.0;
}

bool Sensor::pointInSensor(const glm::vec3& point) const {
    switch (m_type) {
        case SensorType::POINT: {
            float distance = glm::length(point - m_position);
            return distance < m_radius;
        }
        
        case SensorType::VOLUME: {
            // Volume rectangulaire centré sur la position
            glm::vec3 localPoint = point - m_position;
            glm::vec3 halfSize = m_size * 0.5f;
            
            return (std::abs(localPoint.x) <= halfSize.x &&
                    std::abs(localPoint.y) <= halfSize.y &&
                    std::abs(localPoint.z) <= halfSize.z);
        }
        
        case SensorType::SURFACE: {
            // Surface rectangulaire dans le plan XY
            glm::vec3 localPoint = point - m_position;
            
            return (std::abs(localPoint.x) <= m_size.x * 0.5f &&
                    std::abs(localPoint.y) <= m_size.y * 0.5f &&
                    std::abs(localPoint.z) <= 0.01f); // Épaisseur de 2 cm
        }
    }
    
    return false;
}

bool Sensor::rayIntersectsSensor(const Ray& ray, float& t) const {
    switch (m_type) {
        case SensorType::POINT: {
            // Intersection avec une sphère
            glm::vec3 oc = ray.origin - m_position;
            float a = glm::dot(ray.direction, ray.direction);
            float b = 2.0f * glm::dot(oc, ray.direction);
            float c = glm::dot(oc, oc) - m_radius * m_radius;
            
            float discriminant = b * b - 4 * a * c;
            if (discriminant < 0) return false;
            
            float t1 = (-b - std::sqrt(discriminant)) / (2.0f * a);
            float t2 = (-b + std::sqrt(discriminant)) / (2.0f * a);
            
            t = (t1 > ray.tMin) ? t1 : t2;
            return t >= ray.tMin && t <= ray.tMax;
        }
        
        case SensorType::VOLUME:
        case SensorType::SURFACE: {
            // Intersection avec une boîte (AABB)
            glm::vec3 minBounds = m_position - m_size * 0.5f;
            glm::vec3 maxBounds = m_position + m_size * 0.5f;
            
            float tMin = (minBounds.x - ray.origin.x) / ray.direction.x;
            float tMax = (maxBounds.x - ray.origin.x) / ray.direction.x;
            
            if (tMin > tMax) std::swap(tMin, tMax);
            
            float tyMin = (minBounds.y - ray.origin.y) / ray.direction.y;
            float tyMax = (maxBounds.y - ray.origin.y) / ray.direction.y;
            
            if (tyMin > tyMax) std::swap(tyMin, tyMax);
            
            if (tMin > tyMax || tyMin > tMax) return false;
            
            tMin = std::max(tMin, tyMin);
            tMax = std::min(tMax, tyMax);
            
            float tzMin = (minBounds.z - ray.origin.z) / ray.direction.z;
            float tzMax = (maxBounds.z - ray.origin.z) / ray.direction.z;
            
            if (tzMin > tzMax) std::swap(tzMin, tzMax);
            
            if (tMin > tzMax || tzMin > tMax) return false;
            
            tMin = std::max(tMin, tzMin);
            tMax = std::min(tMax, tzMax);
            
            t = (tMin > ray.tMin) ? tMin : tMax;
            return t >= ray.tMin && t <= ray.tMax;
        }
    }
    
    return false;
}

// SensorManager implementation
void SensorManager::addSensor(std::shared_ptr<Sensor> sensor) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_sensors[sensor->getName()] = sensor;
}

void SensorManager::removeSensor(const std::string& name) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_sensors.erase(name);
}

std::shared_ptr<Sensor> SensorManager::getSensor(const std::string& name) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_sensors.find(name);
    return it != m_sensors.end() ? it->second : nullptr;
}

std::vector<std::shared_ptr<Sensor>> SensorManager::getAllSensors() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    std::vector<std::shared_ptr<Sensor>> result;
    result.reserve(m_sensors.size());
    
    for (const auto& pair : m_sensors) {
        result.push_back(pair.second);
    }
    
    return result;
}

std::vector<std::shared_ptr<Sensor>> SensorManager::getEnabledSensors() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    std::vector<std::shared_ptr<Sensor>> result;
    
    for (const auto& pair : m_sensors) {
        if (pair.second->isEnabled()) {
            result.push_back(pair.second);
        }
    }
    
    return result;
}

void SensorManager::clearAllStats() {
    std::lock_guard<std::mutex> lock(m_mutex);
    for (auto& pair : m_sensors) {
        pair.second->clearStats();
    }
}

DetectionStats SensorManager::getTotalStats() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    DetectionStats total;
    
    for (const auto& pair : m_sensors) {
        total += pair.second->getStats();
    }
    
    return total;
}

std::vector<std::shared_ptr<Sensor>> SensorManager::getDetectingSensors(const Particle& particle) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    std::vector<std::shared_ptr<Sensor>> result;
    
    for (const auto& pair : m_sensors) {
        if (pair.second->detectsParticle(particle)) {
            result.push_back(pair.second);
        }
    }
    
    return result;
}