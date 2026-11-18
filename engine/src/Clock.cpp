#include "emma/Clock.hpp"
#include "emma/Calendar.hpp"
#include <algorithm>
#include <chrono>

namespace emma {

void Clock::requestStep() {
    paused = true;
    stepRequested = true;
}

Clock::Report Clock::run(Simulation& sim, double realSeconds) {
    return run(sim, realSeconds, calendar::julianDayNow());
}

Clock::Report Clock::run(Simulation& sim, double realSeconds, double nowJulianDay) {
    if (stepRequested || !sim.hasCalendar()) live = false;  // real time needs dates and no stepping
    Report report;
    if (live) {
        // Catch up to the current time. If the simulation is ahead of it, wait
        if (!paused) {
            owed = nowJulianDay - (sim.epoch() + sim.time());
            sim.limitStep(std::max(owed, 0.0));
        }
        if (owed < 0.0) report.ahead = -owed;
    } else {
        // Coming out of real time, forget the gap to the clock, so the new speed starts at once
        if (wasLive) owed = 0.0;
        sim.limitStep(timeScale / GLIDE_RATE);
        // Cap the frame time, so a long pause (like dragging the window) can't pile up
        if (!paused)
            owed += std::min(realSeconds, MAX_FRAME) * timeScale;
        else if (stepRequested)
            owed += std::max(timeScale / GLIDE_RATE, sim.timeStep());
    }
    stepRequested = false;
    wasLive = live;

    const auto start = std::chrono::steady_clock::now();
    while (owed >= sim.timeStep()) {
        sim.step();
        owed -= sim.timeStep();
        ++report.steps;
        // Stop if this frame has used up its time budget
        if (std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count() > BUDGET) {
            // Drop the rest instead of freezing the window
            report.behind = !live;
            owed = 0.0;
            break;
        }
    }
    return report;
}

}
