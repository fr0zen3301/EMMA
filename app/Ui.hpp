#pragma once
#include "Camera.hpp"
#include "Renderer.hpp"
#include "SceneFrame.hpp"
#include "emma/Body.hpp"
#include "emma/Clock.hpp"
#include "emma/Simulation.hpp"
#include <cstddef>
#include <filesystem>
#include <string>
#include <vector>

// Everything the sidebar can change (time is in emma::Clock)
struct UiState {
    float massExponent = -5.52f;  // log10 of the mass in Msun; -5.52 is one Earth mass
    bool spawnHole = false;       // add a black hole instead
    float holeMassExponent = 1.f; // the hole's log10 mass: 1 = 10 solar masses
    float holeSpin = 0.f;         // the hole's spin, 0 to 0.998
    float speedKms = 0.f;         // launch speed
    float angle = 0.f;            // launch direction, in radians
    bool autoOrbit = true;        // launch on a circular orbit
    bool showPath = true;
    bool showTrails = true;
    bool showLabels = true;
    bool showGrid = true;
    bool bendGrid = false;        // bend the grid where clocks run slower
    float gridDepth = 0.25f;      // how deep the bent grid goes
    std::string follow;           // the body the camera follows; empty for none
    bool galaxyView = false;      // show the solar system moving through the Galaxy
    float sunSpeedKms = static_cast<float>(emma::galaxy::sunVelocityKms().magnitude());  // the Sun's speed there
    bool relativity = true;       // general relativity on or off
    float relativityScale = 1.f;  // how much to exaggerate it (1 = real)
    int holeLook = 0;             // 0 = Sgr A*'s real gas, 1 = a thin disk, 2 = nothing
    bool holeBendLight = true;
    bool holeDoppler = true;
    float holeBrightness = 0.f;   // log2 of the exposure

    bool resetRequested = false;
    bool resetViewRequested = false;
    bool topViewRequested = false;
    int presetRequested = -1;
    int integratorRequested = -1;
    bool retryFetchRequested = false;
    bool goToHoleRequested = false;
};

struct Stats {
    std::size_t bodies = 0;
    float fps = 0.f;
    double energyDrift = 0.0;    // how much the energy has changed (fraction)
    double momentumDrift = 0.0;  // how much the angular momentum has changed (fraction)
    const std::vector<float>* driftHistory = nullptr;
    bool behind = false;         // the physics can't keep up with the chosen speed
    double aheadDays = 0.0;      // in real time, how far the simulation is ahead of the clock
    std::string sourceNote;      // where the solar system's starting positions came from
    bool canRetry = false;       // the download failed; offer to try again
    double sunTravelled = 0.0;   // AU the Sun has moved through the Galaxy
    double clockSlowdown = -1.0; // how slow a clock runs at the middle of the view; -1 for none
};

namespace ui {

// The name used for the black hole at the Galactic center
inline constexpr const char* SGR_A = "Sgr A*";

void applyTheme();
void loadFont(const std::filesystem::path& exeDir);

// anyHole shows the Black holes section. holeName is the hole Go to jumps to, or nullptr
void drawSidebar(UiState& state, emma::Clock& clock, const Stats& stats, emma::Preset preset,
                 const std::vector<emma::Body>& bodies, const char* integratorName, bool anyHole,
                 const char* holeName, float width, float height);
void drawStatusBar(const emma::Simulation& sim, const Camera& camera, const SceneFrame& frame, sf::Vector2f mouse,
                   const emma::Clock& clock, float y, float width, float height);
void drawLabels(const Camera& camera, const SceneFrame& frame, const std::vector<emma::Body>& bodies);
void drawBodyTooltip(const emma::Simulation& sim, const Camera& camera, const SceneFrame& frame, sf::Vector2f mouse);
// The body under a pixel, or -1. Black holes are found separately
int bodyAt(const std::vector<emma::Body>& bodies, const Camera& camera, const SceneFrame& frame, sf::Vector2f at);
// A small grey label next to a point
void drawTag(const Camera& camera, const emma::Vector3D& at, const char* text);
// Is the mouse over where a black hole was drawn?
bool overBlackHole(const Renderer::BlackHoleShot& shot, sf::Vector2f mouse);
// A named black hole's label, next to where it was drawn
void drawBlackHoleLabel(const Camera& camera, const Renderer::BlackHoleShot& shot);
// A black hole's tooltip. where is an optional extra line, like its distance
void drawBlackHoleTooltip(const Renderer::BlackHoleShot& shot, const std::string& where);
void drawLaunchReadout(sf::Vector2f at, double speedKms, double angleRad);

std::string formatMass(double massSolar);
std::string formatDistance(double au);
std::string formatRate(double daysPerSecond);
std::string formatDuration(double days);

}
