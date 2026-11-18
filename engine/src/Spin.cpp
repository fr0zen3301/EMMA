#include "emma/Spin.hpp"
#include "emma/Constants.hpp"
#include "emma/Galaxy.hpp"
#include <cmath>

namespace emma::spin {

namespace {

// Values from NASA NAIF's pck00011.tpc. Flattening is how squashed each body is at its poles
const Model MODELS[] = {
    // name       pole RA     per century   pole Dec    per century   W           per day           flattening
    {"Sun",       286.13,     0.0,          63.87,      0.0,          84.176,     14.1844,          0.0},
    {"Mercury",   281.0103,   -0.0328,      61.4155,    -0.0049,      329.5988,   6.1385108,        0.0},
    {"Venus",     272.76,     0.0,          67.16,      0.0,          160.20,     -1.4813688,       0.0},
    {"Earth",     0.0,        -0.641,       90.0,       -0.557,       190.147,    360.9856235,      0.0033528},
    {"Moon",      269.9949,   0.0031,       66.5392,    0.0130,       38.3213,    13.17635815,      0.0,
     // E1 = 125.045 - 0.0529921 d
     -3.8787, 125.045, -1935.5364525, 1.5419, 125.045, -1935.5364525, 3.5610, 125.045, -1935.5364525},
    {"Mars",      317.269202, -0.10927547,  54.432516,  -0.05827105,  176.049863, 350.891982443297, 0.0058860,
     0.419057, 79.398797, 0.5042615, 1.591274, 166.325722, 0.5042615, 0.584542, 95.391654, 0.5042615},
    {"Jupiter",   268.056595, -0.006499,    64.495303,  0.002413,     284.95,     870.5360000,      0.064874},
    {"Saturn",    40.589,     -0.036,       83.537,     -0.004,       38.90,      810.7939024,      0.097963},
    {"Uranus",    257.311,    0.0,          -15.175,    0.0,          203.81,     -501.1600928,     0.022927},
    {"Neptune",   299.36,     0.0,          43.46,      0.0,          249.978,    541.1397757,      0.017081,
     // N = 357.85 + 52.316 T
     0.70, 357.85, 52.316, -0.51, 357.85, 52.316, -0.48, 357.85, 52.316},
};

}

const Model* find(const std::string& name) {
    for (const Model& m : MODELS)
        if (name == m.name) return &m;
    return nullptr;
}

Axes equatorialAxes(const Model& m, double julianDay) {
    const double d = julianDay - units::J2000_JD;  // days since J2000
    const double T = d / 36525.0;                  // Julian centuries since J2000
    auto angle = [T](double a0, double a1) { return (a0 + a1 * T) * units::DEG; };

    const double ra = (m.poleRa + m.poleRaRate * T + m.raAmplitude * std::sin(angle(m.raAngle, m.raAngleRate))) *
                      units::DEG;
    const double dec =
        (m.poleDec + m.poleDecRate * T + m.decAmplitude * std::cos(angle(m.decAngle, m.decAngleRate))) * units::DEG;
    // Keep the turn angle within one full turn, so it stays precise
    const double wDegrees = std::fmod(m.meridian + m.meridianRate * d, 360.0) +
                            m.meridianAmplitude * std::sin(angle(m.meridianAngle, m.meridianAngleRate));
    const double w = wDegrees * units::DEG;

    const Vector3D pole(std::cos(dec) * std::cos(ra), std::cos(dec) * std::sin(ra), std::sin(dec));
    // Where the body's equator crosses Earth's equator
    const Vector3D node(-std::sin(ra), std::cos(ra), 0.0);
    // Turn from there by w to find longitude 0
    Axes axes;
    axes.x = node * std::cos(w) + pole.cross(node) * std::sin(w);
    axes.z = pole;
    axes.y = pole.cross(axes.x);
    return axes;
}

Axes eclipticAxes(const Model& model, double julianDay) {
    const Axes q = equatorialAxes(model, julianDay);
    return Axes{galaxy::equatorialToEcliptic(q.x), galaxy::equatorialToEcliptic(q.y),
                galaxy::equatorialToEcliptic(q.z)};
}

}
