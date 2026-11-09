#include "emma/Gravity.hpp"
#include "emma/Constants.hpp"
#include <cmath>

namespace emma {

void Gravity::accelerate(std::vector<Body>& bodies) {
    const std::size_t n = bodies.size();
    const double eps2 = SOFTENING * SOFTENING;

    if (n <= DIRECT_LIMIT) {
        // Newton's third law
        for (Body& b : bodies) b.acceleration = Vector2D();
        for (std::size_t i = 0; i < n; ++i) {
            for (std::size_t j = i + 1; j < n; ++j) {
                const Vector2D d = bodies[j].position - bodies[i].position;
                const double r2 = d.lengthSquared() + eps2;
                const double s = units::G / (r2 * std::sqrt(r2));
                bodies[i].acceleration += d * (s * bodies[j].mass);
                bodies[j].acceleration -= d * (s * bodies[i].mass);
            }
        }
        return;
    }

    tree.build(bodies);
    for (std::size_t i = 0; i < n; ++i) bodies[i].acceleration = tree.acceleration(i, THETA, eps2);
}

double Gravity::potentialEnergy(const std::vector<Body>& bodies) const {
    const double eps2 = SOFTENING * SOFTENING;
    double potential = 0.0;
    for (std::size_t i = 0; i < bodies.size(); ++i) {
        for (std::size_t j = i + 1; j < bodies.size(); ++j) {
            const double r2 = (bodies[j].position - bodies[i].position).lengthSquared();
            potential -= units::G * bodies[i].mass * bodies[j].mass / std::sqrt(r2 + eps2);
        }
    }
    return potential;
}

} // namespace emma
