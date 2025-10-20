#include "core/Scene.h"
#include <algorithm>
#include <fstream>

Scene::Scene() {
    // Initialisation des niveaux de fond par défaut
    m_backgroundLevels[RadiationType::GAMMA] = 0.1f;   // μSv/h
    m_backgroundLevels[RadiationType::NEUTRON] = 0.01f;
    m_backgroundLevels[RadiationType::MUON] = 0.05f;
}

// Gestion des objets
void Scene::addObject(std::shared_ptr<Object3D> object) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    m_objects.push_back(object);
    m_objectsByName[object->getName()] = object;
    m_objectsById[object->getId()] = object;
    
    markBVHDirty();
}

void Scene::removeObject(const std::string& name) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    auto it = m_objectsByName.find(name);
    if (it != m_objectsByName.end()) {
        auto object = it->second;
        
        // Suppression de tous les containers
        m_objects.erase(std::remove(m_objects.begin(), m_objects.end(), object), m_objects.end());
        m_objectsByName.erase(name);
        m_objectsById.erase(object->getId());
        
        markBVHDirty();
    }
}

void Scene::removeObject(uint32_t id) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    auto it = m_objectsById.find(id);
    if (it != m_objectsById.end()) {
        auto object = it->second;
        
        m_objects.erase(std::remove(m_objects.begin(), m_objects.end(), object), m_objects.end());
        m_objectsByName.erase(object->getName());
        m_objectsById.erase(id);
        
        markBVHDirty();
    }
}

std::shared_ptr<Object3D> Scene::getObject(const std::string& name) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_objectsByName.find(name);
    return it != m_objectsByName.end() ? it->second : nullptr;
}

std::shared_ptr<Object3D> Scene::getObject(uint32_t id) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_objectsById.find(id);
    return it != m_objectsById.end() ? it->second : nullptr;
}

// Gestion des capteurs
void Scene::addSensor(std::shared_ptr<Sensor> sensor) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_sensors.push_back(sensor);
    m_sensorsByName[sensor->getName()] = sensor;
}

void Scene::removeSensor(const std::string& name) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    auto it = m_sensorsByName.find(name);
    if (it != m_sensorsByName.end()) {
        auto sensor = it->second;
        m_sensors.erase(std::remove(m_sensors.begin(), m_sensors.end(), sensor), m_sensors.end());
        m_sensorsByName.erase(name);
    }
}

std::shared_ptr<Sensor> Scene::getSensor(const std::string& name) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_sensorsByName.find(name);
    return it != m_sensorsByName.end() ? it->second : nullptr;
}

// Gestion des sources
void Scene::addSource(std::shared_ptr<Source> source) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_sources.push_back(source);
    m_sourcesByName[source->getName()] = source;
}

void Scene::removeSource(const std::string& name) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    auto it = m_sourcesByName.find(name);
    if (it != m_sourcesByName.end()) {
        auto source = it->second;
        m_sources.erase(std::remove(m_sources.begin(), m_sources.end(), source), m_sources.end());
        m_sourcesByName.erase(name);
    }
}

std::shared_ptr<Source> Scene::getSource(const std::string& name) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_sourcesByName.find(name);
    return it != m_sourcesByName.end() ? it->second : nullptr;
}

// Intersection avec les rayons
IntersectionResult Scene::intersectRay(const Ray& ray) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    // Utiliser le BVH si disponible et valide
    if (m_bvh && m_bvh->isValid()) {
        return m_bvh->intersect(ray);
    }
    
    // Fallback : test brute force
    IntersectionResult closestHit;
    closestHit.distance = std::numeric_limits<float>::max();
    
    for (const auto& object : m_objects) {
        IntersectionResult hit = object->intersect(ray);
        if (hit.hit && hit.distance < closestHit.distance) {
            closestHit = hit;
        }
    }
    
    return closestHit;
}

bool Scene::intersectRayAny(const Ray& ray) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    // Utiliser le BVH si disponible
    if (m_bvh && m_bvh->isValid()) {
        return m_bvh->intersectAny(ray);
    }
    
    // Fallback : test brute force
    for (const auto& object : m_objects) {
        IntersectionResult hit = object->intersect(ray);
        if (hit.hit) {
            return true;
        }
    }
    
    return false;
}

// Boîte englobante de la scène
AABB Scene::getSceneBounds() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    AABB bounds;
    for (const auto& object : m_objects) {
        bounds.expand(object->getBounds());
    }
    
    return bounds;
}

float Scene::getBackgroundRadiation(RadiationType type) const {
    auto it = m_backgroundLevels.find(type);
    return it != m_backgroundLevels.end() ? it->second : 0.0f;
}

// Optimisations
void Scene::buildAccelerationStructure() {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    if (!m_bvh) {
        m_bvh = std::make_unique<BVH>();
    }
    
    m_bvh->build(m_objects);
    m_bvhDirty = false;
    
    Log::info("Structure d'accélération BVH construite avec " + 
              std::to_string(m_objects.size()) + " objets");
}

void Scene::updateAccelerationStructure() {
    if (m_bvhDirty) {
        buildAccelerationStructure();
    }
}

// Sérialisation (implémentation simplifiée)
void Scene::saveToFile(const std::string& filename) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    std::ofstream file(filename);
    if (!file.is_open()) {
        throw std::runtime_error("Impossible d'ouvrir le fichier pour écriture: " + filename);
    }
    
    // Format JSON simple (sans bibliothèque JSON pour la simplicité)
    file << "{\n";
    file << "  \"version\": \"1.0\",\n";
    file << "  \"objects\": " << m_objects.size() << ",\n";
    file << "  \"sensors\": " << m_sensors.size() << ",\n";
    file << "  \"sources\": " << m_sources.size() << "\n";
    file << "}\n";
    
    file.close();
    Log::info("Scène sauvegardée dans: " + filename);
}

void Scene::loadFromFile(const std::string& filename) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    std::ifstream file(filename);
    if (!file.is_open()) {
        throw std::runtime_error("Impossible d'ouvrir le fichier pour lecture: " + filename);
    }
    
    // Lecture simplifiée - dans une vraie implémentation, on utiliserait
    // une bibliothèque JSON comme nlohmann::json
    
    clear();
    
    file.close();
    Log::info("Scène chargée depuis: " + filename);
}

void Scene::clear() {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    m_objects.clear();
    m_sensors.clear();
    m_sources.clear();
    
    m_objectsByName.clear();
    m_objectsById.clear();
    m_sensorsByName.clear();
    m_sourcesByName.clear();
    
    if (m_bvh) {
        m_bvh->clear();
    }
    m_bvhDirty = true;
    
    Log::info("Scène vidée");
}

// Sélection
std::shared_ptr<Object3D> Scene::selectObject(const Ray& ray) const {
    IntersectionResult hit = intersectRay(ray);
    return hit.hit ? hit.object : nullptr;
}

std::vector<std::shared_ptr<Object3D>> Scene::getSelectedObjects() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    std::vector<std::shared_ptr<Object3D>> selected;
    for (const auto& object : m_objects) {
        if (object->isSelected()) {
            selected.push_back(object);
        }
    }
    
    return selected;
}

void Scene::clearSelection() {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    for (auto& object : m_objects) {
        object->setSelected(false);
    }
}

void Scene::rebuildIndices() {
    m_objectsByName.clear();
    m_objectsById.clear();
    m_sensorsByName.clear();
    m_sourcesByName.clear();
    
    for (const auto& object : m_objects) {
        m_objectsByName[object->getName()] = object;
        m_objectsById[object->getId()] = object;
    }
    
    for (const auto& sensor : m_sensors) {
        m_sensorsByName[sensor->getName()] = sensor;
    }
    
    for (const auto& source : m_sources) {
        m_sourcesByName[source->getName()] = source;
    }
}