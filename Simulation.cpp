#include "Simulation.hpp"
#include "Constants.hpp"
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <iterator>
#include <limits>
#include <numeric>

namespace {

struct OrbitState {
    Vector2D position;
    Vector2D velocity;
};

// Position and velocity on a Kepler orbit, relative to the primary, in the orbit plane.
// mu = G (M + m). Angles in degrees: longitude of perihelion and mean longitude.
OrbitState keplerState(double mu, double a, double e, double longPeriDeg, double meanLongDeg) {
    const double varpi = longPeriDeg * units::DEG;
    const double M = std::remainder((meanLongDeg - longPeriDeg) * units::DEG, 2.0 * units::PI);

    // Kepler's equation E - e sin E = M, by Newton's method
    double E = e < 0.8 ? M : units::PI;
    for (int i = 0; i < 50; ++i) {
        const double f = E - e * std::sin(E) - M;
        E -= f / (1.0 - e * std::cos(E));
        if (std::abs(f) < 1e-14) break;
    }

    const double nu = 2.0 * std::atan2(std::sqrt(1.0 + e) * std::sin(0.5 * E),
                                       std::sqrt(1.0 - e) * std::cos(0.5 * E));
    const double r = a * (1.0 - e * std::cos(E));
    const double h = std::sqrt(mu / (a * (1.0 - e * e)));

    const double theta = nu + varpi;
    const Vector2D position(r * std::cos(theta), r * std::sin(theta));
    // perifocal velocity (-sin nu, e + cos nu) * sqrt(mu / p), rotated by varpi
    const double vx = -h * std::sin(nu);
    const double vy = h * (e + std::cos(nu));
    const Vector2D velocity(vx * std::cos(varpi) - vy * std::sin(varpi),
                            vx * std::sin(varpi) + vy * std::cos(varpi));
    return OrbitState{position, velocity};
}

struct SolarBodyInfo {
    const char* name;
    const char* kind;
    double mass;      // Msun; for planets with moons, the whole system (IAU 2009)
    double radiusKm;  // mean radius
    Rgb color;
};

// In SolarSnapshot order. The Sun's color comes from its temperature in buildSolarSystem().
const SolarBodyInfo SOLAR_BODIES[SOLAR_BODY_COUNT] = {
    {"Sun", "G2V star", 1.0, 695700.0, Rgb{}},
    {"Mercury", "Rocky planet", 1.0 / 6023597.4, 2439.7, Rgb{151, 151, 159}},
    {"Venus", "Rocky planet", 1.0 / 408523.71, 6051.8, Rgb{227, 208, 160}},
    {"Earth", "Rocky planet", units::EARTH_MASS, 6371.0, Rgb{79, 141, 224}},
    {"Moon", "Moon of Earth", units::EARTH_MASS / 81.30056, 1737.4, Rgb{200, 200, 196}},
    {"Mars", "Rocky planet", 1.0 / 3098703.59, 3389.5, Rgb{193, 92, 52}},
    {"Jupiter", "Gas giant", 1.0 / 1047.348644, 69911.0, Rgb{216, 184, 140}},
    {"Saturn", "Gas giant", 1.0 / 3497.9018, 58232.0, Rgb{227, 207, 150}},
    {"Uranus", "Ice giant", 1.0 / 22902.98, 25362.0, Rgb{175, 219, 229}},
    {"Neptune", "Ice giant", 1.0 / 19412.26, 24622.0, Rgb{91, 125, 225}},
};

struct Elements {
    std::size_t body;                 // index into SOLAR_BODIES
    double a, e, longPeri, meanLong;  // AU, -, degrees, degrees
};

// Planets" (JPL), table 1, epoch J2000. Earth and the Moon are placed separately.
const Elements PLANET_ELEMENTS[] = {
    {1, 0.38709927, 0.20563593, 77.45779628, 252.25032350},
    {2, 0.72333566, 0.00677672, 131.60246718, 181.97909950},
    {5, 1.52371034, 0.09339410, -23.94362959, -4.55343205},
    {6, 5.20288700, 0.04838624, 14.72847983, 34.39644051},
    {7, 9.53667594, 0.05386179, 92.59887831, 49.95424423},
    {8, 19.18916464, 0.04725744, 170.95427630, 313.23810451},
    {9, 30.06992276, 0.00859048, 44.96476227, -55.12002969},
};

// Did a and b touch during the last step? Follows the pair along the straight
// segments they moved, so a fast body can't jump through another between steps.
bool sweptContact(const Body& a, const Body& b) {
    const Vector2D start = b.previousPosition - a.previousPosition;
    const Vector2D motion = (b.position - a.position) - start;
    const double mm = motion.lengthSquared();
    const double t = mm > 0.0 ? std::clamp(-start.dot(motion) / mm, 0.0, 1.0) : 0.0;
    const Vector2D closest = start + motion * t;
    const double reach = a.radius + b.radius;
    return closest.lengthSquared() <= reach * reach;
}

} // namespace

// Presets

void Simulation::load(Preset preset, const SolarSnapshot* snapshot) {
    bodies.clear();
    flashes.clear();
    currentPreset = preset;
    simTime = 0.0;
    epochJD = units::J2000_JD;

    switch (preset) {
    case Preset::SolarSystem: buildSolarSystem(snapshot); break;
    case Preset::AlphaCentauri: buildAlphaCentauri(); break;
    case Preset::Empty:
        dt = 0.1;
        calendar = false;
        viewRadiusAU = 2.0;
        timeScaleHint = 30.0;
        break;
    }

    moveToBarycenter();
    accelerationsStale = true;
    ++changes;
}

void Simulation::buildSolarSystem(const SolarSnapshot* snapshot) {
    dt = 0.1;              // days: about 880 steps per orbit of Mercury
    calendar = true;
    viewRadiusAU = 1.8;    // Mars's orbit fills the view
    timeScaleHint = 30.0;  // an Earth year in about 12 seconds

    std::array<Vector2D, SOLAR_BODY_COUNT> position, velocity;
    if (snapshot) {
        // Measured state vectors, from JPL Horizons
        position = snapshot->position;
        velocity = snapshot->velocity;
        epochJD = snapshot->julianDay;
    } else {
        // Mean orbital elements at J2000, with the Sun at the origin
        epochJD = units::J2000_JD;
        for (const Elements& el : PLANET_ELEMENTS) {
            const double mu = units::G * (1.0 + SOLAR_BODIES[el.body].mass);
            const OrbitState s = keplerState(mu, el.a, el.e, el.longPeri, el.meanLong);
            position[el.body] = s.position;
            velocity[el.body] = s.velocity;
        }

        // Earth and Moon: their barycenter on its J2000 orbit, then the pair around it
        const double earthMass = SOLAR_BODIES[3].mass, moonMass = SOLAR_BODIES[4].mass;
        const double pairMass = earthMass + moonMass;
        const OrbitState bary = keplerState(units::G * (1.0 + pairMass), 1.00000261, 0.01671123,
                                            102.93768193, 100.46457166);
        const OrbitState moon = keplerState(units::G * pairMass, 384400.0 / units::KM_PER_AU, 0.0549,
                                            218.3164477 - 134.9633964, 218.3164477);
        position[3] = bary.position - moon.position * (moonMass / pairMass);
        velocity[3] = bary.velocity - moon.velocity * (moonMass / pairMass);
        position[4] = bary.position + moon.position * (earthMass / pairMass);
        velocity[4] = bary.velocity + moon.velocity * (earthMass / pairMass);
    }

    for (std::size_t i = 0; i < SOLAR_BODY_COUNT; ++i) {
        const SolarBodyInfo& info = SOLAR_BODIES[i];
        const Rgb color = i == 0 ? Body::blackbody(units::SUN_TEMPERATURE) : info.color;
        bodies.emplace_back(info.name, info.kind, info.mass, info.radiusKm / units::KM_PER_AU, color,
                            position[i], velocity[i]);
    }
}

void Simulation::buildAlphaCentauri() {
    dt = 1.0;
    calendar = false;
    viewRadiusAU = 22.0;
    timeScaleHint = 2.0 * units::DAYS_PER_YEAR;  // two years per second: one orbit in 40 s

    // Starts at periastron.
    const double mA = 1.0788, mB = 0.9092, total = mA + mB;
    const double period = 79.91 * units::DAYS_PER_YEAR;
    const double a = std::cbrt(units::G * total * period * period / (4.0 * units::PI * units::PI));
    const OrbitState rel = keplerState(units::G * total, a, 0.5179, 0.0, 0.0);

    bodies.emplace_back("Alpha Centauri A", "G2V star", mA, 1.2234 * units::SOLAR_RADIUS_AU,
                        Body::blackbody(5790.0), rel.position * (-mB / total), rel.velocity * (-mB / total));
    bodies.emplace_back("Alpha Centauri B", "K1V star", mB, 0.8632 * units::SOLAR_RADIUS_AU,
                        Body::blackbody(5260.0), rel.position * (mA / total), rel.velocity * (mA / total));
}

void Simulation::moveToBarycenter() {
    double m = 0.0;
    Vector2D r, p;
    for (const Body& b : bodies) {
        m += b.mass;
        r += b.position * b.mass;
        p += b.velocity * b.mass;
    }
    if (m <= 0.0) return;
    const Vector2D com = r / m, drift = p / m;
    for (Body& b : bodies) {
        b.position -= com;
        b.velocity -= drift;
        b.previousPosition = b.position;
    }
}

void Simulation::clear() {
    bodies.clear();
    flashes.clear();
    accelerationsStale = true;
    ++changes;
}

void Simulation::addBody(double mass, const Vector2D& position, const Vector2D& velocity) {
    bodies.emplace_back(mass, position, velocity);
    accelerationsStale = true;  // Verlet needs the new body's acceleration before its first drift
    ++changes;
}

// Integration

void Simulation::step() {
    if (accelerationsStale) {
        computeAccelerations();
        accelerationsStale = false;
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
        double h = std::max(ETA * dynamicalTime(), minPiece);
        if (h >= remaining) h = remaining;
        else if (remaining < 1.5 * h) h = 0.5 * remaining;
        verletStep(h);
        resolveCollisions();
        recordTrails();  // trails stay smooth through a close pass
        remaining -= h;
        ++pieces;
    }
    lastSubsteps = pieces;
}

void Simulation::verletStep(double h) {
    // Velocity Verlet: drift with the stored acceleration, recompute, then kick
    // with the average of the old and new accelerations.
    const double halfHSq = 0.5 * h * h;
    oldAcceleration.resize(bodies.size());
    for (std::size_t i = 0; i < bodies.size(); ++i) {
        Body& b = bodies[i];
        b.previousPosition = b.position;
        b.position += b.velocity * h + b.acceleration * halfHSq;
        oldAcceleration[i] = b.acceleration;
    }
    computeAccelerations();
    for (std::size_t i = 0; i < bodies.size(); ++i)
        bodies[i].velocity += (oldAcceleration[i] + bodies[i].acceleration) * (0.5 * h);

    simTime += h;
    lastSubstep = h;
    jerkValid = true;
}

double Simulation::dynamicalTime() const {
    double t = std::numeric_limits<double>::infinity();
    const std::size_t n = bodies.size();

    if (n <= DIRECT_LIMIT) {
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
    // Use how fast each body's acceleration changed over the last piece: |a| / |da/dt|.
    if (!jerkValid || oldAcceleration.size() != n) return t;
    for (std::size_t i = 0; i < n; ++i) {
        const double change = (bodies[i].acceleration - oldAcceleration[i]).magnitude();
        if (change > 0.0) t = std::min(t, bodies[i].acceleration.magnitude() * lastSubstep / change);
    }
    return t;
}

void Simulation::computeAccelerations() {
    const std::size_t n = bodies.size();
    const double eps2 = SOFTENING * SOFTENING;

    if (n <= DIRECT_LIMIT) {
        // Exact pairwise sum, each pair once (Newton's third law)
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

void Simulation::resolveCollisions() {
    const std::size_t n = bodies.size();
    if (n < 2) return;

    // Sweep and prune along x. Each body covers the stretch it moved this step,
    // padded by its radius; only overlapping stretches get the exact test.
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
                merge(i, j);
                merged = true;
            }
        }
    }

    if (merged) {
        bodies.erase(std::remove_if(bodies.begin(), bodies.end(), [](const Body& b) { return !b.alive; }),
                     bodies.end());
        jerkValid = false;  // indices shifted, oldAcceleration no longer lines up
        ++changes;
    }
}

void Simulation::merge(std::size_t a, std::size_t b) {
    // mass and momentum are conserved and the merged body
    // carries on from the pair's center of mass. The heavier body keeps its
    // name, color and trail.
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
    // so the next drift starts from the right acceleration instead of zero.
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

void Simulation::recordTrails() {
    // A new point each time a body's direction of travel has turned by TRAIL_TURN
    // since the last one
    static const double cosTurn = std::cos(TRAIL_TURN);
    for (Body& b : bodies) {
        const double speed = b.velocity.magnitude();
        if (speed <= 0.0) continue;
        const Vector2D heading = b.velocity / speed;
        if (!b.trail.empty() && heading.dot(b.trailHeading) > cosTurn) continue;
        b.trail.push_back(b.position);
        if (b.trail.size() > MAX_TRAIL) b.trail.pop_front();
        b.trailHeading = heading;
    }
}

void Simulation::updateEffects(float realDt) {
    for (MergeFlash& f : flashes) f.age += realDt;
    flashes.erase(std::remove_if(flashes.begin(), flashes.end(),
                                 [](const MergeFlash& f) { return f.age > FLASH_LIFETIME; }),
                  flashes.end());
}

// Queries

int Simulation::strongestPull(const Vector2D& p, int exclude) const {
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
    const Body& host = bodies[static_cast<std::size_t>(hostIndex)];
    const Vector2D r = p - host.position;
    const double dist = r.magnitude();
    if (dist <= 0.0) return host.velocity;
    const double speed = std::sqrt(units::G * host.mass / dist);
    // counterclockwise seen from above, the way the planets go
    return host.velocity + Vector2D(-r.y, r.x) * (speed / dist);
}

std::vector<Vector2D> Simulation::predictPath(const Vector2D& p, const Vector2D& v) const {
    constexpr int STEPS = 1500;        // steps over the horizon when nothing is close
    constexpr int MAX_ITERATIONS = 8000;  // cap when close passes shorten the steps
    constexpr std::size_t MAX_ATTRACTORS = 16;
    if (bodies.empty()) return {p, p + v * units::DAYS_PER_YEAR};

    // The heaviest bodies carry nearly all the gravity. Integrate them together
    // with a massless test particle, so the planets move during the prediction.
    std::vector<std::size_t> order(bodies.size());
    std::iota(order.begin(), order.end(), std::size_t{0});
    const std::size_t k = std::min(MAX_ATTRACTORS, order.size());
    std::partial_sort(order.begin(), order.begin() + static_cast<std::ptrdiff_t>(k), order.end(),
                      [this](std::size_t a, std::size_t b) { return bodies[a].mass > bodies[b].mass; });
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
    // Kinetic plus the Plummer-softened potential -G m1 m2 / sqrt(r^2 + eps^2),
    // whose gradient is exactly the force used in the integration
    const double eps2 = SOFTENING * SOFTENING;
    double kinetic = 0.0, potential = 0.0;
    for (const Body& b : bodies) kinetic += 0.5 * b.mass * b.velocity.lengthSquared();
    for (std::size_t i = 0; i < bodies.size(); ++i) {
        for (std::size_t j = i + 1; j < bodies.size(); ++j) {
            const double r2 = (bodies[j].position - bodies[i].position).lengthSquared();
            potential -= units::G * bodies[i].mass * bodies[j].mass / std::sqrt(r2 + eps2);
        }
    }
    return kinetic + potential;
}

double Simulation::angularMomentum() const {
    double l = 0.0;
    for (const Body& b : bodies) l += b.mass * b.position.cross(b.velocity);
    return l;
}
