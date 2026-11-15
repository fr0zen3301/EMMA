#include "Renderer.hpp"
#include "BlackHoleShader.hpp"
#include "SurfaceShader.hpp"
#include "emma/BlackHole.hpp"
#include "emma/Galaxy.hpp"
#include "emma/Spin.hpp"
#include <algorithm>
#include <cctype>
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

// Sgr A*
constexpr float HOLE_MARKER = 6.4f;        // px, the shadow's radius in the far-away marker. With its ring
                                           // it's 9 px, the cap screenRadius gives a body this heavy
constexpr double TRACE_FROM_PX = 1.0;      // ray trace once the true shadow's radius is at least this many px
constexpr float SETTLE_SECONDS = 0.25f;    // full resolution once the view has stayed still this long
constexpr double SKY_FOCAL = 1050.0;       // px, the default window's focal length: sizes the stars in the sky map

// Real surfaces
constexpr double SURFACE_FROM_PX = 3.0;    // a body's true radius on screen before its texture is drawn
constexpr float SATURN_RING_IN = 1.2388f;  // Saturn's radii: the C ring's inner edge, 74,658 km
constexpr float SATURN_RING_OUT = 2.2695f; // and the A ring's outer edge, 136,775 km
constexpr float MOON_RELIEF = 0.0115f;     // the height map's range in Moon radii, assuming it spans the
                                           // Moon's full 20 km from lowest to highest

// The glow of hot gas around Sgr A*, from dim red (g = 0) through orange (1) to near white
sf::Color holeColor(double g, std::uint8_t alpha = 255) {
    g = std::max(0.0, g);
    double r, gr, b;
    if (g <= 1.0) {
        r = 110.0 + 145.0 * g;
        gr = 35.0 + 125.0 * g;
        b = 12.0 + 48.0 * g;
    } else {
        const double k = std::min(1.0, (g - 1.0) / 0.6);
        r = 255.0;
        gr = 160.0 + 76.0 * k;
        b = 60.0 + 140.0 * k;
    }
    return sf::Color(static_cast<std::uint8_t>(r), static_cast<std::uint8_t>(gr), static_cast<std::uint8_t>(b), alpha);
}

sf::Glsl::Vec3 glsl(const Vector3D& v) {
    return sf::Glsl::Vec3(static_cast<float>(v.x), static_cast<float>(v.y), static_cast<float>(v.z));
}

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
    // the Milky Way along the Galactic plane, fixed seed
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
                          const std::vector<Body>& bodies, double julianDay) {
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

    // The Sun lights the planets' surfaces
    std::optional<Vector3D> sun;
    for (const Body& b : bodies)
        if (b.name == "Sun") sun = frame.place(b.position);

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
        // Once the true size has taken over from the marker, the real surface
        const double trueRadius = b.radius * camera.focal() / depth[i];
        const bool surface = trueRadius >= SURFACE_FROM_PX && trueRadius + 1e-3 >= r &&
                             drawSurface(target, camera, frame, b, p, static_cast<float>(trueRadius), sun, julianDay);
        if (!surface) drawDisc(target, p, r, toColor(b.color));
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

// ---------------------------------------------------------------------------- Real surfaces

std::optional<sf::Texture> Renderer::loadTexture(const std::string& file, bool wraps) {
    namespace fs = std::filesystem;
    for (const fs::path& path : {fs::path("textures") / file, assetFolder / "textures" / file}) {
        std::error_code ec;
        if (!fs::exists(path, ec)) continue;
        sf::Texture texture;
        if (!texture.loadFromFile(path)) continue;
        texture.setSmooth(true);
        if (wraps) {
            // Maps wrap around in longitude, and smaller copies keep a distant planet from shimmering
            texture.setRepeated(true);
            static_cast<void>(texture.generateMipmap());
        }
        return texture;
    }
    return std::nullopt;
}

const Renderer::Surface& Renderer::surfaceFor(const std::string& name) {
    Surface& maps = surfaces[name];
    if (!maps.tried) {
        maps.tried = true;
        std::string base = name;
        std::transform(base.begin(), base.end(), base.begin(),
                       [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
        maps.color = loadTexture(base + ".jpg", true);
        maps.night = loadTexture(base + "_night.jpg", true);
        maps.clouds = loadTexture(base + "_clouds.jpg", true);
        maps.height = loadTexture(base + "_height.jpg", true);
        maps.rings = loadTexture(base + "_ring.png", false);
    }
    return maps;
}

bool Renderer::prepareSurfaceShader() {
    if (surfaceState == TraceState::Untried) {
        surfaceState = TraceState::Failed;
        if (sf::Shader::isAvailable() && surfaceShader.loadFromMemory(SURFACE_SHADER, sf::Shader::Type::Fragment))
            surfaceState = TraceState::Ready;
    }
    return surfaceState == TraceState::Ready;
}

bool Renderer::drawSurface(sf::RenderTarget& target, const Camera& camera, const SceneFrame& frame, const Body& body,
                           sf::Vector2f at, float radiusPx, const std::optional<Vector3D>& sun, double julianDay) {
    const spin::Model* model = spin::find(body.name);
    if (!model) return false;
    const Surface& maps = surfaceFor(body.name);
    if (!maps.color || !prepareSurfaceShader()) return false;

    // The body's axes now, turned the way the picture is. The shader works in them
    const spin::Axes axes = spin::eclipticAxes(*model, julianDay);
    const Vector3D ax = frame.direction(axes.x), ay = frame.direction(axes.y), az = frame.direction(axes.z);
    auto local = [&](const Vector3D& v) { return Vector3D(v.dot(ax), v.dot(ay), v.dot(az)); };

    // Lengths in the equatorial radius. Body::radius is the mean radius: the one of a
    // sphere with the same volume, so equatorial = mean / cbrt(polar / equatorial)
    const double polar = 1.0 - model->flattening;
    const double equatorial = body.radius / std::cbrt(polar);
    const Vector3D center = frame.place(body.position);
    const Vector3D eye = (camera.target - center) + camera.eyeOffset();  // subtracted in double precision
    const double eyeDistance = eye.magnitude();
    const bool isSun = body.name == "Sun";
    Vector3D light = eye.normalized();  // with no Sun, lit from the eye
    if (sun && !isSun && (*sun - center).lengthSquared() > 0.0) light = (*sun - center).normalized();

    // The square to draw on: the body and its rings, or the whole view when the eye is close
    const bool rings = maps.rings.has_value();
    const double extent = rings ? SATURN_RING_OUT : 1.0;  // equatorial radii
    sf::FloatRect area = camera.viewport;
    if (eyeDistance > 4.0 * extent * equatorial) {
        const float reach = static_cast<float>(radiusPx / std::cbrt(polar) * extent) + 3.f;
        area = sf::FloatRect(at - sf::Vector2f(reach, reach), sf::Vector2f(2.f * reach, 2.f * reach));
    }

    sf::Shader& s = surfaceShader;
    const sf::Vector2f m = camera.middle();
    s.setUniform("uCenter", sf::Glsl::Vec2(m.x, static_cast<float>(target.getSize().y) - m.y));
    s.setUniform("uFocal", static_cast<float>(camera.focal()));
    s.setUniform("uEye", glsl(local(eye) / equatorial));
    s.setUniform("uRight", glsl(local(camera.rightAxis())));
    s.setUniform("uUp", glsl(local(camera.upAxis())));
    s.setUniform("uForward", glsl(local(camera.forwardAxis())));
    s.setUniform("uLight", glsl(local(light)));
    s.setUniform("uPolar", static_cast<float>(polar));
    s.setUniform("uPixel", static_cast<float>(eyeDistance / (camera.focal() * equatorial)));
    s.setUniform("uSun", isSun ? 1.f : 0.f);
    s.setUniform("uColor", *maps.color);
    s.setUniform("uHasNight", maps.night ? 1.f : 0.f);
    if (maps.night) s.setUniform("uNight", *maps.night);
    s.setUniform("uHasClouds", maps.clouds ? 1.f : 0.f);
    if (maps.clouds) s.setUniform("uClouds", *maps.clouds);
    s.setUniform("uHasHeight", maps.height ? 1.f : 0.f);
    if (maps.height) {
        const sf::Vector2u size = maps.height->getSize();
        s.setUniform("uHeight", *maps.height);
        s.setUniform("uHeightTexel", sf::Glsl::Vec2(1.f / static_cast<float>(size.x), 1.f / static_cast<float>(size.y)));
    } else {
        s.setUniform("uHeightTexel", sf::Glsl::Vec2(0.f, 0.f));
    }
    s.setUniform("uRelief", MOON_RELIEF);
    s.setUniform("uHasRings", rings ? 1.f : 0.f);
    if (rings) s.setUniform("uRings", *maps.rings);
    s.setUniform("uRingIn", SATURN_RING_IN);
    s.setUniform("uRingOut", SATURN_RING_OUT);

    sf::VertexArray quad(sf::PrimitiveType::TriangleStrip, 4);
    quad[0].position = area.position;
    quad[1].position = area.position + sf::Vector2f(area.size.x, 0.f);
    quad[2].position = area.position + sf::Vector2f(0.f, area.size.y);
    quad[3].position = area.position + area.size;
    sf::RenderStates states;
    states.shader = &surfaceShader;
    target.draw(quad, states);
    return true;
}

// ---------------------------------------------------------------------------- Sgr A*

Renderer::BlackHoleShot Renderer::drawBlackHole(sf::RenderTarget& target, const Camera& camera,
                                                const SceneFrame& frame, const BlackHoleLook& look) {
    BlackHoleShot shot;
    const double rs = blackhole::schwarzschildRadiusAU(galaxy::SGR_A_MASS);
    const Vector3D center = frame.galacticCenter();
    // The eye relative to the hole. The two big positions are subtracted first, in double
    // precision, so nothing 1.7 billion AU from the origin ever reaches the graphics card
    const Vector3D eye = (camera.target - center) + camera.eyeOffset();
    const double distance = std::max(eye.magnitude() / rs, 1.01);  // Rs
    const double focal = camera.focal();

    // The shadow's true radius on screen, from its angular size seen from the eye
    const double sine =
        std::min(1.0, blackhole::SHADOW_RADIUS / distance * std::sqrt(1.0 - 1.0 / distance));
    const double shadowPx = focal * std::tan(std::min(std::asin(sine), 1.45));

    // Up close the bending shows all around, even with the hole off to the side or behind
    if (shadowPx >= TRACE_FROM_PX) shot.rayTraced = traceBlackHole(target, camera, frame, look, eye / rs, distance);

    const Vector3D c = camera.toCamera(center);
    if (!camera.inFront(c)) return shot;
    const sf::Vector2f p = camera.project(c);
    const float r = std::max(static_cast<float>(shadowPx), HOLE_MARKER);
    shot.visible = true;
    shot.at = p;
    shot.radius = 1.4f * r;
    shot.eyeDistance = distance * rs;
    if (!shot.rayTraced) {
        const Vector3D axis = frame.fromGalactic(galaxy::sgrASpinAxis());
        drawFlatBlackHole(target, p, r, axis.dot(camera.rightAxis()), axis.dot(camera.upAxis()));
    }
    return shot;
}

bool Renderer::prepareRayTracing() {
    if (traceState == TraceState::Untried) {
        traceState = TraceState::Failed;
        if (sf::Shader::isAvailable() &&
            blackHoleShader.loadFromMemory(BLACK_HOLE_SHADER, sf::Shader::Type::Fragment) && bakeSky())
            traceState = TraceState::Ready;
    }
    return traceState == TraceState::Ready;
}

bool Renderer::bakeSky() {
    // The same stars as the starfield, laid out with Galactic longitude across and
    // latitude down, so the shader can look up the sky in any direction a bent ray ends
    // up pointing, including off screen. Done once, the first time it's needed
    const unsigned width = sf::Texture::getMaximumSize() >= 4096u ? 4096u : 2048u;
    const unsigned height = width / 2;
    sf::RenderTexture canvas;
    if (!canvas.resize({width, height})) return false;
    canvas.clear(SPACE_COLOR);

    const double texel = 2.0 * units::PI / width;  // radians per texel
    sf::VertexArray quads(sf::PrimitiveType::Triangles);
    for (const Star& star : sky) {
        const Vector3D& d = star.galactic;
        const double lon = std::atan2(d.y, d.x);
        const double lat = std::asin(std::clamp(d.z, -1.0, 1.0));
        // Each star covers at least the texel it falls in, so none is too small to land on one
        const float cx = std::floor(static_cast<float>((lon / (2.0 * units::PI) + 0.5) * width)) + 0.5f;
        const float cy = std::floor(static_cast<float>((0.5 - lat / units::PI) * height)) + 0.5f;
        const float half = std::max(0.5f, static_cast<float>(star.size / SKY_FOCAL / texel));
        // The map stretches the sky toward the poles, so stars there are drawn wider
        const float wide = std::min(64.f, half / static_cast<float>(std::max(0.05, std::cos(lat))));
        const sf::Vector2f p0(cx - wide, cy - half), p1(cx + wide, cy - half), p2(cx + wide, cy + half),
            p3(cx - wide, cy + half);
        for (const sf::Vector2f& p : {p0, p1, p2, p0, p2, p3}) quads.append(sf::Vertex{p, star.color});
    }
    canvas.draw(quads);
    canvas.display();

    // Copied into a plain texture, which has its first row at the top, as the shader expects
    skyMap.emplace();
    if (!skyMap->loadFromImage(canvas.getTexture().copyToImage())) {
        skyMap.reset();
        return false;
    }
    skyMap->setSmooth(true);
    skyMap->setRepeated(true);  // longitude wraps around
    return true;
}

bool Renderer::traceBlackHole(sf::RenderTarget& target, const Camera& camera, const SceneFrame& frame,
                              const BlackHoleLook& look, const Vector3D& eyeRs, double distanceRs) {
    if (!prepareRayTracing()) return false;

    // Everything in Galactic coordinates, the frame of the sky map
    const Vector3D eye = frame.toGalactic(eyeRs);
    const Vector3D right = frame.toGalactic(camera.rightAxis());
    const Vector3D up = frame.toGalactic(camera.upAxis());
    const Vector3D forward = frame.toGalactic(camera.forwardAxis());
    const sf::FloatRect& vp = camera.viewport;

    // Render again only when the picture would change
    const std::array<double, 16> key = {eye.x / distanceRs,
                                        eye.y / distanceRs,
                                        eye.z / distanceRs,
                                        std::log(distanceRs),
                                        right.x,
                                        right.y,
                                        right.z,
                                        up.x,
                                        up.y,
                                        up.z,
                                        static_cast<double>(look.look),
                                        look.bendLight ? 1.0 : 0.0,
                                        look.doppler ? 1.0 : 0.0,
                                        static_cast<double>(look.brightness),
                                        static_cast<double>(vp.size.x),
                                        static_cast<double>(vp.size.y)};
    bool changed = !tracedOnce;
    for (std::size_t i = 0; i < key.size() && !changed; ++i) changed = std::abs(key[i] - traceKey[i]) > 1e-9;
    if (changed) {
        traceKey = key;
        sinceChanged.restart();
    }
    // Half resolution while the view keeps changing, so turning and zooming stay smooth
    const float scale = sinceChanged.getElapsedTime().asSeconds() > SETTLE_SECONDS ? 1.f : 0.5f;

    if (changed || scale != tracedScale) {
        const sf::Vector2u size(std::max(1u, static_cast<unsigned>(std::lround(vp.size.x * scale))),
                                std::max(1u, static_cast<unsigned>(std::lround(vp.size.y * scale))));
        if (traced.getSize() != size) {
            if (!traced.resize(size)) {
                traceState = TraceState::Failed;
                return false;
            }
            traced.setSmooth(true);
        }
        const float w = static_cast<float>(size.x), h = static_cast<float>(size.y);

        const Vector3D axis = galaxy::sgrASpinAxis();
        const Vector3D axis1 = Vector3D(0.0, 0.0, 1.0).cross(axis).normalized();
        const Vector3D axis2 = axis.cross(axis1);

        sf::Shader& s = blackHoleShader;
        s.setUniform("uCenter", sf::Glsl::Vec2(0.5f * w, 0.5f * h));
        s.setUniform("uFocal", static_cast<float>(camera.focal() * h / vp.size.y));
        s.setUniform("uEye", glsl(eye));
        s.setUniform("uRight", glsl(right));
        s.setUniform("uUp", glsl(up));
        s.setUniform("uForward", glsl(forward));
        s.setUniform("uAxis", glsl(axis));
        s.setUniform("uAxis1", glsl(axis1));
        s.setUniform("uAxis2", glsl(axis2));
        s.setUniform("uLook", static_cast<float>(look.look));
        s.setUniform("uBend", look.bendLight ? 1.f : 0.f);
        s.setUniform("uDoppler", look.doppler ? 1.f : 0.f);
        s.setUniform("uExposure", static_cast<float>(std::pow(2.0, static_cast<double>(look.brightness))));
        s.setUniform("uEyeShift", static_cast<float>(1.0 / std::sqrt(1.0 - 1.0 / distanceRs)));
        s.setUniform("uEscape", static_cast<float>(std::max(1.1 * distanceRs, 40.0)));
        s.setUniform("uSky", *skyMap);

        // One quad over the whole picture: the shader works out every pixel
        sf::VertexArray quad(sf::PrimitiveType::TriangleStrip, 4);
        quad[0].position = sf::Vector2f(0.f, 0.f);
        quad[1].position = sf::Vector2f(w, 0.f);
        quad[2].position = sf::Vector2f(0.f, h);
        quad[3].position = sf::Vector2f(w, h);
        sf::RenderStates states;
        states.shader = &blackHoleShader;
        traced.clear(sf::Color::Black);
        traced.draw(quad, states);
        traced.display();
        tracedScale = scale;
        tracedOnce = true;
    }

    sf::Sprite picture(traced.getTexture());
    const sf::Vector2u size = traced.getSize();
    picture.setPosition(vp.position);
    picture.setScale(sf::Vector2f(vp.size.x / static_cast<float>(size.x), vp.size.y / static_cast<float>(size.y)));
    target.draw(picture);
    return true;
}

void Renderer::drawFlatBlackHole(sf::RenderTarget& target, sf::Vector2f at, float radius, double spinX,
                                 double spinY) const {
    constexpr int SEGMENTS = 96;
    const float w = 0.4f * radius;
    // Doppler: the side of the ring whose gas moves toward the eye is brighter. That's
    // the gas's axis crossed with the direction around the ring, seen along the view
    auto brightness = [&](double angle) { return 0.62 + 0.6 * (spinX * std::sin(angle) + spinY * std::cos(angle)); };
    // A band around the hole, its colors set at its inner and outer edges by angle
    auto band = [&](float inner, float outer, auto innerColor, auto outerColor) {
        sf::VertexArray strip(sf::PrimitiveType::TriangleStrip, 2 * (SEGMENTS + 1));
        for (int i = 0; i <= SEGMENTS; ++i) {
            const double angle = 2.0 * units::PI * i / SEGMENTS;
            const sf::Vector2f dir(static_cast<float>(std::cos(angle)), static_cast<float>(std::sin(angle)));
            const double g = brightness(angle);
            strip[2 * i] = sf::Vertex{at + dir * inner, innerColor(g)};
            strip[2 * i + 1] = sf::Vertex{at + dir * outer, outerColor(g)};
        }
        target.draw(strip);
    };

    // A soft glow, then the ring from its dim outer edge to its bright inner rim
    band(radius, 2.6f * radius, [](double) { return sf::Color(255, 150, 70, 56); },
         [](double) { return sf::Color(255, 150, 70, 0); });
    band(radius + 0.15f * w, radius + 1.05f * w, [](double g) { return holeColor(0.45 * g); },
         [](double g) { return holeColor(0.45 * g); });
    band(radius + 0.025f * w, radius + 0.575f * w, [](double g) { return holeColor(0.8 * g); },
         [](double g) { return holeColor(0.8 * g); });
    band(radius, radius + std::max(1.f, 0.2f * w), [](double g) { return holeColor(g + 0.35); },
         [](double g) { return holeColor(g + 0.35); });
    drawDisc(target, at, radius, sf::Color::Black);
}
