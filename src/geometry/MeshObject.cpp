#include "geometry/MeshObject.h"

#include <algorithm>
#include <cmath>

MeshObject::MeshObject(const std::string &name, std::vector<glm::vec3> vertices, std::vector<uint32_t> indices)
    : GeometricPrimitive(name), m_vertices(std::move(vertices)), m_indices(std::move(indices)) {
    m_boundsDirty = true;
}

IntersectionResult MeshObject::intersectLocal(const Ray &ray) const {
    IntersectionResult closest;

    if (m_vertices.empty() || m_indices.size() < 3)
        return closest;

    float closestDistance = ray.tMax;

    for (size_t i = 0; i + 2 < m_indices.size(); i += 3) {
        uint32_t i0 = m_indices[i];
        uint32_t i1 = m_indices[i + 1];
        uint32_t i2 = m_indices[i + 2];

        if (i0 >= m_vertices.size() || i1 >= m_vertices.size() || i2 >= m_vertices.size())
            continue;

        const glm::vec3 &v0 = m_vertices[i0];
        const glm::vec3 &v1 = m_vertices[i1];
        const glm::vec3 &v2 = m_vertices[i2];

        glm::vec3 edge1 = v1 - v0;
        glm::vec3 edge2 = v2 - v0;
        glm::vec3 pvec = glm::cross(ray.direction, edge2);
        float det = glm::dot(edge1, pvec);

        if (std::abs(det) < EPSILON)
            continue;

        float invDet = 1.0f / det;
        glm::vec3 tvec = ray.origin - v0;
        float u = glm::dot(tvec, pvec) * invDet;
        if (u < 0.0f || u > 1.0f)
            continue;

        glm::vec3 qvec = glm::cross(tvec, edge1);
        float v = glm::dot(ray.direction, qvec) * invDet;
        if (v < 0.0f || u + v > 1.0f)
            continue;

        float t = glm::dot(edge2, qvec) * invDet;
        if (t < ray.tMin || t > closestDistance)
            continue;

        closestDistance = t;
        closest.hit = true;
        closest.distance = t;
        closest.point = ray.at(t);

        glm::vec3 normal = glm::normalize(glm::cross(edge1, edge2));
        if (det < 0.0f)
            normal = -normal;
        closest.normal = normal;
    }

    return closest;
}

AABB MeshObject::computeLocalBounds() const {
    AABB bounds;

    if (m_vertices.empty())
        return bounds;

    for (const auto &vertex : m_vertices) {
        bounds.expand(vertex);
    }

    return bounds;
}
