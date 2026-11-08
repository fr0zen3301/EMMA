#include "Camera.hpp"
#include "Constants.hpp"
#include "Renderer.hpp"
#include "Simulation.hpp"
#include "Ui.hpp"
#include "imgui.h"
#include "imgui-SFML.h"
#include <SFML/Graphics.hpp>
#include <algorithm>
#include <cmath>
#include <filesystem>
#include <optional>
#include <vector>

namespace {
constexpr float SIDEBAR_WIDTH = 290.f;
constexpr float STATUS_HEIGHT = 30.f;
constexpr float MAX_FRAME_DT = 0.1f;        // longest real frame the physics catches up on
constexpr float PHYSICS_BUDGET = 0.012f;    // seconds of stepping per frame before we fall behind
constexpr float ENERGY_REFRESH = 0.25f;     // totalEnergy() is O(N^2): four times a second is plenty
constexpr std::size_t DRIFT_HISTORY = 120;  // 30 seconds of samples
constexpr double KMS_PER_PIXEL = 0.25;      // drag-launch scale: 4 px of drag per km/s
constexpr double MAX_LAUNCH_KMS = 200.0;
constexpr float CLICK_SLOP = 4.f;           // px a press can move and still count as a click
const sf::Color ARROW_COLOR(217, 207, 255);
} // namespace

int main(int argc, char** argv) {
    sf::RenderWindow window(sf::VideoMode({1440u, 900u}), "EMMA (Engine for Multi-body Mechanics and Astrophysics)");
    window.setFramerateLimit(60);
    if (!ImGui::SFML::Init(window)) return -1;
    ImGui::GetIO().IniFilename = nullptr;  // the layout is fixed, nothing to save

    const std::filesystem::path exeDir =
        argc > 0 ? std::filesystem::path(argv[0]).parent_path() : std::filesystem::path();
    ui::loadFont(exeDir);
    ui::applyTheme();

    Simulation sim;
    Renderer renderer;
    Camera camera;
    UiState state;

    auto layout = [&](sf::Vector2u size) {
        const float w = static_cast<float>(size.x), h = static_cast<float>(size.y);
        window.setView(sf::View(sf::FloatRect({0.f, 0.f}, {w, h})));
        camera.viewport = sf::FloatRect({SIDEBAR_WIDTH, 0.f},
                                        {std::max(1.f, w - SIDEBAR_WIDTH), std::max(1.f, h - STATUS_HEIGHT)});
    };
    auto loadPreset = [&](Preset preset) {
        sim.load(preset);
        camera.fit(sim.viewRadius());
        state.timeScale = static_cast<float>(sim.suggestedTimeScale());
    };
    layout(window.getSize());
    loadPreset(Preset::SolarSystem);

    // Input state
    bool pressing = false, dragging = false, panning = false;
    sf::Vector2f pressAt, mouse, lastPan;
    float zoomVelocity = 0.f;

    // Timing and diagnostics
    sf::Clock clock;
    double accumulatorDays = 0.0;
    float behindTimer = 0.f;
    float energyTimer = ENERGY_REFRESH;  // refresh on the first frame
    unsigned long long baselineChange = ~0ull;
    double baseEnergy = 0.0, baseMomentum = 0.0, energyDrift = 0.0, momentumDrift = 0.0;
    std::vector<float> driftHistory;

    auto inViewport = [&](sf::Vector2f p) {
        return camera.viewport.contains(p) && !ImGui::GetIO().WantCaptureMouse;
    };
    // Drag from the press point -> launch speed (km/s) and direction (radians, world frame)
    auto dragLaunch = [&](sf::Vector2f to, double& speedKms, double& angle) {
        const sf::Vector2f d = to - pressAt;
        speedKms = std::min(MAX_LAUNCH_KMS, std::hypot(static_cast<double>(d.x), static_cast<double>(d.y)) * KMS_PER_PIXEL);
        angle = std::atan2(-static_cast<double>(d.y), static_cast<double>(d.x));  // screen y is down
    };
    // Launch velocity in the inertial frame. Speed and direction are relative to the
    // body that dominates gravity at the spawn point, so a launch near Earth moves with Earth.
    auto launchVelocity = [&](const Vector2D& at, double speedKms, double angle) -> Vector2D {
        if (state.autoOrbit) return sim.circularVelocity(at);
        const int host = sim.dominantBody(at);
        const Vector2D base = host >= 0 ? sim.getBodies()[static_cast<std::size_t>(host)].velocity : Vector2D();
        return base + Vector2D(std::cos(angle), std::sin(angle)) * (speedKms * units::KMS_TO_AU_PER_DAY);
    };
    auto spawnMass = [&]() { return std::pow(10.0, static_cast<double>(state.massExponent)); };

    while (window.isOpen()) {
        // ------------------------------------------------------------------ events
        while (const std::optional<sf::Event> event = window.pollEvent()) {
            ImGui::SFML::ProcessEvent(window, *event);

            if (event->is<sf::Event::Closed>()) {
                window.close();
            } else if (const auto* resized = event->getIf<sf::Event::Resized>()) {
                layout(resized->size);
            } else if (const auto* wheel = event->getIf<sf::Event::MouseWheelScrolled>()) {
                if (wheel->wheel == sf::Mouse::Wheel::Vertical && inViewport(sf::Vector2f(wheel->position)))
                    zoomVelocity += wheel->delta;
            } else if (const auto* press = event->getIf<sf::Event::MouseButtonPressed>()) {
                const sf::Vector2f at(press->position);
                if (!inViewport(at)) continue;
                if (press->button == sf::Mouse::Button::Left) {
                    pressing = true;
                    dragging = false;
                    pressAt = at;
                } else if (press->button == sf::Mouse::Button::Right ||
                           press->button == sf::Mouse::Button::Middle) {
                    panning = true;
                    lastPan = at;
                }
            } else if (const auto* release = event->getIf<sf::Event::MouseButtonReleased>()) {
                if (release->button == sf::Mouse::Button::Left) {
                    if (pressing) {
                        const Vector2D at = camera.toWorld(pressAt);
                        double speed = state.speedKms, angle = state.angle;
                        if (dragging) dragLaunch(sf::Vector2f(release->position), speed, angle);
                        sim.addBody(spawnMass(), at, launchVelocity(at, speed, angle));
                    }
                    pressing = false;
                    dragging = false;
                } else {
                    panning = false;
                }
            } else if (const auto* moved = event->getIf<sf::Event::MouseMoved>()) {
                const sf::Vector2f at(moved->position);
                if (panning) {
                    camera.panPixels(at - lastPan);
                    lastPan = at;
                }
                if (pressing && !dragging) {
                    const sf::Vector2f d = at - pressAt;
                    if (d.x * d.x + d.y * d.y > CLICK_SLOP * CLICK_SLOP) {
                        dragging = true;
                        state.autoOrbit = false;  // the drag sets the velocity now
                    }
                }
            } else if (const auto* key = event->getIf<sf::Event::KeyPressed>()) {
                if (ImGui::GetIO().WantCaptureKeyboard) continue;
                switch (key->code) {
                case sf::Keyboard::Key::Space: state.paused = !state.paused; break;
                case sf::Keyboard::Key::N:
                    state.paused = true;
                    state.stepRequested = true;
                    break;
                case sf::Keyboard::Key::Escape:
                    pressing = false;
                    dragging = false;
                    break;
                case sf::Keyboard::Key::F: state.resetViewRequested = true; break;
                default: break;
                }
            }
        }

        const sf::Time elapsed = clock.restart();
        const float frameDt = elapsed.asSeconds();
        ImGui::SFML::Update(window, elapsed);
        mouse = sf::Vector2f(sf::Mouse::getPosition(window));

        // Zoom toward the cursor, with a little inertia
        if (std::abs(zoomVelocity) > 0.01f) {
            camera.zoomAt(mouse, std::pow(1.0 / 0.9, static_cast<double>(zoomVelocity)));
            zoomVelocity *= 0.85f;
        }
        // The panel follows the drag, so a plain click afterwards repeats the same shot
        if (dragging) {
            double speed = 0.0, angle = 0.0;
            dragLaunch(mouse, speed, angle);
            state.speedKms = static_cast<float>(speed);
            state.angle = static_cast<float>(angle);
        }

        // ------------------------------------------------------------------ interface
        const sf::Vector2u size = window.getSize();
        const float width = static_cast<float>(size.x), height = static_cast<float>(size.y);
        Stats stats;
        stats.bodies = sim.getBodyCount();
        stats.fps = ImGui::GetIO().Framerate;
        stats.energyDrift = energyDrift;
        stats.momentumDrift = momentumDrift;
        stats.driftHistory = &driftHistory;
        stats.behind = behindTimer > 0.f;
        ui::drawSidebar(state, stats, sim.preset(), SIDEBAR_WIDTH, height - STATUS_HEIGHT);
        ui::drawStatusBar(sim, camera, mouse, state, height - STATUS_HEIGHT, width, STATUS_HEIGHT);

        if (state.presetRequested >= 0) {
            loadPreset(static_cast<Preset>(state.presetRequested));
            state.presetRequested = -1;
            accumulatorDays = 0.0;
        }
        if (state.resetRequested) {
            loadPreset(sim.preset());
            state.resetRequested = false;
            accumulatorDays = 0.0;
        }
        if (state.clearRequested) {
            sim.clear();
            state.clearRequested = false;
        }
        if (state.resetViewRequested) {
            camera.fit(sim.viewRadius());
            state.resetViewRequested = false;
        }

        // ------------------------------------------------------------------ physics
        // Fixed steps, as many as the chosen speed needs. The frame time is capped so a
        // stall (dragging the window on Windows pauses the loop) can't snowball.
        if (!state.paused)
            accumulatorDays += std::min(frameDt, MAX_FRAME_DT) * static_cast<double>(state.timeScale);
        else if (state.stepRequested)
            accumulatorDays += std::max(static_cast<double>(state.timeScale) / 60.0, sim.timeStep());
        state.stepRequested = false;

        sf::Clock physicsClock;
        bool behind = false;
        int steps = 0;
        while (accumulatorDays >= sim.timeStep()) {
            sim.step();
            accumulatorDays -= sim.timeStep();
            if (++steps % 32 == 0 && physicsClock.getElapsedTime().asSeconds() > PHYSICS_BUDGET) {
                behind = true;  // drop the backlog instead of freezing the window
                accumulatorDays = 0.0;
                break;
            }
        }
        behindTimer = behind ? 1.f : std::max(0.f, behindTimer - frameDt);
        sim.updateEffects(frameDt);

        // Conservation check, a few times per second
        energyTimer += frameDt;
        if (energyTimer >= ENERGY_REFRESH) {
            energyTimer = 0.f;
            const double energy = sim.totalEnergy();
            const double momentum = sim.angularMomentum();
            if (sim.changeCount() != baselineChange) {  // bodies added, merged or loaded: new baseline
                baselineChange = sim.changeCount();
                baseEnergy = energy;
                baseMomentum = momentum;
                driftHistory.clear();
            }
            energyDrift = baseEnergy != 0.0 ? (energy - baseEnergy) / std::abs(baseEnergy) : 0.0;
            momentumDrift = baseMomentum != 0.0 ? (momentum - baseMomentum) / std::abs(baseMomentum) : 0.0;
            driftHistory.push_back(static_cast<float>(energyDrift));
            if (driftHistory.size() > DRIFT_HISTORY) driftHistory.erase(driftHistory.begin());
        }

        // ------------------------------------------------------------------ drawing
        window.clear(sf::Color(4, 4, 9));
        renderer.drawStarfield(window);
        if (state.showTrails) renderer.drawTrails(window, camera, sim.getBodies());
        renderer.drawBodies(window, camera, sim.getBodies());
        renderer.drawFlashes(window, camera, sim.flashes);

        // Preview of the next body: ghost, velocity arrow and predicted path
        const bool hovering = !pressing && !panning && inViewport(mouse);
        if (pressing || hovering) {
            const sf::Vector2f anchor = pressing ? pressAt : mouse;
            const Vector2D at = camera.toWorld(anchor);
            double speed = state.speedKms, angle = state.angle;
            if (dragging) dragLaunch(mouse, speed, angle);
            const Vector2D velocity = launchVelocity(at, speed, angle);
            const double mass = spawnMass();
            const Rgb color = Body::colorForMass(mass);

            if (state.showPath) renderer.drawPath(window, camera, sim.predictPath(at, velocity), color);
            renderer.drawGhost(window, anchor,
                               Renderer::screenRadius(mass, Body::radiusForMass(mass), camera.pxPerAU), color);

            // Arrow for the velocity relative to the dominant body, at the drag scale
            const int host = sim.dominantBody(at);
            const Vector2D relative =
                velocity - (host >= 0 ? sim.getBodies()[static_cast<std::size_t>(host)].velocity : Vector2D());
            const double kms = relative.magnitude() / units::KMS_TO_AU_PER_DAY;
            if (kms > 0.05) {
                const Vector2D dir = relative / relative.magnitude();
                const double px = kms / KMS_PER_PIXEL;
                const sf::Vector2f tip =
                    anchor + sf::Vector2f(static_cast<float>(dir.x * px), static_cast<float>(-dir.y * px));
                renderer.drawArrow(window, anchor, tip, dragging ? ARROW_COLOR : sf::Color(217, 207, 255, 150));
            }
            if (dragging) ui::drawLaunchReadout(mouse, speed, angle);
        }

        if (state.showLabels) ui::drawLabels(camera, sim.getBodies());
        if (hovering) ui::drawBodyTooltip(sim, camera, mouse);

        ImGui::SFML::Render(window);
        window.display();
    }

    ImGui::SFML::Shutdown();
    return 0;
}
