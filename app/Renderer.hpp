#pragma once
#include "Camera.hpp"
#include "SceneFrame.hpp"
#include "emma/Body.hpp"
#include "emma/Simulation.hpp"
#include <SFML/Graphics.hpp>
#include <cstddef>
#include <vector>

// Draws the scene with SFML. Simulation points go through the SceneFrame into scene
// coordinates, then through the Camera into pixels.
class Renderer {
public:
    Renderer();

    void drawStarfield(sf::RenderTarget& target, const Camera& camera, const SceneFrame& frame);
    void drawGrid(sf::RenderTarget& target, const Camera& camera);
    void drawTrails(sf::RenderTarget& target, const Camera& camera, const SceneFrame& frame,
                    const std::vector<emma::Body>& bodies);
    void drawBodies(sf::RenderTarget& target, const Camera& camera, const SceneFrame& frame,
                    const std::vector<emma::Body>& bodies);
    void drawFlashes(sf::RenderTarget& target, const Camera& camera, const SceneFrame& frame,
                     const std::vector<emma::MergeFlash>& flashes) const;
    // A dashed line between two points in scene coordinates
    void drawRay(sf::RenderTarget& target, const Camera& camera, const emma::Vector3D& from,
                 const emma::Vector3D& to, sf::Color color);
    void drawPath(sf::RenderTarget& target, const Camera& camera, const std::vector<emma::Vector3D>& path,
                  emma::Rgb color);
    void drawGhost(sf::RenderTarget& target, sf::Vector2f at, float radius, emma::Rgb color) const;
    void drawArrow(sf::RenderTarget& target, sf::Vector2f from, sf::Vector2f to, sf::Color color) const;

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

    std::vector<Star> sky;
    sf::VertexArray stars{sf::PrimitiveType::Triangles};
    sf::VertexArray lines{sf::PrimitiveType::Lines};
    std::vector<std::size_t> order;
    std::vector<std::size_t> drawn;
    std::vector<sf::Vector2f> screen;
    std::vector<float> radii;
    std::vector<double> depth;
    std::vector<char> visible;
};
