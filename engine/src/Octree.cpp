#include "emma/Octree.hpp"
#include "emma/Body.hpp"
#include "emma/Constants.hpp"
#include <algorithm>
#include <cmath>

namespace emma {

namespace {
// Bodies closer than root size / 2^48 are folded into one point mass.
// They would have collided and merged long before reaching this depth.
constexpr int MAX_DEPTH = 48;
}

void Octree::build(const std::vector<Body>& bodies) {
    source = &bodies;
    nodes.clear();

    bool any = false;
    Vector3D lo, hi;
    for (const Body& b : bodies) {
        if (!b.alive) continue;
        if (!any) {
            lo = b.position;
            hi = b.position;
            any = true;
            continue;
        }
        lo.x = std::min(lo.x, b.position.x);
        lo.y = std::min(lo.y, b.position.y);
        lo.z = std::min(lo.z, b.position.z);
        hi.x = std::max(hi.x, b.position.x);
        hi.y = std::max(hi.y, b.position.y);
        hi.z = std::max(hi.z, b.position.z);
    }
    if (!any) return;

    Node root;
    root.center = (lo + hi) * 0.5;
    root.halfSize = 0.5 * std::max({hi.x - lo.x, hi.y - lo.y, hi.z - lo.z}) * 1.0001 + 1e-12;
    nodes.reserve(8 * bodies.size() + 1);
    nodes.push_back(root);

    for (std::size_t i = 0; i < bodies.size(); ++i) {
        if (bodies[i].alive && bodies[i].mass > 0.0) insert(0, static_cast<int>(i), 0);
    }
}

int Octree::octantOf(const Node& node, const Vector3D& p) {
    int q = 0;
    if (p.x >= node.center.x) q += 1;
    if (p.y >= node.center.y) q += 2;
    if (p.z >= node.center.z) q += 4;
    return q;
}

void Octree::subdivide(int n) {
    const Vector3D c = nodes[n].center;
    const double h = 0.5 * nodes[n].halfSize;
    const int first = static_cast<int>(nodes.size());
    for (int q = 0; q < 8; ++q) {
        Node child;
        child.center = c + Vector3D(q & 1 ? h : -h, q & 2 ? h : -h, q & 4 ? h : -h);
        child.halfSize = h;
        nodes.push_back(child);  // may reallocate: only indices are held across this call
    }
    nodes[n].firstChild = first;
}

void Octree::insert(int n, int bi, int depth) {
    const Vector3D p = (*source)[bi].position;
    const double m = (*source)[bi].mass;

    if (nodes[n].firstChild < 0) {
        if (nodes[n].mass == 0.0) {  // empty leaf: the body lives here
            nodes[n].body = bi;
            nodes[n].mass = m;
            nodes[n].com = p;
            return;
        }
        if (depth >= MAX_DEPTH) {  // unresolvably close: keep them as one point mass
            Node& node = nodes[n];
            const double total = node.mass + m;
            node.com = (node.com * node.mass + p * m) / total;
            node.mass = total;
            node.body = -1;
            return;
        }
        // occupied leaf: split it and push the resident body down a level
        const int resident = nodes[n].body;
        nodes[n].body = -1;
        subdivide(n);
        if (resident >= 0) {
            const int q = octantOf(nodes[n], (*source)[resident].position);
            insert(nodes[n].firstChild + q, resident, depth + 1);
        }
    }

    {   // fetch the node again: subdivide and the recursion above may have grown the array
        Node& node = nodes[n];
        const double total = node.mass + m;
        node.com = (node.com * node.mass + p * m) / total;
        node.mass = total;
    }
    const int q = octantOf(nodes[n], p);
    insert(nodes[n].firstChild + q, bi, depth + 1);
}

Vector3D Octree::acceleration(std::size_t self, double theta, double softeningSq) const {
    Vector3D acc;
    if (nodes.empty()) return acc;

    const Vector3D p = (*source)[self].position;
    const int selfIndex = static_cast<int>(self);
    const double theta2 = theta * theta;

    stack.clear();
    stack.push_back(0);
    while (!stack.empty()) {
        const Node& node = nodes[static_cast<std::size_t>(stack.back())];
        stack.pop_back();
        if (node.mass == 0.0 || node.body == selfIndex) continue;

        const Vector3D d = node.com - p;
        const double dist2 = d.lengthSquared();
        const double size = 2.0 * node.halfSize;

        if (node.firstChild < 0 || size * size < theta2 * dist2) {
            // Plummer-softened point mass: a = G M d / (|d|^2 + eps^2)^(3/2).
            // This is the exact gradient of the potential totalEnergy() uses.
            const double r2 = dist2 + softeningSq;
            if (r2 <= 0.0) continue;
            acc += d * (units::G * node.mass / (r2 * std::sqrt(r2)));
        } else {
            for (int q = 0; q < 8; ++q) stack.push_back(node.firstChild + q);
        }
    }
    return acc;
}

} // namespace emma
