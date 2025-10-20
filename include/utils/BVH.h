#pragma once

#include "common.h"
#include "geometry/Object3D.h"

// Nœud de la hiérarchie de volumes englobants
struct BVHNode {
    AABB bounds;
    std::vector<std::shared_ptr<Object3D>> objects; // Pour les feuilles
    std::unique_ptr<BVHNode> left;
    std::unique_ptr<BVHNode> right;
    bool isLeaf = false;
    
    BVHNode() = default;
    
    bool intersect(const Ray& ray, IntersectionResult& result) const;
    bool intersectAny(const Ray& ray) const;
};

// Hiérarchie de volumes englobants pour accélération spatiale
class BVH {
public:
    BVH() = default;
    ~BVH() = default;
    
    // Construction
    void build(const std::vector<std::shared_ptr<Object3D>>& objects);
    void clear();
    
    // Requêtes
    IntersectionResult intersect(const Ray& ray) const;
    bool intersectAny(const Ray& ray) const;
    
    // État
    bool isValid() const { return m_root != nullptr; }
    size_t getDepth() const;
    size_t getNodeCount() const;
    
    // Statistiques pour debugging
    struct Statistics {
        size_t totalNodes = 0;
        size_t leafNodes = 0;
        size_t maxDepth = 0;
        size_t maxObjectsPerLeaf = 0;
        float averageObjectsPerLeaf = 0.0f;
    };
    
    Statistics getStatistics() const;

private:
    std::unique_ptr<BVHNode> m_root;
    
    // Construction récursive
    std::unique_ptr<BVHNode> buildRecursive(std::vector<std::shared_ptr<Object3D>>& objects, 
                                           int depth = 0);
    
    // Calcul de la boîte englobante pour un ensemble d'objets
    AABB computeBounds(const std::vector<std::shared_ptr<Object3D>>& objects) const;
    
    // Partitionnement des objets
    size_t partitionObjects(std::vector<std::shared_ptr<Object3D>>& objects, 
                           int axis, float splitPos) const;
    
    // Choix de l'axe de division optimal
    int chooseSplitAxis(const std::vector<std::shared_ptr<Object3D>>& objects, 
                       const AABB& bounds) const;
    
    // Calcul de la position de division optimale
    float chooseSplitPosition(const std::vector<std::shared_ptr<Object3D>>& objects, 
                             const AABB& bounds, int axis) const;
    
    // Statistiques récursives
    void computeStatistics(const BVHNode* node, Statistics& stats, size_t depth = 0) const;
    
    // Paramètres de construction
    static constexpr size_t MAX_OBJECTS_PER_LEAF = 4;
    static constexpr size_t MAX_DEPTH = 20;
};