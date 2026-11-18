#include "emma/Presets.hpp"
#include "emma/Galaxy.hpp"
#include <cmath>
#include <cstddef>

namespace emma {

namespace {

struct OrbitState {
    Vector3D position;
    Vector3D velocity;
};

// Position and velocity on an ellipse around the central body, from its orbit's shape and
// angles (in degrees). mu = G (M + m)
OrbitState keplerState(double mu, double a, double e, double longPeriDeg, double meanLongDeg,
                       double inclDeg = 0.0, double nodeDeg = 0.0) {
    const double varpi = longPeriDeg * units::DEG;
    const double node = nodeDeg * units::DEG;
    const double incl = inclDeg * units::DEG;
    const double argPeri = varpi - node;
    const double M = std::remainder((meanLongDeg - longPeriDeg) * units::DEG, 2.0 * units::PI);

    // Solve Kepler's equation for where the body is on its ellipse
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

    const double cw = std::cos(argPeri), sw = std::sin(argPeri);
    const double ci = std::cos(incl), si = std::sin(incl);
    const double cn = std::cos(node), sn = std::sin(node);
    auto orient = [&](double px, double py) {
        const double x1 = px * cw - py * sw;
        const double y1 = px * sw + py * cw;
        const double y2 = y1 * ci;
        return Vector3D(x1 * cn - y2 * sn, x1 * sn + y2 * cn, y1 * si);
    };

    const Vector3D position = orient(r * std::cos(nu), r * std::sin(nu));
    // Velocity in the orbit's plane, then turned into place
    const Vector3D velocity = orient(-h * std::sin(nu), h * (e + std::cos(nu)));
    return OrbitState{position, velocity};
}

struct SolarBodyInfo {
    const char* name;
    const char* kind;
    double mass;      // Msun, including any moons (IAU 2009)
    double radiusKm;  // mean radius
    Rgb color;
};

// The solar system's bodies. The Sun's color comes from its temperature
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
    std::size_t body;                             // index into SOLAR_BODIES
    double a, e, longPeri, meanLong, incl, node;  // AU, -, degrees, degrees, degrees, degrees
};

// Average orbits for the year 2000 (Standish, JPL). Earth and the Moon are placed separately
const Elements PLANET_ELEMENTS[] = {
    {1, 0.38709927, 0.20563593, 77.45779628, 252.25032350, 7.00497902, 48.33076593},
    {2, 0.72333566, 0.00677672, 131.60246718, 181.97909950, 3.39467605, 76.67984255},
    {5, 1.52371034, 0.09339410, -23.94362959, -4.55343205, 1.84969142, 49.55953891},
    {6, 5.20288700, 0.04838624, 14.72847983, 34.39644051, 1.30439695, 100.47390909},
    {7, 9.53667594, 0.05386179, 92.59887831, 49.95424423, 2.48599187, 113.66242448},
    {8, 19.18916464, 0.04725744, 170.95427630, 313.23810451, 0.77263783, 74.01692503},
    {9, 30.06992276, 0.00859048, 44.96476227, -55.12002969, 1.77004347, 131.78422574},
};

Scenario solarSystem(const SolarSnapshot* snapshot) {
    Scenario s;
    s.step = 0.1;          // days
    s.hasCalendar = true;
    s.viewRadius = 1.8;    // Mars's orbit fills the view
    s.timeScale = 30.0;    // a year in about 12 seconds

    std::array<Vector3D, SOLAR_BODY_COUNT> position, velocity;
    if (snapshot) {
        // Today's measured positions, from JPL Horizons
        position = snapshot->position;
        velocity = snapshot->velocity;
        s.epoch = snapshot->julianDay;
    } else {
        // No download: use the average orbits for 2000
        s.epoch = units::J2000_JD;
        for (const Elements& el : PLANET_ELEMENTS) {
            const double mu = units::G * (1.0 + SOLAR_BODIES[el.body].mass);
            const OrbitState o = keplerState(mu, el.a, el.e, el.longPeri, el.meanLong, el.incl, el.node);
            position[el.body] = o.position;
            velocity[el.body] = o.velocity;
        }

        // Earth and the Moon: put their shared center on its orbit, then each around it
        const double earthMass = SOLAR_BODIES[3].mass, moonMass = SOLAR_BODIES[4].mass;
        const double pairMass = earthMass + moonMass;
        const OrbitState bary = keplerState(units::G * (1.0 + pairMass), 1.00000261, 0.01671123,
                                            102.93768193, 100.46457166, -0.00001531, 0.0);
        const OrbitState moon = keplerState(units::G * pairMass, 384400.0 / units::KM_PER_AU, 0.0549,
                                            218.3164477 - 134.9633964, 218.3164477, 5.145396, 125.0445479);
        position[3] = bary.position - moon.position * (moonMass / pairMass);
        velocity[3] = bary.velocity - moon.velocity * (moonMass / pairMass);
        position[4] = bary.position + moon.position * (earthMass / pairMass);
        velocity[4] = bary.velocity + moon.velocity * (earthMass / pairMass);
    }

    for (std::size_t i = 0; i < SOLAR_BODY_COUNT; ++i) {
        const SolarBodyInfo& info = SOLAR_BODIES[i];
        const Rgb color = i == 0 ? Body::blackbody(units::SUN_TEMPERATURE) : info.color;
        s.bodies.emplace_back(info.name, info.kind, info.mass, info.radiusKm / units::KM_PER_AU, color,
                              position[i], velocity[i]);
    }
    return s;
}

Scenario alphaCentauri() {
    Scenario s;
    s.step = 1.0;
    s.hasCalendar = false;
    s.viewRadius = 22.0;
    s.timeScale = 2.0 * units::DAYS_PER_YEAR;  // two years a second: one orbit in 40 s

    // Start with the two stars at their closest
    const double mA = 1.0788, mB = 0.9092, total = mA + mB;
    const double period = 79.91 * units::DAYS_PER_YEAR;
    const double a = std::cbrt(units::G * total * period * period / (4.0 * units::PI * units::PI));
    const OrbitState rel = keplerState(units::G * total, a, 0.5179, 0.0, 0.0);

    s.bodies.emplace_back("Alpha Centauri A", "G2V star", mA, 1.2234 * units::SOLAR_RADIUS_AU,
                          Body::blackbody(5790.0), rel.position * (-mB / total), rel.velocity * (-mB / total));
    s.bodies.emplace_back("Alpha Centauri B", "K1V star", mB, 0.8632 * units::SOLAR_RADIUS_AU,
                          Body::blackbody(5260.0), rel.position * (mA / total), rel.velocity * (mA / total));
    return s;
}

// Sgr A* and the S-stars (GRAVITY 2022, A&A 657, L12). The mass and distance were
// measured together, so they must be used together
constexpr double GRAVITY_MASS = 4.297e6;  // Msun
constexpr double GRAVITY_R0_PC = 8277.0;  // pc

// An orbit as measured on the sky. Angles are in degrees
struct SkyOrbit {
    const char* name;
    double a;       // arcseconds
    double e;
    double incl, node, argPeri;
    double tPeri;   // year of pericenter
};

// The four stars' orbits
const SkyOrbit S_STARS[] = {
    {"S2", 0.12495, 0.88441, 134.70, 228.19, 66.25, 2018.3789},
    {"S29", 0.3975, 0.9693, 144.37, 7.00, 205.79, 2021.4104},
    {"S38", 0.14254, 0.8145, 166.65, 109.45, 27.17, 2022.7044},
    {"S55", 0.10440, 0.7267, 158.52, 314.94, 322.78, 2021.6940},
};

constexpr double START_YEAR = 2017.0;  // just before S2's 2018 close pass

double julianDayOfYear(double year) {
    return units::J2000_JD + (year - 2000.0) * units::DAYS_PER_YEAR;
}

Scenario sgrAStars() {
    Scenario s;
    s.step = 1.0;             // days
    s.hasCalendar = true;
    s.epoch = julianDayOfYear(START_YEAR);
    s.viewRadius = 1500.0;    // AU: the inner orbits fill the view
    s.timeScale = 0.5 * units::DAYS_PER_YEAR;  // half a year a second: one lap of S2 in 32 s

    // The orbits are measured as seen from Earth, so the camera starts out looking from Earth too
    const galaxy::SkyFrame sky = galaxy::skyFrame(galaxy::SGR_A_RA, galaxy::SGR_A_DEC);
    s.lookDirection = sky.away;

    // No spin, to match how the ray tracer draws it
    s.bodies.push_back(Body::blackHole("Sgr A*", "Supermassive black hole", GRAVITY_MASS, 0.0,
                                       galaxy::toEcliptic(galaxy::sgrASpinAxis()), Vector3D(), Vector3D()));

    for (const SkyOrbit& star : S_STARS) {
        // Only S2's mass is known (Habibi et al. 2017). The others get no mass: they're far too
        // light to change the orbits anyway
        const bool s2 = &star == &S_STARS[0];
        const double mass = s2 ? 13.6 : 0.0;
        const double mu = units::G * (GRAVITY_MASS + mass);
        const double a = star.a * GRAVITY_R0_PC;  // arcseconds to AU
        const double period = 2.0 * units::PI * std::sqrt(a * a * a / mu);  // days
        const double meanAnomaly = 360.0 * (s.epoch - julianDayOfYear(star.tPeri)) / period;  // degrees
        // Place the star on its orbit, then turn the result from sky directions into EMMA's frame
        const double longPeri = star.node + star.argPeri;
        const OrbitState o = keplerState(mu, a, star.e, longPeri, longPeri + meanAnomaly, star.incl, star.node);
        const Vector3D position = sky.north * o.position.x + sky.east * o.position.y + sky.away * o.position.z;
        const Vector3D velocity = sky.north * o.velocity.x + sky.east * o.velocity.y + sky.away * o.velocity.z;
        if (s2)
            s.bodies.emplace_back(star.name, "B0-B3 V star", mass, 5.53 * units::SOLAR_RADIUS_AU,
                                  Body::blackbody(28513.0), position, velocity);
        else
            s.bodies.emplace_back(star.name, "S-star, mass not measured", 0.0, 0.0, Body::blackbody(20000.0),
                                  position, velocity);
    }
    return s;
}

Scenario emptySpace() {
    Scenario s;
    s.step = 0.1;
    s.hasCalendar = false;
    s.viewRadius = 2.0;
    s.timeScale = 30.0;
    return s;
}

} // namespace

Scenario makeScenario(Preset preset, const SolarSnapshot* snapshot) {
    switch (preset) {
    case Preset::SolarSystem: return solarSystem(snapshot);
    case Preset::AlphaCentauri: return alphaCentauri();
    case Preset::GalacticCenter: return sgrAStars();
    case Preset::Empty: return emptySpace();
    }
    return emptySpace();
}

} // namespace emma
