#include "visualization/VtkLegacyLoader.h"

#include "common.h"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <sstream>

namespace {
std::string trim(const std::string &text) {
    const auto first = std::find_if_not(text.begin(), text.end(), [](unsigned char c) { return std::isspace(c); });
    const auto last = std::find_if_not(text.rbegin(), text.rend(), [](unsigned char c) { return std::isspace(c); }).base();
    if (first >= last)
        return {};
    return std::string(first, last);
}

std::string toUpper(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) { return static_cast<char>(std::toupper(c)); });
    return value;
}

bool validateIndex(uint32_t idx, size_t vertexCount, std::string *err) {
    if (idx >= vertexCount) {
        if (err)
            *err = "Indice de point hors limites dans le fichier VTK (" + std::to_string(idx) + ")";
        return false;
    }
    return true;
}
}

bool VtkLegacyLoader::load(const std::string &path, VtkMesh &outMesh, std::string *err) {
    outMesh = VtkMesh{};

    std::ifstream input(path);
    if (!input.is_open()) {
        if (err)
            *err = "Impossible d'ouvrir le fichier VTK.";
        return false;
    }

    std::string line;
    if (!std::getline(input, line)) {
        if (err)
            *err = "Fichier VTK vide.";
        return false;
    }

    // Ligne descriptive (commentaire) – ignorée.
    std::getline(input, line);

    // Format (ASCII/BINARY)
    if (!std::getline(input, line)) {
        if (err)
            *err = "Format du fichier VTK invalide (ligne ASCII manquante).";
        return false;
    }
    std::string format = toUpper(trim(line));
    if (format.find("ASCII") == std::string::npos) {
        if (err)
            *err = "Seuls les fichiers VTK ASCII sont supportés.";
        return false;
    }

    if (!std::getline(input, line)) {
        if (err)
            *err = "Entrée VTK incomplète (ligne DATASET manquante).";
        return false;
    }
    std::istringstream datasetStream(line);
    std::string datasetKeyword;
    std::string datasetType;
    datasetStream >> datasetKeyword >> datasetType;
    if (toUpper(datasetKeyword) != "DATASET") {
        if (err)
            *err = "Fichier VTK: mot-clé DATASET manquant.";
        return false;
    }
    datasetType = toUpper(datasetType);
    if (datasetType != "POLYDATA" && datasetType != "UNSTRUCTURED_GRID") {
        if (err)
            *err = "Type de dataset VTK non supporté: " + datasetType;
        return false;
    }

    std::vector<std::vector<uint32_t>> cellConnectivity;
    std::vector<int> cellTypes;
    bool hasCells = false;
    bool hasCellTypes = false;

    auto pushTriangle = [&](uint32_t a, uint32_t b, uint32_t c) -> bool {
        if (!validateIndex(a, outMesh.vertices.size(), err) || !validateIndex(b, outMesh.vertices.size(), err) ||
            !validateIndex(c, outMesh.vertices.size(), err))
            return false;
        outMesh.triangles.push_back(a);
        outMesh.triangles.push_back(b);
        outMesh.triangles.push_back(c);
        return true;
    };

    auto pushLine = [&](uint32_t a, uint32_t b) -> bool {
        if (!validateIndex(a, outMesh.vertices.size(), err) || !validateIndex(b, outMesh.vertices.size(), err))
            return false;
        outMesh.lines.push_back(a);
        outMesh.lines.push_back(b);
        return true;
    };

    auto triangulateFan = [&](const std::vector<uint32_t> &indices) -> bool {
        if (indices.size() < 3)
            return true;
        for (size_t i = 1; i + 1 < indices.size(); ++i) {
            if (!pushTriangle(indices[0], indices[i], indices[i + 1]))
                return false;
        }
        return true;
    };

    auto addPolyline = [&](const std::vector<uint32_t> &indices) -> bool {
        if (indices.size() < 2)
            return true;
        for (size_t i = 0; i + 1 < indices.size(); ++i) {
            if (!pushLine(indices[i], indices[i + 1]))
                return false;
        }
        return true;
    };

    std::string token;
    while (input >> token) {
        std::string upperToken = toUpper(token);
        if (upperToken == "POINTS") {
            size_t pointCount = 0;
            std::string typeName;
            if (!(input >> pointCount >> typeName)) {
                if (err)
                    *err = "Définition des points VTK invalide.";
                return false;
            }
            outMesh.vertices.reserve(pointCount);
            for (size_t i = 0; i < pointCount; ++i) {
                double x = 0.0, y = 0.0, z = 0.0;
                if (!(input >> x >> y >> z)) {
                    if (err)
                        *err = "Lecture des coordonnées des points VTK échouée.";
                    return false;
                }
                glm::vec3 p(static_cast<float>(x), static_cast<float>(y), static_cast<float>(z));
                outMesh.vertices.push_back(p);
                outMesh.bounds.expand(p);
            }
        } else if (upperToken == "POLYGONS") {
            size_t polyCount = 0;
            size_t totalIndices = 0;
            if (!(input >> polyCount >> totalIndices)) {
                if (err)
                    *err = "Définition POLYGONS invalide.";
                return false;
            }
            for (size_t i = 0; i < polyCount; ++i) {
                int vertexCount = 0;
                if (!(input >> vertexCount) || vertexCount < 0) {
                    if (err)
                        *err = "Définition de polygone VTK invalide.";
                    return false;
                }
                std::vector<uint32_t> indices(static_cast<size_t>(vertexCount));
                for (int j = 0; j < vertexCount; ++j) {
                    int idx = -1;
                    if (!(input >> idx)) {
                        if (err)
                            *err = "Lecture des indices de polygone VTK échouée.";
                        return false;
                    }
                    if (idx < 0) {
                        if (err)
                            *err = "Indice négatif trouvé dans un polygone VTK.";
                        return false;
                    }
                    indices[static_cast<size_t>(j)] = static_cast<uint32_t>(idx);
                }
                if (!triangulateFan(indices))
                    return false;
            }
        } else if (upperToken == "LINES") {
            size_t lineCount = 0;
            size_t totalIndices = 0;
            if (!(input >> lineCount >> totalIndices)) {
                if (err)
                    *err = "Définition LINES invalide.";
                return false;
            }
            for (size_t i = 0; i < lineCount; ++i) {
                int vertexCount = 0;
                if (!(input >> vertexCount) || vertexCount < 0) {
                    if (err)
                        *err = "Définition de ligne VTK invalide.";
                    return false;
                }
                std::vector<uint32_t> indices(static_cast<size_t>(vertexCount));
                for (int j = 0; j < vertexCount; ++j) {
                    int idx = -1;
                    if (!(input >> idx)) {
                        if (err)
                            *err = "Lecture des indices de ligne VTK échouée.";
                        return false;
                    }
                    if (idx < 0) {
                        if (err)
                            *err = "Indice négatif trouvé dans une ligne VTK.";
                        return false;
                    }
                    indices[static_cast<size_t>(j)] = static_cast<uint32_t>(idx);
                }
                if (!addPolyline(indices))
                    return false;
            }
        } else if (upperToken == "CELLS") {
            size_t cellCount = 0;
            size_t totalIndices = 0;
            if (!(input >> cellCount >> totalIndices)) {
                if (err)
                    *err = "Définition CELLS invalide.";
                return false;
            }
            cellConnectivity.resize(cellCount);
            for (size_t i = 0; i < cellCount; ++i) {
                int vertexCount = 0;
                if (!(input >> vertexCount) || vertexCount < 0) {
                    if (err)
                        *err = "Définition de cellule VTK invalide.";
                    return false;
                }
                auto &indices = cellConnectivity[i];
                indices.resize(static_cast<size_t>(vertexCount));
                for (int j = 0; j < vertexCount; ++j) {
                    int idx = -1;
                    if (!(input >> idx)) {
                        if (err)
                            *err = "Lecture des indices de cellule VTK échouée.";
                        return false;
                    }
                    if (idx < 0) {
                        if (err)
                            *err = "Indice négatif trouvé dans une cellule VTK.";
                        return false;
                    }
                    indices[static_cast<size_t>(j)] = static_cast<uint32_t>(idx);
                }
            }
            hasCells = true;
        } else if (upperToken == "CELL_TYPES") {
            size_t count = 0;
            if (!(input >> count)) {
                if (err)
                    *err = "Définition CELL_TYPES invalide.";
                return false;
            }
            cellTypes.resize(count);
            for (size_t i = 0; i < count; ++i) {
                int cellType = 0;
                if (!(input >> cellType)) {
                    if (err)
                        *err = "Lecture des CELL_TYPES échouée.";
                    return false;
                }
                cellTypes[i] = cellType;
            }
            hasCellTypes = true;
        } else if (upperToken == "TRIANGLE_STRIPS") {
            if (err)
                *err = "TRIANGLE_STRIPS n'est pas encore supporté.";
            return false;
        } else if (upperToken == "POINT_DATA" || upperToken == "CELL_DATA") {
            // Fin des informations géométriques.
            break;
        } else {
            std::getline(input, line); // ignore reste de la ligne
        }
    }

    if (hasCells) {
        if (!hasCellTypes || cellConnectivity.size() != cellTypes.size()) {
            if (err)
                *err = "CELL_TYPES manquant ou incohérent avec CELLS.";
            return false;
        }
        for (size_t i = 0; i < cellConnectivity.size(); ++i) {
            const auto &indices = cellConnectivity[i];
            int type = cellTypes[i];
            switch (type) {
            case 3:  // VTK_LINE
            case 4:  // VTK_POLY_LINE
                if (!addPolyline(indices))
                    return false;
                break;
            case 5:  // VTK_TRIANGLE
            case 7:  // VTK_POLYGON
            case 9:  // VTK_QUAD
                if (!triangulateFan(indices))
                    return false;
                break;
            case 6: { // VTK_TRIANGLE_STRIP
                if (indices.size() < 3)
                    break;
                for (size_t j = 0; j + 2 < indices.size(); ++j) {
                    uint32_t a = indices[j];
                    uint32_t b = indices[j + 1];
                    uint32_t c = indices[j + 2];
                    if (j % 2 == 0) {
                        if (!pushTriangle(a, b, c))
                            return false;
                    } else {
                        if (!pushTriangle(b, a, c))
                            return false;
                    }
                }
                break;
            }
            default:
                if (err)
                    *err = "Type de cellule VTK non supporté: " + std::to_string(type);
                return false;
            }
        }
    }

    if (outMesh.vertices.empty()) {
        if (err)
            *err = "Aucun point n'a été trouvé dans le fichier VTK.";
        return false;
    }

    return true;
}
