// Tests for the physics engine.
// Run them all:  ctest --test-dir build -C Release --output-on-failure
// Run one:       build/Release/physics_tests earth_year

#include "emma/BlackHole.hpp"
#include "emma/Body.hpp"
#include "emma/Calendar.hpp"
#include "emma/Clock.hpp"
#include "emma/Constants.hpp"
#include "emma/Galaxy.hpp"
#include "emma/Horizons.hpp"
#include "emma/IAS15Integrator.hpp"
#include "emma/Octree.hpp"
#include "emma/Simulation.hpp"
#include "emma/Spin.hpp"
#include "emma/VerletIntegrator.hpp"
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <random>
#include <string>
#include <vector>

using namespace emma;

namespace {

int failures = 0;

#define CHECK(condition, ...)                \
    do {                                     \
        if (!(condition)) {                  \
            std::printf("  FAILED: ");       \
            std::printf(__VA_ARGS__);        \
            std::printf("\n");               \
            ++failures;                      \
        }                                    \
    } while (0)

const Body& find(const Simulation& sim, const char* name) {
    for (const Body& b : sim.getBodies())
        if (b.name == name) return b;
    std::printf("  FAILED: no body named %s\n", name);
    std::exit(1);
}

double relative(double value, double reference) {
    return std::abs((value - reference) / reference);
}

double relative(const Vector3D& value, const Vector3D& reference) {
    return (value - reference).magnitude() / reference.magnitude();
}

// ---------------------------------------------------------------------------

void testCalendar() {
    CHECK(calendar::date(units::J2000_JD) == "2000-01-01", "J2000 is %s", calendar::date(units::J2000_JD).c_str());
    CHECK(calendar::date(2451603.5) == "2000-02-29", "leap day came out as %s", calendar::date(2451603.5).c_str());
    CHECK(calendar::date(2461353.5) == "2026-11-09", "JD 2461353.5 is %s", calendar::date(2461353.5).c_str());
    CHECK(calendar::date(2461353.5 + 0.999) == "2026-11-09", "late in the day rolled over to the next date");
    CHECK(calendar::startOfDay(2461353.5 + 0.7) == 2461353.5, "start of day is %.3f",
          calendar::startOfDay(2461353.5 + 0.7));
}

void testHorizonsParse() {
    // A shortened Horizons reply
    const std::string reply =
        "API VERSION: 1.2\n"
        "API SOURCE: NASA/JPL Horizons API\n\n"
        "*******************************************************************************\n"
        "$$SOE\n"
        "2461353.500000000, A.D. 2026-Nov-09 00:00:00.0000,  6.123456789012345E-01,  7.654321098765432E-01, "
        "-3.123456789012345E-05, -1.345678901234567E-02,  1.098765432109876E-02,  2.345678901234567E-07,\n"
        "$$EOE\n";
    Vector3D p, v;
    std::string error;
    const bool ok = horizons::parseStateVector(reply, p, v, error);
    CHECK(ok, "parse failed: %s", error.c_str());
    CHECK(p.x == 6.123456789012345E-01 && p.y == 7.654321098765432E-01, "position %.15g, %.15g", p.x, p.y);
    CHECK(v.x == -1.345678901234567E-02 && v.y == 1.098765432109876E-02, "velocity %.15g, %.15g", v.x, v.y);
    CHECK(p.z == -3.123456789012345E-05 && v.z == 2.345678901234567E-07, "z %.15g, vz %.15g", p.z, v.z);

    // An error reply: its message should come through
    const std::string failure = "API VERSION: 1.2\nAPI SOURCE: NASA/JPL Horizons API\n\n"
                                "    No ephemeris for target \"Foo\" after A.D. 2200-JAN-01\n";
    error.clear();
    CHECK(!horizons::parseStateVector(failure, p, v, error), "accepted a reply without a table");
    CHECK(error.find("No ephemeris") != std::string::npos, "error message lost: '%s'", error.c_str());

    const std::string url = horizons::requestUrl("399", 2461353.5);
    CHECK(url.find("COMMAND=%27399%27") != std::string::npos, "bad request: %s", url.c_str());
    CHECK(url.find("OUT_UNITS=%27AU-D%27") != std::string::npos, "request not in AU and days: %s", url.c_str());
}

void testHorizonsReply() {
    // A good reply, with escaped characters to unpack
    const std::string good =
        "{\"signature\":{\"source\":\"NASA/JPL Horizons API\",\"version\":\"1.2\"},"
        "\"result\":\"Target body name: Sun (10)\\n$$SOE\\n"
        "2461355.500000000, A.D. 2026-Nov-11 00:00:00.0000, 1.0E-03, -2.0E-03, 0.0, 3.0E-06, 4.0E-06, 0.0,\\n"
        "$$EOE\\nQuote: \\\"done\\\" \\u00e9\\n\"}";
    std::string output, error;
    CHECK(horizons::parseReply(good, 200, output, error), "rejected a good reply: %s", error.c_str());
    CHECK(output.find("$$SOE\n") != std::string::npos, "newlines not unescaped: %s", output.c_str());
    CHECK(output.find("\"done\"") != std::string::npos, "quotes not unescaped: %s", output.c_str());
    CHECK(output.find("\xC3\xA9") != std::string::npos, "\\u00e9 did not become UTF-8");
    Vector3D p, v;
    CHECK(horizons::parseStateVector(output, p, v, error) && p.x == 1.0E-03 && v.y == 4.0E-06,
          "table inside the JSON reply not read: %s", error.c_str());

    // An error message spread over several lines
    const std::string failed =
        "{\"signature\":{\"version\":\"1.2\"},\"error\":\" \\n"
        "Insufficient ephemeris data has been loaded to compute the state of 10 (SUN)\\n"
        "relative to 0 (SOLAR SYSTEM BARYCENTER).\\n\"}";
    error.clear();
    CHECK(!horizons::parseReply(failed, 200, output, error), "accepted a reply with an error field");
    CHECK(error.rfind("Insufficient ephemeris data", 0) == 0 && error.find("(SUN) relative") != std::string::npos,
          "error field not read and joined: '%s'", error.c_str());

    // Other kinds of error replies
    error.clear();
    CHECK(!horizons::parseReply("{\"message\":\"bad TLIST value\"}", 400, output, error) && error == "bad TLIST value",
          "400 message: '%s'", error.c_str());
    error.clear();
    CHECK(!horizons::parseReply("<html>busy</html>", 503, output, error) && error.find("503") != std::string::npos,
          "503 reply: '%s'", error.c_str());

    // The request asks for JSON and a time span
    const std::string url = horizons::requestUrl("10", 2461355.5);
    CHECK(url.find("format=json") != std::string::npos, "request isn't JSON: %s", url.c_str());
    CHECK(url.find("START_TIME=%27JD2461355.500000%27") != std::string::npos && url.find("TLIST") == std::string::npos,
          "request should use START_TIME, not TLIST: %s", url.c_str());
}

void testJ2000Start() {
    // On 1 January 2000 Earth was 0.9833 AU from the Sun, at longitude 100.38 deg
    Simulation sim;
    sim.load(Preset::SolarSystem);
    const Vector3D r = find(sim, "Earth").position - find(sim, "Sun").position;
    const double distance = r.magnitude();
    double longitude = std::atan2(r.y, r.x) / units::DEG;
    if (longitude < 0.0) longitude += 360.0;
    std::printf("  Earth at J2000: %.5f AU from the Sun, longitude %.3f deg\n", distance, longitude);
    CHECK(std::abs(distance - 0.98331) < 1e-3, "distance %.5f AU, expected 0.98331", distance);
    CHECK(std::abs(longitude - 100.38) < 0.2, "longitude %.3f deg, expected 100.38", longitude);
}

void earthYear(Simulation& sim) {
    // Earth (with the Moon) should go once around the Sun in 365.2564 days
    sim.load(Preset::SolarSystem);
    auto angle = [&sim]() {
        const Body& earth = find(sim, "Earth");
        const Body& moon = find(sim, "Moon");
        const Vector3D pair = (earth.position * earth.mass + moon.position * moon.mass) / (earth.mass + moon.mass);
        const Vector3D r = pair - find(sim, "Sun").position;
        return std::atan2(r.y, r.x);
    };

    double previous = angle(), previousTime = sim.time(), travelled = 0.0, period = 0.0;
    while (sim.time() < 400.0) {
        sim.step();
        const double a = angle();
        double d = a - previous;
        if (d > units::PI) d -= 2.0 * units::PI;
        if (d < -units::PI) d += 2.0 * units::PI;
        if (travelled + d >= 2.0 * units::PI) {
            const double fraction = (2.0 * units::PI - travelled) / d;
            period = previousTime + fraction * (sim.time() - previousTime);
            break;
        }
        travelled += d;
        previous = a;
        previousTime = sim.time();
    }
    std::printf("  one orbit: %.4f days\n", period);
    CHECK(std::abs(period - 365.2564) < 0.05, "orbit took %.4f days, expected 365.2564", period);
}

void testEarthYear() {
    Simulation sim;
    earthYear(sim);
}

void testIas15EarthYear() {
    Simulation sim;
    sim.setIntegrator(std::make_unique<IAS15Integrator>());
    earthYear(sim);
}

void testConservation() {
    // Ten years of the solar system
    Simulation sim;
    sim.load(Preset::SolarSystem);
    const double e0 = sim.totalEnergy();
    const Vector3D l0 = sim.angularMomentum();
    const int steps = static_cast<int>(10.0 * units::DAYS_PER_YEAR / sim.timeStep());
    double worstEnergy = 0.0;
    int maxPieces = 1;
    for (int i = 0; i < steps; ++i) {
        sim.step();
        maxPieces = std::max(maxPieces, sim.lastSubstepCount());
        if (i % 100 == 0) worstEnergy = std::max(worstEnergy, relative(sim.totalEnergy(), e0));
    }
    const double momentum = relative(sim.angularMomentum(), l0);
    std::printf("  10 years: worst energy drift %.2e, angular momentum drift %.2e\n", worstEnergy, momentum);
    CHECK(worstEnergy < 1e-6, "energy drifted by %.2e", worstEnergy);
    CHECK(momentum < 1e-9, "angular momentum drifted by %.2e", momentum);
    CHECK(maxPieces == 1, "the planets needed split steps (%d pieces); the solar system should run unsplit",
          maxPieces);
    CHECK(sim.getBodyCount() == SOLAR_BODY_COUNT, "bodies merged: %zu left", sim.getBodyCount());
}

void testIas15Conservation() {
    // Ten years with IAS15: errors should be far smaller than Verlet's
    Simulation sim;
    sim.setIntegrator(std::make_unique<IAS15Integrator>());
    sim.load(Preset::SolarSystem);
    const double e0 = sim.totalEnergy();
    const Vector3D l0 = sim.angularMomentum();
    const int steps = static_cast<int>(10.0 * units::DAYS_PER_YEAR / sim.timeStep());
    double worstEnergy = 0.0;
    for (int i = 0; i < steps; ++i) {
        sim.step();
        if (i % 100 == 0) worstEnergy = std::max(worstEnergy, relative(sim.totalEnergy(), e0));
    }
    const double momentum = relative(sim.angularMomentum(), l0);
    std::printf("  10 years with IAS15: worst energy drift %.2e, angular momentum drift %.2e\n", worstEnergy,
                momentum);
    CHECK(worstEnergy < 1e-10, "energy drifted by %.2e", worstEnergy);
    CHECK(momentum < 1e-10, "angular momentum drifted by %.2e", momentum);
    CHECK(sim.getBodyCount() == SOLAR_BODY_COUNT, "bodies merged: %zu left", sim.getBodyCount());
}

void closePass(Simulation& sim, double tolerance) {
    // Two Earth-mass bodies narrowly miss each other
    sim.load(Preset::Empty);
    const double m = units::EARTH_MASS;
    const double v = 2.0 * units::KMS_TO_AU_PER_DAY;  // each, so 4 km/s apart
    const double b = 4.3e-4;                          // AU, offset between their paths
    sim.addBody(m, Vector3D(-0.01, 0.5 * b), Vector3D(v, 0.0));
    sim.addBody(m, Vector3D(0.01, -0.5 * b), Vector3D(-v, 0.0));

    auto pairEnergy = [&sim, m]() {  // the pair's orbital energy
        const auto& bodies = sim.getBodies();
        const Vector3D dv = bodies[1].velocity - bodies[0].velocity;
        const double r = (bodies[1].position - bodies[0].position).magnitude();
        return 0.5 * dv.lengthSquared() - units::G * 2.0 * m / r;
    };
    const double e0 = pairEnergy();
    int maxPieces = 1;
    while (sim.time() < 20.0 && sim.getBodyCount() == 2) {
        sim.step();
        maxPieces = std::max(maxPieces, sim.lastSubstepCount());
    }
    CHECK(sim.getBodyCount() == 2, "the bodies merged, they should have missed");
    if (sim.getBodyCount() != 2) return;
    const double change = relative(pairEnergy(), e0);
    std::printf("  close pass: steps split into up to %d pieces, energy change %.2e\n", maxPieces, change);
    CHECK(change < tolerance, "energy changed by %.2e through the pass", change);
    CHECK(maxPieces > 1, "the close pass wasn't split into smaller steps");
}

void testClosePass() {
    // Verlet should keep the energy within 1% through the pass
    Simulation sim;
    closePass(sim, 1e-2);
}

void testIas15ClosePass() {
    // IAS15 should be far more accurate
    Simulation sim;
    sim.setIntegrator(std::make_unique<IAS15Integrator>());
    closePass(sim, 1e-7);
}

void headOnMerge(Simulation& sim) {
    // Two bodies collide head-on. One body should be left, with both masses, at rest
    sim.load(Preset::Empty);
    const double m1 = units::EARTH_MASS, m2 = 0.5 * units::EARTH_MASS;
    const double v = 10.0 * units::KMS_TO_AU_PER_DAY;
    sim.addBody(m1, Vector3D(-0.002, 0.0), Vector3D(v, 0.0));
    sim.addBody(m2, Vector3D(0.002, 0.0), Vector3D(-2.0 * v, 0.0));
    const double centerX = (m1 * -0.002 + m2 * 0.002) / (m1 + m2);

    while (sim.time() < 5.0 && sim.getBodyCount() == 2) sim.step();
    CHECK(sim.getBodyCount() == 1, "%zu bodies left, expected a merge", sim.getBodyCount());
    if (sim.getBodyCount() != 1) return;
    const Body& merged = sim.getBodies()[0];
    CHECK(relative(merged.mass, m1 + m2) < 1e-12, "mass %.6e, expected %.6e", merged.mass, m1 + m2);
    CHECK(merged.velocity.magnitude() < 1e-9 * v, "merged body moves at %.2e AU/day, momentum was lost",
          merged.velocity.magnitude());
    CHECK(std::abs(merged.position.x - centerX) < 1e-9 && std::abs(merged.position.y) < 1e-9,
          "merged at %.3e, %.3e, expected %.3e, 0", merged.position.x, merged.position.y, centerX);
}

void testHeadOnMerge() {
    Simulation sim;
    headOnMerge(sim);
}

void testIas15HeadOnMerge() {
    Simulation sim;
    sim.setIntegrator(std::make_unique<IAS15Integrator>());
    headOnMerge(sim);
}

void testCrowdedClosePass() {
    // With many bodies, a close pass right after adding them should still split the step
    Simulation sim;
    sim.load(Preset::Empty);
    for (std::size_t i = 0; i < Simulation::DIRECT_LIMIT; ++i)
        sim.addBody(units::EARTH_MASS, Vector3D(10.0 + static_cast<double>(i), 0.0), Vector3D());
    // Two bodies about a day from meeting
    sim.addBody(units::EARTH_MASS, Vector3D(0.0, 0.0), Vector3D());
    sim.addBody(units::EARTH_MASS, Vector3D(0.01, 0.0), Vector3D(-0.01, 0.0005));
    sim.step();
    CHECK(sim.lastSubstepCount() > 1, "the first step with %zu bodies ran as %d piece(s)",
          sim.getBodyCount(), sim.lastSubstepCount());
}

void testIas15Guard() {
    // Too many bodies: IAS15 should switch to Verlet, and stay switched
    Simulation sim;
    sim.setIntegrator(std::make_unique<IAS15Integrator>());
    sim.load(Preset::Empty);
    for (std::size_t i = 0; i < Simulation::DIRECT_LIMIT; ++i)
        sim.addBody(units::EARTH_MASS, Vector3D(static_cast<double>(i), 0.0), Vector3D());
    CHECK(sim.getIntegrator().needsExactForces(), "IAS15 was dropped at %zu bodies", sim.getBodyCount());

    sim.addBody(units::EARTH_MASS, Vector3D(-1.0, 0.0), Vector3D());
    CHECK(!sim.getIntegrator().needsExactForces(), "IAS15 still runs with %zu bodies", sim.getBodyCount());

    sim.setIntegrator(std::make_unique<IAS15Integrator>());
    CHECK(!sim.getIntegrator().needsExactForces(), "IAS15 could be picked again with %zu bodies",
          sim.getBodyCount());
}

void testBarnesHut() {
    // The tree's forces against exact ones for 300 random bodies
    std::mt19937 rng(42);
    std::uniform_real_distribution<double> coordinate(-10.0, 10.0), mass(1e-6, 1e-3);
    std::vector<Body> bodies;
    for (int i = 0; i < 300; ++i)
        bodies.emplace_back(mass(rng), Vector3D(coordinate(rng), coordinate(rng), coordinate(rng)), Vector3D());

    const double eps2 = Simulation::SOFTENING * Simulation::SOFTENING;
    Octree tree;
    tree.build(bodies);

    double worstExact = 0.0;
    std::vector<double> errors;
    for (std::size_t i = 0; i < bodies.size(); ++i) {
        Vector3D direct;
        for (std::size_t j = 0; j < bodies.size(); ++j) {
            if (j == i) continue;
            const Vector3D d = bodies[j].position - bodies[i].position;
            const double r2 = d.lengthSquared() + eps2;
            direct += d * (units::G * bodies[j].mass / (r2 * std::sqrt(r2)));
        }
        const double scale = direct.magnitude();
        worstExact = std::max(worstExact, (tree.acceleration(i, 0.0, eps2) - direct).magnitude() / scale);
        errors.push_back((tree.acceleration(i, Simulation::THETA, eps2) - direct).magnitude() / scale);
    }
    std::nth_element(errors.begin(), errors.begin() + static_cast<std::ptrdiff_t>(errors.size() / 2), errors.end());
    const double median = errors[errors.size() / 2];
    std::printf("  tree vs exact: %.1e with no approximation, median %.1e at theta %.1f\n", worstExact, median,
                Simulation::THETA);
    CHECK(worstExact < 1e-10, "the tree without approximation is off by %.2e", worstExact);
    CHECK(median < 1e-2, "median force error %.2e at the working opening angle", median);
}

int indexOf(const Simulation& sim, const char* name) {
    const auto& bodies = sim.getBodies();
    for (std::size_t i = 0; i < bodies.size(); ++i)
        if (bodies[i].name == name) return static_cast<int>(i);
    return -1;
}

void testDominantBody() {
    // Which body's gravity rules near each star
    Simulation binary;
    binary.load(Preset::AlphaCentauri);
    const int a = indexOf(binary, "Alpha Centauri A"), b = indexOf(binary, "Alpha Centauri B");
    const auto& stars = binary.getBodies();
    const int nearA = binary.dominantBody(stars[static_cast<std::size_t>(a)].position + Vector3D(0.5, 0.0));
    const int nearB = binary.dominantBody(stars[static_cast<std::size_t>(b)].position + Vector3D(0.5, 0.0));
    CHECK(nearA == a, "0.5 AU from A, body %d rules instead of A (%d)", nearA, a);
    CHECK(nearB == b, "0.5 AU from B, body %d rules instead of B (%d)", nearB, b);

    // In the solar system: Earth near Earth, the Sun between the planets
    Simulation solar;
    solar.load(Preset::SolarSystem);
    const int earth = indexOf(solar, "Earth"), sun = indexOf(solar, "Sun");
    const Vector3D nearEarth = solar.getBodies()[static_cast<std::size_t>(earth)].position + Vector3D(0.001, 0.0);
    CHECK(solar.dominantBody(nearEarth) == earth, "150,000 km from Earth, body %d rules instead of Earth",
          solar.dominantBody(nearEarth));
    CHECK(solar.dominantBody(Vector3D(0.0, 2.5)) == sun, "2.5 AU out, body %d rules instead of the Sun",
          solar.dominantBody(Vector3D(0.0, 2.5)));
}

void testTrailSmoothness() {
    // A planet on a circular orbit should leave a smooth trail
    Simulation sim;
    sim.load(Preset::AlphaCentauri);
    const Vector3D start = find(sim, "Alpha Centauri A").position + Vector3D(1.0, 0.0);
    sim.addBody(units::EARTH_MASS, start, sim.circularVelocity(start));
    while (sim.time() < 2.0 * units::DAYS_PER_YEAR) sim.step();

    CHECK(sim.getBodyCount() == 3, "%zu bodies, the planet merged", sim.getBodyCount());
    if (sim.getBodyCount() != 3) return;
    const Body& planet = sim.getBodies().back();
    const double distance = (planet.position - find(sim, "Alpha Centauri A").position).magnitude();
    CHECK(std::abs(distance - 1.0) < 0.3, "the planet is %.2f AU from A, its circular orbit failed", distance);

    double worst = 0.0;
    const auto& t = planet.trail;
    for (std::size_t k = 2; k < t.size(); ++k) {
        const Vector3D d1 = t[k - 1].position - t[k - 2].position, d2 = t[k].position - t[k - 1].position;
        const double c = d1.dot(d2) / (d1.magnitude() * d2.magnitude());
        worst = std::max(worst, std::acos(std::clamp(c, -1.0, 1.0)) / units::DEG);
    }
    std::printf("  trail: %zu points, sharpest bend %.2f deg\n", t.size(), worst);
    CHECK(t.size() > 100, "only %zu trail points in two orbits", t.size());
    CHECK(worst < 5.0, "the trail bends by %.1f deg at one point, it will look jagged", worst);

    bool ordered = true;
    for (std::size_t k = 1; k < t.size(); ++k) ordered = ordered && t[k].time > t[k - 1].time;
    CHECK(ordered && t.back().time <= sim.time(), "trail times run out of order or past the clock");
}

void testClock() {
    const double frame = 1.0 / 60.0;
    Simulation sim;
    sim.load(Preset::SolarSystem);
    CHECK(sim.hasCalendar(), "the solar system has no calendar");
    if (!sim.hasCalendar()) return;

    // One second at 10 days/s runs 10 days
    emma::Clock clock;
    clock.timeScale = 10.0;
    for (int i = 0; i < 60; ++i) clock.run(sim, frame);
    CHECK(std::abs(sim.time() - 10.0) < 0.15, "a second at 10 days/s ran %.3f days", sim.time());

    // Paused: nothing moves, except for a single step
    clock.paused = true;
    const double before = sim.time();
    clock.run(sim, frame);
    CHECK(sim.time() == before, "time moved while paused");
    clock.requestStep();
    clock.run(sim, frame);
    CHECK(sim.time() > before && clock.paused, "a step request didn't step and stay paused");

    // Real time catches up to now, even from a month behind
    sim.load(Preset::SolarSystem);
    clock = emma::Clock();
    clock.live = true;
    const double now = sim.epoch() + 30.25;
    for (int i = 0; i < 10; ++i) clock.run(sim, frame, now);
    const double lag = (now - (sim.epoch() + sim.time())) * units::SECONDS_PER_DAY;
    std::printf("  real time: %.6f s behind after catching up 30.25 days\n", lag);
    CHECK(std::abs(lag) < 1.0, "real time is %.3f s off the clock", lag);
    CHECK(sim.timeStep() < 1.0 / 86400.0, "the step is %.3g days, real time should step by fractions of a second",
          sim.timeStep());

    // Real time needs dates, so Alpha Centauri turns it off
    sim.load(Preset::AlphaCentauri);
    clock.run(sim, frame, now);
    CHECK(!clock.live, "real time stayed on without a calendar");

    // Run ahead of "now", then turn real time on: it waits, and says how far ahead it is
    sim.load(Preset::SolarSystem);
    clock = emma::Clock();
    clock.timeScale = 365.25;
    for (int i = 0; i < 60; ++i) clock.run(sim, frame, sim.epoch());
    clock.live = true;
    const double ahead = sim.time();
    const emma::Clock::Report waiting = clock.run(sim, frame, sim.epoch());
    CHECK(waiting.steps == 0 && relative(waiting.ahead, ahead) < 1e-9,
          "%.1f days ahead, real time took %d steps and reported %.1f days", ahead, waiting.steps, waiting.ahead);

    // Leaving real time, a slow speed moves at once instead of paying back the gap first
    clock.live = false;
    clock.timeScale = 0.375;  // 9 hours a second
    for (int i = 0; i < 2; ++i) clock.run(sim, frame, sim.epoch());
    CHECK(sim.time() > ahead, "after leaving real time, 9 hours a second didn't move");
}

double inclination(const Simulation& sim, const char* name, const char* around) {
    const Body& body = find(sim, name);
    const Body& host = find(sim, around);
    const Vector3D h = (body.position - host.position).cross(body.velocity - host.velocity);
    return std::acos(h.z / h.magnitude()) / units::DEG;
}

void testInclination() {
    Simulation sim;
    sim.load(Preset::SolarSystem);
    const double mercury = inclination(sim, "Mercury", "Sun");
    const double moon = inclination(sim, "Moon", "Earth");
    std::printf("  at J2000: Mercury %.5f deg, Moon %.5f deg\n", mercury, moon);
    CHECK(std::abs(mercury - 7.00498) < 1e-3, "Mercury's orbit is tilted %.5f deg, expected 7.00498", mercury);
    CHECK(std::abs(moon - 5.1454) < 1e-3, "the Moon's orbit is tilted %.5f deg, expected 5.1454", moon);

    while (sim.time() < 10.0 * units::DAYS_PER_YEAR) sim.step();
    const double later = inclination(sim, "Mercury", "Sun");
    std::printf("  after 10 years: Mercury %.5f deg\n", later);
    CHECK(std::abs(later - 7.00498) < 0.01, "Mercury's orbit tilted to %.5f deg in 10 years", later);
}

// How far a 3x3 matrix is from being a pure rotation
double rotationError(const double (&m)[3][3]) {
    double worst = 0.0;
    for (int i = 0; i < 3; ++i) {
        for (int j = 0; j < 3; ++j) {
            const double dot = m[i][0] * m[j][0] + m[i][1] * m[j][1] + m[i][2] * m[j][2];
            worst = std::max(worst, std::abs(dot - (i == j ? 1.0 : 0.0)));
        }
    }
    return worst;
}

void testGalacticFrame() {
    // The published table should be a rotation, which catches typos
    const double table = rotationError(galaxy::ICRS_TO_GALACTIC);
    const double straightened = rotationError(galaxy::eclipticToGalactic().m);
    std::printf("  the Hipparcos table is off a rotation by %.1e, the matrix EMMA uses by %.1e\n", table,
                straightened);
    CHECK(table < 1e-9, "the Hipparcos table isn't a rotation: off by %.2e", table);
    CHECK(straightened < 1e-14, "the ecliptic to Galactic matrix isn't a rotation: off by %.2e", straightened);

    const double ra = 192.85948 * units::DEG, dec = 27.12825 * units::DEG;
    const Vector3D poleEquatorial(std::cos(dec) * std::cos(ra), std::cos(dec) * std::sin(ra), std::sin(dec));
    const Vector3D pole = galaxy::fromEcliptic(galaxy::equatorialToEcliptic(poleEquatorial));
    CHECK((pole - Vector3D(0.0, 0.0, 1.0)).magnitude() < 1e-8, "the north Galactic pole lands at %.9f, %.9f, %.9f",
          pole.x, pole.y, pole.z);

    const Vector3D eclipticPole = galaxy::fromEcliptic(Vector3D(0.0, 0.0, 1.0));
    const double l = std::atan2(eclipticPole.y, eclipticPole.x) / units::DEG;
    const double b = std::asin(eclipticPole.z) / units::DEG;
    std::printf("  north ecliptic pole at l = %.3f, b = %.3f deg\n", l, b);
    CHECK(std::abs(l - 96.38) < 0.02 && std::abs(b - 29.81) < 0.02, "north ecliptic pole at l %.3f, b %.3f", l, b);

    const Vector3D v(0.3, -1.7, 2.9);
    const double moved = (galaxy::toEcliptic(galaxy::fromEcliptic(v)) - v).magnitude();
    CHECK(moved < 1e-12, "there and back moved the point by %.2e", moved);

    const Vector3D heading = galaxy::toEcliptic(galaxy::sunVelocityKms().normalized());
    const double tilt = std::asin(heading.z) / units::DEG;
    std::printf("  the Sun heads %.2f deg out of the ecliptic plane at %.1f km/s\n", tilt,
                galaxy::sunVelocityKms().magnitude());
    CHECK(tilt > 60.0 && tilt < 61.2, "the Sun heads %.2f deg out of the ecliptic, expected about 60.6", tilt);
}

// ---------------------------------------------------------------------------
// Black hole light. These use the same math as the shader

void testBlackHoleRadius() {
    const double rs = blackhole::schwarzschildRadiusAU(galaxy::SGR_A_MASS);
    const double sunKm = blackhole::schwarzschildRadiusAU(1.0) * units::KM_PER_AU;
    std::printf("  Sgr A*: Rs = %.5f AU (%.0f km); the Sun: %.4f km\n", rs, rs * units::KM_PER_AU, sunKm);
    CHECK(relative(rs, 0.084887) < 1e-4, "Sgr A*'s horizon is %.6f AU, expected 0.084887", rs);
    CHECK(relative(sunKm, 2.9533) < 1e-4, "the Sun's Schwarzschild radius is %.4f km, expected 2.9533", sunKm);
}

void testBlackHoleShadow() {
    // Rays aimed closer than 2.6 Rs fall in
    auto aim = [](double b) {
        blackhole::Ray ray = blackhole::startRay(Vector3D(-1000.0, b, 0.0), Vector3D(1.0, 0.0, 0.0));
        return blackhole::traceRay(ray, 1100.0);
    };
    CHECK(aim(2.5) == blackhole::RayEnd::Horizon, "a ray aimed 2.5 Rs off center didn't fall in");
    CHECK(aim(2.7) == blackhole::RayEnd::Escaped, "a ray aimed 2.7 Rs off center didn't escape");
}

void testBlackHolePhotonSphere() {
    // Light at 1.5 Rs goes around in a circle (checked for a quarter turn)
    blackhole::Ray ray = blackhole::startRay(Vector3D(blackhole::PHOTON_SPHERE, 0.0, 0.0), Vector3D(0.0, 1.0, 0.0));
    double worst = 0.0;
    for (int i = 0; i < 1000; ++i) {
        blackhole::stepRay(ray, blackhole::stepLength(ray.position.magnitude()));
        worst = std::max(worst, relative(ray.position.magnitude(), blackhole::PHOTON_SPHERE));
        if (std::atan2(ray.position.y, ray.position.x) >= 0.5 * units::PI) break;
    }
    std::printf("  over a quarter turn the radius strays by %.2e\n", worst);
    CHECK(worst < 0.02, "light on the photon sphere strayed %.2e from it in a quarter turn", worst);
}

void testBlackHoleBending() {
    // Light passing far away bends by the amount Einstein predicted
    const double b = 1000.0;
    blackhole::Ray ray = blackhole::startRay(Vector3D(-1e5, b, 0.0), Vector3D(1.0, 0.0, 0.0));
    const blackhole::RayEnd end = blackhole::traceRay(ray, 1.1e5);
    const double bend = std::atan2(-ray.velocity.y, ray.velocity.x);
    std::printf("  bent by %.4e rad at b = %.0f Rs, expected %.4e\n", bend, b, 2.0 / b);
    CHECK(end == blackhole::RayEnd::Escaped, "a ray passing 1000 Rs away didn't escape");
    CHECK(relative(bend, 2.0 / b) < 0.02, "a ray passing 1000 Rs away bent by %.4e, expected %.4e", bend, 2.0 / b);
}

void testBlackHoleDoppler() {
    // Gas at the disk's inner edge moves at half the speed of light
    const double v = blackhole::orbitSpeed(blackhole::ISCO);
    const double toward = blackhole::frequencyShift(blackhole::ISCO, v, 1.0, 1e12);
    const double away = blackhole::frequencyShift(blackhole::ISCO, v, -1.0, 1e12);
    std::printf("  at 3 Rs: speed %.3f c, shift %.4f toward the eye and %.4f away\n", v, toward, away);
    CHECK(relative(v, 0.5) < 1e-12, "the orbit speed at 3 Rs is %.6f c, expected 0.5", v);
    CHECK(relative(toward, std::sqrt(2.0)) < 1e-9, "gas moving toward the eye shifts by %.6f, expected 1.414214",
          toward);
    CHECK(relative(away, std::sqrt(2.0) / 3.0) < 1e-9, "gas moving away shifts by %.6f, expected 0.471405", away);
}

// ---------------------------------------------------------------------------
// Black holes as bodies

void testBlackHoleHorizon() {
    // Horizon size: Rs with no spin, 0.5316 Rs at the fastest spin
    const double m = 10.0;
    const double rs = blackhole::schwarzschildRadiusAU(m);
    const double fastest = blackhole::horizonRadiusAU(m, blackhole::MAX_SPIN) / rs;
    std::printf("  horizon at spin 0.998: %.4f Rs\n", fastest);
    CHECK(relative(blackhole::horizonRadiusAU(m, 0.0), rs) < 1e-15, "a hole that doesn't spin isn't Rs across");
    CHECK(std::abs(fastest - 0.531607) < 1e-6, "the horizon at spin 0.998 is %.6f Rs, expected 0.531607", fastest);
    CHECK(blackhole::horizonRadiusAU(m, 1.5) == blackhole::horizonRadiusAU(m, blackhole::MAX_SPIN),
          "a spin above 0.998 wasn't held at 0.998");

    const Body hole = Body::blackHole("", "", m, 0.5, Vector3D(0.0, 0.0, 2.0), Vector3D(), Vector3D());
    CHECK(hole.isBlackHole() && hole.spin == 0.5, "Body::blackHole didn't make a black hole with spin 0.5");
    CHECK(hole.radius == blackhole::horizonRadiusAU(m, 0.5), "a black hole's radius isn't its horizon");
    CHECK(relative(hole.spinAxis.magnitude(), 1.0) < 1e-15, "the spin axis wasn't normalized");
    CHECK(hole.description() == "Stellar-mass black hole", "10 Msun is described as '%s'", hole.description().c_str());
    CHECK(Body::describeBlackHole(4.3e6) == "Supermassive black hole", "Sgr A*'s mass isn't supermassive");
}

void testBlackHoleSwallow() {
    // A black hole swallows a heavier star and keeps its name and spin
    Simulation sim;
    sim.load(Preset::Empty);
    sim.addBody(Body::blackHole("Hole", "", 5.0, 0.5, Vector3D(0.0, 0.0, 1.0), Vector3D(-0.1, 0.0), Vector3D()));
    sim.addBody(20.0, Vector3D(0.1, 0.0), Vector3D());
    while (sim.time() < 5.0 && sim.getBodyCount() == 2) sim.step();
    CHECK(sim.getBodyCount() == 1, "%zu bodies left, the star should have fallen in", sim.getBodyCount());
    if (sim.getBodyCount() != 1) return;
    const Body& left = sim.getBodies()[0];
    CHECK(left.isBlackHole() && left.name == "Hole", "the star survived instead of the hole");
    CHECK(relative(left.mass, 25.0) < 1e-12 && left.spin == 0.5, "mass %.6f and spin %.3f, expected 25 and 0.5",
          left.mass, left.spin);
    CHECK(relative(left.radius, blackhole::horizonRadiusAU(25.0, 0.5)) < 1e-12,
          "the horizon didn't grow with the mass");
    CHECK(left.velocity.magnitude() < 1e-9, "the merged hole moves at %.2e AU/day, momentum was lost",
          left.velocity.magnitude());
}

// ---------------------------------------------------------------------------
// General relativity: orbits that turn

// An arrow pointing at the orbit's closest point, as long as its eccentricity
Vector3D eccentricityVector(const Body& host, const Body& b) {
    const double mu = units::G * (host.mass + b.mass);
    const Vector3D r = b.position - host.position, v = b.velocity - host.velocity;
    return (r * (v.lengthSquared() - mu / r.magnitude()) - v * r.dot(v)) / mu;
}

// An orbit at its closest or farthest point, where the turn is measured
struct Apsis {
    double time = 0.0;      // days
    Vector3D e;             // the eccentricity vector
    double distance = 0.0;  // AU
    double speed = 0.0;     // AU per day
};

// Runs until the star reaches its next closest (or farthest) point. False if it takes too long
bool nextApsis(Simulation& sim, const char* host, const char* star, bool pericenter, double maxDays, Apsis& out) {
    auto measure = [&](double& rv, Apsis& a) {
        const Body& h = find(sim, host);
        const Body& s = find(sim, star);
        const Vector3D r = s.position - h.position, v = s.velocity - h.velocity;
        rv = r.dot(v);
        a = Apsis{sim.time(), eccentricityVector(h, s), r.magnitude(), v.magnitude()};
    };
    double rv0 = 0.0, rv1 = 0.0;
    Apsis before, after;
    measure(rv0, before);
    const double end = sim.time() + maxDays;
    while (sim.time() < end) {
        sim.step();
        measure(rv1, after);
        if (pericenter ? (rv0 < 0.0 && rv1 >= 0.0) : (rv0 > 0.0 && rv1 <= 0.0)) {
            const double f = rv0 / (rv0 - rv1);
            out.time = before.time + (after.time - before.time) * f;
            out.e = before.e + (after.e - before.e) * f;
            out.distance = before.distance + (after.distance - before.distance) * f;
            out.speed = before.speed + (after.speed - before.speed) * f;
            return true;
        }
        rv0 = rv1;
        before = after;
    }
    return false;
}

// How far the orbit turned between two measurements, in radians
double turned(const Vector3D& e1, const Vector3D& e2, const Vector3D& h) {
    return std::atan2(e1.cross(e2).dot(h.normalized()), e1.dot(e2));
}

// How much relativity turns an orbit each lap
double relativisticTurn(double totalMass, double a, double e) {
    return 6.0 * units::PI * units::G * totalMass / (units::SPEED_OF_LIGHT * units::SPEED_OF_LIGHT * a * (1.0 - e * e));
}

constexpr double ARCSEC = units::DEG / 3600.0;
constexpr double ARCMIN = units::DEG / 60.0;

// How much Mercury's orbit turns in a century, with just the Sun and Mercury.
// Also returns relativity's prediction in predicted
double mercuryTurn(double relativity, double& predicted) {
    Simulation sim;
    sim.setIntegrator(std::make_unique<IAS15Integrator>());
    sim.load(Preset::Empty);
    sim.setRelativity(relativity);
    const double mSun = 1.0, mMercury = 1.0 / 6023597.4;
    const double a = 0.38709927, e = 0.20563593;
    const double rp = a * (1.0 - e), vp = std::sqrt(units::G * (mSun + mMercury) * (1.0 + e) / rp);
    sim.addBody(Body("Sun", "", mSun, units::SOLAR_RADIUS_AU, Rgb{}, Vector3D(), Vector3D()));
    sim.addBody(Body("Mercury", "", mMercury, 2439.7 / units::KM_PER_AU, Rgb{}, Vector3D(rp, 0.0), Vector3D(0.0, vp)));

    const Vector3D e0 = eccentricityVector(find(sim, "Sun"), find(sim, "Mercury"));  // starting at its closest
    Apsis last{0.0, e0, rp, vp};
    int laps = 0;
    Apsis next;
    while (last.time < 100.0 * units::DAYS_PER_YEAR && nextApsis(sim, "Sun", "Mercury", true, 100.0, next)) {
        last = next;
        ++laps;
    }
    const Vector3D h = Vector3D(rp, 0.0).cross(Vector3D(0.0, vp));
    const double century = 100.0 * units::DAYS_PER_YEAR / last.time;
    predicted = relativity * laps * relativisticTurn(mSun + mMercury, a, e) * century / ARCSEC;
    return turned(e0, last.e, h) * century / ARCSEC;
}

void testMercuryPrecession() {
    double predicted = 0.0, none = 0.0;
    const double withRelativity = mercuryTurn(1.0, predicted);
    const double newton = mercuryTurn(0.0, none);
    std::printf("  Mercury's perihelion turns %.3f\" a century, relativity predicts %.3f\" (observed: 43\")\n",
                withRelativity, predicted);
    std::printf("  with Newton's gravity alone it turns %.2e\"\n", newton);
    CHECK(std::abs(withRelativity - predicted) < 0.01 * predicted, "the perihelion turned %.3f\", expected %.3f\"",
          withRelativity, predicted);
    CHECK(std::abs(predicted - 42.98) < 0.05, "relativity's prediction is %.3f\", expected 42.98\"", predicted);
    CHECK(std::abs(newton) < 0.01 * predicted, "without relativity the perihelion still turned %.3f\"", newton);
}

// Runs S2's orbit: its 2018 close pass, then how much the orbit turns over one lap
struct S2Run {
    bool ok = false;
    double epoch = 0.0;      // Julian day at time 0
    Apsis pericenter;
    double turn = 0.0;       // radians
    double predicted = 0.0;  // radians
    double energyDrift = 0.0;
};

S2Run runS2(std::unique_ptr<Integrator> integrator, double relativity) {
    Simulation sim;
    sim.setIntegrator(std::move(integrator));
    sim.load(Preset::GalacticCenter);
    sim.setRelativity(relativity);
    const double e0 = sim.totalEnergy();
    S2Run run;
    run.epoch = sim.epoch();
    Apsis first, second;
    run.ok = nextApsis(sim, "Sgr A*", "S2", true, 3.0 * units::DAYS_PER_YEAR, run.pericenter) &&
             nextApsis(sim, "Sgr A*", "S2", false, 20.0 * units::DAYS_PER_YEAR, first) &&
             nextApsis(sim, "Sgr A*", "S2", false, 20.0 * units::DAYS_PER_YEAR, second);
    if (!run.ok) return run;
    run.energyDrift = relative(sim.totalEnergy(), e0);
    const Body& hole = find(sim, "Sgr A*");
    const Body& s2 = find(sim, "S2");
    const Vector3D h = (s2.position - hole.position).cross(s2.velocity - hole.velocity);
    run.turn = turned(first.e, second.e, h);
    const double e = first.e.magnitude(), a = first.distance / (1.0 + e);
    run.predicted = relativity * relativisticTurn(hole.mass + s2.mass, a, e);
    return run;
}

void testS2Precession() {
    // S2's 2018 close pass and its orbit's turn, against GRAVITY's measurements
    const S2Run ias15 = runS2(std::make_unique<IAS15Integrator>(), 1.0);
    CHECK(ias15.ok, "S2 didn't pass pericenter and two apocenters");
    if (!ias15.ok) return;
    const double year = 2000.0 + (ias15.epoch + ias15.pericenter.time - units::J2000_JD) / units::DAYS_PER_YEAR;
    const double kms = ias15.pericenter.speed / units::KMS_TO_AU_PER_DAY;
    std::printf("  S2 at pericenter: %.4f, %.1f AU from Sgr A* at %.0f km/s\n", year, ias15.pericenter.distance, kms);
    CHECK(std::abs(year - 2018.3789) < 0.01, "S2 passed pericenter in %.4f, expected 2018.3789", year);
    const double peri = 0.12495 * 8277.0 * (1.0 - 0.88441);
    CHECK(relative(ias15.pericenter.distance, peri) < 0.02, "S2 passed %.1f AU from Sgr A*, expected %.1f",
          ias15.pericenter.distance, peri);
    CHECK(kms > 7500.0 && kms < 7900.0, "S2 passed pericenter at %.0f km/s, expected about 7,700", kms);

    std::printf("  IAS15: S2's orbit turns %.3f' a lap, relativity predicts %.3f' (GRAVITY: 12.1')\n",
                ias15.turn / ARCMIN, ias15.predicted / ARCMIN);
    CHECK(std::abs(ias15.turn - ias15.predicted) < 0.01 * ias15.predicted, "S2's orbit turned %.3f', expected %.3f'",
          ias15.turn / ARCMIN, ias15.predicted / ARCMIN);
    std::printf("  energy drift over 25 years with relativity: %.2e\n", ias15.energyDrift);
    CHECK(ias15.energyDrift < 1e-10, "energy drifted by %.2e over 25 years with relativity", ias15.energyDrift);

    const S2Run newton = runS2(std::make_unique<IAS15Integrator>(), 0.0);
    std::printf("  IAS15 with Newton's gravity alone: %.4f'\n", newton.turn / ARCMIN);
    CHECK(newton.ok && std::abs(newton.turn) < 0.01 * ias15.predicted, "without relativity S2's orbit turned %.3f'",
          newton.turn / ARCMIN);

    // Verlet for comparison: its own errors turn the orbit too. Printed, not checked
    const S2Run verlet = runS2(std::make_unique<VerletIntegrator>(), 1.0);
    const S2Run verletNewton = runS2(std::make_unique<VerletIntegrator>(), 0.0);
    if (verlet.ok && verletNewton.ok)
        std::printf("  Verlet: %.3f' with relativity, %.3f' without: its own error is %.0f%% of relativity's turn\n",
                    verlet.turn / ARCMIN, verletNewton.turn / ARCMIN,
                    100.0 * std::abs(verletNewton.turn) / ias15.predicted);
}

// ---------------------------------------------------------------------------
// Which way the bodies point, for drawing their surfaces

void testBodySpin() {
    const spin::Model* earth = spin::find("Earth");
    const spin::Model* moon = spin::find("Moon");
    const spin::Model* saturn = spin::find("Saturn");
    CHECK(earth && moon && saturn, "a spin model is missing");
    if (!earth || !moon || !saturn) return;
    CHECK(spin::find("Alpha Centauri A") == nullptr, "a body without a model found one");

    // Greenwich should point the right way on 1 January 2000
    const spin::Axes q = spin::equatorialAxes(*earth, units::J2000_JD);
    double greenwich = std::atan2(q.x.y, q.x.x) / units::DEG;
    if (greenwich < 0.0) greenwich += 360.0;
    std::printf("  Greenwich at right ascension %.4f deg at J2000\n", greenwich);
    CHECK(std::abs(greenwich - 280.147) < 1e-6, "Greenwich points to right ascension %.6f, expected 280.147",
          greenwich);

    // Axis tilts: Earth 23.44 degrees, Saturn 28.05, the Moon about 1.5
    auto tilt = [](const spin::Model& m) {
        return std::acos(spin::eclipticAxes(m, units::J2000_JD).z.z) / units::DEG;
    };
    std::printf("  tilts: Earth %.3f, Saturn %.3f, Moon %.3f deg\n", tilt(*earth), tilt(*saturn), tilt(*moon));
    CHECK(std::abs(tilt(*earth) - 23.4393) < 1e-3, "Earth's axis tilts %.4f deg, expected 23.4393", tilt(*earth));
    CHECK(std::abs(tilt(*saturn) - 28.05) < 0.1, "Saturn's axis tilts %.3f deg, expected about 28.05", tilt(*saturn));
    CHECK(tilt(*moon) > 1.4 && tilt(*moon) < 1.7, "the Moon's axis tilts %.3f deg, expected about 1.5", tilt(*moon));

    // The axes stay accurate after years of spinning
    for (const char* name : {"Earth", "Moon", "Mars", "Jupiter", "Neptune"}) {
        const spin::Axes a = spin::eclipticAxes(*spin::find(name), units::J2000_JD + 9000.5);
        double worst = 0.0;
        for (const Vector3D& v : {a.x, a.y, a.z}) worst = std::max(worst, std::abs(v.magnitude() - 1.0));
        worst = std::max({worst, std::abs(a.x.dot(a.y)), std::abs(a.y.dot(a.z)), std::abs(a.z.dot(a.x))});
        CHECK(worst < 1e-12, "%s's axes are off a rotation by %.2e", name, worst);
    }
}

struct Test {
    const char* name;
    void (*run)();
};

const Test TESTS[] = {
    {"calendar", testCalendar},
    {"horizons_parse", testHorizonsParse},
    {"horizons_reply", testHorizonsReply},
    {"j2000_start", testJ2000Start},
    {"earth_year", testEarthYear},
    {"conservation", testConservation},
    {"close_pass", testClosePass},
    {"head_on_merge", testHeadOnMerge},
    {"barnes_hut", testBarnesHut},
    {"dominant_body", testDominantBody},
    {"trail_smoothness", testTrailSmoothness},
    {"clock", testClock},
    {"ias15_earth_year", testIas15EarthYear},
    {"ias15_conservation", testIas15Conservation},
    {"ias15_close_pass", testIas15ClosePass},
    {"ias15_head_on_merge", testIas15HeadOnMerge},
    {"ias15_guard", testIas15Guard},
    {"crowded_close_pass", testCrowdedClosePass},
    {"inclination", testInclination},
    {"galactic_frame", testGalacticFrame},
    {"black_hole_radius", testBlackHoleRadius},
    {"black_hole_shadow", testBlackHoleShadow},
    {"black_hole_photon_sphere", testBlackHolePhotonSphere},
    {"black_hole_bending", testBlackHoleBending},
    {"black_hole_doppler", testBlackHoleDoppler},
    {"body_spin", testBodySpin},
    {"black_hole_horizon", testBlackHoleHorizon},
    {"black_hole_swallow", testBlackHoleSwallow},
    {"mercury_precession", testMercuryPrecession},
    {"s2_precession", testS2Precession},
};

}

int main(int argc, char** argv) {
    const char* only = argc > 1 ? argv[1] : nullptr;
    bool ran = false;
    for (const Test& t : TESTS) {
        if (only && std::strcmp(only, t.name) != 0) continue;
        std::printf("%s\n", t.name);
        t.run();
        ran = true;
    }
    if (!ran) {
        std::printf("no test named %s\n", only);
        return 1;
    }
    if (failures) std::printf("%d check(s) failed\n", failures);
    else std::printf("all checks passed\n");
    return failures ? 1 : 0;
}
