#pragma once

#include "common.h"

// Transformations géométriques
struct Transform {
    glm::vec3 position{0.0f};
    glm::quat rotation{1.0f, 0.0f, 0.0f, 0.0f};
    glm::vec3 scale{1.0f};
    
    glm::mat4 getMatrix() const;
    glm::mat4 getInverseMatrix() const;
    void setFromMatrix(const glm::mat4& matrix);
};

// Boîte englobante alignée sur les axes
struct AABB {
    glm::vec3 min{std::numeric_limits<float>::max()};
    glm::vec3 max{std::numeric_limits<float>::lowest()};
    
    AABB() = default;
    AABB(const glm::vec3& minPoint, const glm::vec3& maxPoint) : min(minPoint), max(maxPoint) {}
    
    bool isValid() const { return min.x <= max.x && min.y <= max.y && min.z <= max.z; }
    glm::vec3 center() const { return (min + max) * 0.5f; }
    glm::vec3 size() const { return max - min; }
    float surfaceArea() const;
    float volume() const;
    
    void expand(const glm::vec3& point);
    void expand(const AABB& other);
    bool contains(const glm::vec3& point) const;
    bool intersects(const AABB& other) const;
    bool intersects(const Ray& ray, float& tMin, float& tMax) const;
};

// Classe de base pour tous les objets 3D
class Object3D : public std::enable_shared_from_this<Object3D> {
public:
    Object3D(const std::string& name);
    virtual ~Object3D() = default;

    // Propriétés de base
    const std::string& getName() const { return m_name; }
    void setName(const std::string& name) { m_name = name; }
    
    uint32_t getId() const { return m_id; }

    // Transformation
    const Transform& getTransform() const { return m_transform; }
    void setTransform(const Transform& transform) { 
        m_transform = transform; 
        m_boundsDirty = true; 
    }
    
    void setPosition(const glm::vec3& position) { 
        m_transform.position = position; 
        m_boundsDirty = true; 
    }
    
    void setRotation(const glm::quat& rotation) { 
        m_transform.rotation = rotation; 
        m_boundsDirty = true; 
    }
    
    void setScale(const glm::vec3& scale) { 
        m_transform.scale = scale; 
        m_boundsDirty = true; 
    }

    // Matériau
    std::shared_ptr<Material> getMaterial() const { return m_material; }
    void setMaterial(std::shared_ptr<Material> material) { m_material = material; }

    // Intersection avec les rayons (méthode virtuelle pure)
    virtual IntersectionResult intersect(const Ray& ray) const = 0;
    
    // Boîte englobante
    const AABB& getBounds() const;
    virtual AABB computeLocalBounds() const = 0;

    // Propriétés de rendu
    bool isVisible() const { return m_visible; }
    void setVisible(bool visible) { m_visible = visible; }
    
    const glm::vec3& getColor() const { return m_color; }
    void setColor(const glm::vec3& color) { m_color = color; }
    
    float getOpacity() const { return m_opacity; }
    void setOpacity(float opacity) { m_opacity = glm::clamp(opacity, 0.0f, 1.0f); }

    // Sélection
    bool isSelected() const { return m_selected; }
    void setSelected(bool selected) { m_selected = selected; }

protected:
    std::string m_name;
    uint32_t m_id;
    Transform m_transform;
    std::shared_ptr<Material> m_material;
    
    // Boîte englobante mise en cache
    mutable AABB m_bounds;
    mutable bool m_boundsDirty = true;
    
    // Propriétés de rendu
    bool m_visible = true;
    glm::vec3 m_color{0.7f, 0.7f, 0.7f};
    float m_opacity = 1.0f;
    bool m_selected = false;

private:
    static uint32_t s_nextId;
};

// Classe de base pour les primitives géométriques
class GeometricPrimitive : public Object3D {
public:
    GeometricPrimitive(const std::string& name) : Object3D(name) {}
    
    // Intersection dans l'espace local (sans transformation)
    virtual IntersectionResult intersectLocal(const Ray& ray) const = 0;
    
    // Implémentation générique avec transformation
    IntersectionResult intersect(const Ray& ray) const override;

protected:
    // Transformation du rayon dans l'espace local
    Ray transformRayToLocal(const Ray& ray) const;
    
    // Transformation du résultat vers l'espace monde
    IntersectionResult transformResultToWorld(const IntersectionResult& result, const Ray& originalRay) const;
};