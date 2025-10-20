#pragma once

#include "geometry/Object3D.h"

class Plane : public GeometricPrimitive {
public:
    Plane(const std::string& name, const glm::vec3& normal = glm::vec3(0.0f, 1.0f, 0.0f), float distance = 0.0f);
    Plane(const std::string& name, const glm::vec3& point, const glm::vec3& normal);
    
    // Équation du plan : normal · point + distance = 0
    const glm::vec3& getNormal() const { return m_normal; }
    void setNormal(const glm::vec3& normal) { 
        m_normal = glm::normalize(normal); 
        m_boundsDirty = true; 
    }
    
    float getDistance() const { return m_distance; }
    void setDistance(float distance) { 
        m_distance = distance; 
        m_boundsDirty = true; 
    }
    
    // Configuration à partir d'un point sur le plan
    void setFromPointAndNormal(const glm::vec3& point, const glm::vec3& normal);
    
    // Dimensions pour la visualisation (plan infini par défaut)
    const glm::vec2& getSize() const { return m_size; }
    void setSize(const glm::vec2& size) { 
        m_size = size; 
        m_boundsDirty = true; 
    }
    
    bool isInfinite() const { return m_size.x <= 0.0f || m_size.y <= 0.0f; }

    // Géométrie
    IntersectionResult intersectLocal(const Ray& ray) const override;
    AABB computeLocalBounds() const override;
    
    // Distance signée d'un point au plan
    float distanceToPoint(const glm::vec3& point) const;
    
    // Test de quel côté du plan se trouve un point
    bool isPointAbove(const glm::vec3& point) const { return distanceToPoint(point) > 0.0f; }
    
    // Projection d'un point sur le plan
    glm::vec3 projectPoint(const glm::vec3& point) const;
    
    // Création de plans spécialisés
    static std::shared_ptr<Plane> createFloor(const std::string& name, float y = 0.0f);
    static std::shared_ptr<Plane> createWall(const std::string& name, const glm::vec3& point, const glm::vec3& normal);
    static std::shared_ptr<Plane> createFinitePlane(const std::string& name, const glm::vec3& center, 
                                                   const glm::vec3& normal, const glm::vec2& size);

private:
    glm::vec3 m_normal;
    float m_distance;
    glm::vec2 m_size{0.0f}; // 0 = infini, > 0 = fini
    
    // Système de coordonnées local pour les plans finis
    glm::vec3 m_uAxis, m_vAxis;
    void updateLocalAxes();
    
    // Test si un point projeté est dans les limites du plan fini
    bool isPointInBounds(const glm::vec3& localPoint) const;
};