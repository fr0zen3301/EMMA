#include "emma/Gravity.hpp"
#include "emma/Constants.hpp"
#include <algorithm>
#include <cmath>
#include <numeric>

namespace emma {

void Gravity::markHosts(const std::vector<Body>& bodies, std::vector<char>& marks) {
    const std::size_t n = bodies.size();
    marks.assign(n, 0);
    std::vector<std::size_t> order(n);
    std::iota(order.begin(), order.end(), std::size_t{0});
    const std::size_t k = std::min(RELATIVITY_HOSTS, n);
    std::partial_sort(order.begin(), order.begin() + static_cast<std::ptrdiff_t>(k), order.end(),
                      [&bodies](std::size_t a, std::size_t b) {
                          return bodies[a].mass != bodies[b].mass ? bodies[a].mass > bodies[b].mass : a < b;
                      });
    for (std::size_t i = 0; i < k; ++i) marks[order[i]] = 1;
}

void Gravity::accelerate(std::vector<Body>& bodies) {
    const std::size_t n = bodies.size();
    const double eps2 = SOFTENING * SOFTENING;
    const double gr = relativity * RELATIVITY_PULL;

    if (n <= DIRECT_LIMIT) {
        // Each pair pulls on both bodies equally (Newton's third law)
        for (Body& b : bodies) b.acceleration = Vector3D();
        for (std::size_t i = 0; i < n; ++i) {
            for (std::size_t j = i + 1; j < n; ++j) {
                const Vector3D d = bodies[j].position - bodies[i].position;
                const double r2 = d.lengthSquared() + eps2;
                double s = units::G / (r2 * std::sqrt(r2));
                if (gr != 0.0) s += gr * (bodies[i].mass + bodies[j].mass) / (r2 * r2);
                bodies[i].acceleration += d * (s * bodies[j].mass);
                bodies[j].acceleration -= d * (s * bodies[i].mass);
            }
        }
        return;
    }

    tree.build(bodies);
    for (std::size_t i = 0; i < n; ++i) bodies[i].acceleration = tree.acceleration(i, THETA, eps2);
    if (gr == 0.0) return;

    // Add the relativity pull for pairs that include a heavy body, counting each pair once
    markHosts(bodies, hosts);
    for (std::size_t h = 0; h < n; ++h) {
        if (!hosts[h]) continue;
        for (std::size_t i = 0; i < n; ++i) {
            if (i == h || (hosts[i] && i < h)) continue;
            const Vector3D d = bodies[i].position - bodies[h].position;
            const double r2 = d.lengthSquared() + eps2;
            const double s = gr * (bodies[h].mass + bodies[i].mass) / (r2 * r2);
            bodies[h].acceleration += d * (s * bodies[i].mass);
            bodies[i].acceleration -= d * (s * bodies[h].mass);
        }
    }
}

double Gravity::potentialEnergy(const std::vector<Body>& bodies) const {
    const std::size_t n = bodies.size();
    const double eps2 = SOFTENING * SOFTENING;
    // The relativity term's energy uses half the pull's constant
    const double gr = 0.5 * relativity * RELATIVITY_PULL;
    std::vector<char> marks;
    const bool everyPair = n <= DIRECT_LIMIT;
    if (gr != 0.0 && !everyPair) markHosts(bodies, marks);

    double potential = 0.0;
    for (std::size_t i = 0; i < n; ++i) {
        for (std::size_t j = i + 1; j < n; ++j) {
            const double r2 = (bodies[j].position - bodies[i].position).lengthSquared() + eps2;
            const double mi = bodies[i].mass, mj = bodies[j].mass;
            potential -= units::G * mi * mj / std::sqrt(r2);
            if (gr != 0.0 && (everyPair || marks[i] || marks[j])) potential -= gr * mi * mj * (mi + mj) / r2;
        }
    }
    return potential;
}

} // namespace emma
