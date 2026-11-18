#include "emma/Collisions.hpp"
#include "emma/BlackHole.hpp"
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
    // The heavier body survives and keeps its name, color and trail. A black hole always
    // survives. Mass and momentum are kept
    const bool holeA = bodies[a].isBlackHole(), holeB = bodies[b].isBlackHole();
    const std::size_t keepIndex = holeA != holeB ? (holeA ? a : b) : (bodies[a].mass >= bodies[b].mass ? a : b);
    const std::size_t goneIndex = keepIndex == a ? b : a;
    Body& keep = bodies[keepIndex];
    Body& gone = bodies[goneIndex];

    const double total = keep.mass + gone.mass;
    // If neither body has mass, the merged body sits halfway between them
    const double wk = total > 0.0 ? keep.mass / total : 0.5;
    const double wg = total > 0.0 ? gone.mass / total : 0.5;
    keep.position = keep.position * wk + gone.position * wg;
    keep.previousPosition = keep.previousPosition * wk + gone.previousPosition * wg;
    keep.velocity = keep.velocity * wk + gone.velocity * wg;
    // Keep the acceleration too, so the next step starts right
    keep.acceleration = keep.acceleration * wk + gone.acceleration * wg;

    if (keep.isBlackHole()) {
        // The horizon grows with the mass. The spin stays the same for now
        keep.radius = blackhole::horizonRadiusAU(total, keep.spin);
    } else if (keep.isNamed()) {
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

    // Only check pairs that overlap along x, to skip most pairs quickly
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
