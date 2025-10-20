#pragma once

#include "geometry/Object3D.h"

class Sphere : public GeometricPrimitive {
public:
    Sphere(const std::string& name, float radius = 1.0f);
    
    // Rayon
    float getRadius() const { return m_radius; }
    void setRadius(float radius) { 
        m_radius = std::max(0.0f, radius); 
        m_boundsDirty = true; 
    }

    // Géométrie
    IntersectionResult intersectLocal(const Ray& ray) const override;
    AABB computeLocalBounds() const override;
    
    // Propriétés géométriques
    float getVolume() const { return (4.0f / 3.0f) * PI * m_radius * m_radius * m_radius; }
    float getSurfaceArea() const { return 4.0f * PI * m_radius * m_radius; }
    float getDiameter() const { return 2.0f * m_radius; }
    
    // Test de point intérieur
    bool containsPoint(const glm::vec3& point) const;
    
    // Distance au centre
    float distanceToCenter(const glm::vec3& point) const;
    
    // Création de sphères spécialisées
    static std::shared_ptr<Sphere> createHollowSphere(const std::string& name, float innerRadius, float outerRadius);

private:
    float m_radius;
    
    // Calcul de la normale (toujours vers l'extérieur)
    glm::vec3 computeNormal(const glm::vec3& point) const;
};