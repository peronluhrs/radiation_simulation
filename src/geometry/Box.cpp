#include "geometry/Box.h"
#include <algorithm>

Box::Box(const std::string& name, const glm::vec3& size) 
    : GeometricPrimitive(name), m_size(size) {
}

IntersectionResult Box::intersectLocal(const Ray& ray) const {
    IntersectionResult result;
    
    // Boîte centrée à l'origine avec dimensions m_size
    glm::vec3 minBounds = -m_size * 0.5f;
    glm::vec3 maxBounds = m_size * 0.5f;
    
    // Test d'intersection avec l'AABB
    float tMin = (minBounds.x - ray.origin.x) / ray.direction.x;
    float tMax = (maxBounds.x - ray.origin.x) / ray.direction.x;
    
    if (tMin > tMax) std::swap(tMin, tMax);
    
    float tyMin = (minBounds.y - ray.origin.y) / ray.direction.y;
    float tyMax = (maxBounds.y - ray.origin.y) / ray.direction.y;
    
    if (tyMin > tyMax) std::swap(tyMin, tyMax);
    
    if (tMin > tyMax || tyMin > tMax) {
        return result; // Pas d'intersection
    }
    
    tMin = std::max(tMin, tyMin);
    tMax = std::min(tMax, tyMax);
    
    float tzMin = (minBounds.z - ray.origin.z) / ray.direction.z;
    float tzMax = (maxBounds.z - ray.origin.z) / ray.direction.z;
    
    if (tzMin > tzMax) std::swap(tzMin, tzMax);
    
    if (tMin > tzMax || tzMin > tMax) {
        return result; // Pas d'intersection
    }
    
    tMin = std::max(tMin, tzMin);
    tMax = std::min(tMax, tzMax);
    
    // Vérification des limites du rayon
    if (tMax < ray.tMin || tMin > ray.tMax) {
        return result;
    }
    
    // Intersection trouvée
    float t = tMin > ray.tMin ? tMin : tMax;
    if (t < ray.tMin || t > ray.tMax) {
        return result;
    }
    
    result.hit = true;
    result.distance = t;
    result.point = ray.at(t);
    result.normal = computeNormal(result.point);
    
    return result;
}

AABB Box::computeLocalBounds() const {
    glm::vec3 halfSize = m_size * 0.5f;
    return AABB(-halfSize, halfSize);
}

bool Box::containsPoint(const glm::vec3& point) const {
    glm::vec3 halfSize = m_size * 0.5f;
    return (point.x >= -halfSize.x && point.x <= halfSize.x &&
            point.y >= -halfSize.y && point.y <= halfSize.y &&
            point.z >= -halfSize.z && point.z <= halfSize.z);
}

glm::vec3 Box::computeNormal(const glm::vec3& point) const {
    glm::vec3 halfSize = m_size * 0.5f;
    glm::vec3 abs_point = glm::abs(point);
    
    // Trouver la face la plus proche
    float maxComponent = std::max({abs_point.x / halfSize.x, 
                                  abs_point.y / halfSize.y, 
                                  abs_point.z / halfSize.z});
    
    glm::vec3 normal(0.0f);
    
    if (std::abs(abs_point.x / halfSize.x - maxComponent) < EPSILON) {
        normal.x = point.x > 0.0f ? 1.0f : -1.0f;
    } else if (std::abs(abs_point.y / halfSize.y - maxComponent) < EPSILON) {
        normal.y = point.y > 0.0f ? 1.0f : -1.0f;
    } else {
        normal.z = point.z > 0.0f ? 1.0f : -1.0f;
    }
    
    return normal;
}

std::shared_ptr<Box> Box::createWall(const std::string& name, float width, float height, float thickness) {
    return std::make_shared<Box>(name, glm::vec3(width, height, thickness));
}

std::shared_ptr<Box> Box::createRoom(const std::string& name, const glm::vec3& dimensions, float wallThickness) {
    // Créer une pièce en tant que boîte creuse
    auto room = std::make_shared<Box>(name + "_Exterior", dimensions);
    
    // Note: Pour une vraie pièce creuse, nous aurions besoin d'une CompositeObject
    // avec soustraction booléenne. Pour l'instant, retournons juste l'extérieur.
    
    return room;
}