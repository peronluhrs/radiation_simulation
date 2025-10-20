#pragma once

#include "common.h"
#include "geometry/Object3D.h"
#include "core/Material.h"
#include "core/Sensor.h"
#include "core/Source.h"
#include "utils/BVH.h"

class Scene {
public:
    Scene();
    ~Scene() = default;

    // Gestion des objets
    void addObject(std::shared_ptr<Object3D> object);
    void removeObject(const std::string& name);
    void removeObject(uint32_t id);
    std::shared_ptr<Object3D> getObject(const std::string& name) const;
    std::shared_ptr<Object3D> getObject(uint32_t id) const;
    const std::vector<std::shared_ptr<Object3D>>& getAllObjects() const { return m_objects; }
    
    // Gestion des capteurs
    void addSensor(std::shared_ptr<Sensor> sensor);
    void removeSensor(const std::string& name);
    std::shared_ptr<Sensor> getSensor(const std::string& name) const;
    const std::vector<std::shared_ptr<Sensor>>& getAllSensors() const { return m_sensors; }
    
    // Gestion des sources
    void addSource(std::shared_ptr<Source> source);
    void removeSource(const std::string& name);
    std::shared_ptr<Source> getSource(const std::string& name) const;
    const std::vector<std::shared_ptr<Source>>& getAllSources() const { return m_sources; }

    // Intersection avec les rayons (accélérée par BVH)
    IntersectionResult intersectRay(const Ray& ray) const;
    bool intersectRayAny(const Ray& ray) const; // Test d'occlusion rapide
    
    // Boîte englobante de la scène
    AABB getSceneBounds() const;
    
    // Propriétés ambiantes
    void setBackgroundRadiation(RadiationType type, float level) { 
        m_backgroundLevels[type] = level; 
    }
    float getBackgroundRadiation(RadiationType type) const;
    
    // Optimisations
    void buildAccelerationStructure();
    void updateAccelerationStructure();
    bool isAccelerationStructureValid() const { return m_bvh && m_bvh->isValid(); }
    
    // Sérialisation
    void saveToFile(const std::string& filename) const;
    void loadFromFile(const std::string& filename);
    void clear();
    
    // Statistiques
    size_t getObjectCount() const { return m_objects.size(); }
    size_t getSensorCount() const { return m_sensors.size(); }
    size_t getSourceCount() const { return m_sources.size(); }
    
    // Sélection et manipulation
    std::shared_ptr<Object3D> selectObject(const Ray& ray) const;
    std::vector<std::shared_ptr<Object3D>> getSelectedObjects() const;
    void clearSelection();

private:
    std::vector<std::shared_ptr<Object3D>> m_objects;
    std::vector<std::shared_ptr<Sensor>> m_sensors;
    std::vector<std::shared_ptr<Source>> m_sources;
    
    // Index pour recherche rapide
    std::map<std::string, std::shared_ptr<Object3D>> m_objectsByName;
    std::map<uint32_t, std::shared_ptr<Object3D>> m_objectsById;
    std::map<std::string, std::shared_ptr<Sensor>> m_sensorsByName;
    std::map<std::string, std::shared_ptr<Source>> m_sourcesByName;
    
    // Structure d'accélération
    std::unique_ptr<BVH> m_bvh;
    bool m_bvhDirty = true;
    
    // Propriétés ambiantes
    std::map<RadiationType, float> m_backgroundLevels;
    
    // Thread safety
    mutable std::mutex m_mutex;
    
    // Helpers privés
    void rebuildIndices();
    void markBVHDirty() { m_bvhDirty = true; }
};