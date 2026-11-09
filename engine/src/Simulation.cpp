#include "emma/Simulation.hpp"
#include "emma/Constants.hpp"
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <iterator>
#include <limits>
#include <numeric>
#include <utility>

namespace emma {

// Loading and editing

void Simulation::load(Preset preset, const SolarSnapshot* snapshot) {
    Scenario scenario = makeScenario(preset, snapshot);
    world.bodies = std::move(scenario.bodies);
    world.time = 0.0;
    world.epoch = scenario.epoch;
    world.hasCalendar = scenario.hasCalendar;
    dt = baseDt = scenario.step;
    viewRadiusAU = scenario.viewRadius;
    timeScaleHint = scenario.timeScale;
    currentPreset = preset;
    flashes.clear();

    world.moveToBarycenter();
    integrator.reset();
    ++world.changes;
}

void Simulation::clear() {
    world.bodies.clear();
    flashes.clear();
    integrator.reset();
    ++world.changes;
}

void Simulation::addBody(double mass, const Vector2D& position, const Vector2D& velocity) {
    world.bodies.emplace_back(mass, position, velocity);
    integrator.reset();  // Verlet needs the new body's acceleration before its first drift
    ++world.changes;
}

// Stepping

void Simulation::limitStep(double maxDays) {
    dt = std::clamp(maxDays, MIN_STEP, baseDt);
}

void Simulation::step() {
    lastSubsteps = integrator.step(world, gravity, dt, [this] {
        if (resolveCollisions(world.bodies, flashes)) {
            integrator.forgetHistory();  // indices shifted, the last piece's accelerations no longer line up
            ++world.changes;
        }
        world.recordTrails();  // after every piece, so trails stay smooth through a close pass
    });
}

void Simulation::updateEffects(float realDt) {
    for (MergeFlash& f : flashes) f.age += realDt;
    flashes.erase(std::remove_if(flashes.begin(), flashes.end(),
                                 [](const MergeFlash& f) { return f.age > FLASH_LIFETIME; }),
                  flashes.end());
}

// Queries

int Simulation::strongestPull(const Vector2D& p, int exclude) const {
    const std::vector<Body>& bodies = world.bodies;
    int best = -1;
    double bestPull = -1.0;
    for (std::size_t i = 0; i < bodies.size(); ++i) {
        if (static_cast<int>(i) == exclude) continue;
        const double r2 = (bodies[i].position - p).lengthSquared() + SOFTENING * SOFTENING;
        const double pull = bodies[i].mass / r2;
        if (pull > bestPull) {
            bestPull = pull;
            best = static_cast<int>(i);
        }
    }
    return best;
}

int Simulation::dominantBody(const Vector2D& p, int exclude) const {
    // Laplace sphere of influence r = d (m / M)^(2/5), with M the body pulling hardest on m
    const std::vector<Body>& bodies = world.bodies;
    int best = -1;
    double bestRadius = std::numeric_limits<double>::infinity();
    for (std::size_t i = 0; i < bodies.size(); ++i) {
        if (static_cast<int>(i) == exclude) continue;
        const Body& b = bodies[i];
        const int parent = strongestPull(b.position, static_cast<int>(i));
        if (parent < 0) continue;
        const Body& host = bodies[static_cast<std::size_t>(parent)];
        if (b.mass > 0.1 * host.mass) continue;  // similar masses: a binary companion, not a satellite
        const double soi = (b.position - host.position).magnitude() * std::pow(b.mass / host.mass, 0.4);
        if (soi < bestRadius && (p - b.position).magnitude() < soi) {
            best = static_cast<int>(i);
            bestRadius = soi;
        }
    }
    return best >= 0 ? best : strongestPull(p, exclude);
}

Vector2D Simulation::circularVelocity(const Vector2D& p) const {
    const int hostIndex = dominantBody(p);
    if (hostIndex < 0) return Vector2D();
    const Body& host = world.bodies[static_cast<std::size_t>(hostIndex)];
    const Vector2D r = p - host.position;
    const double dist = r.magnitude();
    if (dist <= 0.0) return host.velocity;
    const double speed = std::sqrt(units::G * host.mass / dist);
    // counterclockwise seen from above, the way the planets go
    return host.velocity + Vector2D(-r.y, r.x) * (speed / dist);
}

std::vector<Vector2D> Simulation::predictPath(const Vector2D& p, const Vector2D& v) const {
    constexpr int STEPS = 1500;           // steps over the horizon when nothing is close
    constexpr int MAX_ITERATIONS = 8000;  // cap when close passes shorten the steps
    constexpr std::size_t MAX_ATTRACTORS = 16;
    const std::vector<Body>& bodies = world.bodies;
    if (bodies.empty()) return {p, p + v * units::DAYS_PER_YEAR};

    // The heaviest bodies carry nearly all the gravity. Integrate them together
    // with a massless test particle, so the planets move during the prediction.
    std::vector<std::size_t> order(bodies.size());
    std::iota(order.begin(), order.end(), std::size_t{0});
    const std::size_t k = std::min(MAX_ATTRACTORS, order.size());
    std::partial_sort(order.begin(), order.begin() + static_cast<std::ptrdiff_t>(k), order.end(),
                      [&bodies](std::size_t a, std::size_t b) { return bodies[a].mass > bodies[b].mass; });
    order.resize(k);
    const int hostIndex = dominantBody(p);
    if (hostIndex >= 0 &&
        std::find(order.begin(), order.end(), static_cast<std::size_t>(hostIndex)) == order.end())
        order.back() = static_cast<std::size_t>(hostIndex);  // keep the local host, even if light

    // Horizon: one orbit if bound to the host, otherwise long enough to leave
    double horizon = units::DAYS_PER_YEAR;
    if (hostIndex >= 0) {
        const Body& host = bodies[static_cast<std::size_t>(hostIndex)];
        const Vector2D r = p - host.position, u = v - host.velocity;
        const double mu = units::G * host.mass;
        const double dist = std::max(r.magnitude(), 1e-12);
        const double energy = 0.5 * u.lengthSquared() - mu / dist;
        if (energy < 0.0) {
            const double a = -mu / (2.0 * energy);
            horizon = 2.0 * units::PI * std::sqrt(a * a * a / mu);
        } else {
            horizon = 20.0 * dist / std::max(u.magnitude(), 1e-12);
        }
    }
    horizon = std::clamp(horizon, 20.0 * dt, 500.0 * units::DAYS_PER_YEAR);
    const double hBase = horizon / STEPS;
    const double hMin = hBase / 1024.0;
    const double eps2 = SOFTENING * SOFTENING;

    std::vector<Vector2D> pos(k), vel(k), acc(k), oldAcc(k);
    std::vector<double> mass(k), radius(k);
    for (std::size_t i = 0; i < k; ++i) {
        const Body& b = bodies[order[i]];
        pos[i] = b.position;
        vel[i] = b.velocity;
        mass[i] = b.mass;
        radius[i] = b.radius;
    }
    Vector2D tp = p, tv = v, ta;

    auto computeAcc = [&]() {
        std::fill(acc.begin(), acc.end(), Vector2D());
        ta = Vector2D();
        for (std::size_t i = 0; i < k; ++i) {
            for (std::size_t j = i + 1; j < k; ++j) {
                const Vector2D d = pos[j] - pos[i];
                const double r2 = d.lengthSquared() + eps2;
                const double s = units::G / (r2 * std::sqrt(r2));
                acc[i] += d * (s * mass[j]);
                acc[j] -= d * (s * mass[i]);
            }
            const Vector2D d = pos[i] - tp;
            const double r2 = d.lengthSquared() + eps2;
            ta += d * (units::G * mass[i] / (r2 * std::sqrt(r2)));
        }
    };

    std::vector<Vector2D> path;
    path.reserve(STEPS / 2 + 2);
    path.push_back(tp);
    computeAcc();
    double elapsed = 0.0, nextSample = 2.0 * hBase;
    for (int n = 0; n < MAX_ITERATIONS && elapsed < horizon; ++n) {
        // Shorter steps near an attractor, by the same rule as step()
        double tMin = std::numeric_limits<double>::infinity();
        for (std::size_t i = 0; i < k; ++i) {
            const double r2 = (pos[i] - tp).lengthSquared();
            tMin = std::min(tMin, std::sqrt(r2 * std::sqrt(r2) / (units::G * mass[i])));
            const double v2 = (vel[i] - tv).lengthSquared();
            if (v2 > 0.0) tMin = std::min(tMin, std::sqrt(r2 / v2));
        }
        const double h = std::clamp(ETA * tMin, hMin, hBase);
        const double hh = 0.5 * h * h;

        for (std::size_t i = 0; i < k; ++i) pos[i] += vel[i] * h + acc[i] * hh;
        tp += tv * h + ta * hh;
        oldAcc = acc;
        const Vector2D oldTa = ta;
        computeAcc();
        for (std::size_t i = 0; i < k; ++i) vel[i] += (oldAcc[i] + acc[i]) * (0.5 * h);
        tv += (oldTa + ta) * (0.5 * h);
        elapsed += h;

        bool hit = false;
        for (std::size_t i = 0; i < k && !hit; ++i)
            hit = (pos[i] - tp).lengthSquared() <= radius[i] * radius[i];
        const bool closePass = h < 0.25 * hBase && n % 4 == 0;  // extra points through a close pass
        if (hit || closePass || elapsed >= nextSample) {
            path.push_back(tp);
            nextSample = elapsed + 2.0 * hBase;
        }
        if (hit) break;
    }
    return path;
}

double Simulation::totalEnergy() const {
    // Kinetic plus gravity's softened potential whose gradient is exactly the
    // force used in the integration
    return world.kineticEnergy() + gravity.potentialEnergy(world.bodies);
}

double Simulation::angularMomentum() const {
    return world.angularMomentum();
}

} // namespace emma
