#pragma once

#include "geometry/Object3D.h"

enum class CylinderAxis {
    X_AXIS,
    Y_AXIS,
    Z_AXIS
};

class Cylinder : public GeometricPrimitive {
public:
    Cylinder(const std::string& name, float radius = 1.0f, float height = 2.0f, CylinderAxis axis = CylinderAxis::Y_AXIS);
    
    // Dimensions
    float getRadius() const { return m_radius; }
    void setRadius(float radius) { 
        m_radius = std::max(0.0f, radius); 
        m_boundsDirty = true; 
    }
    
    float getHeight() const { return m_height; }
    void setHeight(float height) { 
        m_height = std::max(0.0f, height); 
        m_boundsDirty = true; 
    }
    
    CylinderAxis getAxis() const { return m_axis; }
    void setAxis(CylinderAxis axis) { 
        m_axis = axis; 
        m_boundsDirty = true; 
    }

    // Géométrie
    IntersectionResult intersectLocal(const Ray& ray) const override;
    AABB computeLocalBounds() const override;
    
    // Propriétés géométriques
    float getVolume() const { return PI * m_radius * m_radius * m_height; }
    float getSurfaceArea() const { 
        return 2.0f * PI * m_radius * (m_radius + m_height); 
    }
    
    // Test de point intérieur
    bool containsPoint(const glm::vec3& point) const;
    
    // Création de cylindres spécialisés
    static std::shared_ptr<Cylinder> createTube(const std::string& name, float innerRadius, float outerRadius, float height);
    static std::shared_ptr<Cylinder> createPipe(const std::string& name, float radius, float height, float thickness);

private:
    float m_radius;
    float m_height;
    CylinderAxis m_axis;
    
    // Helpers pour les différents axes
    glm::vec3 getAxisVector() const;
    void getAxisIndices(int& axisIndex, int& u, int& v) const;
    
    // Calcul de la normale
    glm::vec3 computeNormal(const glm::vec3& point, bool onCap = false) const;
    
    // Intersection avec les caps (bases)
    bool intersectCaps(const Ray& ray, float& t, glm::vec3& normal) const;
    bool intersectCap(const Ray& ray, float capZ, float& t) const;
};