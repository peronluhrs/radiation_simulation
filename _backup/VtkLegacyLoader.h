#pragma once
#include <string>
#include <vector>
#include <array>

struct VtkMesh {
    std::vector<std::array<float, 3>> vertices; // x,y,z
    std::vector<std::array<int, 3>> triangles;  // indices
    std::vector<std::array<int, 2>> lines;      // indices (edges)
    bool empty() const { return vertices.empty(); }
};

class VtkLegacyLoader {
  public:
    // Charge un fichier VTK legacy (POLYDATA ASCII) très basique:
    // - POINTS <N> float
    // - POLYGONS <M> ...
    // - LINES <K> ...
    static bool load(const std::string &path, VtkMesh &out, std::string *err = nullptr);
};
