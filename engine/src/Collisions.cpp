#include "emma/Collisions.hpp"
#include <algorithm>
#include <cmath>
#include <cstddef>

namespace emma {

namespace {

// Did a and b touch during the last step?
bool sweptContact(const Body& a, const Body& b) {
    const Vector3D start = b.previousPosition - a.previousPosition;
    const Vector3D motion = (b.position - a.position) - start;
    const double mm = motion.lengthSquared();
    const double t = mm > 0.0 ? std::clamp(-start.dot(motion) / mm, 0.0, 1.0) : 0.0;
    const Vector3D closest = start + motion * t;
    const double reach = a.radius + b.radius;
    return closest.lengthSquared() <= reach * reach;
}

void mergePair(std::vector<Body>& bodies, std::size_t a, std::size_t b, std::vector<MergeFlash>& flashes) {
    // mass and momentum are conserved and the merged body
    // carries on from the pair's center of mass. The heavier body keeps its
    // name, color and trail
    const std::size_t keepIndex = bodies[a].mass >= bodies[b].mass ? a : b;
    const std::size_t goneIndex = keepIndex == a ? b : a;
    Body& keep = bodies[keepIndex];
    Body& gone = bodies[goneIndex];

    const double total = keep.mass + gone.mass;
    const double wk = keep.mass / total, wg = gone.mass / total;
    keep.position = keep.position * wk + gone.position * wg;
    keep.previousPosition = keep.previousPosition * wk + gone.previousPosition * wg;
    keep.velocity = keep.velocity * wk + gone.velocity * wg;
    // The center of mass accelerates at the mass-weighted mean (internal forces cancel),
    // so the next drift starts from the right acceleration instead of zero
    keep.acceleration = keep.acceleration * wk + gone.acceleration * wg;

    if (keep.isNamed()) {
        const double rk = keep.radius, rg = gone.radius;
        keep.radius = std::cbrt(rk * rk * rk + rg * rg * rg);  // volumes add
    } else {
        keep.radius = Body::radiusForMass(total);
        keep.color = Body::colorForMass(total);
    }
    keep.mass = total;
    gone.alive = false;
    flashes.push_back(MergeFlash{keep.position, 0.f});
}

} // namespace

bool resolveCollisions(std::vector<Body>& bodies, std::vector<MergeFlash>& flashes) {
    const std::size_t n = bodies.size();
    if (n < 2) return false;

    // Sweep and prune along x
    struct Span {
        double lo, hi;
        std::size_t index;
    };
    std::vector<Span> spans;
    spans.reserve(n);
    for (std::size_t i = 0; i < n; ++i) {
        const Body& b = bodies[i];
        spans.push_back(Span{std::min(b.previousPosition.x, b.position.x) - b.radius,
                             std::max(b.previousPosition.x, b.position.x) + b.radius, i});
    }
    std::sort(spans.begin(), spans.end(), [](const Span& l, const Span& r) { return l.lo < r.lo; });

    bool merged = false;
    for (std::size_t s = 0; s < n; ++s) {
        for (std::size_t t = s + 1; t < n && spans[t].lo <= spans[s].hi; ++t) {
            const std::size_t i = spans[s].index, j = spans[t].index;
            if (!bodies[i].alive || !bodies[j].alive) continue;
            if (sweptContact(bodies[i], bodies[j])) {
                mergePair(bodies, i, j, flashes);
                merged = true;
            }
        }
    }

    if (merged)
        bodies.erase(std::remove_if(bodies.begin(), bodies.end(), [](const Body& b) { return !b.alive; }),
                     bodies.end());
    return merged;
}

} // namespace emma
