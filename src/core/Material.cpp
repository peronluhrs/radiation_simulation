#include "utils/Random.h"
#include "core/Material.h"
#include <algorithm>
#include <fstream>
// #include <json/json.h> // Pas nécessaire pour la démo

Material::Material(const std::string& name, float density) 
    : m_name(name), m_density(density) {
}

void Material::addElement(int atomicNumber, const std::string& symbol, 
                         float massFraction, float atomicMass) {
    ElementComposition element;
    element.atomicNumber = atomicNumber;
    element.symbol = symbol;
    element.massFraction = massFraction;
    element.atomicMass = atomicMass;
    m_composition.push_back(element);
}

float Material::getHydrogenContent() const {
    for (const auto& element : m_composition) {
        if (element.atomicNumber == 1) { // Hydrogène
            return element.massFraction;
        }
    }
    return 0.0f;
}

void Material::addAttenuationData(RadiationType type, float energy, 
                                 float linearCoeff, float massCoeff, float crossSection) {
    AttenuationData data;
    data.energy = energy;
    data.linearCoeff = linearCoeff;
    data.massCoeff = massCoeff;
    data.crossSection = crossSection;
    
    auto& table = m_attenuationTables[type];
    
    // Insertion triée par énergie
    auto it = std::lower_bound(table.begin(), table.end(), data,
        [](const AttenuationData& a, const AttenuationData& b) {
            return a.energy < b.energy;
        });
    
    table.insert(it, data);
}

float Material::getLinearAttenuation(RadiationType type, float energy) const {
    auto it = m_attenuationTables.find(type);
    if (it == m_attenuationTables.end()) return 0.0f;
    
    return interpolateAttenuation(it->second, energy, 
        [](const AttenuationData& data) { return data.linearCoeff; });
}

float Material::getMassAttenuation(RadiationType type, float energy) const {
    auto it = m_attenuationTables.find(type);
    if (it == m_attenuationTables.end()) return 0.0f;
    
    return interpolateAttenuation(it->second, energy,
        [](const AttenuationData& data) { return data.massCoeff; });
}

float Material::getCrossSection(RadiationType type, float energy) const {
    auto it = m_attenuationTables.find(type);
    if (it == m_attenuationTables.end()) return 0.0f;
    
    return interpolateAttenuation(it->second, energy,
        [](const AttenuationData& data) { return data.crossSection; });
}

float Material::interpolateAttenuation(const std::vector<AttenuationData>& table, float energy,
                                      std::function<float(const AttenuationData&)> getter) const {
    if (table.empty()) return 0.0f;
    if (table.size() == 1) return getter(table[0]);
    
    // Extrapolation aux bords
    if (energy <= table.front().energy) return getter(table.front());
    if (energy >= table.back().energy) return getter(table.back());
    
    // Recherche de l'intervalle
    auto it = std::lower_bound(table.begin(), table.end(), energy,
        [](const AttenuationData& data, float e) { return data.energy < e; });
    
    if (it == table.begin()) return getter(*it);
    
    auto it1 = it - 1;
    auto it2 = it;
    
    // Interpolation linéaire en log-log pour les coefficients d'atténuation
    float e1 = it1->energy, e2 = it2->energy;
    float v1 = getter(*it1), v2 = getter(*it2);
    
    if (v1 <= 0.0f || v2 <= 0.0f) {
        // Interpolation linéaire standard
        float t = (energy - e1) / (e2 - e1);
        return v1 + t * (v2 - v1);
    } else {
        // Interpolation log-log
        float logE1 = std::log(e1), logE2 = std::log(e2);
        float logV1 = std::log(v1), logV2 = std::log(v2);
        float logE = std::log(energy);
        
        float t = (logE - logE1) / (logE2 - logE1);
        float logV = logV1 + t * (logV2 - logV1);
        return std::exp(logV);
    }
}

InteractionType Material::sampleInteraction(RadiationType type, float energy) const {
    // Probabilités d'interaction simplifiées
    float mu = getLinearAttenuation(type, energy);
    if (mu <= 0.0f) return InteractionType::TRANSMISSION;
    
    float r = RandomGenerator::random();
    
    switch (type) {
        case RadiationType::GAMMA:
            if (r < 0.7f) return InteractionType::SCATTERING; // Compton
            else return InteractionType::ABSORPTION; // Photoélectrique
            
        case RadiationType::NEUTRON:
            if (r < 0.5f) return InteractionType::SCATTERING;
            else return InteractionType::CAPTURE;
            
        default:
            if (r < 0.8f) return InteractionType::SCATTERING;
            else return InteractionType::ABSORPTION;
    }
}

float Material::getMeanFreePath(RadiationType type, float energy) const {
    float mu = getLinearAttenuation(type, energy);
    return mu > 0.0f ? 1.0f / mu : std::numeric_limits<float>::max();
}

glm::vec3 Material::sampleScattering(const glm::vec3& incident, RadiationType type, float energy) const {
    switch (type) {
        case RadiationType::GAMMA: {
            // Diffusion Compton
            float cosTheta = RandomGenerator::randomRange(-1.0f, 1.0f);
            float phi = RandomGenerator::randomRange(0.0f, TWO_PI);
            
            // Construction d'un système de coordonnées local
            glm::vec3 w = incident;
            glm::vec3 u = std::abs(w.x) > 0.1f ? 
                          glm::vec3(0.0f, 1.0f, 0.0f) : glm::vec3(1.0f, 0.0f, 0.0f);
            u = glm::normalize(glm::cross(u, w));
            glm::vec3 v = glm::cross(w, u);
            
            float sinTheta = std::sqrt(1.0f - cosTheta * cosTheta);
            glm::vec3 newDir = sinTheta * std::cos(phi) * u + 
                              sinTheta * std::sin(phi) * v + 
                              cosTheta * w;
            
            return glm::normalize(newDir);
        }
        
        case RadiationType::NEUTRON: {
            // Diffusion isotrope dans le centre de masse
            return RandomGenerator::randomDirection();
        }
        
        default:
            // Diffusion isotrope par défaut
            return RandomGenerator::randomDirection();
    }
}

// Matériaux prédéfinis
std::shared_ptr<Material> Material::createLead() {
    auto lead = std::make_shared<Material>("Plomb", 11.34f); // g/cm³
    lead->addElement(82, "Pb", 1.0f, 207.2f);
    
    // Données d'atténuation gamma (approximatives)
    for (float energy = 10.0f; energy <= 10000.0f; energy *= 1.5f) {
        float mu = 11.34f * (5.0f * std::pow(energy / 1000.0f, -0.7f)); // cm⁻¹
        lead->addAttenuationData(RadiationType::GAMMA, energy, mu, mu / 11.34f);
    }
    
    return lead;
}

std::shared_ptr<Material> Material::createSteel() {
    auto steel = std::make_shared<Material>("Acier", 7.87f);
    steel->addElement(26, "Fe", 0.98f, 55.845f);
    steel->addElement(6, "C", 0.02f, 12.011f);
    
    for (float energy = 10.0f; energy <= 10000.0f; energy *= 1.5f) {
        float mu = 7.87f * (0.8f * std::pow(energy / 1000.0f, -0.5f));
        steel->addAttenuationData(RadiationType::GAMMA, energy, mu, mu / 7.87f);
    }
    
    return steel;
}

std::shared_ptr<Material> Material::createCopper() {
    auto copper = std::make_shared<Material>("Cuivre", 8.96f);
    copper->addElement(29, "Cu", 1.0f, 63.546f);
    
    for (float energy = 10.0f; energy <= 10000.0f; energy *= 1.5f) {
        float mu = 8.96f * (1.2f * std::pow(energy / 1000.0f, -0.6f));
        copper->addAttenuationData(RadiationType::GAMMA, energy, mu, mu / 8.96f);
    }
    
    return copper;
}

std::shared_ptr<Material> Material::createPolyethylene() {
    auto poly = std::make_shared<Material>("Polyéthylène", 0.92f);
    poly->addElement(1, "H", 0.143f, 1.008f);  // Riche en hydrogène
    poly->addElement(6, "C", 0.857f, 12.011f);
    
    // Excellent pour les neutrons grâce à l'hydrogène
    for (float energy = 0.01f; energy <= 1000.0f; energy *= 2.0f) {
        float sigma = 20.0f * std::pow(energy, -0.5f); // barns
        poly->addAttenuationData(RadiationType::NEUTRON, energy, 0.0f, 0.0f, sigma);
    }
    
    return poly;
}

std::shared_ptr<Material> Material::createConcrete() {
    auto concrete = std::make_shared<Material>("Béton", 2.3f);
    // Composition simplifiée
    concrete->addElement(14, "Si", 0.315f, 28.085f);
    concrete->addElement(20, "Ca", 0.444f, 40.078f);
    concrete->addElement(8, "O", 0.241f, 15.999f);
    
    for (float energy = 10.0f; energy <= 10000.0f; energy *= 1.5f) {
        float mu = 2.3f * (0.3f * std::pow(energy / 1000.0f, -0.4f));
        concrete->addAttenuationData(RadiationType::GAMMA, energy, mu, mu / 2.3f);
    }
    
    return concrete;
}

std::shared_ptr<Material> Material::createWater() {
    auto water = std::make_shared<Material>("Eau", 1.0f);
    water->addElement(1, "H", 0.111f, 1.008f);
    water->addElement(8, "O", 0.889f, 15.999f);
    
    for (float energy = 10.0f; energy <= 10000.0f; energy *= 1.5f) {
        float mu = 1.0f * (0.15f * std::pow(energy / 1000.0f, -0.3f));
        water->addAttenuationData(RadiationType::GAMMA, energy, mu, mu / 1.0f);
    }
    
    return water;
}

std::shared_ptr<Material> Material::createAir() {
    auto air = std::make_shared<Material>("Air", 0.001225f);
    air->addElement(7, "N", 0.781f, 14.007f);
    air->addElement(8, "O", 0.209f, 15.999f);
    air->addElement(18, "Ar", 0.01f, 39.948f);
    
    // Atténuation très faible
    for (float energy = 10.0f; energy <= 10000.0f; energy *= 1.5f) {
        float mu = 0.001225f * (0.001f * std::pow(energy / 1000.0f, -0.3f));
        air->addAttenuationData(RadiationType::GAMMA, energy, mu, mu / 0.001225f);
    }
    
    return air;
}

std::shared_ptr<Material> Material::createVacuum() {
    auto vacuum = std::make_shared<Material>("Vide", 0.0f);
    // Aucune atténuation
    return vacuum;
}

// MaterialLibrary
MaterialLibrary& MaterialLibrary::getInstance() {
    static MaterialLibrary instance;
    return instance;
}

void MaterialLibrary::addMaterial(std::shared_ptr<Material> material) {
    m_materials[material->getName()] = material;
}

std::shared_ptr<Material> MaterialLibrary::getMaterial(const std::string& name) const {
    auto it = m_materials.find(name);
    return it != m_materials.end() ? it->second : nullptr;
}

std::vector<std::string> MaterialLibrary::getMaterialNames() const {
    std::vector<std::string> names;
    for (const auto& pair : m_materials) {
        names.push_back(pair.first);
    }
    return names;
}

void MaterialLibrary::loadDefaults() {
    addMaterial(Material::createLead());
    addMaterial(Material::createSteel());
    addMaterial(Material::createCopper());
    addMaterial(Material::createPolyethylene());
    addMaterial(Material::createConcrete());
    addMaterial(Material::createWater());
    addMaterial(Material::createAir());
    addMaterial(Material::createVacuum());
}