#include "Ui.hpp"
#include "Renderer.hpp"
#include "emma/Calendar.hpp"
#include "emma/Constants.hpp"
#include "imgui.h"
#include "imgui-SFML.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <iostream>
#include <system_error>
#include <cstring>

using namespace emma;

namespace {

const ImVec4 ACCENT(0.851f, 0.812f, 1.0f, 1.0f);
const ImVec4 MUTED(0.55f, 0.56f, 0.60f, 1.0f);
const ImVec4 GOOD(0.56f, 0.89f, 0.69f, 1.0f);
const ImVec4 WARN(1.0f, 0.75f, 0.42f, 1.0f);
constexpr float LABEL_WIDTH = 84.f;
constexpr float VALUE_COLUMN = 150.f;

template <typename... Args>
std::string printf_string(const char* pattern, Args... args) {
    char buffer[160];
    std::snprintf(buffer, sizeof(buffer), pattern, args...);
    return std::string(buffer);
}

std::string number(double v) {
    const double a = std::abs(v);
    if (a == 0.0 || (a >= 0.01 && a < 10000.0)) return printf_string("%.2f", v);
    return printf_string("%.2e", v);
}

// A left-hand label, with the widget that follows lined up in a column
void rowLabel(const char* text) {
    ImGui::AlignTextToFramePadding();
    ImGui::TextUnformatted(text);
    ImGui::SameLine(ImGui::GetStyle().WindowPadding.x + LABEL_WIDTH);
}

void hint(const char* text) {
    ImGui::PushStyleColor(ImGuiCol_Text, MUTED);
    ImGui::TextWrapped("%s", text);
    ImGui::PopStyleColor();
}

ImVec4 driftColor(double drift) {
    const double a = std::abs(drift);
    if (a < 1e-6) return GOOD;
    if (a < 1e-3) return ImGui::GetStyleColorVec4(ImGuiCol_Text);
    return WARN;
}

// A round dial for the launch direction. Angle in radians, counterclockwise from +x
bool angleDial(const char* id, float& angle, float radius) {
    const ImVec2 origin = ImGui::GetCursorScreenPos();
    ImGui::InvisibleButton(id, ImVec2(radius * 2.f, radius * 2.f));
    const ImVec2 c(origin.x + radius, origin.y + radius);

    bool changed = false;
    if (ImGui::IsItemActive()) {
        const ImVec2 m = ImGui::GetIO().MousePos;
        const float dx = m.x - c.x, dy = m.y - c.y;
        if (dx * dx + dy * dy > 9.f) {
            angle = std::atan2(-dy, dx);  // screen y points down, world y points up
            changed = true;
        }
    }

    ImDrawList* dl = ImGui::GetWindowDrawList();
    dl->AddCircleFilled(c, radius, ImGui::GetColorU32(ImGuiCol_FrameBg), 32);
    dl->AddCircle(c, radius, ImGui::GetColorU32(ImGuiCol_Border), 32, 1.f);
    const ImVec2 tip(c.x + std::cos(angle) * (radius - 4.f), c.y - std::sin(angle) * (radius - 4.f));
    dl->AddLine(c, tip, ImGui::GetColorU32(ACCENT), 2.f);
    dl->AddCircleFilled(c, 2.5f, ImGui::GetColorU32(ACCENT));
    return changed;
}

} // namespace

namespace ui {

void applyTheme() {
    ImGuiStyle& s = ImGui::GetStyle();
    s.WindowRounding = 0.f;
    s.FrameRounding = 4.f;
    s.GrabRounding = 4.f;
    s.WindowBorderSize = 0.f;
    s.WindowPadding = ImVec2(14.f, 12.f);
    s.FramePadding = ImVec2(8.f, 5.f);
    s.ItemSpacing = ImVec2(8.f, 7.f);

    ImVec4* c = s.Colors;
    c[ImGuiCol_WindowBg] = ImVec4(0.067f, 0.071f, 0.102f, 0.97f);
    c[ImGuiCol_PopupBg] = ImVec4(0.08f, 0.08f, 0.11f, 0.98f);
    c[ImGuiCol_Text] = ImVec4(0.86f, 0.86f, 0.89f, 1.f);
    c[ImGuiCol_TextDisabled] = MUTED;
    c[ImGuiCol_Border] = ImVec4(0.17f, 0.17f, 0.22f, 1.f);
    c[ImGuiCol_Separator] = ImVec4(0.17f, 0.17f, 0.22f, 1.f);
    c[ImGuiCol_FrameBg] = ImVec4(0.11f, 0.11f, 0.15f, 1.f);
    c[ImGuiCol_FrameBgHovered] = ImVec4(0.15f, 0.15f, 0.21f, 1.f);
    c[ImGuiCol_FrameBgActive] = ImVec4(0.18f, 0.18f, 0.25f, 1.f);
    c[ImGuiCol_Button] = ImVec4(0.11f, 0.11f, 0.15f, 1.f);
    c[ImGuiCol_ButtonHovered] = ImVec4(0.16f, 0.16f, 0.22f, 1.f);
    c[ImGuiCol_ButtonActive] = ImVec4(0.21f, 0.20f, 0.30f, 1.f);
    c[ImGuiCol_Header] = ImVec4(0.11f, 0.11f, 0.15f, 1.f);
    c[ImGuiCol_HeaderHovered] = ImVec4(0.15f, 0.15f, 0.21f, 1.f);
    c[ImGuiCol_HeaderActive] = ImVec4(0.18f, 0.18f, 0.25f, 1.f);
    c[ImGuiCol_SliderGrab] = ACCENT;
    c[ImGuiCol_SliderGrabActive] = ImVec4(1.f, 1.f, 1.f, 1.f);
    c[ImGuiCol_CheckMark] = ACCENT;
    c[ImGuiCol_PlotLines] = ACCENT;
}

void loadFont(const std::filesystem::path& exeDir) {
    namespace fs = std::filesystem;
    const fs::path candidates[] = {fs::path("fonts") / "font.otf", exeDir / "fonts" / "font.otf"};
    for (const fs::path& path : candidates) {
        std::error_code ec;
        if (!fs::exists(path, ec)) continue;

        ImGuiIO& io = ImGui::GetIO();
        io.Fonts->Clear();
        if (io.Fonts->AddFontFromFileTTF(path.string().c_str(), 16.f) && ImGui::SFML::UpdateFontTexture())
            return;

        std::cerr << "EMMA: couldn't load " << path.string() << ", using ImGui's built-in font\n";
        io.Fonts->Clear();
        io.Fonts->AddFontDefault();
        if (!ImGui::SFML::UpdateFontTexture()) std::cerr << "EMMA: font texture upload failed\n";
        return;
    }
    std::cerr << "EMMA: fonts/font.otf not found, using ImGui's built-in font\n";
}

void drawSidebar(UiState& st, emma::Clock& clock, const Stats& stats, Preset preset, const char* integratorName, float width, float height) {
    ImGui::SetNextWindowPos(ImVec2(0.f, 0.f));
    ImGui::SetNextWindowSize(ImVec2(width, height));
    const ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
                                   ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse |
                                   ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoBringToFrontOnFocus;
    ImGui::Begin("EMMA##sidebar", nullptr, flags);
    const ImGuiStyle& style = ImGui::GetStyle();

    // Header: the name on the left, run controls on the right
    ImGui::AlignTextToFramePadding();
    ImGui::TextColored(ACCENT, "EMMA");
    ImGui::SameLine(width - style.WindowPadding.x - 116.f);
    if (ImGui::Button(clock.paused ? "Run###run" : "Pause###run", ImVec2(62.f, 0.f))) clock.paused = !clock.paused;
    ImGui::SameLine();
    if (ImGui::Button("Step", ImVec2(46.f, 0.f))) clock.requestStep();
    ImGui::Spacing();

    if (ImGui::CollapsingHeader("Simulation", ImGuiTreeNodeFlags_DefaultOpen)) {
        rowLabel("Time");
        ImGui::SetNextItemWidth(-1.f);
        const std::string rate = clock.live ? std::string("Real time") : formatRate(clock.timeScale);
        float scale = static_cast<float>(clock.timeScale);
        if (ImGui::SliderFloat("##timescale", &scale, 0.1f, 3650.f, rate.c_str(), ImGuiSliderFlags_Logarithmic)) {
            clock.timeScale = static_cast<double>(scale);
            clock.live = false;  // picking a speed leaves real time
        }
        if (preset == Preset::SolarSystem) {
            if (ImGui::Checkbox("Real time", &clock.live) && clock.live) clock.paused = false;
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("One second per second, in step with your clock: the planets where they are right now");
        }
        // Which integrator moves the bodies
        static const char* const integrators[] = {"Velocity Verlet", "IAS15"};
        int currentIntegrator = 0;
        for (int i = 0; i < IM_ARRAYSIZE(integrators); ++i) {
            if (std::strcmp(integratorName, integrators[i]) == 0) currentIntegrator = i;
        }
        rowLabel("Integrator");
        ImGui::SetNextItemWidth(-1.f);
        if (ImGui::Combo("##integrator", &currentIntegrator, integrators, IM_ARRAYSIZE(integrators)))
            st.integratorRequested = currentIntegrator;
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("Velocity Verlet: fast, with a small energy wobble.\n"
                              "IAS15: as accurate as a double allows but slower, for up to %zu bodies.",
                              Simulation::DIRECT_LIMIT);
        if (stats.bodies > Simulation::DIRECT_LIMIT)
            hint(printf_string("Above %zu bodies EMMA uses Verlet: IAS15 is built for exact forces.",
                               Simulation::DIRECT_LIMIT).c_str());
        if (ImGui::Button("Clear")) st.clearRequested = true;
        ImGui::SameLine();
        if (ImGui::Button("Reset")) st.resetRequested = true;
        ImGui::SameLine();
        if (ImGui::Button("Fit view")) st.resetViewRequested = true;
        if (stats.behind) ImGui::TextColored(WARN, "Running behind, try a lower speed");
    }

    if (ImGui::CollapsingHeader("System", ImGuiTreeNodeFlags_DefaultOpen)) {
        static const char* const names[] = {"Solar system", "Alpha Centauri", "Empty space"};
        const int current = static_cast<int>(preset);
        for (int i = 0; i < 3; ++i) {
            if (ImGui::RadioButton(names[i], current == i)) st.presetRequested = i;
        }
        switch (preset) {
        case Preset::SolarSystem:
            hint(stats.sourceNote.c_str());
            if (stats.canRetry && ImGui::Button("Try again")) st.retryFetchRequested = true;
            break;
        case Preset::AlphaCentauri:
            hint("Two Sun-like stars on their 80-year orbit, starting at closest approach.");
            break;
        case Preset::Empty: hint("Nothing yet. Click to add bodies."); break;
        }
    }

    if (ImGui::CollapsingHeader("New body", ImGuiTreeNodeFlags_DefaultOpen)) {
        const double mass = std::pow(10.0, static_cast<double>(st.massExponent));
        rowLabel("Mass");
        ImGui::SetNextItemWidth(-1.f);
        ImGui::SliderFloat("##mass", &st.massExponent, -10.f, 2.f, formatMass(mass).c_str());

        // Swatch and class of the body a click would create
        ImGui::SetCursorPosX(style.WindowPadding.x + LABEL_WIDTH);
        const Rgb c = Body::colorForMass(mass);
        const ImVec2 at = ImGui::GetCursorScreenPos();
        const float lineHeight = ImGui::GetTextLineHeight();
        ImGui::GetWindowDrawList()->AddCircleFilled(ImVec2(at.x + 6.f, at.y + lineHeight * 0.5f), 5.f,
                                                    IM_COL32(c.r, c.g, c.b, 255));
        ImGui::Dummy(ImVec2(16.f, lineHeight));
        ImGui::SameLine();
        ImGui::TextDisabled("%s", Body::describe(mass).c_str());

        ImGui::BeginDisabled(st.autoOrbit);
        rowLabel("Speed");
        ImGui::SetNextItemWidth(-1.f);
        ImGui::SliderFloat("##speed", &st.speedKms, 0.f, 200.f, "%.1f km/s");
        rowLabel("Direction");
        angleDial("##direction", st.angle, 17.f);
        ImGui::SameLine();
        ImGui::Text("%.0f deg", static_cast<double>(st.angle) * 180.0 / units::PI);
        ImGui::EndDisabled();

        ImGui::Checkbox("Circular orbit", &st.autoOrbit);
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("Launch on a circular orbit around the body that dominates gravity at that spot");
        ImGui::Checkbox("Show predicted path", &st.showPath);
        hint("Click to place, or drag to set speed and direction. Speed is measured relative to the "
             "body that dominates gravity at that spot.");
    }

    if (ImGui::CollapsingHeader("View", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Checkbox("Trails", &st.showTrails);
        ImGui::SameLine(style.WindowPadding.x + LABEL_WIDTH + 20.f);
        ImGui::Checkbox("Labels", &st.showLabels);
        hint("Scroll to zoom, right-drag to pan, F to fit. Space pauses, N steps, Esc cancels a launch.");
    }

    if (ImGui::CollapsingHeader("Stats", ImGuiTreeNodeFlags_DefaultOpen)) {
        const float column = style.WindowPadding.x + VALUE_COLUMN;
        ImGui::Text("Bodies %zu", stats.bodies);
        ImGui::SameLine(column);
        ImGui::Text("FPS %.0f", static_cast<double>(stats.fps));

        ImGui::TextUnformatted("Energy drift");
        ImGui::SameLine(column);
        ImGui::TextColored(driftColor(stats.energyDrift), "%+.1e", stats.energyDrift);
        ImGui::TextUnformatted("Momentum drift");
        ImGui::SameLine(column);
        ImGui::TextColored(driftColor(stats.momentumDrift), "%+.1e", stats.momentumDrift);

        if (stats.driftHistory && !stats.driftHistory->empty()) {
            float scale = 1e-12f;
            for (float v : *stats.driftHistory) scale = std::max(scale, std::abs(v));
            ImGui::PlotLines("##drift", stats.driftHistory->data(), static_cast<int>(stats.driftHistory->size()),
                             0, nullptr, -1.2f * scale, 1.2f * scale, ImVec2(-1.f, 42.f));
        }
        hint("Relative change in total energy and angular momentum since bodies were last added, "
             "merged or loaded.");
    }

    ImGui::End();
}

void drawStatusBar(const Simulation& sim, const Camera& camera, sf::Vector2f mouse, const emma::Clock& clock,
                   float y, float width, float height) {
    ImGui::SetNextWindowPos(ImVec2(0.f, y));
    ImGui::SetNextWindowSize(ImVec2(width, height));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(12.f, 5.f));
    ImGui::Begin("##statusbar", nullptr,
                 ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings |
                     ImGuiWindowFlags_NoBringToFrontOnFocus);
    ImGui::PopStyleVar();

    const double viewWidth = camera.viewport.size.x / camera.pxPerAU;
    ImGui::TextDisabled("View %s across", formatDistance(viewWidth).c_str());
    if (camera.viewport.contains(mouse)) {
        const Vector3D w = camera.toWorld(mouse);
        ImGui::SameLine(0.f, 28.f);
        ImGui::TextDisabled("Cursor %.4f, %.4f AU", w.x, w.y);
    }
    ImGui::SameLine(0.f, 28.f);
    if (sim.hasCalendar() && clock.live)
        ImGui::TextDisabled("Date %s", calendar::localDateTime(sim.epoch() + sim.time()).c_str());
    else if (sim.hasCalendar())
        ImGui::TextDisabled("Date %s", calendar::date(sim.epoch() + sim.time()).c_str());
    else
        ImGui::TextDisabled("Elapsed %s", formatDuration(sim.time()).c_str());
    ImGui::SameLine(0.f, 28.f);
    if (sim.lastSubstepCount() > 1)
        ImGui::TextDisabled("Step %s, split in %d for a close pass", formatDuration(sim.timeStep()).c_str(),
                            sim.lastSubstepCount());
    else
        ImGui::TextDisabled("Step %s", formatDuration(sim.timeStep()).c_str());

    ImGui::SameLine(0.f, 28.f);
    ImGui::TextDisabled("%s", sim.getIntegrator().name());
    const char* status = clock.paused ? "Paused" : "Running";
    ImGui::SameLine(width - 12.f - ImGui::CalcTextSize(status).x);
    ImGui::TextColored(clock.paused ? WARN : ACCENT, "%s", status);
    ImGui::End();
}

void drawLabels(const Camera& camera, const std::vector<Body>& bodies) {
    ImDrawList* dl = ImGui::GetBackgroundDrawList();
    const ImU32 color = ImGui::GetColorU32(ImVec4(0.78f, 0.78f, 0.84f, 0.85f));
    for (const Body& b : bodies) {
        if (!b.isNamed()) continue;
        const sf::Vector2f p = camera.toScreen(b.position);
        if (!camera.viewport.contains(p)) continue;

        // Skip the label while a heavier named body covers this one (the Moon at solar-system zoom)
        bool covered = false;
        for (const Body& o : bodies) {
            if (&o == &b || !o.isNamed() || o.mass <= b.mass) continue;
            const sf::Vector2f q = camera.toScreen(o.position);
            const float reach = Renderer::screenRadius(o.mass, o.radius, camera.pxPerAU) + 8.f;
            const float dx = q.x - p.x, dy = q.y - p.y;
            if (dx * dx + dy * dy < reach * reach) {
                covered = true;
                break;
            }
        }
        if (covered) continue;

        const float r = Renderer::screenRadius(b.mass, b.radius, camera.pxPerAU);
        dl->AddText(ImVec2(p.x + r + 4.f, p.y - r - 14.f), color, b.name.c_str());
    }
}

void drawBodyTooltip(const Simulation& sim, const Camera& camera, sf::Vector2f mouse) {
    const std::vector<Body>& bodies = sim.getBodies();
    int hit = -1;
    float best = 1e30f;
    for (std::size_t i = 0; i < bodies.size(); ++i) {
        const sf::Vector2f p = camera.toScreen(bodies[i].position);
        const float r = std::max(Renderer::screenRadius(bodies[i].mass, bodies[i].radius, camera.pxPerAU), 6.f);
        const float dx = p.x - mouse.x, dy = p.y - mouse.y, d2 = dx * dx + dy * dy;
        if (d2 <= r * r && d2 < best) {
            best = d2;
            hit = static_cast<int>(i);
        }
    }
    if (hit < 0) return;

    const Body& b = bodies[static_cast<std::size_t>(hit)];
    const std::string kind = b.description();
    ImGui::BeginTooltip();
    ImGui::TextColored(ACCENT, "%s", b.isNamed() ? b.name.c_str() : kind.c_str());
    if (b.isNamed()) ImGui::TextDisabled("%s", kind.c_str());
    ImGui::Text("Mass: %s", formatMass(b.mass).c_str());
    ImGui::Text("Radius: %.0f km", b.radius * units::KM_PER_AU);

    const int hostIndex = sim.dominantBody(b.position, hit);
    if (hostIndex >= 0) {
        const Body& host = bodies[static_cast<std::size_t>(hostIndex)];
        const std::string hostName = host.isNamed() ? host.name : host.description();
        ImGui::Text("Distance: %s from %s", formatDistance((b.position - host.position).magnitude()).c_str(),
                    hostName.c_str());
        ImGui::Text("Speed: %.2f km/s relative to %s",
                    (b.velocity - host.velocity).magnitude() / units::KMS_TO_AU_PER_DAY, hostName.c_str());
    }
    ImGui::EndTooltip();
}

void drawLaunchReadout(sf::Vector2f at, double speedKms, double angleRad) {
    const std::string text = printf_string("%.1f km/s  %.0f deg", speedKms, angleRad * 180.0 / units::PI);
    ImGui::GetForegroundDrawList()->AddText(ImVec2(at.x + 14.f, at.y - 22.f), ImGui::GetColorU32(ACCENT),
                                            text.c_str());
}

std::string formatMass(double m) {
    if (m >= 0.08) return number(m) + " solar masses";
    const double jupiters = m / units::JUPITER_MASS;
    if (jupiters >= 0.1) return number(jupiters) + " Jupiter masses";
    return number(m / units::EARTH_MASS) + " Earth masses";
}

std::string formatDistance(double au) {
    if (au >= 1000.0) return printf_string("%.0f AU", au);
    if (au >= 0.01) return printf_string("%.3f AU", au);
    return printf_string("%.0f km", au * units::KM_PER_AU);
}

std::string formatRate(double daysPerSecond) {
    if (daysPerSecond >= units::DAYS_PER_YEAR) return printf_string("%.2f years/s", daysPerSecond / units::DAYS_PER_YEAR);
    if (daysPerSecond >= 1.0) return printf_string("%.1f days/s", daysPerSecond);
    return printf_string("%.1f hours/s", daysPerSecond * 24.0);
}

std::string formatDuration(double days) {
    if (days >= units::DAYS_PER_YEAR) return printf_string("%.2f years", days / units::DAYS_PER_YEAR);
    if (days >= 1.0) return printf_string("%.1f days", days);
    const double seconds = days * units::SECONDS_PER_DAY;
    if (seconds >= 3600.0) return printf_string("%.1f hours", seconds / 3600.0);
    if (seconds >= 60.0) return printf_string("%.1f minutes", seconds / 60.0);
    return printf_string("%.3f s", seconds);
}

}