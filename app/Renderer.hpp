#pragma once
#include "Camera.hpp"
#include "emma/Body.hpp"
#include "emma/Simulation.hpp"
#include <SFML/Graphics.hpp>
#include <cstddef>
#include <vector>

// Draws the scene with SFML. Everything is converted to pixels through the Camera.
class Renderer {
public:
    Renderer();

    void drawStarfield(sf::RenderTarget& target) const;
    void drawTrails(sf::RenderTarget& target, const Camera& camera, const std::vector<emma::Body>& bodies);
    void drawBodies(sf::RenderTarget& target, const Camera& camera, const std::vector<emma::Body>& bodies);
    void drawFlashes(sf::RenderTarget& target, const Camera& camera,
                     const std::vector<emma::MergeFlash>& flashes) const;
    void drawPath(sf::RenderTarget& target, const Camera& camera, const std::vector<emma::Vector2D>& path,
                  emma::Rgb color);
    void drawGhost(sf::RenderTarget& target, sf::Vector2f at, float radius, emma::Rgb color) const;
    void drawArrow(sf::RenderTarget& target, sf::Vector2f from, sf::Vector2f to, sf::Color color) const;

    // On-screen radius in pixels: the true size once you zoom in far enough,
    // otherwise a marker that grows with the logarithm of the mass
    static float screenRadius(double mass, double radiusAU, double pxPerAU);

private:
    sf::VertexArray stars{sf::PrimitiveType::Triangles};
    sf::VertexArray lines{sf::PrimitiveType::Lines};
    std::vector<std::size_t> drawOrder;
    std::vector<sf::Vector2f> screen;
    std::vector<float> radii;
};
