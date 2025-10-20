#pragma once

#include "common.h"

// Types de sources
enum class SourceType {
    ISOTROPIC,      // Source isotropique (émet dans toutes les directions)
    DIRECTIONAL,    // Faisceau directionnel
    AMBIENT,        // Fond ambiant
    POINT,          // Source ponctuelle
    SURFACE,        // Source surfacique
    VOLUME          // Source volumique
};

// Spectre d'énergie
struct EnergySpectrum {
    enum Type {
        MONOENERGETIC,  // Énergie unique
        CONTINUOUS,     // Spectre continu
        DISCRETE        // Raies discrètes
    } type = MONOENERGETIC;
    
    float energy = 1000.0f; // keV (pour monoénergétique)
    std::vector<std::pair<float, float>> spectrum; // (énergie, intensité relative)
    
    float sampleEnergy() const;
    
private:
    float interpolateIntensity(float energy) const;
};

// Source de radiation de base
class Source {
public:
    Source(const std::string& name, SourceType type, RadiationType radiationType);
    virtual ~Source() = default;

    // Propriétés de base
    const std::string& getName() const { return m_name; }
    void setName(const std::string& name) { m_name = name; }
    
    SourceType getSourceType() const { return m_sourceType; }
    RadiationType getRadiationType() const { return m_radiationType; }
    void setRadiationType(RadiationType type) { m_radiationType = type; }

    // Position et orientation
    const glm::vec3& getPosition() const { return m_position; }
    void setPosition(const glm::vec3& position) { m_position = position; }
    
    const glm::vec3& getDirection() const { return m_direction; }
    void setDirection(const glm::vec3& direction) { m_direction = glm::normalize(direction); }

    // Intensité et spectre
    float getIntensity() const { return m_intensity; }
    void setIntensity(float intensity) { m_intensity = intensity; }
    
    const EnergySpectrum& getSpectrum() const { return m_spectrum; }
    void setSpectrum(const EnergySpectrum& spectrum) { m_spectrum = spectrum; }

    // Configuration
    bool isEnabled() const { return m_enabled; }
    void setEnabled(bool enabled) { m_enabled = enabled; }

    // Émission de particules
    virtual Particle emitParticle() const = 0;
    virtual glm::vec3 sampleDirection() const;
    virtual glm::vec3 samplePosition() const;

    // Statistiques
    uint64_t getEmittedCount() const { return m_emittedCount; }
    void incrementEmitted() const { ++m_emittedCount; }
    void resetStats() { m_emittedCount = 0; }

    // Visualisation
    bool isVisible() const { return m_visible; }
    void setVisible(bool visible) { m_visible = visible; }
    
    const glm::vec3& getColor() const { return m_color; }
    void setColor(const glm::vec3& color) { m_color = color; }

protected:
    std::string m_name;
    SourceType m_sourceType;
    RadiationType m_radiationType;
    
    glm::vec3 m_position{0.0f};
    glm::vec3 m_direction{0.0f, 0.0f, 1.0f};
    
    float m_intensity = 1.0f; // particles/s ou Bq
    EnergySpectrum m_spectrum;
    
    bool m_enabled = true;
    mutable std::atomic<uint64_t> m_emittedCount{0};
    
    bool m_visible = true;
    glm::vec3 m_color{1.0f, 1.0f, 0.0f}; // Jaune par défaut
};

// Source isotropique
class IsotropicSource : public Source {
public:
    IsotropicSource(const std::string& name, RadiationType radiationType);
    
    Particle emitParticle() const override;
    glm::vec3 sampleDirection() const override;
};

// Source directionnelle (faisceau)
class DirectionalSource : public Source {
public:
    DirectionalSource(const std::string& name, RadiationType radiationType);
    
    Particle emitParticle() const override;
    glm::vec3 sampleDirection() const override;
    
    float getBeamAngle() const { return m_beamAngle; }
    void setBeamAngle(float angle) { m_beamAngle = angle; }

private:
    float m_beamAngle = 0.1f; // rad (ouverture du faisceau)
};

// Source de fond ambiant
class AmbientSource : public Source {
public:
    AmbientSource(const std::string& name, RadiationType radiationType);
    
    Particle emitParticle() const override;
    glm::vec3 sampleDirection() const override;
    glm::vec3 samplePosition() const override;
    
    void setBounds(const glm::vec3& minBounds, const glm::vec3& maxBounds) {
        m_minBounds = minBounds;
        m_maxBounds = maxBounds;
    }

private:
    glm::vec3 m_minBounds{-10.0f};
    glm::vec3 m_maxBounds{10.0f};
};

// Gestionnaire de sources
class SourceManager {
public:
    void addSource(std::shared_ptr<Source> source);
    void removeSource(const std::string& name);
    std::shared_ptr<Source> getSource(const std::string& name) const;
    std::vector<std::shared_ptr<Source>> getAllSources() const;
    std::vector<std::shared_ptr<Source>> getEnabledSources() const;
    
    void clearAllStats();
    uint64_t getTotalEmitted() const;
    
    // Création de sources prédéfinies
    std::shared_ptr<Source> createCosmicBackground();
    std::shared_ptr<Source> createGammaPoint(const std::string& name, float energy, float activity);
    std::shared_ptr<Source> createNeutronBeam(const std::string& name, float energy, float flux);

private:
    std::map<std::string, std::shared_ptr<Source>> m_sources;
    mutable std::mutex m_mutex;
};