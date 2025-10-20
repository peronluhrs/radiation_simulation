#include "geometry/Object3D.h"

uint32_t Object3D::s_nextId = 1;

// Transform implementation
glm::mat4 Transform::getMatrix() const {
    glm::mat4 T = glm::translate(glm::mat4(1.0f), position);
    glm::mat4 R = glm::mat4_cast(rotation);
    glm::mat4 S = glm::scale(glm::mat4(1.0f), scale);
    return T * R * S;
}

glm::mat4 Transform::getInverseMatrix() const {
    glm::mat4 invS = glm::scale(glm::mat4(1.0f), 1.0f / scale);
    glm::mat4 invR = glm::mat4_cast(glm::conjugate(rotation));
    glm::mat4 invT = glm::translate(glm::mat4(1.0f), -position);
    return invS * invR * invT;
}

void Transform::setFromMatrix(const glm::mat4& matrix) {
    // Décomposition de matrice simplifiée
    position = glm::vec3(matrix[3]);
    
    // Extraction de l'échelle
    scale.x = glm::length(glm::vec3(matrix[0]));
    scale.y = glm::length(glm::vec3(matrix[1]));
    scale.z = glm::length(glm::vec3(matrix[2]));
    
    // Extraction de la rotation (simplifiée)
    if (scale.x != 0.0f && scale.y != 0.0f && scale.z != 0.0f) {
        // Pour la démonstration, on utilise une rotation identité
        rotation = glm::quat(); // Identité
    }
}

// AABB implementation
float AABB::surfaceArea() const {
    if (!isValid()) return 0.0f;
    glm::vec3 d = max - min;
    return 2.0f * (d.x * d.y + d.y * d.z + d.z * d.x);
}

float AABB::volume() const {
    if (!isValid()) return 0.0f;
    glm::vec3 d = max - min;
    return d.x * d.y * d.z;
}

void AABB::expand(const glm::vec3& point) {
    min = glm::min(min, point);
    max = glm::max(max, point);
}

void AABB::expand(const AABB& other) {
    if (!other.isValid()) return;
    if (!isValid()) {
        *this = other;
        return;
    }
    min = glm::min(min, other.min);
    max = glm::max(max, other.max);
}

bool AABB::contains(const glm::vec3& point) const {
    return (point.x >= min.x && point.x <= max.x &&
            point.y >= min.y && point.y <= max.y &&
            point.z >= min.z && point.z <= max.z);
}

bool AABB::intersects(const AABB& other) const {
    return (min.x <= other.max.x && max.x >= other.min.x &&
            min.y <= other.max.y && max.y >= other.min.y &&
            min.z <= other.max.z && max.z >= other.min.z);
}

bool AABB::intersects(const Ray& ray, float& tMin, float& tMax) const {
    tMin = (min.x - ray.origin.x) / ray.direction.x;
    tMax = (max.x - ray.origin.x) / ray.direction.x;
    
    if (tMin > tMax) std::swap(tMin, tMax);
    
    float tyMin = (min.y - ray.origin.y) / ray.direction.y;
    float tyMax = (max.y - ray.origin.y) / ray.direction.y;
    
    if (tyMin > tyMax) std::swap(tyMin, tyMax);
    
    if (tMin > tyMax || tyMin > tMax) return false;
    
    tMin = std::max(tMin, tyMin);
    tMax = std::min(tMax, tyMax);
    
    float tzMin = (min.z - ray.origin.z) / ray.direction.z;
    float tzMax = (max.z - ray.origin.z) / ray.direction.z;
    
    if (tzMin > tzMax) std::swap(tzMin, tzMax);
    
    if (tMin > tzMax || tzMin > tMax) return false;
    
    tMin = std::max(tMin, tzMin);
    tMax = std::min(tMax, tzMax);
    
    return tMax >= 0.0f;
}

// Object3D implementation
Object3D::Object3D(const std::string& name) 
    : m_name(name), m_id(s_nextId++) {
}

const AABB& Object3D::getBounds() const {
    if (m_boundsDirty) {
        // Calcul des bornes locales
        AABB localBounds = computeLocalBounds();
        
        if (localBounds.isValid()) {
            // Transformation vers l'espace monde
            glm::mat4 transform = m_transform.getMatrix();
            
            // Transformation des 8 coins de la boîte
            m_bounds = AABB();
            glm::vec3 corners[8] = {
                {localBounds.min.x, localBounds.min.y, localBounds.min.z},
                {localBounds.max.x, localBounds.min.y, localBounds.min.z},
                {localBounds.min.x, localBounds.max.y, localBounds.min.z},
                {localBounds.max.x, localBounds.max.y, localBounds.min.z},
                {localBounds.min.x, localBounds.min.y, localBounds.max.z},
                {localBounds.max.x, localBounds.min.y, localBounds.max.z},
                {localBounds.min.x, localBounds.max.y, localBounds.max.z},
                {localBounds.max.x, localBounds.max.y, localBounds.max.z}
            };
            
            for (int i = 0; i < 8; ++i) {
                glm::vec4 worldCorner = transform * glm::vec4(corners[i], 1.0f);
                m_bounds.expand(glm::vec3(worldCorner));
            }
        }
        
        m_boundsDirty = false;
    }
    
    return m_bounds;
}

// GeometricPrimitive implementation
IntersectionResult GeometricPrimitive::intersect(const Ray& ray) const {
    // Transformation du rayon vers l'espace local
    Ray localRay = transformRayToLocal(ray);
    
    // Intersection dans l'espace local
    IntersectionResult localResult = intersectLocal(localRay);
    
    if (!localResult.hit) {
        return localResult;
    }
    
    // Transformation du résultat vers l'espace monde
    return transformResultToWorld(localResult, ray);
}

Ray GeometricPrimitive::transformRayToLocal(const Ray& ray) const {
    glm::mat4 invTransform = m_transform.getInverseMatrix();
    
    glm::vec4 localOrigin = invTransform * glm::vec4(ray.origin, 1.0f);
    glm::vec4 localDirection = invTransform * glm::vec4(ray.direction, 0.0f);
    
    Ray localRay;
    localRay.origin = glm::vec3(localOrigin);
    localRay.direction = glm::normalize(glm::vec3(localDirection));
    localRay.tMin = ray.tMin;
    localRay.tMax = ray.tMax;
    
    return localRay;
}

IntersectionResult GeometricPrimitive::transformResultToWorld(const IntersectionResult& result, 
                                                             const Ray& originalRay) const {
    IntersectionResult worldResult = result;
    
    glm::mat4 transform = m_transform.getMatrix();
    glm::mat4 normalTransform = glm::transpose(m_transform.getInverseMatrix());
    
    // Transformation du point d'intersection
    glm::vec4 worldPoint = transform * glm::vec4(result.point, 1.0f);
    worldResult.point = glm::vec3(worldPoint);
    
    // Transformation de la normale
    glm::vec4 worldNormal = normalTransform * glm::vec4(result.normal, 0.0f);
    worldResult.normal = glm::normalize(glm::vec3(worldNormal));
    
    // Recalcul de la distance dans l'espace monde
    worldResult.distance = glm::length(worldResult.point - originalRay.origin);
    
    // Référence vers cet objet
    worldResult.object = std::const_pointer_cast<Object3D>(shared_from_this());
    worldResult.material = m_material;
    
    return worldResult;
}