// Run them all:  ctest --test-dir build -C Release --output-on-failure
// Run one:       build/Release/physics_tests earth_year

#include "emma/Body.hpp"
#include "emma/Calendar.hpp"
#include "emma/Clock.hpp"
#include "emma/Constants.hpp"
#include "emma/Horizons.hpp"
#include "emma/IAS15Integrator.hpp"
#include "emma/QuadTree.hpp"
#include "emma/Simulation.hpp"
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
    // A trimmed Horizons VECTORS reply, CSV format, AU and days
    const std::string reply =
        "API VERSION: 1.2\n"
        "API SOURCE: NASA/JPL Horizons API\n\n"
        "*******************************************************************************\n"
        "$$SOE\n"
        "2461353.500000000, A.D. 2026-Nov-09 00:00:00.0000,  6.123456789012345E-01,  7.654321098765432E-01, "
        "-3.123456789012345E-05, -1.345678901234567E-02,  1.098765432109876E-02,  2.345678901234567E-07,\n"
        "$$EOE\n";
    Vector2D p, v;
    std::string error;
    const bool ok = horizons::parseStateVector(reply, p, v, error);
    CHECK(ok, "parse failed: %s", error.c_str());
    CHECK(p.x == 6.123456789012345E-01 && p.y == 7.654321098765432E-01, "position %.15g, %.15g", p.x, p.y);
    CHECK(v.x == -1.345678901234567E-02 && v.y == 1.098765432109876E-02, "velocity %.15g, %.15g", v.x, v.y);

    // An error reply has no table: its message should come through
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
    // A good JSON reply: the table sits in "result", with escaped newlines and quotes
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
    Vector2D p, v;
    CHECK(horizons::parseStateVector(output, p, v, error) && p.x == 1.0E-03 && v.y == 4.0E-06,
          "table inside the JSON reply not read: %s", error.c_str());

    // Horizons puts errors it detects in "error", sometimes over several lines
    const std::string failed =
        "{\"signature\":{\"version\":\"1.2\"},\"error\":\" \\n"
        "Insufficient ephemeris data has been loaded to compute the state of 10 (SUN)\\n"
        "relative to 0 (SOLAR SYSTEM BARYCENTER).\\n\"}";
    error.clear();
    CHECK(!horizons::parseReply(failed, 200, output, error), "accepted a reply with an error field");
    CHECK(error.rfind("Insufficient ephemeris data", 0) == 0 && error.find("(SUN) relative") != std::string::npos,
          "error field not read and joined: '%s'", error.c_str());

    // A 400 explains itself in "message"; a 503 may not be JSON at all
    error.clear();
    CHECK(!horizons::parseReply("{\"message\":\"bad TLIST value\"}", 400, output, error) && error == "bad TLIST value",
          "400 message: '%s'", error.c_str());
    error.clear();
    CHECK(!horizons::parseReply("<html>busy</html>", 503, output, error) && error.find("503") != std::string::npos,
          "503 reply: '%s'", error.c_str());

    // JSON, and a START_TIME span instead of TLIST, which Horizons misread in November 2026
    const std::string url = horizons::requestUrl("10", 2461355.5);
    CHECK(url.find("format=json") != std::string::npos, "request isn't JSON: %s", url.c_str());
    CHECK(url.find("START_TIME=%27JD2461355.500000%27") != std::string::npos && url.find("TLIST") == std::string::npos,
          "request should use START_TIME, not TLIST: %s", url.c_str());
}

void testJ2000Start() {
    // On 1 January 2000, two days before perihelion, Earth was 0.9833 AU from the
    // Sun at heliocentric longitude 100.38 deg
    Simulation sim;
    sim.load(Preset::SolarSystem);
    const Vector2D r = find(sim, "Earth").position - find(sim, "Sun").position;
    const double distance = r.magnitude();
    double longitude = std::atan2(r.y, r.x) / units::DEG;
    if (longitude < 0.0) longitude += 360.0;
    std::printf("  Earth at J2000: %.5f AU from the Sun, longitude %.3f deg\n", distance, longitude);
    CHECK(std::abs(distance - 0.98331) < 1e-3, "distance %.5f AU, expected 0.98331", distance);
    CHECK(std::abs(longitude - 100.38) < 0.2, "longitude %.3f deg, expected 100.38", longitude);
}

void earthYear(Simulation& sim) {
    // Time for the Earth-Moon barycenter to go once around the Sun. The sidereal
    // year is 365.2564 days
    sim.load(Preset::SolarSystem);
    auto angle = [&sim]() {
        const Body& earth = find(sim, "Earth");
        const Body& moon = find(sim, "Moon");
        const Vector2D pair = (earth.position * earth.mass + moon.position * moon.mass) / (earth.mass + moon.mass);
        const Vector2D r = pair - find(sim, "Sun").position;
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
    const double e0 = sim.totalEnergy(), l0 = sim.angularMomentum();
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
    // The same ten years with IAS15: the errors should sit near rounding, far below Verlet's
    Simulation sim;
    sim.setIntegrator(std::make_unique<IAS15Integrator>());
    sim.load(Preset::SolarSystem);
    const double e0 = sim.totalEnergy(), l0 = sim.angularMomentum();
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
    // Two Earth-mass bodies miss each other by about 5 Earth radii at 4 km/s
    sim.load(Preset::Empty);
    const double m = units::EARTH_MASS;
    const double v = 2.0 * units::KMS_TO_AU_PER_DAY;  // each, so 4 km/s apart
    const double b = 4.3e-4;                          // AU, offset between their paths
    sim.addBody(m, Vector2D(-0.01, 0.5 * b), Vector2D(v, 0.0));
    sim.addBody(m, Vector2D(0.01, -0.5 * b), Vector2D(-v, 0.0));

    auto pairEnergy = [&sim, m]() {  // energy of the relative orbit, per unit reduced mass
        const auto& bodies = sim.getBodies();
        const Vector2D dv = bodies[1].velocity - bodies[0].velocity;
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
    // Verlet's split steps leave an error near 1e-4; an unsplit step through this pass is off by order 1
    Simulation sim;
    closePass(sim, 1e-2);
}

void testIas15ClosePass() {
    // IAS15 shortens its own pieces through the pass and should stay far more accurate
    Simulation sim;
    sim.setIntegrator(std::make_unique<IAS15Integrator>());
    closePass(sim, 1e-7);
}

void headOnMerge(Simulation& sim) {
    // Head-on collision with zero total momentum: one body should remain, with
    // the combined mass, at rest, at the original center of mass
    sim.load(Preset::Empty);
    const double m1 = units::EARTH_MASS, m2 = 0.5 * units::EARTH_MASS;
    const double v = 10.0 * units::KMS_TO_AU_PER_DAY;
    sim.addBody(m1, Vector2D(-0.002, 0.0), Vector2D(v, 0.0));
    sim.addBody(m2, Vector2D(0.002, 0.0), Vector2D(-2.0 * v, 0.0));
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
    // Above DIRECT_LIMIT bodies Verlet sizes its pieces from how the accelerations
    // changed over the last piece. Right after bodies are added or merge there is no
    // last piece, so it must fall back to the pairwise check, not take the whole step
    Simulation sim;
    sim.load(Preset::Empty);
    for (std::size_t i = 0; i < Simulation::DIRECT_LIMIT; ++i)
        sim.addBody(units::EARTH_MASS, Vector2D(10.0 + static_cast<double>(i), 0.0), Vector2D());
    // Two bodies 0.01 AU apart, closing at 0.01 AU/day: about a day from meeting,
    // so the 0.1-day step has to split
    sim.addBody(units::EARTH_MASS, Vector2D(0.0, 0.0), Vector2D());
    sim.addBody(units::EARTH_MASS, Vector2D(0.01, 0.0), Vector2D(-0.01, 0.0005));
    sim.step();
    CHECK(sim.lastSubstepCount() > 1, "the first step with %zu bodies ran as %d piece(s)",
          sim.getBodyCount(), sim.lastSubstepCount());
}

void testIas15Guard() {
    // Above DIRECT_LIMIT bodies the tree's approximate forces would make IAS15 crawl,
    // so Verlet takes over, and IAS15 can't be picked again until the count drops
    Simulation sim;
    sim.setIntegrator(std::make_unique<IAS15Integrator>());
    sim.load(Preset::Empty);
    for (std::size_t i = 0; i < Simulation::DIRECT_LIMIT; ++i)
        sim.addBody(units::EARTH_MASS, Vector2D(static_cast<double>(i), 0.0), Vector2D());
    CHECK(sim.getIntegrator().needsExactForces(), "IAS15 was dropped at %zu bodies", sim.getBodyCount());

    sim.addBody(units::EARTH_MASS, Vector2D(-1.0, 0.0), Vector2D());
    CHECK(!sim.getIntegrator().needsExactForces(), "IAS15 still runs with %zu bodies", sim.getBodyCount());

    sim.setIntegrator(std::make_unique<IAS15Integrator>());
    CHECK(!sim.getIntegrator().needsExactForces(), "IAS15 could be picked again with %zu bodies",
          sim.getBodyCount());
}

void testBarnesHut() {
    // The tree against an exact sum over 300 random bodies. With an opening angle
    // of 0 the tree never approximates, so it must match to rounding
    std::mt19937 rng(42);
    std::uniform_real_distribution<double> coordinate(-10.0, 10.0), mass(1e-6, 1e-3);
    std::vector<Body> bodies;
    for (int i = 0; i < 300; ++i)
        bodies.emplace_back(mass(rng), Vector2D(coordinate(rng), coordinate(rng)), Vector2D());

    const double eps2 = Simulation::SOFTENING * Simulation::SOFTENING;
    QuadTree tree;
    tree.build(bodies);

    double worstExact = 0.0;
    std::vector<double> errors;
    for (std::size_t i = 0; i < bodies.size(); ++i) {
        Vector2D direct;
        for (std::size_t j = 0; j < bodies.size(); ++j) {
            if (j == i) continue;
            const Vector2D d = bodies[j].position - bodies[i].position;
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
    // The body that rules a spot sets circular orbits, launch speeds and tooltips
    Simulation binary;
    binary.load(Preset::AlphaCentauri);
    const int a = indexOf(binary, "Alpha Centauri A"), b = indexOf(binary, "Alpha Centauri B");
    const auto& stars = binary.getBodies();
    const int nearA = binary.dominantBody(stars[static_cast<std::size_t>(a)].position + Vector2D(0.5, 0.0));
    const int nearB = binary.dominantBody(stars[static_cast<std::size_t>(b)].position + Vector2D(0.5, 0.0));
    CHECK(nearA == a, "0.5 AU from A, body %d rules instead of A (%d)", nearA, a);
    CHECK(nearB == b, "0.5 AU from B, body %d rules instead of B (%d)", nearB, b);

    // In the solar system: Earth close to Earth, the Sun out between the planets
    Simulation solar;
    solar.load(Preset::SolarSystem);
    const int earth = indexOf(solar, "Earth"), sun = indexOf(solar, "Sun");
    const Vector2D nearEarth = solar.getBodies()[static_cast<std::size_t>(earth)].position + Vector2D(0.001, 0.0);
    CHECK(solar.dominantBody(nearEarth) == earth, "150,000 km from Earth, body %d rules instead of Earth",
          solar.dominantBody(nearEarth));
    CHECK(solar.dominantBody(Vector2D(0.0, 2.5)) == sun, "2.5 AU out, body %d rules instead of the Sun",
          solar.dominantBody(Vector2D(0.0, 2.5)));
}

void testTrailSmoothness() {
    // A planet put on a circular 1 AU orbit around Alpha Centauri A
    Simulation sim;
    sim.load(Preset::AlphaCentauri);
    const Vector2D start = find(sim, "Alpha Centauri A").position + Vector2D(1.0, 0.0);
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
        const Vector2D d1 = t[k - 1] - t[k - 2], d2 = t[k] - t[k - 1];
        const double c = d1.dot(d2) / (d1.magnitude() * d2.magnitude());
        worst = std::max(worst, std::acos(std::clamp(c, -1.0, 1.0)) / units::DEG);
    }
    std::printf("  trail: %zu points, sharpest bend %.2f deg\n", t.size(), worst);
    CHECK(t.size() > 100, "only %zu trail points in two orbits", t.size());
    CHECK(worst < 5.0, "the trail bends by %.1f deg at one point, it will look jagged", worst);
}

void testClock() {
    const double frame = 1.0 / 60.0;
    Simulation sim;
    sim.load(Preset::SolarSystem);
    CHECK(sim.hasCalendar(), "the solar system has no calendar");
    if (!sim.hasCalendar()) return;

    // Speed: one real second at 10 days/s runs 10 simulated days
    emma::Clock clock;
    clock.timeScale = 10.0;
    for (int i = 0; i < 60; ++i) clock.run(sim, frame);
    CHECK(std::abs(sim.time() - 10.0) < 0.15, "a second at 10 days/s ran %.3f days", sim.time());

    // Paused: nothing moves, until a step request, which steps and stays paused
    clock.paused = true;
    const double before = sim.time();
    clock.run(sim, frame);
    CHECK(sim.time() == before, "time moved while paused");
    clock.requestStep();
    clock.run(sim, frame);
    CHECK(sim.time() > before && clock.paused, "a step request didn't step and stay paused");

    // Real time: the simulation's clock lands on "now", even a month behind
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

    // Real time needs dates: Alpha Centauri turns it off
    sim.load(Preset::AlphaCentauri);
    clock.run(sim, frame, now);
    CHECK(!clock.live, "real time stayed on without a calendar");
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
