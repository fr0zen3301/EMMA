#pragma once
#include "emma/Vector3D.hpp"
#include <cstddef>
#include <vector>

namespace emma {

class Body;

// Barnes-Hut tree: treats a far-away group of bodies as one point mass, to save time.
// Its nodes live in one array that's reused every step
class Octree {
public:
    void build(const std::vector<Body>& bodies);

    // The pull on bodies[self], in AU/day^2
    Vector3D acceleration(std::size_t self, double theta, double softeningSq) const;

private:
    struct Node {
        Vector3D center;       // geometric center of this cube
        double halfSize = 0.0;
        Vector3D com;          // center of mass of everything inside
        double mass = 0.0;
        int body = -1;         // the single body stored in a leaf, -1 otherwise
        int firstChild = -1;   // first of eight consecutive children, -1 for a leaf
    };

    void insert(int node, int bodyIndex, int depth);
    void subdivide(int node);
    static int octantOf(const Node& node, const Vector3D& p);

    std::vector<Node> nodes;
    const std::vector<Body>* source = nullptr;
    mutable std::vector<int> stack;  // scratch space for walking the tree
};

} // namespace emma
