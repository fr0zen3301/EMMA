#pragma once
#include "Body.hpp"
#include "Camera.hpp"
#include "Simulation.hpp"
#include <cstddef>
#include <filesystem>
#include <string>
#include <vector>

// Everything the sidebar can change. main.cpp reads the *Requested flags each
// frame and clears them once handled.
struct UiState {
    float timeScale = 30.f;  // simulated days per real second
    bool paused = false;
    bool stepRequested = false;

    float massExponent = -5.52f;  // log10(mass / Msun); -5.52 is one Earth mass
    float speedKms = 0.f;         // launch speed, relative to the dominant body
    float angle = 0.f;            // launch direction, radians counterclockwise from +x
    bool autoOrbit = true;        // launch on a circular orbit instead
    bool showPath = true;
    bool showTrails = true;
    bool showLabels = true;

    bool clearRequested = false;
    bool resetRequested = false;
    bool resetViewRequested = false;
    int presetRequested = -1;
    bool retryFetchRequested = false;
};

struct Stats {
    std::size_t bodies = 0;
    float fps = 0.f;
    double energyDrift = 0.0;    // relative change since the last baseline
    double momentumDrift = 0.0;  // angular momentum, relative
    const std::vector<float>* driftHistory = nullptr;
    bool behind = false;         // the physics couldn't keep up with the chosen speed
    std::string sourceNote;      // where the solar system's starting positions came from
    bool canRetry = false;       // the Horizons download failed and can be tried again
};

namespace ui {

void applyTheme();
void loadFont(const std::filesystem::path& exeDir);

void drawSidebar(UiState& state, const Stats& stats, Preset preset, float width, float height);
void drawStatusBar(const Simulation& sim, const Camera& camera, sf::Vector2f mouse, const UiState& state,
                   float y, float width, float height);
void drawLabels(const Camera& camera, const std::vector<Body>& bodies);
void drawBodyTooltip(const Simulation& sim, const Camera& camera, sf::Vector2f mouse);
void drawLaunchReadout(sf::Vector2f at, double speedKms, double angleRad);

std::string formatMass(double massSolar);
std::string formatDistance(double au);
std::string formatRate(double daysPerSecond);
std::string formatDuration(double days);

} // namespace ui
