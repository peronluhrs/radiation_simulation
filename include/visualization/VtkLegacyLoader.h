#pragma once

#include <optional>
#include <string>
#include <vector>

#include <glm/vec3.hpp>

class VtkLegacyLoader {
  public:
    struct MeshData {
        std::vector<glm::vec3> vertices;
        std::vector<uint32_t> indices; // Triangulated faces (3 indices per face)
    };

    static std::optional<MeshData> load(const std::string &filePath, std::string &errorMessage);
};
