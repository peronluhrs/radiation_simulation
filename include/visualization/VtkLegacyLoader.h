#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "glm_simple.h"
#include "geometry/Object3D.h"

struct VtkMesh {
    std::vector<glm::vec3> vertices;
    std::vector<uint32_t> triangles;
    std::vector<uint32_t> lines;
    AABB bounds;
};

class VtkLegacyLoader {
  public:
    static bool load(const std::string &path, VtkMesh &outMesh, std::string *err = nullptr);
};
