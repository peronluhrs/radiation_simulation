#pragma once

#include "common.h"

// Structure pour les propriétés d'atténuation
struct AttenuationData {
    float linearCoeff = 0.0f;      // Coefficient d'atténuation linéaire μ (cm⁻¹)
    float massCoeff = 0.0f;        // Coefficient d'atténuation massique μ/ρ (cm²/g)
    float crossSection = 0.0f;     // Section efficace pour neutrons (barns)
    float energy = 0.0f;           // Énergie associée (keV)
};

// Structure pour la composition chimique
struct ElementComposition {
    int atomicNumber = 0;
    std::string symbol;
    float massFraction = 0.0f;
    float atomicMass = 0.0f;
};

class Material {
public:
    Material(const std::string& name, float density);
    ~Material() = default;

    // Propriétés de base
    const std::string& getName() const { return m_name; }
    float getDensity() const { return m_density; }
    void setDensity(float density) { m_density = density; }

    // Composition chimique
    void addElement(int atomicNumber, const std::string& symbol, float massFraction, float atomicMass);
    const std::vector<ElementComposition>& getComposition() const { return m_composition; }
    float getHydrogenContent() const;

    // Données d'atténuation
    void addAttenuationData(RadiationType type, float energy, float linearCoeff, float massCoeff = 0.0f, float crossSection = 0.0f);
    float getLinearAttenuation(RadiationType type, float energy) const;
    float getMassAttenuation(RadiationType type, float energy) const;
    float getCrossSection(RadiationType type, float energy) const;

    // Interaction des particules
    InteractionType sampleInteraction(RadiationType type, float energy) const;
    float getMeanFreePath(RadiationType type, float energy) const;
    glm::vec3 sampleScattering(const glm::vec3& incident, RadiationType type, float energy) const;

    // Matériaux prédéfinis
    static std::shared_ptr<Material> createLead();
    static std::shared_ptr<Material> createSteel();
    static std::shared_ptr<Material> createCopper();
    static std::shared_ptr<Material> createPolyethylene();
    static std::shared_ptr<Material> createConcrete();
    static std::shared_ptr<Material> createWater();
    static std::shared_ptr<Material> createAir();
    static std::shared_ptr<Material> createVacuum();

private:
    std::string m_name;
    float m_density; // g/cm³
    std::vector<ElementComposition> m_composition;
    
    // Tables d'atténuation par type de radiation
    std::map<RadiationType, std::vector<AttenuationData>> m_attenuationTables;

    // Interpolation linéaire dans les tables
    float interpolateAttenuation(const std::vector<AttenuationData>& table, float energy, 
                               std::function<float(const AttenuationData&)> getter) const;
};

// Gestionnaire de bibliothèque de matériaux
class MaterialLibrary {
public:
    static MaterialLibrary& getInstance();
    
    void addMaterial(std::shared_ptr<Material> material);
    std::shared_ptr<Material> getMaterial(const std::string& name) const;
    std::vector<std::string> getMaterialNames() const;
    void loadDefaults();
    
    // Sérialisation
    void saveToFile(const std::string& filename) const;
    void loadFromFile(const std::string& filename);

private:
    MaterialLibrary() = default;
    std::map<std::string, std::shared_ptr<Material>> m_materials;
};