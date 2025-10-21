#include "io/VtkLegacyLoader.h"
#include <fstream>
#include <sstream>
#include <cctype>
#include <algorithm>

static std::string up(const std::string &s) {
    std::string t = s;
    std::transform(t.begin(), t.end(), t.begin(), [](unsigned char c) { return std::toupper(c); });
    return t;
}

bool VtkLegacyLoader::load(const std::string &path, VtkMesh &out, std::string *err) {
    std::ifstream f(path);
    if (!f) {
        if (err)
            *err = "Impossible d'ouvrir: " + path;
        return false;
    }

    std::string line;
    bool inPoints = false, inPolys = false, inLines = false;
    int expectPts = 0, expectPolys = 0, expectLines = 0;
    int polyIntsTotal = 0, lineIntsTotal = 0;

    while (std::getline(f, line)) {
        if (line.size() == 0)
            continue;
        // supprime commentaires après '#', sauf première ligne header (ok)
        if (line[0] == '#' && out.vertices.empty() && out.triangles.empty() && out.lines.empty()) {
            continue; // header, on ignore
        }

        std::istringstream iss(line);
        std::string kw;
        iss >> kw;
        auto KW = up(kw);

        if (KW == "DATASET") { /* ignore */
            continue;
        }

        if (KW == "POINTS") {
            inPoints = true;
            inPolys = false;
            inLines = false;
            std::string type;
            iss >> expectPts >> type;
            out.vertices.reserve(expectPts);
            // lire les coordonnées à suivre (sur plusieurs lignes possiblement)
            int read = 0;
            while (read < expectPts && std::getline(f, line)) {
                std::istringstream ls(line);
                while (read < expectPts) {
                    float x, y, z;
                    if (!(ls >> x >> y >> z))
                        break;
                    out.vertices.push_back({x, y, z});
                    read++;
                }
                if (read == expectPts)
                    break;
            }
            inPoints = false;
            continue;
        }

        if (KW == "POLYGONS") {
            inPolys = true;
            inPoints = false;
            inLines = false;
            iss >> expectPolys >> polyIntsTotal;
            out.triangles.reserve(out.triangles.size() + expectPolys);
            int readPolys = 0;
            while (readPolys < expectPolys && std::getline(f, line)) {
                if (line.empty())
                    continue;
                std::istringstream ls(line);
                int n;
                if (!(ls >> n))
                    continue; // taille du polygone
                if (n < 3) {
                    continue;
                }
                std::vector<int> idx(n);
                for (int i = 0; i < n; i++)
                    ls >> idx[i];
                // triangulation en éventail
                for (int t = 1; t < n - 1; t++) {
                    out.triangles.push_back({idx[0], idx[t], idx[t + 1]});
                }
                readPolys++;
            }
            inPolys = false;
            continue;
        }

        if (KW == "LINES") {
            inLines = true;
            inPoints = false;
            inPolys = false;
            iss >> expectLines >> lineIntsTotal;
            out.lines.reserve(out.lines.size() + expectLines);
            int readLines = 0;
            while (readLines < expectLines && std::getline(f, line)) {
                if (line.empty())
                    continue;
                std::istringstream ls(line);
                int n;
                if (!(ls >> n))
                    continue;
                std::vector<int> idx(n);
                for (int i = 0; i < n; i++)
                    ls >> idx[i];
                // convertit chaque segment consécutif en arête
                for (int i = 0; i + 1 < n; i++) {
                    out.lines.push_back({idx[i], idx[i + 1]});
                }
                readLines++;
            }
            inLines = false;
            continue;
        }
        // autres directives ignorées (VERTICES, NORMALS, etc.)
    }

    if (out.vertices.empty()) {
        if (err)
            *err = "Aucun POINTS lu dans " + path;
        return false;
    }
    return true;
}
