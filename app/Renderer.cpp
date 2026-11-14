#include "Renderer.hpp"
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <numeric>
#include <optional>
#include <random>

using namespace emma;

namespace {

constexpr int STAR_COUNT = 12000;  // over the whole sky, about 700 in view at once
constexpr int BAND_COUNT = 9000;   // crowded into the Milky Way along the Galactic plane
constexpr int GRID_LINES = 20;     // grid lines on each side of the middle
const sf::Color GRID_COLOR(120, 126, 165);

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

// The smallest 1, 2 or 5 times a power of ten that is at least raw
double niceStep(double raw) {
    const double decade = std::pow(10.0, std::floor(std::log10(raw)));
    for (const double m : {1.0, 2.0, 5.0})
        if (m * decade >= raw) return m * decade;
    return 10.0 * decade;
}

}

Renderer::Renderer() {
    // Background stars are effectively infinitely far away, so they turn with the
    // view but stay put while the camera pans and zooms. They are made in Galactic
    // coordinates: most spread evenly over the sky, the rest crowded into the band of
    // the Milky Way along the Galactic plane. Fixed seed: the same sky every run.
    std::mt19937 rng(1337u);
    std::normal_distribution<double> axis(0.0, 1.0);  // three normals make a direction uniform over the sky
    std::normal_distribution<double> latitude(0.0, 5.0 * units::DEG);  // how far band stars stray from the plane
    std::uniform_real_distribution<double> longitude(0.0, 2.0 * units::PI);
    std::uniform_real_distribution<float> size(0.5f, 1.4f), bandSize(0.4f, 0.9f);
    std::uniform_int_distribution<int> bright(60, 210), dim(35, 110), tint(0, 2);

    auto add = [&](const Vector3D& galactic, int b, float s) {
        sf::Color c(static_cast<std::uint8_t>(b), static_cast<std::uint8_t>(b), static_cast<std::uint8_t>(b));
        switch (tint(rng)) {
        case 0: c.b = static_cast<std::uint8_t>(std::min(255, b + 35)); break;  // hot, blue-white
        case 1: c.r = static_cast<std::uint8_t>(std::min(255, b + 35)); break;  // cool, orange
        default: break;
        }
        sky.push_back(Star{galactic, galaxy::toEcliptic(galactic), c, s});
    };

    sky.reserve(STAR_COUNT + BAND_COUNT);
    for (int i = 0; i < STAR_COUNT; ++i) {
        const double x = axis(rng), y = axis(rng), z = axis(rng);
        const int b = bright(rng);
        add(Vector3D(x, y, z).normalized(), b, size(rng));
    }
    for (int i = 0; i < BAND_COUNT; ++i) {
        const double l = longitude(rng), b = latitude(rng);
        const int shade = dim(rng);
        add(Vector3D(std::cos(b) * std::cos(l), std::cos(b) * std::sin(l), std::sin(b)), shade, bandSize(rng));
    }
}

void Renderer::drawStarfield(sf::RenderTarget& target, const Camera& camera, const SceneFrame& frame) {
    stars.clear();
    for (const Star& star : sky) {
        const std::optional<sf::Vector2f> at = camera.toScreenDirection(frame.galactic ? star.galactic : star.ecliptic);
        if (!at) continue;
        const float x = at->x, y = at->y, s = star.size;
        const sf::Vector2f p0(x - s, y - s), p1(x + s, y - s), p2(x + s, y + s), p3(x - s, y + s);
        for (const sf::Vector2f& p : {p0, p1, p2, p0, p2, p3}) stars.append(sf::Vertex{p, star.color});
    }
    target.draw(stars);
}

void Renderer::addSegment(const Camera& camera, Vector3D a, Vector3D b, sf::Color ca, sf::Color cb) {
    if (!camera.clip(a, b)) return;
    lines.append(sf::Vertex{camera.project(a), ca});
    lines.append(sf::Vertex{camera.project(b), cb});
}

void Renderer::drawGrid(sf::RenderTarget& target, const Camera& camera) {
    // Square grid on the plane through the target, parallel to the ecliptic. The
    // spacing follows the zoom, and the lines fade out away from the middle
    const double step = niceStep(camera.distance / 5.0);
    const Vector3D& t = camera.target;
    const double cx = std::round(t.x / step) * step, cy = std::round(t.y / step) * step;
    const double reach = GRID_LINES * step;
    auto shade = [&](double x, double y) {
        const double fade = std::max(0.0, 1.0 - std::hypot(x - cx, y - cy) / reach);
        return sf::Color(GRID_COLOR.r, GRID_COLOR.g, GRID_COLOR.b, static_cast<std::uint8_t>(40.0 * fade));
    };

    lines.clear();
    for (int k = -GRID_LINES; k <= GRID_LINES; ++k) {
        const double across = k * step;
        for (int j = -GRID_LINES; j < GRID_LINES; ++j) {
            const double from = j * step, to = (j + 1) * step;
            addSegment(camera, camera.toCamera(Vector3D(cx + across, cy + from, t.z)),
                       camera.toCamera(Vector3D(cx + across, cy + to, t.z)), shade(cx + across, cy + from),
                       shade(cx + across, cy + to));
            addSegment(camera, camera.toCamera(Vector3D(cx + from, cy + across, t.z)),
                       camera.toCamera(Vector3D(cx + to, cy + across, t.z)), shade(cx + from, cy + across),
                       shade(cx + to, cy + across));
        }
    }
    target.draw(lines);
}

float Renderer::screenRadius(double mass, double radiusAU, double pxPerAU) {
    const double trueRadius = radiusAU * pxPerAU;
    const double marker = std::clamp(1.5 + 0.6 * (std::log10(std::max(mass, 1e-12)) + 10.0), 1.5, 9.0);
    return static_cast<float>(std::max(trueRadius, marker));
}

void Renderer::drawTrails(sf::RenderTarget& target, const Camera& camera, const SceneFrame& frame,
                          const std::vector<Body>& bodies) {
    lines.clear();
    for (const Body& b : bodies) {
        const std::size_t n = b.trail.size();
        if (n == 0) continue;
        auto alpha = [n](std::size_t k) { return static_cast<std::uint8_t>(170 * k / n); };
        // Each point is placed at the moment it was recorded, so in the galaxy view the
        // trail stretches out along the Sun's path
        auto at = [&](std::size_t i) {
            return i < n ? frame.place(b.trail[i].position, b.trail[i].time) : frame.place(b.position);
        };
        Vector3D prev = camera.toCamera(at(0));
        for (std::size_t i = 1; i <= n; ++i) {
            // the last segment joins the newest sample to the body itself
            const Vector3D cur = camera.toCamera(at(i));
            addSegment(camera, prev, cur, toColor(b.color, alpha(i - 1)), toColor(b.color, alpha(i)));
            prev = cur;
        }
    }
    target.draw(lines);
}

void Renderer::drawBodies(sf::RenderTarget& target, const Camera& camera, const SceneFrame& frame,
                          const std::vector<Body>& bodies) {
    const std::size_t n = bodies.size();
    screen.resize(n);
    radii.assign(n, 0.f);
    depth.resize(n);
    visible.assign(n, 0);
    for (std::size_t i = 0; i < n; ++i) {
        const Vector3D c = camera.toCamera(frame.place(bodies[i].position));
        depth[i] = c.z;
        if (!camera.inFront(c)) continue;
        screen[i] = camera.project(c);
        radii[i] = screenRadius(bodies[i].mass, bodies[i].radius, camera.focal() / c.z);
        visible[i] = 1;
    }

    // At low zoom a moon sits inside its planet's marker; hide it until they separate.
    // Checked heaviest first, against the heaviest bodies
    order.resize(n);
    std::iota(order.begin(), order.end(), std::size_t{0});
    std::sort(order.begin(), order.end(),
              [&bodies](std::size_t a, std::size_t b) { return bodies[a].mass > bodies[b].mass; });

    const sf::FloatRect& vp = camera.viewport;
    drawn.clear();
    for (std::size_t rank = 0; rank < n; ++rank) {
        const std::size_t i = order[rank];
        if (!visible[i]) continue;
        const sf::Vector2f p = screen[i];
        const float reach = radii[i] * 3.f;
        if (p.x + reach < vp.position.x || p.x - reach > vp.position.x + vp.size.x ||
            p.y + reach < vp.position.y || p.y - reach > vp.position.y + vp.size.y)
            continue;

        bool hidden = false;
        for (std::size_t k = 0; k < std::min<std::size_t>(rank, 32); ++k) {
            const std::size_t j = order[k];
            if (!visible[j]) continue;
            const float dx = screen[j].x - p.x, dy = screen[j].y - p.y;
            if (dx * dx + dy * dy < radii[j] * radii[j]) {
                hidden = true;
                break;
            }
        }
        if (!hidden) drawn.push_back(i);
    }

    // Farthest first, so nearer bodies land on top
    std::sort(drawn.begin(), drawn.end(), [this](std::size_t a, std::size_t b) { return depth[a] > depth[b]; });
    for (const std::size_t i : drawn) {
        const Body& b = bodies[i];
        const sf::Vector2f p = screen[i];
        const float r = radii[i];
        if (Body::classify(b.mass) == BodyClass::Star) {
            // a soft halo so stars read as light sources
            drawDisc(target, p, r * 2.8f, toColor(b.color, 16));
            drawDisc(target, p, r * 1.9f, toColor(b.color, 36));
        }
        drawDisc(target, p, r, toColor(b.color));
    }
}

void Renderer::drawFlashes(sf::RenderTarget& target, const Camera& camera, const SceneFrame& frame,
                           const std::vector<MergeFlash>& flashes) const {
    for (const MergeFlash& f : flashes) {
        const std::optional<sf::Vector2f> at = camera.toScreen(frame.place(f.position));
        if (!at) continue;
        const float t = std::clamp(f.age / Simulation::FLASH_LIFETIME, 0.f, 1.f);
        const float r = 5.f + 30.f * t;
        sf::CircleShape ring(r, 48);
        ring.setOrigin(sf::Vector2f(r, r));
        ring.setPosition(*at);
        ring.setFillColor(sf::Color::Transparent);
        ring.setOutlineThickness(2.f);
        ring.setOutlineColor(sf::Color(255, 255, 255, static_cast<std::uint8_t>(255.f * (1.f - t))));
        target.draw(ring);
    }
}

void Renderer::drawRay(sf::RenderTarget& target, const Camera& camera, const Vector3D& from, const Vector3D& to,
                       sf::Color color) {
    constexpr int DASHES = 16;
    lines.clear();
    const Vector3D step = (to - from) / (2.0 * DASHES);
    for (int k = 0; k < 2 * DASHES; k += 2)
        addSegment(camera, camera.toCamera(from + step * k), camera.toCamera(from + step * (k + 1)), color, color);
    target.draw(lines);
}

void Renderer::drawPath(sf::RenderTarget& target, const Camera& camera, const std::vector<Vector3D>& path,
                        Rgb color) {
    if (path.size() < 2) return;
    // Every other segment, so the line is dashed. The points are equally spaced in
    // time, so long dashes mean the body is moving fast.
    lines.clear();
    const sf::Color c = toColor(color, 200);
    for (std::size_t i = 1; i < path.size(); i += 2)
        addSegment(camera, camera.toCamera(path[i - 1]), camera.toCamera(path[i]), c, c);
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
