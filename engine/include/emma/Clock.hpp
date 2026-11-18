#pragma once
#include "emma/Simulation.hpp"

namespace emma {

// Decides how much simulated time passes each frame: speed, pause, single steps, real time.
// The app sets the fields and calls run() once per frame
class Clock {
public:
    static constexpr double MAX_FRAME = 0.1;    // seconds: the longest real frame the physics catches up on
    static constexpr double BUDGET = 0.012;     // seconds of stepping per frame before giving up
    static constexpr double GLIDE_RATE = 60.0;  // at slow speeds, one step per 1/60 s so bodies move smoothly

    double timeScale = 30.0;  // simulated days per real second
    bool paused = false;
    bool live = false;        // real time: one second per second

    void requestStep();
    // Drops any time still owed. Call after loading a new state
    void reset() { owed = 0.0; }

    struct Report {
        int steps = 0;
        bool behind = false;
        double ahead = 0.0;  // days the simulation is ahead of the clock, so real time is waiting
    };
    // Steps the simulation for one frame. In real time it catches up to nowJulianDay
    Report run(Simulation& sim, double realSeconds, double nowJulianDay);
    Report run(Simulation& sim, double realSeconds);  // now from the system clock

private:
    double owed = 0.0;  // simulated days still to step
    bool stepRequested = false;
    bool wasLive = false;  // real time was on last frame
};

}
