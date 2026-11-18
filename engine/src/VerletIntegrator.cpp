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

    // When bodies get close, split the step into short pieces so the close pass is followed
    // properly. A single long step would overshoot and fling the pair apart
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
    // Move the bodies, work out the new pull, then update the speeds with the average of
    // the old and new pull
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

    // Many bodies: guess from how fast each body's pull changed over the last piece
    if (n > Gravity::DIRECT_LIMIT && jerkValid && oldAcceleration.size() == n) {
        for (std::size_t i = 0; i < n; ++i) {
            const double change = (bodies[i].acceleration - oldAcceleration[i]).magnitude();
            if (change > 0.0) t = std::min(t, bodies[i].acceleration.magnitude() * lastPiece / change);
        }
        return t;
    }

    // Otherwise check every pair: how quickly they can swing around each other, and how
    // soon they'd meet at their current speed
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

} // namespace emma
