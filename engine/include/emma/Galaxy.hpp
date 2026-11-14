#pragma once
#include "emma/Constants.hpp"
#include "emma/Vector3D.hpp"
#include <cmath>

// The Galactic frame and the Sun's motion through it. Galactic coordinates (IAU 1958,
// in the ICRS form of the Hipparcos Catalogue) put x toward the Galactic center, y
// toward Galactic rotation (longitude 90 degrees) and z toward the north Galactic
// pole. EMMA works in the J2000 ecliptic frame, so a direction goes
// ecliptic -> equatorial (ICRS) -> Galactic
namespace emma::galaxy {

constexpr double OBLIQUITY = 84381.448 / 3600.0 * units::DEG;  // tilt of the ecliptic to the equator at J2000

// ICRS to Galactic: each row is one Galactic axis written in ICRS coordinates
// (The Hipparcos and Tycho Catalogues, ESA 1997, vol. 1, p. 92)
constexpr double ICRS_TO_GALACTIC[3][3] = {
    {-0.0548755604, -0.8734370902, -0.4838350155},
    {+0.4941094279, -0.4448296300, +0.7469822445},
    {-0.8676661490, -0.1980763734, +0.4559837762},
};

constexpr double R0_KPC = 8.15;             // Sun to the Galactic center (Reid et al. 2019)
constexpr double ORBIT_SPEED_KMS = 247.0;   // the Sun's speed along its orbit, Theta0 + V_sun (Reid et al. 2019)
constexpr double U_SUN_KMS = 11.1;          // the Sun's drift toward the Galactic center (Schoenrich et al. 2010)
constexpr double W_SUN_KMS = 7.25;          // and toward the north Galactic pole (Schoenrich et al. 2010)

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

// Ecliptic to equatorial is a turn by the obliquity about the x axis (the equinox)
inline Vector3D eclipticToEquatorial(const Vector3D& e) {
    const double c = std::cos(OBLIQUITY), s = std::sin(OBLIQUITY);
    return Vector3D(e.x, c * e.y - s * e.z, s * e.y + c * e.z);
}

inline Vector3D equatorialToEcliptic(const Vector3D& q) {
    const double c = std::cos(OBLIQUITY), s = std::sin(OBLIQUITY);
    return Vector3D(q.x, c * q.y + s * q.z, -s * q.y + c * q.z);
}

// Ecliptic to Galactic in one matrix: the ICRS matrix after the obliquity turn.
// Worked out once, on first use
inline const Rotation& eclipticToGalactic() {
    static const Rotation rotation = [] {
        const double c = std::cos(OBLIQUITY), s = std::sin(OBLIQUITY);
        Rotation r{};
        for (int i = 0; i < 3; ++i) {
            const double a = ICRS_TO_GALACTIC[i][0], b = ICRS_TO_GALACTIC[i][1], d = ICRS_TO_GALACTIC[i][2];
            r.m[i][0] = a;
            r.m[i][1] = b * c + d * s;
            r.m[i][2] = -b * s + d * c;
        }
        return r;
    }();
    return rotation;
}

inline Vector3D fromEcliptic(const Vector3D& e) { return eclipticToGalactic().apply(e); }
inline Vector3D toEcliptic(const Vector3D& g) { return eclipticToGalactic().applyInverse(g); }

// The Sun's velocity relative to the Galactic center, in Galactic coordinates (km/s)
inline Vector3D sunVelocityKms() { return Vector3D(U_SUN_KMS, ORBIT_SPEED_KMS, W_SUN_KMS); }

}
