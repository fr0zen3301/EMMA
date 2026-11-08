#pragma once
#include "Vector2D.hpp"
#include <cstddef>
#include <vector>

class Body;

// Barnes-Hut quadtree. Nodes live in one flat array that is rebuilt every step,
// so once the array has grown, building the tree no longer touches the heap.
class QuadTree {
public:
    void build(const std::vector<Body>& bodies);

    // Gravitational acceleration on bodies[self], in AU/day^2
    Vector2D acceleration(std::size_t self, double theta, double softeningSq) const;

private:
    struct Node {
        Vector2D center;       // geometric center of this square
        double halfSize = 0.0;
        Vector2D com;          // center of mass of everything inside
        double mass = 0.0;
        int body = -1;         // the single body stored in a leaf, -1 otherwise
        int firstChild = -1;   // first of four consecutive children, -1 for a leaf
    };

    void insert(int node, int bodyIndex, int depth);
    void subdivide(int node);
    static int quadrantOf(const Node& node, const Vector2D& p);

    std::vector<Node> nodes;
    const std::vector<Body>* source = nullptr;
    mutable std::vector<int> stack;  // traversal scratch space
};
