#pragma once
#include "emma/Body.hpp"
#include "emma/Constants.hpp"
#include "emma/Vector3D.hpp"
#include <array>
#include <cstddef>
#include <vector>

namespace emma {

// Listed in the sidebar in this order
enum class Preset { SolarSystem = 0, AlphaCentauri = 1, GalacticCenter = 2, Empty = 3 };

// The solar system's bodies: Sun, Mercury, Venus, Earth, Moon, Mars, Jupiter, Saturn,
// Uranus, Neptune
constexpr std::size_t SOLAR_BODY_COUNT = 10;

// Measured positions and velocities for the solar system (AU, AU/day)
struct SolarSnapshot {
    double julianDay = 0.0;  // UTC
    std::array<Vector3D, SOLAR_BODY_COUNT> position;
    std::array<Vector3D, SOLAR_BODY_COUNT> velocity;
};

// A preset's bodies and settings
struct Scenario {
    std::vector<Body> bodies;
    double step = 0.1;               // days, the base time step
    bool hasCalendar = false;        // true when time 0 is a real date
    double epoch = units::J2000_JD;  // Julian day at time 0
    double viewRadius = 2.0;         // AU, what the camera should show
    Vector3D lookDirection;          // which way the camera should look; zero for any
    double timeScale = 30.0;         // simulated days per real second
};

// Builds a preset. The solar system uses the snapshot if there is one, else average orbits
Scenario makeScenario(Preset preset, const SolarSnapshot* snapshot = nullptr);

}
