#include "Camera.hpp"
#include "Renderer.hpp"
#include "SceneFrame.hpp"
#include "Ui.hpp"
#include "emma/BlackHole.hpp"
#include "emma/Calendar.hpp"
#include "emma/Clock.hpp"
#include "emma/Constants.hpp"
#include "emma/Galaxy.hpp"
#include "emma/Horizons.hpp"
#include "emma/Simulation.hpp"
#include "imgui.h"
#include "imgui-SFML.h"
#include "emma/IAS15Integrator.hpp"
#include "emma/VerletIntegrator.hpp"
#include <memory>
#include <SFML/Graphics.hpp>
#include <algorithm>
#include <atomic>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <cstdio>
#include <filesystem>
#include <future>
#include <optional>
#include <string>
#include <vector>

using namespace emma;

namespace {
constexpr float SIDEBAR_WIDTH = 290.f;
constexpr float STATUS_HEIGHT = 30.f;
constexpr float ENERGY_REFRESH = 0.25f;     // seconds between energy checks (they're slow)
constexpr std::size_t DRIFT_HISTORY = 120;  // 30 seconds of samples
constexpr double KMS_PER_PIXEL = 0.25;      // 4 px of drag = 1 km/s
constexpr double MAX_LAUNCH_KMS = 200.0;
constexpr float CLICK_SLOP = 4.f;           // px the mouse can move and still count as a click
constexpr double ORBIT_RATE = 0.005;        // radians the view turns per px of right-drag
const sf::Color ARROW_COLOR(217, 207, 255);
constexpr double HOLE_VIEW_DISTANCE = 28.0;  // Rs: how far "Go to" puts the camera from a hole
constexpr double HOLE_MIN_DISTANCE = 3.0;    // Rs: the closest the camera gets to a hole
constexpr double HOLE_VIEW_TILT = 7.0;       // degrees above the disk that "Go to" looks from
constexpr float DOUBLE_CLICK_SECONDS = 0.4f; // longest gap between the two clicks of a double-click
constexpr float GLIDE_SECONDS = 0.4f;        // how long the camera takes to glide to a body
constexpr double FOLLOW_MIN_RADII = 1.15;    // the closest the camera gets to a body, in its radii
constexpr std::size_t SHEET_MASSES = 32;     // how many of the heaviest bodies bend the grid
} // namespace

int main(int argc, char** argv) {
    // Smooth the edges of planets and trails
    sf::ContextSettings settings;
    settings.antiAliasingLevel = 8;
    sf::RenderWindow window(sf::VideoMode({1440u, 900u}), "EMMA (Engine for Multi-body Mechanics and Astrophysics)",
                            sf::Style::Default, sf::State::Windowed, settings);
    window.setFramerateLimit(60);
    if (!ImGui::SFML::Init(window)) return -1;
    ImGui::GetIO().IniFilename = nullptr;  // don't save the window layout

    const std::filesystem::path exeDir =
        argc > 0 ? std::filesystem::path(argv[0]).parent_path() : std::filesystem::path();
    ui::loadFont(exeDir);
    ui::applyTheme();

    Simulation sim;
    Renderer renderer;
    renderer.setAssetFolder(exeDir);
    Camera camera;
    SceneFrame frame;
    bool wasGalactic = false;
    UiState state;
    emma::Clock clock;

    auto layout = [&](sf::Vector2u size) {
        const float w = static_cast<float>(size.x), h = static_cast<float>(size.y);
        window.setView(sf::View(sf::FloatRect({0.f, 0.f}, {w, h})));
        camera.viewport = sf::FloatRect({SIDEBAR_WIDTH, 0.f},
                                        {std::max(1.f, w - SIDEBAR_WIDTH), std::max(1.f, h - STATUS_HEIGHT)});
    };
    // Starting positions: download today's, or use the last saved ones, or average orbits
    const double today = calendar::startOfDay(calendar::julianDayNow());
    const std::filesystem::path cachePath = horizons::defaultCachePath();
    std::optional<SolarSnapshot> solarStart = horizons::loadCache(cachePath);
    std::atomic<int> fetchProgress{0};
    std::atomic<bool> fetchCancel{false};
    // Must come after the atomics above, so it's destroyed first
    std::future<horizons::Result> fetch;
    bool pausedForFetch = false;
    unsigned long long changesAtFetch = 0;
    std::string sourceNote;
    bool canRetry = false;

    bool presetTurnedView = false;  // the last preset turned the camera, so turn it back next time
    auto loadPreset = [&](Preset preset) {
        sim.load(preset, preset == Preset::SolarSystem && solarStart ? &*solarStart : nullptr);
        camera.fit(sim.viewRadius());
        const Vector3D look = sim.lookDirection();
        if (look.lengthSquared() > 0.0) {
            // Point the camera along the preset's direction
            const Vector3D d = look.normalized();
            camera.setAngles(std::atan2(-d.x, d.y), -std::asin(std::clamp(d.z, -1.0, 1.0)));
            presetTurnedView = true;
        } else if (presetTurnedView) {
            camera.topView();
            presetTurnedView = false;
        }
        clock.reset();
        clock.timeScale = sim.suggestedTimeScale();
        clock.live = preset == Preset::SolarSystem;  // the solar system starts in real time
    };
    auto describeStart = [&]() -> std::string {
        if (!solarStart) return "Mean orbits for 1 January 2000.";
        return "Positions for " + calendar::date(solarStart->julianDay) + " from JPL Horizons.";
    };
    auto startFetch = [&]() {
        fetchProgress = 0;
        fetchCancel = false;
        fetch = std::async(std::launch::async, horizons::fetchSolarSystem, today, &fetchProgress, &fetchCancel);
        canRetry = false;
        changesAtFetch = sim.changeCount();
        if (sim.preset() == Preset::SolarSystem && !clock.paused) {
            clock.paused = true;  // wait for today's positions
            pausedForFetch = true;
        }
    };

    layout(window.getSize());
    loadPreset(Preset::SolarSystem);
    if (solarStart && solarStart->julianDay == today)
        sourceNote = describeStart();  // already downloaded today
    else
        startFetch();

    // Input state
    bool pressing = false, dragging = false, panning = false, orbiting = false;
    sf::Vector2f pressAt, mouse, lastDrag;
    float zoomVelocity = 0.f;
    bool pressOnBody = false;          // the click started on a body
    bool clickedBody = false;          // the last click was on a body too (for double-clicks)
    sf::Clock sinceClick;
    sf::Vector2f lastClickAt;
    std::optional<Vector3D> glideFrom;  // where the camera started gliding from
    float glideTime = 0.f;
    std::vector<Renderer::BlackHoleShot> lastHoles;  // where black holes were drawn last frame, for clicks

    // Timing and diagnostics
    sf::Clock frameClock;
    float behindTimer = 0.f;
    double realTimeAhead = 0.0;  // days the simulation is ahead of the clock, from last frame
    float energyTimer = ENERGY_REFRESH;  // check on the first frame
    unsigned long long baselineChange = ~0ull;
    double clockSlowdown = -1.0;  // for the sidebar's clock readout
    double baseEnergy = 0.0, energyDrift = 0.0, momentumDrift = 0.0;
    Vector3D baseMomentum;
    std::vector<float> driftHistory;

    auto inViewport = [&](sf::Vector2f p) {
        return camera.viewport.contains(p) && !ImGui::GetIO().WantCaptureMouse;
    };
    // Turns a mouse drag into a launch speed and direction
    auto dragLaunch = [&](sf::Vector2f to, double& speedKms, double& angle) {
        const sf::Vector2f d = to - pressAt;
        speedKms = std::min(MAX_LAUNCH_KMS, std::hypot(static_cast<double>(d.x), static_cast<double>(d.y)) * KMS_PER_PIXEL);
        const std::optional<Vector3D> from = camera.pick(pressAt), toward = camera.pick(to);
        if (from && toward && (toward->x != from->x || toward->y != from->y))
            angle = std::atan2(toward->y - from->y, toward->x - from->x);
        else
            angle = std::atan2(-static_cast<double>(d.y), static_cast<double>(d.x)) + camera.yaw();  // screen y points down
    };
    // The new body's velocity. It's relative to whatever it's near, so a launch near Earth
    // moves along with Earth
    auto launchVelocity = [&](const Vector3D& at, double speedKms, double angle) -> Vector3D {
        if (state.autoOrbit) return sim.circularVelocity(at);
        const int host = sim.dominantBody(at);
        const Vector3D base = host >= 0 ? sim.getBodies()[static_cast<std::size_t>(host)].velocity : Vector3D();
        return base + Vector3D(std::cos(angle), std::sin(angle)) * (speedKms * units::KMS_TO_AU_PER_DAY);
    };
    auto spawnMass = [&]() {
        return std::pow(10.0, static_cast<double>(state.spawnHole ? state.holeMassExponent : state.massExponent));
    };
    auto spawnRadius = [&]() {
        const double mass = spawnMass();
        return state.spawnHole ? blackhole::horizonRadiusAU(mass, state.holeSpin) : Body::radiusForMass(mass);
    };

    // The name of whatever is under a pixel, or empty
    auto nameUnder = [&](sf::Vector2f p) -> std::string {
        const std::vector<Body>& bodies = sim.getBodies();
        const int i = ui::bodyAt(bodies, camera, frame, p);
        if (i >= 0) return bodies[static_cast<std::size_t>(i)].name;
        for (const Renderer::BlackHoleShot& hole : lastHoles)
            if (ui::overBlackHole(hole, p)) return hole.hole.name;
        return {};
    };
    // A line for a black hole's tooltip saying where it is
    auto holeWhere = [&](const Renderer::BlackHoleShot& shot) -> std::string {
        const std::vector<Body>& bodies = sim.getBodies();
        char line[160];
        if (shot.hole.body < 0) {
            double sunDistance = galaxy::R0_AU;
            for (const Body& b : bodies)
                if (b.name == "Sun") sunDistance = (frame.place(b.position) - shot.hole.center).magnitude();
            std::snprintf(line, sizeof(line), "Distance: %s from the Sun", ui::formatDistance(sunDistance).c_str());
            return line;
        }
        const std::size_t index = static_cast<std::size_t>(shot.hole.body);
        if (index >= bodies.size()) return {};
        const Body& hole = bodies[index];
        const int hostIndex = sim.dominantBody(hole.position, shot.hole.body);
        if (hostIndex < 0 || bodies[static_cast<std::size_t>(hostIndex)].mass <= hole.mass) return {};
        const Body& host = bodies[static_cast<std::size_t>(hostIndex)];
        const std::string hostName = host.isNamed() ? host.name : host.description();
        std::snprintf(line, sizeof(line), "Distance: %s from %s\nSpeed: %.2f km/s relative to %s",
                      ui::formatDistance((hole.position - host.position).magnitude()).c_str(), hostName.c_str(),
                      (hole.velocity - host.velocity).magnitude() / units::KMS_TO_AU_PER_DAY, hostName.c_str());
        return line;
    };
    // Starts following something, gliding there smoothly
    auto startFollowing = [&](const std::string& name) {
        if (name.empty() || state.follow == name) return;
        state.follow = name;
        glideFrom = camera.target;
        glideTime = 0.f;
    };
    // Points the camera at a followed body (partway, while still gliding)
    auto aimAt = [&](const Vector3D& p, float dt) {
        if (!glideFrom) {
            camera.target = p;
            return;
        }
        glideTime += dt;
        const double k = std::min(1.0, static_cast<double>(glideTime / GLIDE_SECONDS));
        camera.target = *glideFrom + (p - *glideFrom) * (k * k * (3.0 - 2.0 * k));
        if (k >= 1.0) glideFrom.reset();
    };

    while (window.isOpen()) {
        // ------------------------------------------------------------------ events
        while (const std::optional<sf::Event> event = window.pollEvent()) {
            ImGui::SFML::ProcessEvent(window, *event);

            if (event->is<sf::Event::Closed>()) {
                window.close();
            } else if (const auto* resized = event->getIf<sf::Event::Resized>()) {
                layout(resized->size);
            } else if (const auto* wheel = event->getIf<sf::Event::MouseWheelScrolled>()) {
                if (wheel->wheel == sf::Mouse::Wheel::Vertical && inViewport(sf::Vector2f(wheel->position))) {
                    // Scrolling in over a body starts following it
                    if (wheel->delta > 0.f) startFollowing(nameUnder(sf::Vector2f(wheel->position)));
                    zoomVelocity += wheel->delta;
                }
            } else if (const auto* press = event->getIf<sf::Event::MouseButtonPressed>()) {
                const sf::Vector2f at(press->position);
                if (!inViewport(at)) continue;
                if (press->button == sf::Mouse::Button::Left) {
                    pressing = true;
                    dragging = false;
                    pressAt = at;
                    pressOnBody = !nameUnder(at).empty();
                } else if (press->button == sf::Mouse::Button::Right ||
                           press->button == sf::Mouse::Button::Middle) {
                    const bool shift = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::LShift) ||
                                       sf::Keyboard::isKeyPressed(sf::Keyboard::Key::RShift);
                    if (press->button == sf::Mouse::Button::Middle || shift) panning = true;
                    else orbiting = true;
                    lastDrag = at;
                }
            } else if (const auto* release = event->getIf<sf::Event::MouseButtonReleased>()) {
                if (release->button == sf::Mouse::Button::Left) {
                    if (pressing && pressOnBody && !dragging) {
                        // Clicking a body doesn't add a new one. Double-clicking follows it
                        const sf::Vector2f at(release->position);
                        const sf::Vector2f d = at - lastClickAt;
                        const bool twice = clickedBody &&
                                           sinceClick.getElapsedTime().asSeconds() < DOUBLE_CLICK_SECONDS &&
                                           d.x * d.x + d.y * d.y < 4.f * CLICK_SLOP * CLICK_SLOP;
                        if (twice) {
                            startFollowing(nameUnder(at));
                            clickedBody = false;
                        } else {
                            clickedBody = true;
                            lastClickAt = at;
                            sinceClick.restart();
                        }
                    } else if (pressing && !frame.galactic) {
                        if (const std::optional<Vector3D> at = camera.pick(pressAt)) {
                            double speed = state.speedKms, angle = state.angle;
                            if (dragging) dragLaunch(sf::Vector2f(release->position), speed, angle);
                            const Vector3D velocity = launchVelocity(*at, speed, angle);
                            // A new hole spins around the grid's up axis
                            if (state.spawnHole)
                                sim.addBody(Body::blackHole("", "", spawnMass(), state.holeSpin,
                                                            Vector3D(0.0, 0.0, 1.0), *at, velocity));
                            else
                                sim.addBody(spawnMass(), *at, velocity);
                        }
                        clickedBody = false;
                    }
                    pressing = false;
                    dragging = false;
                } else {
                    panning = false;
                    orbiting = false;
                }
            } else if (const auto* moved = event->getIf<sf::Event::MouseMoved>()) {
                const sf::Vector2f at(moved->position);
                if (panning) {
                    camera.pan(lastDrag, at);
                    state.follow.clear();  // panning stops following
                    glideFrom.reset();
                    lastDrag = at;
                } else if (orbiting) {
                    camera.orbit(-(at.x - lastDrag.x) * ORBIT_RATE, (at.y - lastDrag.y) * ORBIT_RATE);
                    lastDrag = at;
                }
                if (pressing && !dragging) {
                    const sf::Vector2f d = at - pressAt;
                    if (d.x * d.x + d.y * d.y > CLICK_SLOP * CLICK_SLOP) {
                        dragging = true;
                        state.autoOrbit = false;  // the drag sets the velocity
                    }
                }
            } else if (const auto* key = event->getIf<sf::Event::KeyPressed>()) {
                if (ImGui::GetIO().WantCaptureKeyboard) continue;
                switch (key->code) {
                case sf::Keyboard::Key::Space: clock.paused = !clock.paused; break;
                case sf::Keyboard::Key::N: clock.requestStep(); break;
                case sf::Keyboard::Key::Escape:
                    pressing = false;
                    dragging = false;
                    break;
                case sf::Keyboard::Key::F: state.resetViewRequested = true; break;
                case sf::Keyboard::Key::T: state.topViewRequested = true; break;
                default: break;
                }
            }
        }

        const sf::Time elapsed = frameClock.restart();
        const float frameDt = elapsed.asSeconds();
        ImGui::SFML::Update(window, elapsed);
        mouse = sf::Vector2f(sf::Mouse::getPosition(window));

        // Smooth zoom, toward the cursor or the followed body
        if (std::abs(zoomVelocity) > 0.01f) {
            const double factor = std::pow(1.0 / 0.9, static_cast<double>(zoomVelocity));
            if (state.follow.empty()) camera.zoomAt(mouse, factor);
            else camera.zoom(factor);
            zoomVelocity *= 0.85f;
        }
        // Copy the drag into the sidebar, so the next click repeats it
        if (dragging) {
            double speed = 0.0, angle = 0.0;
            dragLaunch(mouse, speed, angle);
            state.speedKms = static_cast<float>(speed);
            state.angle = static_cast<float>(angle);
        }

        // ------------------------------------------------------------------ JPL Horizons
        if (fetch.valid()) {
            if (fetch.wait_for(std::chrono::seconds(0)) == std::future_status::ready) {
                horizons::Result result = fetch.get();
                if (result.ok) {
                    solarStart = result.snapshot;
                    horizons::saveCache(cachePath, *solarStart);
                    // Use today's positions, unless the user changed things while waiting
                    if (sim.preset() == Preset::SolarSystem && sim.changeCount() == changesAtFetch) {
                        sim.load(Preset::SolarSystem, &*solarStart);
                        clock.reset();
                    }
                    sourceNote = describeStart();
                } else {
                    sourceNote = "Couldn't get today's positions from JPL Horizons: " + result.error + ". " + describeStart();
                    canRetry = true;
                }
                if (pausedForFetch && clock.paused) clock.paused = false;
                pausedForFetch = false;
            } else {
                sourceNote = "Getting today's positions from JPL Horizons (" + std::to_string(fetchProgress.load()) +
                             " of " + std::to_string(SOLAR_BODY_COUNT) + ")...";
            }
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
        stats.aheadDays = realTimeAhead;
        stats.sourceNote = sourceNote;
        stats.canRetry = canRetry && !fetch.valid();
        stats.sunTravelled = state.sunSpeedKms * units::KMS_TO_AU_PER_DAY * sim.time();
        stats.clockSlowdown = clockSlowdown;
        // Which black hole Go to jumps to
        bool anyHole = sim.preset() == Preset::SolarSystem;
        std::string holeTarget = anyHole ? ui::SGR_A : "";
        for (const Body& b : sim.getBodies()) {
            if (!b.isBlackHole()) continue;
            anyHole = true;
            if (holeTarget.empty() && b.isNamed()) holeTarget = b.name;
        }
        ui::drawSidebar(state, clock, stats, sim.preset(), sim.getBodies(), sim.getIntegrator().name(), anyHole,
                        holeTarget.empty() ? nullptr : holeTarget.c_str(), SIDEBAR_WIDTH, height - STATUS_HEIGHT);
        ui::drawStatusBar(sim, camera, frame, mouse, clock, height - STATUS_HEIGHT, width, STATUS_HEIGHT);

        if (state.presetRequested >= 0) {
            loadPreset(static_cast<Preset>(state.presetRequested));
            state.presetRequested = -1;
        }
        if (state.integratorRequested >= 0) {
            if (state.integratorRequested == 1) sim.setIntegrator(std::make_unique<IAS15Integrator>());
            else sim.setIntegrator(std::make_unique<VerletIntegrator>());
            state.integratorRequested = -1;
            baselineChange = ~0ull;
        }
        const double relativity = state.relativity ? static_cast<double>(state.relativityScale) : 0.0;
        if (relativity != sim.relativity()) {
            sim.setRelativity(relativity);
            baselineChange = ~0ull;  // the energy changes too, so restart the drift readout
        }
        if (state.resetRequested) {
            loadPreset(sim.preset());
            state.resetRequested = false;
        }
        if (state.resetViewRequested) {
            camera.fit(sim.viewRadius());
            if (frame.galactic) state.follow = "Sun";  // in the galaxy view, stay with the Sun
            else state.follow.clear();
            state.resetViewRequested = false;
        }
        if (state.topViewRequested) {
            camera.topView();
            state.topViewRequested = false;
        }
        if (state.retryFetchRequested) {
            state.retryFetchRequested = false;
            if (!fetch.valid()) startFetch();
        }
        if (state.goToHoleRequested) {
            state.goToHoleRequested = false;
            // Find the hole's mass and the axis its gas turns around
            double holeMass = 0.0;
            Vector3D holeAxis;
            if (sim.preset() == Preset::SolarSystem && holeTarget == ui::SGR_A) {
                holeMass = galaxy::SGR_A_MASS;
                holeAxis = galaxy::sgrASpinAxis();
            } else {
                for (const Body& b : sim.getBodies()) {
                    if (b.isBlackHole() && !holeTarget.empty() && b.name == holeTarget) {
                        holeMass = b.mass;
                        holeAxis = galaxy::fromEcliptic(b.spinAxis);
                        break;
                    }
                }
            }
            if (holeMass > 0.0) {
                state.follow = holeTarget;
                glideFrom.reset();  // jump straight there
                camera.distance = HOLE_VIEW_DISTANCE * blackhole::schwarzschildRadiusAU(holeMass);
                // Look at the disk from just above it, so it lies flat across the picture
                Vector3D axis = frame.fromGalactic(holeAxis);
                if (axis.z < 0.0) axis = -axis;
                const double upright = std::acos(std::min(1.0, axis.z));
                const double tilt = HOLE_VIEW_TILT * units::DEG;
                camera.setAngles(std::atan2(-axis.x, axis.y), upright > tilt ? upright - tilt : upright + tilt);
            }
        }

        // ------------------------------------------------------------------ physics
        const emma::Clock::Report ran = clock.run(sim, static_cast<double>(frameDt));
        behindTimer = ran.behind ? 1.f : std::max(0.f, behindTimer - frameDt);
        realTimeAhead = ran.ahead;
        sim.updateEffects(frameDt);

        // The galaxy view (solar system only)
        frame.galactic = state.galaxyView && sim.preset() == Preset::SolarSystem;
        frame.velocity = galaxy::sunVelocityKms().normalized() * (state.sunSpeedKms * units::KMS_TO_AU_PER_DAY);
        frame.now = sim.time();
        if (frame.galactic != wasGalactic) {
            wasGalactic = frame.galactic;
            if (frame.galactic) {
                // Follow the Sun, looking across its path from a little above
                state.follow = "Sun";
                camera.setAngles(0.5 * units::PI, 25.0 * units::DEG);
            } else {
                state.follow.clear();
                camera.fit(sim.viewRadius());
            }
        }

        // Keep the followed body in the middle of the view
        if (state.follow == ui::SGR_A && sim.preset() == Preset::SolarSystem) {
            aimAt(frame.galacticCenter(), frameDt);
            camera.distance = std::max(camera.distance,
                                       HOLE_MIN_DISTANCE * blackhole::schwarzschildRadiusAU(galaxy::SGR_A_MASS));
        } else if (!state.follow.empty()) {
            const std::vector<Body>& bodies = sim.getBodies();
            const auto followed = std::find_if(bodies.begin(), bodies.end(),
                                               [&state](const Body& b) { return b.name == state.follow; });
            if (followed != bodies.end()) {
                aimAt(frame.place(followed->position), frameDt);
                // Don't zoom inside the body
                const double closest = followed->isBlackHole()
                                           ? HOLE_MIN_DISTANCE * blackhole::schwarzschildRadiusAU(followed->mass)
                                           : FOLLOW_MIN_RADII * followed->radius;
                camera.distance = std::max(camera.distance, closest);
            } else {
                state.follow.clear();
            }
        }
        if (state.follow.empty()) glideFrom.reset();

        // Check energy and momentum a few times a second
        energyTimer += frameDt;
        if (energyTimer >= ENERGY_REFRESH) {
            energyTimer = 0.f;
            const double energy = sim.totalEnergy();
            const Vector3D momentum = sim.angularMomentum();
            if (sim.changeCount() != baselineChange) {  // bodies changed: start a new baseline
                baselineChange = sim.changeCount();
                baseEnergy = energy;
                baseMomentum = momentum;
                driftHistory.clear();
            }
            energyDrift = baseEnergy != 0.0 ? (energy - baseEnergy) / std::abs(baseEnergy) : 0.0;
            const double baseMomentumSize = baseMomentum.magnitude();
            momentumDrift = baseMomentumSize > 0.0 ? (momentum - baseMomentum).magnitude() / baseMomentumSize : 0.0;
            driftHistory.push_back(static_cast<float>(energyDrift));
            if (driftHistory.size() > DRIFT_HISTORY) driftHistory.erase(driftHistory.begin());
        }

        // ------------------------------------------------------------------ drawing
        window.clear(Renderer::SPACE_COLOR);
        renderer.drawStarfield(window, camera, frame);
        // Black holes go first, since a ray-traced one covers the whole view
        std::vector<Renderer::BlackHoleSight> sights;
        if (sim.preset() == Preset::SolarSystem) {
            Renderer::BlackHoleSight landmark;
            landmark.center = frame.galacticCenter();
            landmark.mass = galaxy::SGR_A_MASS;
            landmark.axis = galaxy::sgrASpinAxis();
            landmark.name = ui::SGR_A;
            landmark.kind = "Supermassive black hole";
            sights.push_back(landmark);
        }
        for (std::size_t i = 0; i < sim.getBodies().size(); ++i) {
            const Body& b = sim.getBodies()[i];
            if (!b.isBlackHole()) continue;
            Renderer::BlackHoleSight sight;
            sight.center = frame.place(b.position);
            sight.mass = b.mass;
            sight.spin = b.spin;
            sight.axis = galaxy::fromEcliptic(b.spinAxis);
            sight.name = b.name;
            sight.kind = b.description();
            sight.body = static_cast<int>(i);
            sights.push_back(sight);
        }
        // Only ray trace the one that looks biggest
        std::size_t traced = sights.size();
        double biggest = -1.0;
        for (std::size_t k = 0; k < sights.size(); ++k) {
            const double px = Renderer::shadowPixels(camera, sights[k].center, sights[k].mass);
            if (px > biggest) {
                biggest = px;
                traced = k;
            }
        }
        const Renderer::BlackHoleLook look{state.holeLook, state.holeBendLight, state.holeDoppler,
                                           state.holeBrightness};
        std::vector<Renderer::BlackHoleShot> holes;
        if (traced < sights.size()) holes.push_back(renderer.drawBlackHole(window, camera, frame, look, sights[traced], true));
        for (std::size_t k = 0; k < sights.size(); ++k)
            if (k != traced) holes.push_back(renderer.drawBlackHole(window, camera, frame, look, sights[k], false));
        lastHoles = holes;
        clockSlowdown = -1.0;
        if (state.showGrid && state.bendGrid) {
            // The masses that bend the grid: the heaviest bodies, plus Sgr A* in the solar system
            std::vector<sheet::Mass> masses;
            if (sim.preset() == Preset::SolarSystem)
                masses.push_back(sheet::Mass{frame.galacticCenter(), galaxy::SGR_A_MASS,
                                             blackhole::schwarzschildRadiusAU(galaxy::SGR_A_MASS)});
            const std::vector<Body>& bodies = sim.getBodies();
            std::vector<std::size_t> heaviest;
            for (std::size_t i = 0; i < bodies.size(); ++i)
                if (bodies[i].mass > 0.0) heaviest.push_back(i);
            const std::size_t keep = std::min(SHEET_MASSES, heaviest.size());
            std::partial_sort(heaviest.begin(), heaviest.begin() + static_cast<std::ptrdiff_t>(keep), heaviest.end(),
                              [&bodies](std::size_t a, std::size_t b) { return bodies[a].mass > bodies[b].mass; });
            for (std::size_t k = 0; k < keep; ++k) {
                const Body& b = bodies[heaviest[k]];
                masses.push_back(sheet::Mass{frame.place(b.position), b.mass, b.radius});
            }
            const double psi = renderer.drawGrid(window, camera, masses, static_cast<double>(state.gridDepth));
            clockSlowdown = masses.empty() ? -1.0 : sheet::clockSlowdown(psi);
        } else if (state.showGrid) {
            renderer.drawGrid(window, camera);
        }
        if (state.showTrails) renderer.drawTrails(window, camera, frame, sim.getBodies());
        // Surfaces turn with the date (year 2000 if there's no calendar)
        renderer.drawBodies(window, camera, frame, sim.getBodies(),
                            sim.hasCalendar() ? sim.epoch() + sim.time() : units::J2000_JD);
        renderer.drawFlashes(window, camera, frame, sim.flashes);
        if (frame.galactic && frame.velocity.lengthSquared() > 0.0) {
            // An arrow showing which way the solar system is heading
            const Vector3D from = frame.place(Vector3D());
            const Vector3D to = from + frame.heading().normalized() * (0.6 * camera.distance);
            renderer.drawRay(window, camera, from, to, sf::Color(217, 207, 255, 140));
            ui::drawTag(camera, to, "Sun's direction of motion");
        }

        // Preview of the next body: ghost, arrow and predicted path
        const bool hovering = !pressing && !panning && !orbiting && inViewport(mouse);
        const sf::Vector2f anchor = pressing ? pressAt : mouse;
        // No preview over a body, since clicking there selects it
        const bool onBody = pressing ? pressOnBody && !dragging : hovering && !nameUnder(mouse).empty();
        const std::optional<Vector3D> at =
            (pressing || hovering) && !onBody && !frame.galactic ? camera.pick(anchor) : std::nullopt;
        if (at) {
            double speed = state.speedKms, angle = state.angle;
            if (dragging) dragLaunch(mouse, speed, angle);
            const Vector3D velocity = launchVelocity(*at, speed, angle);
            const double mass = spawnMass();
            const Rgb color = state.spawnHole ? Body::HOLE_COLOR : Body::colorForMass(mass);

            if (state.showPath) renderer.drawPath(window, camera, sim.predictPath(*at, velocity), color);
            renderer.drawGhost(window, anchor, Renderer::screenRadius(mass, spawnRadius(), camera.pxPerAUAt(*at)),
                               color);

            // The velocity arrow: its length shows the speed
            const int host = sim.dominantBody(*at);
            const Vector3D relative =
                velocity - (host >= 0 ? sim.getBodies()[static_cast<std::size_t>(host)].velocity : Vector3D());
            const double kms = relative.magnitude() / units::KMS_TO_AU_PER_DAY;
            const std::optional<sf::Vector2f> from = camera.toScreen(*at);
            const std::optional<sf::Vector2f> ahead =
                camera.toScreen(*at + relative.normalized() * (1e-3 * camera.distance));
            if (kms > 0.05 && from && ahead) {
                const sf::Vector2f d = *ahead - *from;
                const float len = std::hypot(d.x, d.y);
                if (len > 0.f) {
                    const sf::Vector2f tip = anchor + d * (static_cast<float>(kms / KMS_PER_PIXEL) / len);
                    renderer.drawArrow(window, anchor, tip, dragging ? ARROW_COLOR : sf::Color(217, 207, 255, 150));
                }
            }
            if (dragging) ui::drawLaunchReadout(mouse, speed, angle);
        }

        if (state.showLabels) {
            ui::drawLabels(camera, frame, sim.getBodies());
            for (const Renderer::BlackHoleShot& h : holes) ui::drawBlackHoleLabel(camera, h);
        }
        if (hovering) {
            // A body in front of a black hole gets the tooltip
            const Renderer::BlackHoleShot* overHole = nullptr;
            if (ui::bodyAt(sim.getBodies(), camera, frame, mouse) < 0)
                for (const Renderer::BlackHoleShot& h : holes)
                    if (ui::overBlackHole(h, mouse)) {
                        overHole = &h;
                        break;
                    }
            if (overHole) ui::drawBlackHoleTooltip(*overHole, holeWhere(*overHole));
            else ui::drawBodyTooltip(sim, camera, frame, mouse);
        }

        ImGui::SFML::Render(window);
        window.display();
    }

    fetchCancel = true;  // stop any download that's still running
    ImGui::SFML::Shutdown();
    return 0;
}
