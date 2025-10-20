#pragma once

#include "common.h"

// Types de capteurs
enum class SensorType {
    POINT,      // Capteur ponctuel
    VOLUME,     // Volume de détection
    SURFACE     // Surface de détection
};

// Accumulateur de statistiques
struct DetectionStats {
    std::atomic<uint64_t> totalCounts{0};
    std::atomic<uint64_t> gammaCounts{0};
    std::atomic<uint64_t> neutronCounts{0};
    std::atomic<uint64_t> muonCounts{0};
    std::atomic<double> totalEnergy{0.0};
    std::atomic<double> totalDose{0.0};
    
    // Constructeurs pour permettre la copie
    DetectionStats() = default;
    DetectionStats(const DetectionStats& other) {
        totalCounts.store(other.totalCounts.load());
        gammaCounts.store(other.gammaCounts.load());
        neutronCounts.store(other.neutronCounts.load());
        muonCounts.store(other.muonCounts.load());
        totalEnergy.store(other.totalEnergy.load());
        totalDose.store(other.totalDose.load());
    }
    
    DetectionStats& operator=(const DetectionStats& other) {
        if (this != &other) {
            totalCounts.store(other.totalCounts.load());
            gammaCounts.store(other.gammaCounts.load());
            neutronCounts.store(other.neutronCounts.load());
            muonCounts.store(other.muonCounts.load());
            totalEnergy.store(other.totalEnergy.load());
            totalDose.store(other.totalDose.load());
        }
        return *this;
    }
    
    void clear() {
        totalCounts = 0;
        gammaCounts = 0;
        neutronCounts = 0;
        muonCounts = 0;
        totalEnergy = 0.0;
        totalDose = 0.0;
    }
    
    DetectionStats& operator+=(const DetectionStats& other) {
        totalCounts.fetch_add(other.totalCounts.load());
        gammaCounts.fetch_add(other.gammaCounts.load());
        neutronCounts.fetch_add(other.neutronCounts.load());
        muonCounts.fetch_add(other.muonCounts.load());
        totalEnergy.fetch_add(other.totalEnergy.load());
        totalDose.fetch_add(other.totalDose.load());
        return *this;
    }
};

class Sensor {
public:
    Sensor(const std::string& name, SensorType type, const glm::vec3& position);
    ~Sensor() = default;

    // Propriétés de base
    const std::string& getName() const { return m_name; }
    void setName(const std::string& name) { m_name = name; }
    
    SensorType getType() const { return m_type; }
    void setType(SensorType type) { m_type = type; }

    // Position et orientation
    const glm::vec3& getPosition() const { return m_position; }
    void setPosition(const glm::vec3& position) { m_position = position; }
    
    const glm::vec3& getOrientation() const { return m_orientation; }
    void setOrientation(const glm::vec3& orientation) { m_orientation = orientation; }

    // Géométrie du capteur
    const glm::vec3& getSize() const { return m_size; }
    void setSize(const glm::vec3& size) { m_size = size; }
    
    float getRadius() const { return m_radius; }
    void setRadius(float radius) { m_radius = radius; }

    // Configuration
    bool isEnabled() const { return m_enabled; }
    void setEnabled(bool enabled) { m_enabled = enabled; }
    
    void setEnergyRange(float minEnergy, float maxEnergy) {
        m_minEnergy = minEnergy;
        m_maxEnergy = maxEnergy;
    }
    
    void setRadiationFilter(const std::vector<RadiationType>& types) {
        m_radiationFilter = types;
    }

    // Détection
    bool detectsParticle(const Particle& particle) const;
    void recordDetection(const Particle& particle);
    
    // Statistiques
    const DetectionStats& getStats() const { return m_stats; }
    void clearStats() { m_stats.clear(); }
    
    // Calculs dérivés
    double getCountRate() const; // counts/s
    double getDoseRate() const;  // μSv/h
    double getEfficiency() const;
    double getAttenuationFactor(double incidentIntensity) const;

    // Visualisation
    bool isVisible() const { return m_visible; }
    void setVisible(bool visible) { m_visible = visible; }
    
    const glm::vec3& getColor() const { return m_color; }
    void setColor(const glm::vec3& color) { m_color = color; }

private:
    float m_doseRate_uSv_h{0.f};
    std::string m_name;
    SensorType m_type;
    
    // Géométrie
    glm::vec3 m_position{0.0f};
    glm::vec3 m_orientation{0.0f, 0.0f, 1.0f};
    glm::vec3 m_size{1.0f}; // Pour capteurs volumiques
    float m_radius = 1.0f;  // Pour capteurs sphériques
    
    // Configuration
    bool m_enabled = true;
    float m_minEnergy = 0.0f;      // keV
    float m_maxEnergy = 10000.0f;  // keV
    std::vector<RadiationType> m_radiationFilter; // Types acceptés (vide = tous)
    
    // Statistiques
    DetectionStats m_stats;
    std::chrono::steady_clock::time_point m_startTime;
    
    // Visualisation
    bool m_visible = true;
    glm::vec3 m_color{0.0f, 1.0f, 0.0f}; // Vert par défaut

    // Tests géométriques
    bool pointInSensor(const glm::vec3& point) const;
    bool rayIntersectsSensor(const Ray& ray, float& t) const;
};

// Gestionnaire de capteurs
class SensorManager {
public:
    void addSensor(std::shared_ptr<Sensor> sensor);
    void removeSensor(const std::string& name);
    std::shared_ptr<Sensor> getSensor(const std::string& name) const;
    std::vector<std::shared_ptr<Sensor>> getAllSensors() const;
    std::vector<std::shared_ptr<Sensor>> getEnabledSensors() const;
    
    void clearAllStats();
    DetectionStats getTotalStats() const;
    
    // Tests de détection pour une particule
    std::vector<std::shared_ptr<Sensor>> getDetectingSensors(const Particle& particle) const;

private:
    std::map<std::string, std::shared_ptr<Sensor>> m_sensors;
    mutable std::mutex m_mutex;
};
