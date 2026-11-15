#pragma once
#include "Camera.hpp"
#include "SceneFrame.hpp"
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

// Draws the scene with SFML. Simulation points go through the SceneFrame into scene
// coordinates, then through the Camera into pixels.
class Renderer {
public:
    static constexpr sf::Color SPACE_COLOR{4, 4, 9};  // the window's background

    // How Sgr A* looks up close
    struct BlackHoleLook {
        int look = 1;            // 0 the gas flow Sgr A* really has, 1 a thin disk, 2 nothing
        bool bendLight = true;   // false sends light straight, to show what gravity does
        bool doppler = true;     // the gas's motion changes its color and brightness
        float brightness = 0.f;  // log2 of the exposure
    };

    // Where Sgr A* was drawn, for its label and tooltip
    struct BlackHoleShot {
        bool visible = false;     // in front of the eye
        sf::Vector2f at;          // pixels
        float radius = 0.f;       // pixels, the shadow and its ring
        bool rayTraced = false;
        double eyeDistance = 0.0; // AU
    };

    Renderer();

    // Where to look for the textures folder besides the working directory: next to the executable
    void setAssetFolder(const std::filesystem::path& exeDir) { assetFolder = exeDir; }

    void drawStarfield(sf::RenderTarget& target, const Camera& camera, const SceneFrame& frame);
    void drawGrid(sf::RenderTarget& target, const Camera& camera);
    void drawTrails(sf::RenderTarget& target, const Camera& camera, const SceneFrame& frame,
                    const std::vector<emma::Body>& bodies);
    // Bodies with a texture are drawn with their real surface, turned the way they face at
    // julianDay, once they're at least a few pixels across at their true size
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

    // Sgr A* at the Galactic center. Close enough that its shadow is at least a pixel
    // across, it's ray traced over the whole view, starfield included. Farther out it's
    // a small ring marker. Draw it right after the starfield
    BlackHoleShot drawBlackHole(sf::RenderTarget& target, const Camera& camera, const SceneFrame& frame,
                                const BlackHoleLook& look);

    // the true size once you zoom in far enough
    // otherwise a marker that grows with the logarithm of the mass
    static float screenRadius(double mass, double radiusAU, double pxPerAU);

private:
    // A background star's direction in both frames, so neither needs turning every frame
    struct Star {
        emma::Vector3D galactic;
        emma::Vector3D ecliptic;
        sf::Color color;
        float size;
    };

    // Appends a line between two points in camera coordinates, cut at the near plane
    void addSegment(const Camera& camera, emma::Vector3D a, emma::Vector3D b, sf::Color ca, sf::Color cb);

    // Sgr A*, ray traced: loads the shader and bakes the sky the first time, renders the
    // view again only when it would change, at half resolution while it keeps changing.
    // False when ray tracing isn't available
    bool traceBlackHole(sf::RenderTarget& target, const Camera& camera, const SceneFrame& frame,
                        const BlackHoleLook& look, const emma::Vector3D& eyeRs, double distanceRs);
    bool prepareRayTracing();
    // The starfield as one image of the whole sky, by Galactic longitude and latitude
    bool bakeSky();

    // A body's maps from the textures folder, named after it in lowercase: earth.jpg,
    // earth_night.jpg, earth_clouds.jpg, moon_height.jpg, saturn_ring.png. Each is
    // loaded the first time the body is drawn large enough, and only once
    struct Surface {
        bool tried = false;
        std::optional<sf::Texture> color, night, clouds, height, rings;
    };
    const Surface& surfaceFor(const std::string& name);
    std::optional<sf::Texture> loadTexture(const std::string& file, bool wraps);
    bool prepareSurfaceShader();
    // Draws the body's surface with the shader. False when it can't (no texture, no
    // shaders), and the caller draws the plain disc
    bool drawSurface(sf::RenderTarget& target, const Camera& camera, const SceneFrame& frame, const emma::Body& body,
                     sf::Vector2f at, float radiusPx, const std::optional<emma::Vector3D>& sun, double julianDay);
    // Sgr A* without ray tracing: a black disc in a ring that's brighter where its gas
    // moves toward the eye. spinX and spinY are the gas's axis on screen
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

    enum class TraceState { Untried, Ready, Failed };
    TraceState traceState = TraceState::Untried;
    sf::Shader blackHoleShader;
    std::optional<sf::Texture> skyMap;
    sf::RenderTexture traced;            // the last ray-traced picture
    std::array<double, 16> traceKey{};   // what it was rendered for
    bool tracedOnce = false;
    float tracedScale = 0.f;             // its resolution, as a fraction of the view's
    sf::Clock sinceChanged;

    std::filesystem::path assetFolder;
    std::map<std::string, Surface> surfaces;
    sf::Shader surfaceShader;
    TraceState surfaceState = TraceState::Untried;
};
