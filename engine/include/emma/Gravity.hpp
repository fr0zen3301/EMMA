#pragma once
#include "emma/Body.hpp"
#include "emma/Constants.hpp"
#include "emma/Octree.hpp"
#include <cstddef>
#include <vector>

namespace emma {

// Newton's gravity: exact for up to DIRECT_LIMIT bodies, a Barnes-Hut tree above that.
// Plus general relativity's main effect: orbits slowly turn, a little each lap
// (43" a century for Mercury). See docs/relativity.md
class Gravity {
public:
    static constexpr double SOFTENING = 1e-7;        // AU (15 km): stops the force blowing up at zero distance
    static constexpr double THETA = 0.5;             // Barnes-Hut opening angle
    static constexpr std::size_t DIRECT_LIMIT = 64;  // up to this many bodies, exact pairwise forces
    // Above DIRECT_LIMIT, relativity only acts on pairs that include one of this many
    // heaviest bodies, to keep it fast
    static constexpr std::size_t RELATIVITY_HOSTS = 16;
    // 6 G^2 / c^2, the strength of the relativity pull
    static constexpr double RELATIVITY_PULL =
        6.0 * units::G * units::G / (units::SPEED_OF_LIGHT * units::SPEED_OF_LIGHT);

    // 0 = Newton only, 1 = real relativity, more = exaggerated so it shows on screen
    double relativity = 1.0;

    // Sets every body's acceleration, in AU/day^2
    void accelerate(std::vector<Body>& bodies);
    // Total potential energy, matching the forces exactly. Msun AU^2 / day^2
    double potentialEnergy(const std::vector<Body>& bodies) const;

private:
    // Marks the heaviest bodies for the relativity term
    static void markHosts(const std::vector<Body>& bodies, std::vector<char>& hosts);

    Octree tree;
    std::vector<char> hosts;
};

}
