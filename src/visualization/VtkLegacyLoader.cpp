#include "io/VtkLegacyLoader.h"
#include <fstream>
#include <sstream>
#include <cctype>
#include <algorithm>

static inline std::string trim(const std::string &s) {
    size_t a = 0, b = s.size();
    while (a < b && std::isspace((unsigned char)s[a]))
        ++a;
    while (b > a && std::isspace((unsigned char)s[b - 1]))
        --b;
    return s.substr(a, b - a);
}

bool VtkLegacyLoader::load(const std::string &path, VtkMesh &out, std::string *err) {
    std::ifstream f(path);
    if (!f) {
        if (err)
            *err = "Impossible d'ouvrir le fichier: " + path;
        return false;
    }

    out.vertices.clear();
    out.triangles.clear();
    out.lines.clear();

    std::string line;
    size_t expectPoints = 0, readPoints = 0;
    size_t expectPolys = 0, readPolys = 0;
    size_t expectLines = 0, readLines = 0;

    while (std::getline(f, line)) {
        line = trim(line);
        if (line.empty() || line[0] == '#')
            continue;
        std::string u = line;
        std::transform(u.begin(), u.end(), u.begin(), [](unsigned char c) { return std::toupper(c); });

        if (u.rfind("POINTS", 0) == 0) {
            std::istringstream iss(line);
            std::string kw, dtype;
            size_t n = 0;
            iss >> kw >> n >> dtype;
            if (n == 0) {
                if (err)
                    *err = "POINTS: nombre invalide";
                return false;
            }
            expectPoints = n;
            out.vertices.reserve(n);

            // lire floats sur les lignes suivantes
            while (readPoints < expectPoints && std::getline(f, line)) {
                std::istringstream is2(line);
                float x, y, z;
                while (is2 >> x >> y >> z) {
                    out.vertices.push_back({x, y, z});
                    ++readPoints;
                    if (readPoints >= expectPoints)
                        break;
                }
            }
            if (readPoints != expectPoints) {
                if (err)
                    *err = "POINTS: nombre de sommets lus différent du compte annoncé";
                return false;
            }
            continue;
        }

        if (u.rfind("POLYGONS", 0) == 0) {
            std::istringstream iss(line);
            std::string kw;
            size_t m = 0, k = 0;
            iss >> kw >> m >> k;
            if (m == 0) {
                if (err)
                    *err = "POLYGONS: nombre invalide";
                return false;
            }
            expectPolys = m;

            while (readPolys < expectPolys && std::getline(f, line)) {
                line = trim(line);
                if (line.empty())
                    continue;
                std::istringstream is2(line);
                int n = 0;
                is2 >> n;
                if (n < 3) {
                    if (err)
                        *err = "POLYGONS: face avec <3 indices";
                    return false;
                }

                std::vector<int> idx;
                idx.reserve(n);
                for (int i = 0; i < n; i++) {
                    long long t;
                    if (!(is2 >> t)) {
                        if (err)
                            *err = "POLYGONS: indices insuffisants";
                        return false;
                    }
                    if (t < 0 || (size_t)t >= out.vertices.size()) {
                        if (err)
                            *err = "POLYGONS: indice hors bornes";
                        return false;
                    }
                    idx.push_back((int)t);
                }
                if (n == 3) {
                    out.triangles.push_back(std::array<int, 3>{idx[0], idx[1], idx[2]});
                } else if (n == 4) {
                    out.triangles.push_back(std::array<int, 3>{idx[0], idx[1], idx[2]});
                    out.triangles.push_back(std::array<int, 3>{idx[0], idx[2], idx[3]});
                } else {
                    for (int i = 1; i + 1 < n; i++) {
                        out.triangles.push_back(std::array<int, 3>{idx[0], idx[i], idx[i + 1]});
                    }
                }
                ++readPolys;
            }
            if (readPolys != expectPolys) {
                if (err)
                    *err = "POLYGONS: nombre de faces lues différent du compte annoncé";
                return false;
            }
            continue;
        }

        if (u.rfind("LINES", 0) == 0) {
            std::istringstream iss(line);
            std::string kw;
            size_t m = 0, k = 0;
            iss >> kw >> m >> k;
            expectLines = m;

            while (readLines < expectLines && std::getline(f, line)) {
                line = trim(line);
                if (line.empty())
                    continue;
                std::istringstream is2(line);
                int n = 0;
                is2 >> n;
                if (n < 2) {
                    if (err)
                        *err = "LINES: entrée <2 indices";
                    return false;
                }
                std::vector<int> idx;
                idx.reserve(n);
                for (int i = 0; i < n; i++) {
                    long long t;
                    if (!(is2 >> t)) {
                        if (err)
                            *err = "LINES: indices insuffisants";
                        return false;
                    }
                    if (t < 0 || (size_t)t >= out.vertices.size()) {
                        if (err)
                            *err = "LINES: indice hors bornes";
                        return false;
                    }
                    idx.push_back((int)t);
                }
                // on convertit en segments (i0,i1), (i1,i2), ...
                for (int i = 0; i + 1 < n; i++) {
                    out.lines.push_back(std::array<int, 2>{idx[i], idx[i + 1]});
                }
                ++readLines;
            }
            continue;
        }
        // on ignore le reste (POINT_DATA, CELL_DATA, etc.)
    }

    if (out.vertices.empty() || out.triangles.empty()) {
        if (err)
            *err = "Maillage incomplet (aucun point ou aucun triangle).";
        return false;
    }
    return true;
}
