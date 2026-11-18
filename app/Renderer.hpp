#pragma once
#include "Camera.hpp"
#include "SceneFrame.hpp"
#include "SpacetimeSheet.hpp"
#include "emma/Body.hpp"
#include "emma/Simulation.hpp"
#include <SFML/Graphics.hpp>
#include <array>
#include <cstddef>
#include <filesystem>
#include <map>
#include <optional>
#include <string>
#include <vector>

// Draws everything on screen with SFML
class Renderer {
public:
    static constexpr sf::Color SPACE_COLOR{4, 4, 9};  // the window's background

    // How a black hole looks up close
    struct BlackHoleLook {
        int look = 0;            // 0 = Sgr A*'s real gas, 1 = a thin disk, 2 = nothing
        bool bendLight = true;   // false = light goes straight, to show what gravity does
        bool doppler = true;     // moving gas changes color and brightness
        float brightness = 0.f;  // log2 of the exposure
    };

    // A black hole to draw: a black-hole body, or Sgr A* as seen from the solar system
    struct BlackHoleSight {
        emma::Vector3D center;    // scene coordinates
        double mass = 0.0;        // Msun
        double spin = 0.0;        // only changes the horizon size for now
        emma::Vector3D axis;      // the axis its gas turns around (Galactic coordinates)
        std::string name;         // empty for a hole you added
        std::string kind;
        int body = -1;            // its index among the bodies; -1 for the solar system's Sgr A*
    };

    // Where a black hole ended up on screen, for its label, tooltip and clicks
    struct BlackHoleShot {
        bool visible = false;     // in front of the eye
        sf::Vector2f at;          // pixels
        float radius = 0.f;       // pixels
        bool rayTraced = false;
        double eyeDistance = 0.0; // AU
        BlackHoleSight hole;
    };

    Renderer();

    // Where to look for the textures folder
    void setAssetFolder(const std::filesystem::path& exeDir) { assetFolder = exeDir; }

    void drawStarfield(sf::RenderTarget& target, const Camera& camera, const SceneFrame& frame);
    // Draws the grid. With masses and a sheetDepth above 0, it bends where clocks run slower.
    // Returns psi at the middle of the view (for the clock readout)
    double drawGrid(sf::RenderTarget& target, const Camera& camera, const std::vector<sheet::Mass>& masses = {},
                    double sheetDepth = 0.0);
    void drawTrails(sf::RenderTarget& target, const Camera& camera, const SceneFrame& frame,
                    const std::vector<emma::Body>& bodies);
    // Draws the bodies. Big enough ones show their real surface. Black holes are drawn separately
    void drawBodies(sf::RenderTarget& target, const Camera& camera, const SceneFrame& frame,
                    const std::vector<emma::Body>& bodies, double julianDay);
    void drawFlashes(sf::RenderTarget& target, const Camera& camera, const SceneFrame& frame,
                     const std::vector<emma::MergeFlash>& flashes) const;
    // A dashed line between two points in scene coordinates
    void drawRay(sf::RenderTarget& target, const Camera& camera, const emma::Vector3D& from,
                 const emma::Vector3D& to, sf::Color color);
    void drawPath(sf::RenderTarget& target, const Camera& camera, const std::vector<emma::Vector3D>& path,
                  emma::Rgb color);
    void drawGhost(sf::RenderTarget& target, sf::Vector2f at, float radius, emma::Rgb color) const;
    void drawArrow(sf::RenderTarget& target, sf::Vector2f from, sf::Vector2f to, sf::Color color) const;

    // Draws a black hole: ray traced if it's big enough and mayTrace is true, otherwise a
    // small ring. Only one can be ray traced at a time
    BlackHoleShot drawBlackHole(sf::RenderTarget& target, const Camera& camera, const SceneFrame& frame,
                                const BlackHoleLook& look, const BlackHoleSight& hole, bool mayTrace);
    // How big a hole's shadow looks on screen, in pixels
    static double shadowPixels(const Camera& camera, const emma::Vector3D& center, double mass);

    // How big to draw a body: its true size up close, otherwise a marker sized by its mass
    static float screenRadius(double mass, double radiusAU, double pxPerAU);

private:
    // A background star, stored in both frames so it never needs converting
    struct Star {
        emma::Vector3D galactic;
        emma::Vector3D ecliptic;
        sf::Color color;
        float size;
    };

    // Adds a line between two points, cut off where it goes behind the camera
    void addSegment(const Camera& camera, emma::Vector3D a, emma::Vector3D b, sf::Color ca, sf::Color cb);

    // Ray traces a black hole. Only redraws when the view changes. False if ray tracing
    // isn't available
    bool traceBlackHole(sf::RenderTarget& target, const Camera& camera, const SceneFrame& frame,
                        const BlackHoleLook& look, const emma::Vector3D& axis, const emma::Vector3D& eyeRs,
                        double distanceRs);
    bool prepareRayTracing();
    // Paints the starfield into one image of the whole sky
    bool bakeSky();

    // A body's surface images from the textures folder (earth.jpg, earth_night.jpg, ...).
    // Loaded once, the first time they're needed
    struct Surface {
        bool tried = false;
        std::optional<sf::Texture> color, night, clouds, height, rings;
    };
    const Surface& surfaceFor(const std::string& name);
    std::optional<sf::Texture> loadTexture(const std::string& file, bool wraps);
    bool prepareSurfaceShader();
    // Draws a body's real surface. False if it can't, so the caller draws a plain disc
    bool drawSurface(sf::RenderTarget& target, const Camera& camera, const SceneFrame& frame, const emma::Body& body,
                     sf::Vector2f at, float radiusPx, const std::optional<emma::Vector3D>& sun, double julianDay);
    // A simple black hole without ray tracing: a black disc in a glowing ring
    void drawFlatBlackHole(sf::RenderTarget& target, sf::Vector2f at, float radius, double spinX, double spinY) const;

    std::vector<Star> sky;
    sf::VertexArray stars{sf::PrimitiveType::Triangles};
    sf::VertexArray lines{sf::PrimitiveType::Lines};
    std::vector<std::size_t> order;
    std::vector<std::size_t> drawn;
    std::vector<sf::Vector2f> screen;
    std::vector<float> radii;
    std::vector<double> depth;
    std::vector<char> visible;
    std::vector<double> sheetDips;  // the bent grid's dip at each of its points
    double sheetScale = 0.0;        // how much the dips are exaggerated

    enum class TraceState { Untried, Ready, Failed };
    TraceState traceState = TraceState::Untried;
    sf::Shader blackHoleShader;
    std::optional<sf::Texture> skyMap;
    sf::RenderTexture traced;            // the last ray-traced picture
    std::array<double, 19> traceKey{};   // the view it was drawn for
    bool tracedOnce = false;
    float tracedScale = 0.f;             // its resolution, as a fraction of full size
    sf::Clock sinceChanged;

    std::filesystem::path assetFolder;
    std::map<std::string, Surface> surfaces;
    sf::Shader surfaceShader;
    TraceState surfaceState = TraceState::Untried;
};
