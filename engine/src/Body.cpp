#include "emma/Body.hpp"
#include "emma/Constants.hpp"
#include <algorithm>
#include <cmath>
#include <utility>

namespace emma {

namespace {

// Class boundaries in solar masses
constexpr double TERRAN_LIMIT = 2.04 * units::EARTH_MASS;
constexpr double NEPTUNIAN_LIMIT = 0.414 * units::JUPITER_MASS;
constexpr double DEUTERIUM_LIMIT = 13.0 * units::JUPITER_MASS;   // planet / brown dwarf
constexpr double HYDROGEN_LIMIT = 0.08;                          // brown dwarf / star

// Main-sequence relations in solar units
double mainSequenceRadius(double m) {
    return m < 1.0 ? std::pow(m, 0.8) : std::pow(m, 0.57);
}

double mainSequenceLuminosity(double m) {
    if (m < 0.43) return 0.23 * std::pow(m, 2.3);
    if (m < 2.0) return std::pow(m, 4.0);
    if (m < 55.0) return 1.4 * std::pow(m, 3.5);
    return 32000.0 * m;
}

} // namespace

Body::Body(double m, const Vector2D& pos, const Vector2D& vel)
    : mass(m), radius(radiusForMass(m)), position(pos), velocity(vel),
      previousPosition(pos), color(colorForMass(m)) {}

Body::Body(std::string n, std::string k, double m, double r, Rgb c,
           const Vector2D& pos, const Vector2D& vel)
    : mass(m), radius(r), position(pos), velocity(vel), previousPosition(pos),
      color(c), name(std::move(n)), kind(std::move(k)) {}

BodyClass Body::classify(double m) {
    if (m < TERRAN_LIMIT) return BodyClass::RockyBody;
    if (m < NEPTUNIAN_LIMIT) return BodyClass::NeptuneLike;
    if (m < DEUTERIUM_LIMIT) return BodyClass::GasGiant;
    if (m < HYDROGEN_LIMIT) return BodyClass::BrownDwarf;
    return BodyClass::Star;
}

std::string Body::describe(double m) {
    switch (classify(m)) {
    case BodyClass::RockyBody: return "Rocky body";
    case BodyClass::NeptuneLike: return "Neptune-like planet";
    case BodyClass::GasGiant: return "Gas giant";
    case BodyClass::BrownDwarf: return "Brown dwarf";
    case BodyClass::Star: break;
    }
    const double t = stellarTemperature(m);
    const char* type = t >= 30000.0 ? "O"
                     : t >= 10000.0 ? "B"
                     : t >= 7500.0  ? "A"
                     : t >= 6000.0  ? "F"
                     : t >= 5200.0  ? "G"
                     : t >= 3700.0  ? "K"
                                    : "M";
    return std::string(type) + "-type star";
}

double Body::radiusForMass(double m) {
    if (m >= HYDROGEN_LIMIT) return mainSequenceRadius(m) * units::SOLAR_RADIUS_AU;

    // Chen & Kipping (2017) piecewise power law, worked in Earth units:
    // rocky R ~ M^0.279, Neptunian R ~ M^0.589, Jovian and brown dwarfs R ~ M^-0.044
    const double me = m / units::EARTH_MASS;
    const double terranEdge = TERRAN_LIMIT / units::EARTH_MASS;
    const double neptunianEdge = NEPTUNIAN_LIMIT / units::EARTH_MASS;
    const double rAtTerranEdge = 1.008 * std::pow(terranEdge, 0.279);
    const double rAtNeptunianEdge = rAtTerranEdge * std::pow(neptunianEdge / terranEdge, 0.589);

    double re;
    if (me < terranEdge) re = 1.008 * std::pow(me, 0.279);
    else if (me < neptunianEdge) re = rAtTerranEdge * std::pow(me / terranEdge, 0.589);
    else re = rAtNeptunianEdge * std::pow(me / neptunianEdge, -0.044);
    return re * units::EARTH_RADIUS_AU;
}

double Body::stellarTemperature(double m) {
    // Stefan-Boltzmann: L = 4 pi R^2 sigma T^4, so T = T_sun (L / R^2)^(1/4) in solar units
    const double r = mainSequenceRadius(m);
    return units::SUN_TEMPERATURE * std::pow(mainSequenceLuminosity(m) / (r * r), 0.25);
}

Rgb Body::colorForMass(double m) {
    switch (classify(m)) {
    case BodyClass::RockyBody: return Rgb{160, 150, 140};
    case BodyClass::NeptuneLike: return Rgb{125, 175, 225};
    case BodyClass::GasGiant: return Rgb{216, 184, 140};
    case BodyClass::BrownDwarf: {
        // Brown dwarfs run from about 800 K to 2300 K; heavier ones are hotter
        const double t = std::log(m / DEUTERIUM_LIMIT) / std::log(HYDROGEN_LIMIT / DEUTERIUM_LIMIT);
        return blackbody(800.0 + 1500.0 * t);
    }
    case BodyClass::Star: return blackbody(stellarTemperature(m));
    }
    return Rgb{};
}

Rgb Body::blackbody(double kelvin) {
    // Tanner Helland's fit to the CIE blackbody colors, good from 1000 K to 40000 K
    const double t = std::clamp(kelvin, 1000.0, 40000.0) / 100.0;
    double r, g, b;
    if (t <= 66.0) {
        r = 255.0;
        g = 99.4708025861 * std::log(t) - 161.1195681661;
    } else {
        r = 329.698727446 * std::pow(t - 60.0, -0.1332047592);
        g = 288.1221695283 * std::pow(t - 60.0, -0.0755148492);
    }
    if (t >= 66.0) b = 255.0;
    else if (t <= 19.0) b = 0.0;
    else b = 138.5177312231 * std::log(t - 10.0) - 305.0447927307;

    auto channel = [](double v) { return static_cast<std::uint8_t>(std::clamp(v, 0.0, 255.0)); };
    return Rgb{channel(r), channel(g), channel(b)};
}

} // namespace emma
