#pragma once

#include "geometry/Object3D.h"

#include <vector>

class MeshObject : public GeometricPrimitive {
  public:
    MeshObject(const std::string &name, std::vector<glm::vec3> vertices, std::vector<uint32_t> indices);

    const std::vector<glm::vec3> &getVertices() const { return m_vertices; }
    const std::vector<uint32_t> &getIndices() const { return m_indices; }

    size_t getVertexCount() const { return m_vertices.size(); }
    size_t getTriangleCount() const { return m_indices.size() / 3; }

  protected:
    IntersectionResult intersectLocal(const Ray &ray) const override;
    AABB computeLocalBounds() const override;

  private:
    std::vector<glm::vec3> m_vertices;
    std::vector<uint32_t> m_indices;
};
