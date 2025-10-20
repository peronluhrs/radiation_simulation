#pragma once

#include "geometry/Object3D.h"

class Box : public GeometricPrimitive {
public:
    Box(const std::string& name, const glm::vec3& size = glm::vec3(1.0f));
    
    // Dimensions
    const glm::vec3& getSize() const { return m_size; }
    void setSize(const glm::vec3& size) { 
        m_size = size; 
        m_boundsDirty = true; 
    }
    
    float getWidth() const { return m_size.x; }
    float getHeight() const { return m_size.y; }
    float getDepth() const { return m_size.z; }
    
    void setWidth(float width) { m_size.x = width; m_boundsDirty = true; }
    void setHeight(float height) { m_size.y = height; m_boundsDirty = true; }
    void setDepth(float depth) { m_size.z = depth; m_boundsDirty = true; }

    // Géométrie
    IntersectionResult intersectLocal(const Ray& ray) const override;
    AABB computeLocalBounds() const override;
    
    // Propriétés géométriques
    float getVolume() const { return m_size.x * m_size.y * m_size.z; }
    float getSurfaceArea() const { 
        return 2.0f * (m_size.x * m_size.y + m_size.y * m_size.z + m_size.z * m_size.x); 
    }
    
    // Test de point intérieur
    bool containsPoint(const glm::vec3& point) const;
    
    // Création de boîtes spécialisées
    static std::shared_ptr<Box> createWall(const std::string& name, float width, float height, float thickness);
    static std::shared_ptr<Box> createRoom(const std::string& name, const glm::vec3& dimensions, float wallThickness);

private:
    glm::vec3 m_size;
    
    // Calcul de la normale pour une intersection
    glm::vec3 computeNormal(const glm::vec3& point) const;
};