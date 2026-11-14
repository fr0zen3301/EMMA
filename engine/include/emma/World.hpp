#pragma once
#include "emma/Body.hpp"
#include "emma/Constants.hpp"
#include <cstddef>
#include <vector>

namespace emma {

// The state of a system: its bodies and where it is in time. Gravity, the
// integrator and collisions all work on a World
struct World {
    static constexpr std::size_t MAX_TRAIL = 540;           // points per trail: three full turns
    static constexpr double TRAIL_TURN = 2.0 * units::DEG;  // a new trail point after this much turning

    std::vector<Body> bodies;
    double time = 0.0;               // days since time 0
    double epoch = units::J2000_JD;  // Julian day at time 0
    bool hasCalendar = false;        // true when time 0 is a real date
    unsigned long long changes = 0;  // bumps whenever bodies are added, merged, loaded or cleared

    void moveToBarycenter();         // puts the center of mass at the origin, at rest
    void recordTrails();             // a new point for each body that has turned TRAIL_TURN since its last
    double kineticEnergy() const;    // Msun AU^2 / day^2
    Vector3D angularMomentum() const;  // Msun AU^2 / day, about the origin
};

}
