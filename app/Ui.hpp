#pragma once
#include "Camera.hpp"
#include "SceneFrame.hpp"
#include "emma/Body.hpp"
#include "emma/Clock.hpp"
#include "emma/Simulation.hpp"
#include <cstddef>
#include <filesystem>
#include <string>
#include <vector>

// Everything the sidebar can change, apart from time (emma::Clock)
struct UiState {
    float massExponent = -5.52f;  // log10(mass / Msun); -5.52 is one Earth mass
    float speedKms = 0.f;         // launch speed, relative to the dominant body
    float angle = 0.f;            // launch direction, radians counterclockwise from +x
    bool autoOrbit = true;        // launch on a circular orbit instead
    bool showPath = true;
    bool showTrails = true;
    bool showLabels = true;
    bool showGrid = true;
    std::string follow;           // the named body the camera follows, empty for none
    bool galaxyView = false;      // draw the solar system moving through the Galaxy
    float sunSpeedKms = static_cast<float>(emma::galaxy::sunVelocityKms().magnitude());  // its speed there

    bool clearRequested = false;
    bool resetRequested = false;
    bool resetViewRequested = false;
    bool topViewRequested = false;
    int presetRequested = -1;
    int integratorRequested = -1;
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
    bool canRetry = false;       // the Horizons download failed, try again
    double sunTravelled = 0.0;   // AU the Sun has moved through the Galaxy since time 0, at the chosen speed
};

namespace ui {

void applyTheme();
void loadFont(const std::filesystem::path& exeDir);

void drawSidebar(UiState& state, emma::Clock& clock, const Stats& stats, emma::Preset preset,
                 const std::vector<emma::Body>& bodies, const char* integratorName, float width, float height);
void drawStatusBar(const emma::Simulation& sim, const Camera& camera, const SceneFrame& frame, sf::Vector2f mouse,
                   const emma::Clock& clock, float y, float width, float height);
void drawLabels(const Camera& camera, const SceneFrame& frame, const std::vector<emma::Body>& bodies);
void drawBodyTooltip(const emma::Simulation& sim, const Camera& camera, const SceneFrame& frame, sf::Vector2f mouse);
// A small muted label next to a point in scene coordinates
void drawTag(const Camera& camera, const emma::Vector3D& at, const char* text);
void drawLaunchReadout(sf::Vector2f at, double speedKms, double angleRad);

std::string formatMass(double massSolar);
std::string formatDistance(double au);
std::string formatRate(double daysPerSecond);
std::string formatDuration(double days);

}
