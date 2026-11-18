#pragma once
#include "emma/Constants.hpp"
#include "emma/Vector3D.hpp"
#include <algorithm>
#include <cmath>
#include <vector>

// Math for the bent grid. The sheet dips where clocks run slower, near masses.
// psi = G M / (c^2 r) is how much slower. See docs/spacetime-grid.md
namespace sheet {

// A mass that bends the sheet. Its well stops getting deeper inside its radius
struct Mass {
    emma::Vector3D at;
    double mass = 0.0;
    double radius = 0.0;
};

// G/c^2 in AU per solar mass (about 1.48 km)
constexpr double GM_PER_C2 = emma::units::G / (emma::units::SPEED_OF_LIGHT * emma::units::SPEED_OF_LIGHT);

// How deep one mass's well is at distance r. Inside radius s it levels off smoothly
inline double well(double mass, double r, double s) {
    if (r >= s && r > 0.0) return GM_PER_C2 * mass / r;
    if (s <= 0.0) return 0.0;  // right on a point mass: no sensible value
    return GM_PER_C2 * mass * (3.0 * s * s - r * r) / (2.0 * s * s * s);
}

// How steep one mass's well is at offset d from it
inline emma::Vector3D wellSlope(double mass, const emma::Vector3D& d, double s) {
    const double r = d.magnitude();
    const double reach = std::max(r, s);
    if (reach <= 0.0) return emma::Vector3D();
    return d * (-GM_PER_C2 * mass / (reach * reach * reach));
}

// psi at p: all the wells added up. smoothing widens narrow wells so grid lines can't miss them
inline double depthAt(const std::vector<Mass>& masses, const emma::Vector3D& p, double smoothing = 0.0) {
    double psi = 0.0;
    for (const Mass& m : masses) psi += well(m.mass, (p - m.at).magnitude(), std::max(m.radius, smoothing));
    return psi;
}

inline emma::Vector3D slopeAt(const std::vector<Mass>& masses, const emma::Vector3D& p, double smoothing = 0.0) {
    emma::Vector3D slope;
    for (const Mass& m : masses) slope += wellSlope(m.mass, p - m.at, std::max(m.radius, smoothing));
    return slope;
}

// How much slower a clock runs at depth psi (0 = normal, 1 = stopped, as at a horizon)
inline double clockSlowdown(double psi) {
    const double rate = 1.0 - 2.0 * psi;
    return rate > 0.0 ? 1.0 - std::sqrt(rate) : 1.0;
}

// The sheet as seen by someone falling freely at a point. They can't feel an even pull, so
// the sheet's level and slope there are taken away. That's what lets Earth's own dip show
// when you follow Earth
struct FallingFrame {
    emma::Vector3D at;
    double level = 0.0;
    emma::Vector3D slope;
};

inline FallingFrame fallingAt(const std::vector<Mass>& masses, const emma::Vector3D& at, double smoothing = 0.0) {
    return FallingFrame{at, depthAt(masses, at, smoothing), slopeAt(masses, at, smoothing)};
}

// How much deeper than the falling point the sheet is at p
inline double dip(const std::vector<Mass>& masses, const FallingFrame& frame, const emma::Vector3D& p,
                  double smoothing = 0.0) {
    return depthAt(masses, p, smoothing) - frame.level - frame.slope.dot(p - frame.at);
}

}
