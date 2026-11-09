#pragma once

// EMMA works in the units of solar system dynamics: lengths in astronomical
// units (AU), masses in solar masses (Msun), time in days. In these units G is
// the square of the Gaussian gravitational constant k.
namespace emma::units {

constexpr double PI = 3.14159265358979323846;
constexpr double DEG = PI / 180.0;

constexpr double GAUSS_K = 0.01720209895;          // AU^(3/2) Msun^(-1/2) day^-1
constexpr double G = GAUSS_K * GAUSS_K;            // AU^3 / (Msun day^2)

constexpr double KM_PER_AU = 149597870.7;          // IAU 2012, exact
constexpr double SECONDS_PER_DAY = 86400.0;
constexpr double DAYS_PER_YEAR = 365.25;           // Julian year
constexpr double KMS_TO_AU_PER_DAY = SECONDS_PER_DAY / KM_PER_AU;

constexpr double EARTH_MASS = 1.0 / 332946.0487;   // Msun, IAU 2009
constexpr double JUPITER_MASS = 1.0 / 1047.348644; // Msun, IAU 2009
constexpr double SOLAR_RADIUS_AU = 695700.0 / KM_PER_AU;  // IAU 2015 nominal
constexpr double EARTH_RADIUS_AU = 6371.0 / KM_PER_AU;
constexpr double SUN_TEMPERATURE = 5772.0;         // K, IAU 2015 nominal

constexpr double J2000_JD = 2451545.0;             // 2000-01-01 12:00 TT

} // namespace emma::units
