#include "utils/BVH.h"
#include <algorithm>
#include <queue>

// BVHNode implementation
bool BVHNode::intersect(const Ray& ray, IntersectionResult& result) const {
    // Test d'intersection avec la boîte englobante
    float tMin, tMax;
    if (!bounds.intersects(ray, tMin, tMax)) {
        return false;
    }
    
    if (isLeaf) {
        // Nœud feuille - tester tous les objets
        bool hit = false;
        IntersectionResult closestHit;
        closestHit.distance = std::numeric_limits<float>::max();
        
        for (const auto& object : objects) {
            IntersectionResult objectHit = object->intersect(ray);
            if (objectHit.hit && objectHit.distance < closestHit.distance) {
                closestHit = objectHit;
                hit = true;
            }
        }
        
        if (hit) {
            result = closestHit;
        }
        return hit;
        
    } else {
        // Nœud interne - tester les enfants
        IntersectionResult leftHit, rightHit;
        bool hitLeft = left && left->intersect(ray, leftHit);
        bool hitRight = right && right->intersect(ray, rightHit);
        
        if (hitLeft && hitRight) {
            // Prendre l'intersection la plus proche
            result = (leftHit.distance < rightHit.distance) ? leftHit : rightHit;
            return true;
        } else if (hitLeft) {
            result = leftHit;
            return true;
        } else if (hitRight) {
            result = rightHit;
            return true;
        }
        
        return false;
    }
}

bool BVHNode::intersectAny(const Ray& ray) const {
    // Test d'intersection avec la boîte englobante
    float tMin, tMax;
    if (!bounds.intersects(ray, tMin, tMax)) {
        return false;
    }
    
    if (isLeaf) {
        // Nœud feuille - tester tous les objets
        for (const auto& object : objects) {
            IntersectionResult hit = object->intersect(ray);
            if (hit.hit) {
                return true;
            }
        }
        return false;
        
    } else {
        // Nœud interne - tester les enfants
        return (left && left->intersectAny(ray)) || (right && right->intersectAny(ray));
    }
}

// BVH implementation
void BVH::build(const std::vector<std::shared_ptr<Object3D>>& objects) {
    clear();
    
    if (objects.empty()) {
        return;
    }
    
    // Copie des objets pour manipulation
    std::vector<std::shared_ptr<Object3D>> objectsCopy = objects;
    
    // Construction récursive
    m_root = buildRecursive(objectsCopy, 0);
}

void BVH::clear() {
    m_root.reset();
}

IntersectionResult BVH::intersect(const Ray& ray) const {
    IntersectionResult result;
    
    if (m_root) {
        m_root->intersect(ray, result);
    }
    
    return result;
}

bool BVH::intersectAny(const Ray& ray) const {
    if (m_root) {
        return m_root->intersectAny(ray);
    }
    return false;
}

std::unique_ptr<BVHNode> BVH::buildRecursive(std::vector<std::shared_ptr<Object3D>>& objects, 
                                             int depth) {
    auto node = std::make_unique<BVHNode>();
    
    // Calcul de la boîte englobante
    node->bounds = computeBounds(objects);
    
    // Condition d'arrêt : créer une feuille
    if (objects.size() <= MAX_OBJECTS_PER_LEAF || depth >= MAX_DEPTH) {
        node->isLeaf = true;
        node->objects = objects;
        return node;
    }
    
    // Choix de l'axe de division
    int splitAxis = chooseSplitAxis(objects, node->bounds);
    float splitPos = chooseSplitPosition(objects, node->bounds, splitAxis);
    
    // Partitionnement des objets
    size_t splitIndex = partitionObjects(objects, splitAxis, splitPos);
    
    // Éviter les partitions déséquilibrées
    if (splitIndex == 0 || splitIndex == objects.size()) {
        splitIndex = objects.size() / 2;
    }
    
    // Création des sous-ensembles
    std::vector<std::shared_ptr<Object3D>> leftObjects(objects.begin(), 
                                                       objects.begin() + splitIndex);
    std::vector<std::shared_ptr<Object3D>> rightObjects(objects.begin() + splitIndex, 
                                                        objects.end());
    
    // Construction récursive des enfants
    if (!leftObjects.empty()) {
        node->left = buildRecursive(leftObjects, depth + 1);
    }
    if (!rightObjects.empty()) {
        node->right = buildRecursive(rightObjects, depth + 1);
    }
    
    return node;
}

AABB BVH::computeBounds(const std::vector<std::shared_ptr<Object3D>>& objects) const {
    AABB bounds;
    
    for (const auto& object : objects) {
        bounds.expand(object->getBounds());
    }
    
    return bounds;
}

size_t BVH::partitionObjects(std::vector<std::shared_ptr<Object3D>>& objects, 
                            int axis, float splitPos) const {
    return std::partition(objects.begin(), objects.end(),
        [axis, splitPos](const std::shared_ptr<Object3D>& obj) {
            glm::vec3 center = obj->getBounds().center();
            return center[axis] < splitPos;
        }) - objects.begin();
}

int BVH::chooseSplitAxis(const std::vector<std::shared_ptr<Object3D>>& objects, 
                        const AABB& bounds) const {
    // Choisir l'axe avec la plus grande étendue
    glm::vec3 extent = bounds.size();
    
    if (extent.x >= extent.y && extent.x >= extent.z) {
        return 0; // X
    } else if (extent.y >= extent.z) {
        return 1; // Y
    } else {
        return 2; // Z
    }
}

float BVH::chooseSplitPosition(const std::vector<std::shared_ptr<Object3D>>& objects, 
                              const AABB& bounds, int axis) const {
    // Position médiane simple
    return bounds.center()[axis];
    
    // Alternative : médiane des centres d'objets
    /*
    std::vector<float> positions;
    positions.reserve(objects.size());
    
    for (const auto& object : objects) {
        positions.push_back(object->getBounds().center()[axis]);
    }
    
    std::sort(positions.begin(), positions.end());
    return positions[positions.size() / 2];
    */
}

size_t BVH::getDepth() const {
    if (!m_root) return 0;
    
    std::function<size_t(const BVHNode*)> computeDepth = [&](const BVHNode* node) -> size_t {
        if (!node || node->isLeaf) return 1;
        
        size_t leftDepth = node->left ? computeDepth(node->left.get()) : 0;
        size_t rightDepth = node->right ? computeDepth(node->right.get()) : 0;
        
        return 1 + std::max(leftDepth, rightDepth);
    };
    
    return computeDepth(m_root.get());
}

size_t BVH::getNodeCount() const {
    if (!m_root) return 0;
    
    std::function<size_t(const BVHNode*)> countNodes = [&](const BVHNode* node) -> size_t {
        if (!node) return 0;
        
        size_t count = 1;
        if (node->left) count += countNodes(node->left.get());
        if (node->right) count += countNodes(node->right.get());
        
        return count;
    };
    
    return countNodes(m_root.get());
}

BVH::Statistics BVH::getStatistics() const {
    Statistics stats;
    
    if (m_root) {
        computeStatistics(m_root.get(), stats, 0);
        
        if (stats.leafNodes > 0) {
            stats.averageObjectsPerLeaf = static_cast<float>(stats.totalNodes) / stats.leafNodes;
        }
    }
    
    return stats;
}

void BVH::computeStatistics(const BVHNode* node, Statistics& stats, size_t depth) const {
    if (!node) return;
    
    stats.totalNodes++;
    stats.maxDepth = std::max(stats.maxDepth, depth);
    
    if (node->isLeaf) {
        stats.leafNodes++;
        stats.maxObjectsPerLeaf = std::max(stats.maxObjectsPerLeaf, node->objects.size());
    } else {
        if (node->left) computeStatistics(node->left.get(), stats, depth + 1);
        if (node->right) computeStatistics(node->right.get(), stats, depth + 1);
    }
}