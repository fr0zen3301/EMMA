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
    if (stepRequested || !sim.hasCalendar()) live = false;  // stepping, or no dates to follow
    if (live) {
        // run until the simulation's clock reads nowJulianDay
        // after a pause or a load it catches up at the full step
        if (!paused) {
            owed = nowJulianDay - (sim.epoch() + sim.time());
            sim.limitStep(std::max(owed, 0.0));
        }
    } else {
        sim.limitStep(timeScale / GLIDE_RATE);
        // The frame time is capped so a stall (dragging the window on Windows pauses
        // the loop) can't snowball
        if (!paused)
            owed += std::min(realSeconds, MAX_FRAME) * timeScale;
        else if (stepRequested)
            owed += std::max(timeScale / GLIDE_RATE, sim.timeStep());
    }
    stepRequested = false;

    Report report;
    const auto start = std::chrono::steady_clock::now();
    while (owed >= sim.timeStep()) {
        sim.step();
        owed -= sim.timeStep();
        ++report.steps;
        // Checked after every step: a single step can take long (a close pass split into
        // thousands of pieces, IAS15, many bodies), and reading the clock costs far less
        if (std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count() > BUDGET) {
            // Drop the backlog instead of freezing the window. Real time works out
            // what's owed from the clock again next frame, so nothing is lost there
            report.behind = !live;
            owed = 0.0;
            break;
        }
    }
    return report;
}

}
