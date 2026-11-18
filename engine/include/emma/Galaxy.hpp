#pragma once
#include "emma/Constants.hpp"
#include "emma/Vector3D.hpp"
#include <cmath>

// The Galactic frame and the Sun's path around the Galaxy. In Galactic coordinates x points
// to the Galactic center, y the way the Galaxy turns, and z to the north Galactic pole
namespace emma::galaxy {

constexpr double OBLIQUITY = 84381.448 / 3600.0 * units::DEG;  // Earth's axial tilt at J2000

// Turns equatorial (ICRS) directions into Galactic ones (Hipparcos Catalogue, ESA 1997)
constexpr double ICRS_TO_GALACTIC[3][3] = {
    {-0.0548755604, -0.8734370902, -0.4838350155},
    {+0.4941094279, -0.4448296300, +0.7469822445},
    {-0.8676661490, -0.1980763734, +0.4559837762},
};

constexpr double R0_KPC = 8.15;             // Sun to the Galactic center (Reid et al. 2019)
constexpr double ORBIT_SPEED_KMS = 247.0;   // the Sun's speed around the Galaxy (Reid et al. 2019)
constexpr double U_SUN_KMS = 11.1;          // the Sun's drift toward the Galactic center (Schoenrich et al. 2010)
constexpr double W_SUN_KMS = 7.25;          // and toward the north Galactic pole (Schoenrich et al. 2010)

constexpr double R0_AU = R0_KPC * 1000.0 * units::AU_PER_PARSEC;  // about 1.68 billion AU

constexpr double SGR_A_MASS = 4.3e6;  // Msun, the black hole at the Galactic center (GRAVITY Collaboration)

// Where Sgr A* is, relative to the Sun's starting point, in Galactic coordinates
inline Vector3D galacticCenter() { return Vector3D(R0_AU, 0.0, 0.0); }

// Sgr A*'s position on the sky (Reid & Brunthaler 2004)
constexpr double SGR_A_RA = (17.0 + 45.0 / 60.0 + 40.0409 / 3600.0) * 15.0 * units::DEG;
constexpr double SGR_A_DEC = -(29.0 + 0.0 / 60.0 + 28.118 / 3600.0) * units::DEG;

// The axis Sgr A*'s gas turns around, 35 degrees from the direction to the Sun.
// The real angle isn't known
inline Vector3D sgrASpinAxis() { return Vector3D(-0.819, 0.574, 0.0).normalized(); }

// A 3x3 rotation. Its inverse is its transpose
struct Rotation {
    double m[3][3];

    Vector3D apply(const Vector3D& v) const {
        return Vector3D(m[0][0] * v.x + m[0][1] * v.y + m[0][2] * v.z,
                        m[1][0] * v.x + m[1][1] * v.y + m[1][2] * v.z,
                        m[2][0] * v.x + m[2][1] * v.y + m[2][2] * v.z);
    }
    Vector3D applyInverse(const Vector3D& v) const {
        return Vector3D(m[0][0] * v.x + m[1][0] * v.y + m[2][0] * v.z,
                        m[0][1] * v.x + m[1][1] * v.y + m[2][1] * v.z,
                        m[0][2] * v.x + m[1][2] * v.y + m[2][2] * v.z);
    }
};

// Ecliptic to equatorial: a turn by Earth's tilt around the x axis
inline Vector3D eclipticToEquatorial(const Vector3D& e) {
    const double c = std::cos(OBLIQUITY), s = std::sin(OBLIQUITY);
    return Vector3D(e.x, c * e.y - s * e.z, s * e.y + c * e.z);
}

inline Vector3D equatorialToEcliptic(const Vector3D& q) {
    const double c = std::cos(OBLIQUITY), s = std::sin(OBLIQUITY);
    return Vector3D(q.x, c * q.y + s * q.z, -s * q.y + c * q.z);
}

// Ecliptic to Galactic in one matrix. The published table has only 10 digits, so the
// matrix is cleaned up into an exact rotation as it's built
inline const Rotation& eclipticToGalactic() {
    static const Rotation rotation = [] {
        const double c = std::cos(OBLIQUITY), s = std::sin(OBLIQUITY);
        Vector3D row[3];
        for (int i = 0; i < 3; ++i) {
            const double a = ICRS_TO_GALACTIC[i][0], b = ICRS_TO_GALACTIC[i][1], d = ICRS_TO_GALACTIC[i][2];
            row[i] = Vector3D(a, b * c + d * s, -b * s + d * c);
        }
        const Vector3D z = row[2].normalized();
        const Vector3D x = (row[0] - z * row[0].dot(z)).normalized();
        const Vector3D axes[3] = {x, z.cross(x), z};
        Rotation r{};
        for (int i = 0; i < 3; ++i) {
            r.m[i][0] = axes[i].x;
            r.m[i][1] = axes[i].y;
            r.m[i][2] = axes[i].z;
        }
        return r;
    }();
    return rotation;
}

inline Vector3D fromEcliptic(const Vector3D& e) { return eclipticToGalactic().apply(e); }
inline Vector3D toEcliptic(const Vector3D& g) { return eclipticToGalactic().applyInverse(g); }

// Three directions on the sky at (ra, dec), seen from Earth: north, east, and the line of
// sight away from us. Orbits measured on the sky, like the S-stars', use these
struct SkyFrame {
    Vector3D north, east, away;
};

inline SkyFrame skyFrame(double ra, double dec) {
    const double ca = std::cos(ra), sa = std::sin(ra), cd = std::cos(dec), sd = std::sin(dec);
    return SkyFrame{equatorialToEcliptic(Vector3D(-sd * ca, -sd * sa, cd)),
                    equatorialToEcliptic(Vector3D(-sa, ca, 0.0)),
                    equatorialToEcliptic(Vector3D(cd * ca, cd * sa, sd))};
}

// The Sun's velocity relative to the Galactic center, in Galactic coordinates (km/s)
inline Vector3D sunVelocityKms() { return Vector3D(U_SUN_KMS, ORBIT_SPEED_KMS, W_SUN_KMS); }

// How far the Sun has moved t days after time 0 (AU, Galactic). It circles the Galactic
// center; its small drifts toward the center and the pole are kept as straight lines.
// 2 sin^2(a/2) is used instead of 1 - cos a, which loses precision for tiny angles
inline Vector3D sunDisplacement(const Vector3D& v, double t) {
    const double angle = v.y * t / R0_AU;
    const double half = std::sin(0.5 * angle);
    return Vector3D(2.0 * R0_AU * half * half + v.x * t, R0_AU * std::sin(angle), v.z * t);
}

// The Sun's velocity t days after time 0, turned along its circle
inline Vector3D sunVelocityAt(const Vector3D& v, double t) {
    const double angle = v.y * t / R0_AU;
    return Vector3D(v.y * std::sin(angle) + v.x, v.y * std::cos(angle), v.z);
}

}
