#pragma once
#include "emma/Body.hpp"
#include "emma/Constants.hpp"
#include "emma/Vector3D.hpp"
#include <array>
#include <cstddef>
#include <vector>

namespace emma {

enum class Preset { SolarSystem = 0, AlphaCentauri = 1, Empty = 2 };

// The solar system preset's bodies, in this order:
// Sun, Mercury, Venus, Earth, Moon, Mars, Jupiter, Saturn, Uranus, Neptune
constexpr std::size_t SOLAR_BODY_COUNT = 10;

// Measured starting state for the solar system (AU and AU/day, J2000 ecliptic frame)
// Any origin works: loading moves everything to the barycenter.
struct SolarSnapshot {
    double julianDay = 0.0;  // UTC
    std::array<Vector3D, SOLAR_BODY_COUNT> position;
    std::array<Vector3D, SOLAR_BODY_COUNT> velocity;
};

// Everything a system starts with: its bodies and the settings that suit it
struct Scenario {
    std::vector<Body> bodies;
    double step = 0.1;               // days, the base time step
    bool hasCalendar = false;        // true when time 0 is a real date
    double epoch = units::J2000_JD;  // Julian day at time 0
    double viewRadius = 2.0;         // AU, what the camera should show
    double timeScale = 30.0;         // simulated days per real second
};

// With a snapshot, the solar system starts from it; without, from the J2000 orbital elements
Scenario makeScenario(Preset preset, const SolarSnapshot* snapshot = nullptr);

}
