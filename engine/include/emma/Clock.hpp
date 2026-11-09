#pragma once
#include "emma/Simulation.hpp"

namespace emma {

// Keeps simulated time in pace with real time: the speed, pause, single steps and real time
// No graphics in here: the app sets the fields and calls run() once per frame
class Clock {
public:
    static constexpr double MAX_FRAME = 0.1;    // seconds: the longest real frame the physics catches up on
    static constexpr double BUDGET = 0.012;     // seconds of stepping per frame before falling behind
    static constexpr double GLIDE_RATE = 60.0;  // at slow speeds, one step per 1/60 s, so bodies glide

    double timeScale = 30.0;  // simulated days per real second
    bool paused = false;
    bool live = false;        // real time: one second per second, in step with the computer's clock

    void requestStep();
    // Forgets time that was due but not yet stepped: call after loading a new state
    void reset() { owed = 0.0; }

    struct Report {
        int steps = 0;
        bool behind = false;
    };
    // Steps sim for a frame that took realSeconds. Real time runs until the
    // simulation's clock reads nowJulianDay
    Report run(Simulation& sim, double realSeconds, double nowJulianDay);
    Report run(Simulation& sim, double realSeconds);  // now from the system clock

private:
    double owed = 0.0;  // simulated days due but not yet stepped
    bool stepRequested = false;
};

}
