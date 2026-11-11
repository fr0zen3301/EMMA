#include "emma/VerletIntegrator.hpp"
#include "emma/Constants.hpp"
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <limits>

namespace emma {

int VerletIntegrator::step(World& world, Gravity& gravity, double dt, const std::function<void()>& afterPiece) {
    if (stale) {
        gravity.accelerate(world.bodies);
        stale = false;
        jerkValid = false;
    }

    // Split the step when bodies come close: each piece is at most ETA times the
    // tightest dynamical time, so a near miss is followed through its closest
    // approach. One long step through a near miss overshoots the closest point and
    // the pair flies apart faster than it came in, which looks like repulsion and
    // breaks energy conservation. In quiet stretches the whole step is a single
    // piece, so Verlet stays symplectic.
    const double minPiece = dt / MAX_SUBSTEPS;
    double remaining = dt;
    int pieces = 0;
    while (remaining > 1e-12 * dt) {
        double h = std::max(ETA * dynamicalTime(world.bodies), minPiece);
        if (h >= remaining) h = remaining;
        else if (remaining < 1.5 * h) h = 0.5 * remaining;
        piece(world, gravity, h);
        if (afterPiece) afterPiece();
        remaining -= h;
        ++pieces;
    }
    return pieces;
}

void VerletIntegrator::piece(World& world, Gravity& gravity, double h) {
    // Velocity Verlet: drift with the stored acceleration, recompute, then kick
    // with the average of the old and new accelerations
    std::vector<Body>& bodies = world.bodies;
    const double halfHSq = 0.5 * h * h;
    oldAcceleration.resize(bodies.size());
    for (std::size_t i = 0; i < bodies.size(); ++i) {
        Body& b = bodies[i];
        b.previousPosition = b.position;
        b.position += b.velocity * h + b.acceleration * halfHSq;
        oldAcceleration[i] = b.acceleration;
    }
    gravity.accelerate(bodies);
    for (std::size_t i = 0; i < bodies.size(); ++i)
        bodies[i].velocity += (oldAcceleration[i] + bodies[i].acceleration) * (0.5 * h);

    world.time += h;
    lastPiece = h;
    jerkValid = true;
}

double VerletIntegrator::dynamicalTime(const std::vector<Body>& bodies) const {
    double t = std::numeric_limits<double>::infinity();
    const std::size_t n = bodies.size();

    if (n <= Gravity::DIRECT_LIMIT) {
        // For each pair, the shorter of its dynamical time sqrt(r^3 / G(m1 + m2))
        // and the time it takes to close the gap at the current relative speed
        for (std::size_t i = 0; i < n; ++i) {
            for (std::size_t j = i + 1; j < n; ++j) {
                const double r2 = (bodies[j].position - bodies[i].position).lengthSquared();
                const double mu = units::G * (bodies[i].mass + bodies[j].mass);
                if (mu > 0.0) t = std::min(t, std::sqrt(r2 * std::sqrt(r2) / mu));
                const double v2 = (bodies[j].velocity - bodies[i].velocity).lengthSquared();
                if (v2 > 0.0) t = std::min(t, std::sqrt(r2 / v2));
            }
        }
        return t;
    }

    // With many bodies, scanning every pair would cost more than the tree saves.
    // Use how fast each body's acceleration changed over the last piece: |a| / |da/dt|
    if (!jerkValid || oldAcceleration.size() != n) return t;
    for (std::size_t i = 0; i < n; ++i) {
        const double change = (bodies[i].acceleration - oldAcceleration[i]).magnitude();
        if (change > 0.0) t = std::min(t, bodies[i].acceleration.magnitude() * lastPiece / change);
    }
    return t;
}

} // namespace emma
