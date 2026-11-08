#include "Renderer.hpp"
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <numeric>
#include <random>

namespace {

sf::Color toColor(Rgb c, std::uint8_t alpha = 255) {
    return sf::Color(c.r, c.g, c.b, alpha);
}

void drawDisc(sf::RenderTarget& target, sf::Vector2f at, float radius, sf::Color fill) {
    const std::size_t points = static_cast<std::size_t>(std::clamp(radius * 0.5f, 16.f, 256.f));
    sf::CircleShape disc(radius, points);
    disc.setOrigin(sf::Vector2f(radius, radius));
    disc.setPosition(at);
    disc.setFillColor(fill);
    target.draw(disc);
}

}

Renderer::Renderer() {
    // Background stars are effectively infinitely far away, so they stay fixed on
    // screen while the camera pans and zooms. Fixed seed: the same sky every run.
    std::mt19937 rng(1337u);
    std::uniform_real_distribution<float> px(0.f, 3840.f), py(0.f, 2160.f), size(0.5f, 1.4f);
    std::uniform_int_distribution<int> bright(60, 210), tint(0, 2);

    for (int i = 0; i < 6000; ++i) {
        const float x = px(rng), y = py(rng), s = size(rng);
        const int b = bright(rng);
        sf::Color c(static_cast<std::uint8_t>(b), static_cast<std::uint8_t>(b), static_cast<std::uint8_t>(b));
        switch (tint(rng)) {
        case 0: c.b = static_cast<std::uint8_t>(std::min(255, b + 35)); break;  // hot, blue-white
        case 1: c.r = static_cast<std::uint8_t>(std::min(255, b + 35)); break;  // cool, orange
        default: break;
        }
        const sf::Vector2f p0(x - s, y - s), p1(x + s, y - s), p2(x + s, y + s), p3(x - s, y + s);
        for (const sf::Vector2f& p : {p0, p1, p2, p0, p2, p3}) stars.append(sf::Vertex{p, c});
    }
}

void Renderer::drawStarfield(sf::RenderTarget& target) const {
    target.draw(stars);
}

float Renderer::screenRadius(double mass, double radiusAU, double pxPerAU) {
    const double trueRadius = radiusAU * pxPerAU;
    const double marker = std::clamp(1.5 + 0.6 * (std::log10(std::max(mass, 1e-12)) + 10.0), 1.5, 9.0);
    return static_cast<float>(std::max(trueRadius, marker));
}

void Renderer::drawTrails(sf::RenderTarget& target, const Camera& camera, const std::vector<Body>& bodies) {
    lines.clear();
    for (const Body& b : bodies) {
        const std::size_t n = b.trail.size();
        if (n == 0) continue;
        auto alpha = [n](std::size_t k) { return static_cast<std::uint8_t>(170 * k / n); };
        sf::Vector2f prev = camera.toScreen(b.trail.front());
        for (std::size_t i = 1; i <= n; ++i) {
            // the last segment joins the newest sample to the body itself
            const sf::Vector2f cur = camera.toScreen(i < n ? b.trail[i] : b.position);
            lines.append(sf::Vertex{prev, toColor(b.color, alpha(i - 1))});
            lines.append(sf::Vertex{cur, toColor(b.color, alpha(i))});
            prev = cur;
        }
    }
    target.draw(lines);
}

void Renderer::drawBodies(sf::RenderTarget& target, const Camera& camera, const std::vector<Body>& bodies) {
    const std::size_t n = bodies.size();

    // Heaviest first, so lighter bodies land on top
    drawOrder.resize(n);
    std::iota(drawOrder.begin(), drawOrder.end(), std::size_t{0});
    std::sort(drawOrder.begin(), drawOrder.end(),
              [&bodies](std::size_t a, std::size_t b) { return bodies[a].mass > bodies[b].mass; });

    screen.resize(n);
    radii.resize(n);
    for (std::size_t i = 0; i < n; ++i) {
        screen[i] = camera.toScreen(bodies[i].position);
        radii[i] = screenRadius(bodies[i].mass, bodies[i].radius, camera.pxPerAU);
    }

    const sf::FloatRect& vp = camera.viewport;
    for (std::size_t rank = 0; rank < n; ++rank) {
        const std::size_t i = drawOrder[rank];
        const Body& b = bodies[i];
        const sf::Vector2f p = screen[i];
        const float r = radii[i];
        const float reach = r * 3.f;
        if (p.x + reach < vp.position.x || p.x - reach > vp.position.x + vp.size.x ||
            p.y + reach < vp.position.y || p.y - reach > vp.position.y + vp.size.y)
            continue;

        // At low zoom a moon sits inside its planet's marker; hide it until they separate
        bool hidden = false;
        for (std::size_t k = 0; k < std::min<std::size_t>(rank, 32); ++k) {
            const std::size_t j = drawOrder[k];
            const float dx = screen[j].x - p.x, dy = screen[j].y - p.y;
            if (dx * dx + dy * dy < radii[j] * radii[j]) {
                hidden = true;
                break;
            }
        }
        if (hidden) continue;

        if (Body::classify(b.mass) == BodyClass::Star) {
            // a soft halo so stars read as light sources
            drawDisc(target, p, r * 2.8f, toColor(b.color, 16));
            drawDisc(target, p, r * 1.9f, toColor(b.color, 36));
        }
        drawDisc(target, p, r, toColor(b.color));
    }
}

void Renderer::drawFlashes(sf::RenderTarget& target, const Camera& camera,
                           const std::vector<MergeFlash>& flashes) const {
    for (const MergeFlash& f : flashes) {
        const float t = std::clamp(f.age / Simulation::FLASH_LIFETIME, 0.f, 1.f);
        const float r = 5.f + 30.f * t;
        sf::CircleShape ring(r, 48);
        ring.setOrigin(sf::Vector2f(r, r));
        ring.setPosition(camera.toScreen(f.position));
        ring.setFillColor(sf::Color::Transparent);
        ring.setOutlineThickness(2.f);
        ring.setOutlineColor(sf::Color(255, 255, 255, static_cast<std::uint8_t>(255.f * (1.f - t))));
        target.draw(ring);
    }
}

void Renderer::drawPath(sf::RenderTarget& target, const Camera& camera, const std::vector<Vector2D>& path,
                        Rgb color) {
    if (path.size() < 2) return;
    // Every other segment, so the line is dashed. The points are equally spaced in
    // time, so long dashes mean the body is moving fast.
    lines.clear();
    const sf::Color c = toColor(color, 200);
    for (std::size_t i = 1; i < path.size(); i += 2) {
        lines.append(sf::Vertex{camera.toScreen(path[i - 1]), c});
        lines.append(sf::Vertex{camera.toScreen(path[i]), c});
    }
    target.draw(lines);
}

void Renderer::drawGhost(sf::RenderTarget& target, sf::Vector2f at, float radius, Rgb color) const {
    sf::CircleShape ghost(radius, 32);
    ghost.setOrigin(sf::Vector2f(radius, radius));
    ghost.setPosition(at);
    ghost.setFillColor(toColor(color, 70));
    ghost.setOutlineThickness(1.f);
    ghost.setOutlineColor(toColor(color, 200));
    target.draw(ghost);
}

void Renderer::drawArrow(sf::RenderTarget& target, sf::Vector2f from, sf::Vector2f to, sf::Color color) const {
    const sf::Vector2f d = to - from;
    const float len = std::sqrt(d.x * d.x + d.y * d.y);
    if (len < 1.f) return;
    const sf::Vector2f u = d / len;
    const sf::Vector2f n(-u.y, u.x);
    const float head = std::min(10.f, len * 0.5f);
    const sf::Vector2f base = to - u * head;
    const float w = 1.f;  // half the shaft width

    const sf::Vertex shaft[] = {
        sf::Vertex{from + n * w, color}, sf::Vertex{base + n * w, color}, sf::Vertex{base - n * w, color},
        sf::Vertex{from + n * w, color}, sf::Vertex{base - n * w, color}, sf::Vertex{from - n * w, color},
    };
    target.draw(shaft, 6, sf::PrimitiveType::Triangles);

    const sf::Vertex tip[] = {
        sf::Vertex{to, color},
        sf::Vertex{base + n * (head * 0.45f), color},
        sf::Vertex{base - n * (head * 0.45f), color},
    };
    target.draw(tip, 3, sf::PrimitiveType::Triangles);
}
