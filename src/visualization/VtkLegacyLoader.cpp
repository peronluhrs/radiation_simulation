#include "visualization/VtkLegacyLoader.h"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <limits>
#include <sstream>
#include <vector>

namespace {

std::string trim(const std::string &value) {
    const auto begin = value.find_first_not_of(" \t\r\n");
    if (begin == std::string::npos)
        return std::string();
    const auto end = value.find_last_not_of(" \t\r\n");
    return value.substr(begin, end - begin + 1);
}

std::string toUpper(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) {
        return static_cast<char>(std::toupper(c));
    });
    return value;
}

bool readPolygons(std::istream &stream, size_t polygonCount, std::vector<uint32_t> &indices,
                  size_t vertexCount, std::string &errorMessage) {
    size_t processed = 0;
    while (processed < polygonCount) {
        int vertexPerCell = 0;
        if (!(stream >> vertexPerCell)) {
            errorMessage = "Bloc POLYGONS incomplet.";
            return false;
        }

        if (vertexPerCell < 0) {
            errorMessage = "Nombre de sommets négatif dans une cellule.";
            return false;
        }

        std::vector<uint32_t> cell(static_cast<size_t>(vertexPerCell));
        for (int i = 0; i < vertexPerCell; ++i) {
            int index = -1;
            if (!(stream >> index)) {
                errorMessage = "Indice de sommet manquant dans une cellule.";
                return false;
            }
            if (index < 0 || static_cast<size_t>(index) >= vertexCount) {
                errorMessage = "Indice de sommet hors limites.";
                return false;
            }
            cell[static_cast<size_t>(i)] = static_cast<uint32_t>(index);
        }

        if (vertexPerCell >= 3) {
            for (int i = 1; i + 1 < vertexPerCell; ++i) {
                indices.push_back(cell[0]);
                indices.push_back(cell[static_cast<size_t>(i)]);
                indices.push_back(cell[static_cast<size_t>(i + 1)]);
            }
        }

        ++processed;
    }

    stream.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    return true;
}

} // namespace

std::optional<VtkLegacyLoader::MeshData> VtkLegacyLoader::load(const std::string &filePath, std::string &errorMessage) {
    std::ifstream file(filePath);
    if (!file.is_open()) {
        errorMessage = "Impossible d'ouvrir le fichier VTK: " + filePath;
        return std::nullopt;
    }

    std::string line;
    if (!std::getline(file, line)) {
        errorMessage = "Fichier VTK vide.";
        return std::nullopt;
    }

    std::string header = toUpper(trim(line));
    if (header.find("VTK DATAFILE") == std::string::npos) {
        errorMessage = "En-tête VTK invalide.";
        return std::nullopt;
    }

    // Ligne commentaire (ignorée)
    if (!std::getline(file, line)) {
        errorMessage = "Format VTK invalide (commentaire manquant).";
        return std::nullopt;
    }

    if (!std::getline(file, line)) {
        errorMessage = "Format VTK invalide (type ASCII/BINARY manquant).";
        return std::nullopt;
    }

    std::string format = toUpper(trim(line));
    if (format != "ASCII") {
        errorMessage = "Seul le format ASCII des fichiers VTK est pris en charge.";
        return std::nullopt;
    }

    MeshData mesh;
    bool pointsLoaded = false;
    bool polygonsLoaded = false;
    size_t expectedCellTypes = 0;

    while (std::getline(file, line)) {
        std::string trimmed = trim(line);
        if (trimmed.empty())
            continue;

        std::string keywordUpper = toUpper(trimmed);

        if (keywordUpper.rfind("DATASET", 0) == 0) {
            // Aucune vérification stricte du type pour garder la compatibilité (POLYDATA ou UNSTRUCTURED_GRID)
            continue;
        }

        if (keywordUpper.rfind("POINTS", 0) == 0) {
            std::stringstream ss(trimmed);
            std::string keyword;
            size_t count = 0;
            std::string type;
            ss >> keyword >> count >> type;
            if (count == 0) {
                errorMessage = "Le bloc POINTS ne contient aucun sommet.";
                return std::nullopt;
            }

            mesh.vertices.clear();
            mesh.vertices.reserve(count);

            for (size_t i = 0; i < count; ++i) {
                float x = 0.0f, y = 0.0f, z = 0.0f;
                if (!(file >> x >> y >> z)) {
                    errorMessage = "Le bloc POINTS est incomplet.";
                    return std::nullopt;
                }
                mesh.vertices.emplace_back(x, y, z);
            }

            file.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            pointsLoaded = true;
            continue;
        }

        if (keywordUpper.rfind("POLYGONS", 0) == 0 || keywordUpper.rfind("TRIANGLES", 0) == 0 ||
            keywordUpper.rfind("CELLS", 0) == 0) {
            std::stringstream ss(trimmed);
            std::string keyword;
            size_t polygonCount = 0;
            size_t totalIndices = 0;
            ss >> keyword >> polygonCount >> totalIndices;

            if (!pointsLoaded) {
                errorMessage = "Bloc POLYGONS rencontré avant POINTS.";
                return std::nullopt;
            }

            mesh.indices.clear();
            mesh.indices.reserve(polygonCount * 3);

            if (!readPolygons(file, polygonCount, mesh.indices, mesh.vertices.size(), errorMessage))
                return std::nullopt;

            polygonsLoaded = true;
            if (keywordUpper.rfind("CELLS", 0) == 0)
                expectedCellTypes = polygonCount;
            continue;
        }

        if (keywordUpper.rfind("CELL_TYPES", 0) == 0) {
            std::stringstream ss(trimmed);
            std::string keyword;
            size_t count = 0;
            ss >> keyword >> count;
            if (expectedCellTypes != 0 && count != expectedCellTypes) {
                // On lit malgré tout pour garder le flux cohérent.
                expectedCellTypes = count;
            }

            for (size_t i = 0; i < count; ++i) {
                int cellType = 0;
                if (!(file >> cellType)) {
                    errorMessage = "Bloc CELL_TYPES incomplet.";
                    return std::nullopt;
                }
            }

            file.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            continue;
        }

        if (keywordUpper.rfind("POINT_DATA", 0) == 0 || keywordUpper.rfind("CELL_DATA", 0) == 0)
            break; // Fin des données géométriques
    }

    if (!pointsLoaded) {
        errorMessage = "Le fichier VTK ne contient pas de bloc POINTS valide.";
        return std::nullopt;
    }

    if (!polygonsLoaded || mesh.indices.empty()) {
        errorMessage = "Le fichier VTK ne contient pas de maillage polygonal compatible.";
        return std::nullopt;
    }

    return mesh;
}
