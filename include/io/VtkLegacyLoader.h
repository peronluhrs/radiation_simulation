#pragma once
#include <string>
#include <vector>
#include <array>
#include <glm/vec3.hpp>

// Maillage VTK minimal (legacy ASCII)
// - vertices : positions 3D
// - triangles : faces triangulées (index)
// - lines : segments optionnels
struct VtkMesh {
    std::vector<glm::vec3> vertices;
    std::vector<std::array<int, 3>> triangles;
    std::vector<std::array<int, 2>> lines;
};

class VtkLegacyLoader {
  public:
    // Charge .vtk legacy ASCII (POLYDATA) avec POINTS / POLYGONS (/LINES optionnel).
    static bool load(const std::string &path, VtkMesh &out, std::string *err = nullptr);
};
