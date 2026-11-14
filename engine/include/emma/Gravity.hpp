#pragma once
#include "emma/Body.hpp"
#include "emma/Octree.hpp"
#include <cstddef>
#include <vector>

namespace emma {

// Newtonian gravity with Plummer softening: an exact pairwise sum up to
// DIRECT_LIMIT bodies, a Barnes-Hut tree above that
class Gravity {
public:
    static constexpr double SOFTENING = 1e-7;        // AU (15 km), far below any body's radius
    static constexpr double THETA = 0.5;             // Barnes-Hut opening angle
    static constexpr std::size_t DIRECT_LIMIT = 64;  // up to this many bodies, exact pairwise forces

    // Sets every body's acceleration, in AU/day^2
    void accelerate(std::vector<Body>& bodies);
    // The softened potential, -G m1 m2 / sqrt(r^2 + eps^2) per pair, whose
    // gradient is exactly the force accelerate() applies. Msun AU^2 / day^2
    double potentialEnergy(const std::vector<Body>& bodies) const;

private:
    Octree tree;
};

}
