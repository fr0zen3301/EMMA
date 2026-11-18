#include "Ui.hpp"
#include "Renderer.hpp"
#include "emma/BlackHole.hpp"
#include "emma/Calendar.hpp"
#include "emma/Constants.hpp"
#include "imgui.h"
#include "imgui-SFML.h"
#include <algorithm>
#include <cfloat>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <initializer_list>
#include <iostream>
#include <system_error>

using namespace emma;

namespace {

const ImVec4 ACCENT(0.851f, 0.812f, 1.0f, 1.0f);
const ImVec4 MUTED(0.55f, 0.56f, 0.60f, 1.0f);
const ImVec4 GOOD(0.56f, 0.89f, 0.69f, 1.0f);
const ImVec4 WARN(1.0f, 0.75f, 0.42f, 1.0f);
const ImVec4 SELECTED(0.184f, 0.169f, 0.271f, 1.0f);  // the lit part of a switch
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

// Above this many AU, show distances in parsecs
constexpr double PARSEC_FROM_AU = 1e5;
constexpr double AU_PER_KPC = 1e3 * units::AU_PER_PARSEC;

// A distance in light-years
std::string lightYears(double au) {
    const double ly = au / units::AU_PER_LIGHT_YEAR;
    if (ly < 10.0) return printf_string("%.1f ly", ly);
    return printf_string("%.0f ly", ly);
}

// A label on the left, with its control lined up next to it
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

// Says in words how slow a clock runs at the middle of the view, for the bent grid
std::string clockLine(double lag) {
    if (lag >= 1.0) return "A clock here has stopped: this is at or inside a horizon. Not to scale.";
    const double seconds = lag * units::DAYS_PER_YEAR * units::SECONDS_PER_DAY;  // lost in a year
    const std::string amount = lag >= 1e-3 ? printf_string("%.1f%%", 100.0 * lag) : printf_string("%.1e", lag);
    std::string yearly;
    if (seconds >= units::SECONDS_PER_DAY) yearly = printf_string("%.1f days", seconds / units::SECONDS_PER_DAY);
    else if (seconds >= 1.0) yearly = printf_string("%.2f s", seconds);
    else if (seconds >= 1e-3) yearly = printf_string("%.2f ms", seconds * 1e3);
    else yearly = printf_string("%.2f microseconds", seconds * 1e6);
    return "A clock here runs " + amount + " slow: " + yearly + " a year. Not to scale.";
}

// A tooltip for the last control
void tip(const char* text) {
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s", text);
}

// A row of buttons that work as one choice, filling the rest of the line. Returns true
// when the choice changes
bool segmented(const char* id, int& value, std::initializer_list<const char*> labels,
               std::initializer_list<const char*> tips) {
    const float gap = 2.f;
    const float count = static_cast<float>(labels.size());
    const float w = (ImGui::GetContentRegionAvail().x - gap * (count - 1.f)) / count;
    bool changed = false;
    ImGui::PushID(id);
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(gap, ImGui::GetStyle().ItemSpacing.y));
    const char* const* text = tips.begin();
    int i = 0;
    for (const char* label : labels) {
        const bool on = i == value;
        ImGui::PushStyleColor(ImGuiCol_Button, on ? SELECTED : ImGui::GetStyleColorVec4(ImGuiCol_FrameBg));
        ImGui::PushStyleColor(ImGuiCol_Text, on ? ACCENT : MUTED);
        if (ImGui::Button(label, ImVec2(w, 0.f)) && !on) {
            value = i;
            changed = true;
        }
        ImGui::PopStyleColor(2);
        if (text != tips.end()) tip(*text++);
        if (++i < static_cast<int>(labels.size())) ImGui::SameLine();
    }
    ImGui::PopStyleVar();
    ImGui::PopID();
    return changed;
}

// A button that stays lit while it's on. Returns true when clicked
bool toggleChip(const char* label, bool& on, float width) {
    ImGui::PushStyleColor(ImGuiCol_Button, on ? SELECTED : ImGui::GetStyleColorVec4(ImGuiCol_FrameBg));
    ImGui::PushStyleColor(ImGuiCol_Text, on ? ACCENT : MUTED);
    const bool clicked = ImGui::Button(label, ImVec2(width, 0.f));
    ImGui::PopStyleColor(2);
    if (clicked) on = !on;
    return clicked;
}

ImVec4 driftColor(double drift) {
    const double a = std::abs(drift);
    if (a < 1e-6) return GOOD;
    if (a < 1e-3) return ImGui::GetStyleColorVec4(ImGuiCol_Text);
    return WARN;
}

// A round dial for picking the launch direction (angle in radians)
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
    c[ImGuiCol_Tab] = ImVec4(0.067f, 0.071f, 0.102f, 1.f);
    c[ImGuiCol_TabHovered] = ImVec4(0.15f, 0.15f, 0.21f, 1.f);
    c[ImGuiCol_TabSelected] = SELECTED;
    c[ImGuiCol_TabSelectedOverline] = ACCENT;
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

namespace {

// The Add tab: what the next click puts in space
void addTab(UiState& st) {
    const ImGuiStyle& style = ImGui::GetStyle();
    int kind = st.spawnHole ? 1 : 0;
    if (segmented("##kind", kind, {"Planet or star", "Black hole"},
                  {"A planet, moon or star", "No surface: swallows whatever touches it"}))
        st.spawnHole = kind == 1;

    const double mass = std::pow(10.0, static_cast<double>(st.spawnHole ? st.holeMassExponent : st.massExponent));
    rowLabel("Mass");
    ImGui::SetNextItemWidth(-1.f);
    if (st.spawnHole)
        ImGui::SliderFloat("##holemass", &st.holeMassExponent, 0.f, 10.f, formatMass(mass).c_str());
    else
        ImGui::SliderFloat("##mass", &st.massExponent, -10.f, 2.f, formatMass(mass).c_str());

    // Preview of the body a click would add: its color and kind
    ImGui::SetCursorPosX(style.WindowPadding.x + LABEL_WIDTH);
    const Rgb c = st.spawnHole ? Body::HOLE_COLOR : Body::colorForMass(mass);
    const ImVec2 at = ImGui::GetCursorScreenPos();
    const float lineHeight = ImGui::GetTextLineHeight();
    ImDrawList* dl = ImGui::GetWindowDrawList();
    const ImVec2 dot(at.x + 6.f, at.y + lineHeight * 0.5f);
    dl->AddCircleFilled(dot, 5.f, IM_COL32(c.r, c.g, c.b, 255));
    if (st.spawnHole) dl->AddCircleFilled(dot, 3.f, IM_COL32(0, 0, 0, 255));  // the shadow in its ring
    ImGui::Dummy(ImVec2(16.f, lineHeight));
    ImGui::SameLine();
    ImGui::TextDisabled("%s", (st.spawnHole ? Body::describeBlackHole(mass) : Body::describe(mass)).c_str());

    if (st.spawnHole) {
        rowLabel("Spin");
        ImGui::SetNextItemWidth(-1.f);
        ImGui::SliderFloat("##holespin", &st.holeSpin, 0.f, static_cast<float>(blackhole::MAX_SPIN), "%.3f");
        tip("0 is no spin, 0.998 is the fastest. Spin shrinks the horizon");
    }

    ImGui::Spacing();
    int launch = st.autoOrbit ? 0 : 1;
    if (segmented("##launch", launch, {"Circular orbit", "Custom"},
                  {"Orbit whatever is nearest", "Set the speed and direction yourself"}))
        st.autoOrbit = launch == 0;
    if (!st.autoOrbit) {
        rowLabel("Speed");
        ImGui::SetNextItemWidth(-1.f);
        ImGui::SliderFloat("##speed", &st.speedKms, 0.f, 200.f, "%.1f km/s");
        tip("Relative to whatever is nearest");
        rowLabel("Direction");
        angleDial("##direction", st.angle, 17.f);
        ImGui::SameLine();
        ImGui::Text("%.0f deg", static_cast<double>(st.angle) * 180.0 / units::PI);
    }
    ImGui::Checkbox("Show path preview", &st.showPath);
    tip("A dashed line showing where it will go");
    hint("Click to place. Drag to throw.");
}

// The View tab: the camera and what's drawn
void viewTab(UiState& st, const Stats& stats, Preset preset, const std::vector<Body>& bodies) {
    const ImGuiStyle& style = ImGui::GetStyle();
    rowLabel("Follow");
    ImGui::SetNextItemWidth(-1.f);
    if (ImGui::BeginCombo("##follow", st.follow.empty() ? "Nothing" : st.follow.c_str())) {
        if (ImGui::Selectable("Nothing", st.follow.empty())) st.follow.clear();
        for (const Body& b : bodies) {
            if (b.isNamed() && ImGui::Selectable(b.name.c_str(), st.follow == b.name)) st.follow = b.name;
        }
        if (preset == Preset::SolarSystem && ImGui::Selectable(SGR_A, st.follow == SGR_A)) st.follow = SGR_A;
        ImGui::EndCombo();
    }
    tip("Keep a body in the middle of the view");

    const float half = 0.5f * (ImGui::GetContentRegionAvail().x - style.ItemSpacing.x);
    if (ImGui::Button("Fit view (F)", ImVec2(half, 0.f))) st.resetViewRequested = true;
    tip("Fit the whole system in view");
    ImGui::SameLine();
    if (ImGui::Button("Top view (T)", ImVec2(half, 0.f))) st.topViewRequested = true;
    tip("Look straight down");

    rowLabel("Show");
    const float third = (ImGui::GetContentRegionAvail().x - 2.f * style.ItemSpacing.x) / 3.f;
    toggleChip("Trails", st.showTrails, third);
    tip("The paths bodies have taken");
    ImGui::SameLine();
    toggleChip("Labels", st.showLabels, third);
    tip("Names next to bodies");
    ImGui::SameLine();
    toggleChip("Grid", st.showGrid, third);
    tip("The plane new bodies are placed on");

    if (ImGui::Checkbox("Bend the grid", &st.bendGrid) && st.bendGrid) st.showGrid = true;
    tip("The grid dips where clocks run slower");
    if (st.bendGrid) {
        ImGui::Indent();
        rowLabel("Depth");
        ImGui::SetNextItemWidth(-1.f);
        // ImGui reads the label as a format, so the % is doubled, and it mustn't round through it
        const std::string deep = printf_string("%.0f%%%%", 100.0 * static_cast<double>(st.gridDepth));
        ImGui::SliderFloat("##griddepth", &st.gridDepth, 0.05f, 0.5f, deep.c_str(), ImGuiSliderFlags_NoRoundToFormat);
        tip("How deep the dips are drawn, as a share of the view");
        if (stats.clockSlowdown >= 0.0) hint(clockLine(stats.clockSlowdown).c_str());
        ImGui::Unindent();
    }

    if (preset == Preset::SolarSystem) {
        ImGui::Checkbox("Galaxy view", &st.galaxyView);
        tip("Show the solar system moving through the Galaxy");
        if (st.galaxyView) {
            const float real = static_cast<float>(galaxy::sunVelocityKms().magnitude());
            ImGui::Indent();
            rowLabel("Speed");
            ImGui::SetNextItemWidth(-56.f);
            ImGui::SliderFloat("##sunspeed", &st.sunSpeedKms, 0.f, 300.f, "%.0f km/s");
            ImGui::SameLine();
            if (ImGui::Button("Real")) st.sunSpeedKms = real;
            if (ImGui::IsItemHovered()) ImGui::SetTooltip("The Sun's real speed, %.0f km/s", static_cast<double>(real));
            hint(printf_string("The Sun has moved %s.", formatDistance(stats.sunTravelled).c_str()).c_str());
            ImGui::Unindent();
        }
    }
}

// The Physics tab: how bodies move, and how accurately
void physicsTab(UiState& st, const Stats& stats, const char* integratorName) {
    const ImGuiStyle& style = ImGui::GetStyle();
    int integrator = std::strcmp(integratorName, "IAS15") == 0 ? 1 : 0;
    rowLabel("Integrator");
    if (segmented("##integrator", integrator, {"Verlet", "IAS15"},
                  {"Fast, with a small energy wobble", "Most accurate, but slower"}))
        st.integratorRequested = integrator;
    if (stats.bodies > Simulation::DIRECT_LIMIT)
        hint(printf_string("With more than %zu bodies, Verlet is used.", Simulation::DIRECT_LIMIT).c_str());
    else
        hint(integrator == 0 ? "Fast, with a small energy wobble." : "Most accurate, but slower.");

    ImGui::Checkbox("General relativity", &st.relativity);
    tip("Einstein's gravity: orbits slowly turn each lap");
    if (st.relativity) {
        ImGui::Indent();
        rowLabel("Exaggerate");
        ImGui::SetNextItemWidth(-1.f);
        const std::string boost = printf_string("%.0fx", static_cast<double>(st.relativityScale));
        ImGui::SliderFloat("##relativity", &st.relativityScale, 1.f, 100.f, boost.c_str(),
                           ImGuiSliderFlags_Logarithmic);
        tip("1x is real. Raise it to see orbits turn");
        ImGui::Unindent();
    }

    // How well energy and angular momentum are kept
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();
    const float column = style.WindowPadding.x + VALUE_COLUMN;
    ImGui::TextUnformatted("Energy drift");
    ImGui::SameLine(column);
    ImGui::TextColored(driftColor(stats.energyDrift), "%+.1e", stats.energyDrift);
    ImGui::TextUnformatted("Momentum drift");
    ImGui::SameLine(column);
    ImGui::TextColored(driftColor(stats.momentumDrift), "%+.1e", stats.momentumDrift);
    if (stats.driftHistory && !stats.driftHistory->empty()) {
        float scale = 1e-12f;
        for (float v : *stats.driftHistory) scale = std::max(scale, std::abs(v));
        ImGui::PlotLines("##drift", stats.driftHistory->data(), static_cast<int>(stats.driftHistory->size()), 0,
                         nullptr, -1.2f * scale, 1.2f * scale, ImVec2(-1.f, 42.f));
    }
    ImGui::TextDisabled("%zu bodies", stats.bodies);
    ImGui::SameLine(column);
    ImGui::TextDisabled("%.0f FPS", static_cast<double>(stats.fps));
    hint("Drift since bodies were last added or merged.");
}

// The Black holes tab: how they look up close
void blackHoleTab(UiState& st, Preset preset, const char* holeName) {
    const ImGuiStyle& style = ImGui::GetStyle();
    if (holeName) {
        const std::string go = std::string("Go to ") + holeName;
        if (ImGui::Button(go.c_str(), ImVec2(-FLT_MIN, 0.f))) st.goToHoleRequested = true;
        tip(preset == Preset::SolarSystem ? "Jump 8.15 kpc to the Galactic center and look at the hole"
                                          : "Jump to it and look at the hole");
    }
    segmented("##look", st.holeLook, {"Real gas", "Thin disk", "No disk"},
              {"The dim hot gas Sgr A* really has", "A textbook disk. Sgr A* doesn't have one",
               "Only the stars, bent around the hole"});
    ImGui::Checkbox("Bend light", &st.holeBendLight);
    tip("Turn off to see the view without gravity");
    ImGui::SameLine(style.WindowPadding.x + LABEL_WIDTH + 20.f);
    ImGui::Checkbox("Doppler", &st.holeDoppler);
    tip("Gas moving toward you looks brighter and bluer");
    rowLabel("Brightness");
    ImGui::SetNextItemWidth(-1.f);
    const std::string exposure = printf_string("%.1fx", std::pow(2.0, static_cast<double>(st.holeBrightness)));
    ImGui::SliderFloat("##holebrightness", &st.holeBrightness, -2.f, 2.f, exposure.c_str());
    hint("Up close, the biggest-looking hole is ray traced.");
}

} // namespace

void drawSidebar(UiState& st, emma::Clock& clock, const Stats& stats, Preset preset, const std::vector<Body>& bodies,
                 const char* integratorName, bool anyHole, const char* holeName, float width, float height) {
    ImGui::SetNextWindowPos(ImVec2(0.f, 0.f));
    ImGui::SetNextWindowSize(ImVec2(width, height));
    const ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
                                   ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse |
                                   ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoBringToFrontOnFocus;
    ImGui::Begin("EMMA##sidebar", nullptr, flags);
    const ImGuiStyle& style = ImGui::GetStyle();

    // The name, and the shortcuts behind a ? button
    ImGui::AlignTextToFramePadding();
    ImGui::TextColored(ACCENT, "EMMA");
    ImGui::SameLine(width - style.WindowPadding.x - 28.f);
    if (ImGui::Button("?", ImVec2(28.f, 0.f))) ImGui::OpenPopup("##shortcuts");
    tip("Keyboard and mouse shortcuts");
    if (ImGui::BeginPopup("##shortcuts")) {
        ImGui::TextUnformatted("Scroll: zoom. Double-click a body: follow it.");
        ImGui::TextUnformatted("Right-drag: turn. Shift+right-drag or middle-drag: pan.");
        ImGui::TextUnformatted("F: fit view. T: top view.");
        ImGui::TextUnformatted("Space: pause. N: step. Esc: cancel a launch.");
        ImGui::EndPopup();
    }

    // Time: run, step, real time and speed
    if (ImGui::Button(clock.paused ? "Run###run" : "Pause###run", ImVec2(70.f, 0.f))) clock.paused = !clock.paused;
    tip("Space");
    ImGui::SameLine();
    if (ImGui::Button("Step", ImVec2(56.f, 0.f))) clock.requestStep();
    tip("One step (N)");
    if (preset == Preset::SolarSystem) {
        ImGui::SameLine();
        if (toggleChip("Real time", clock.live, -FLT_MIN) && clock.live) clock.paused = false;
        tip("Run at one second per second, matching your clock");
    }
    ImGui::SetNextItemWidth(-1.f);
    const std::string rate = clock.live ? std::string("Real time") : formatRate(clock.timeScale);
    float scale = static_cast<float>(clock.timeScale);
    if (ImGui::SliderFloat("##timescale", &scale, 0.1f, 3650.f, rate.c_str(), ImGuiSliderFlags_Logarithmic)) {
        clock.timeScale = static_cast<double>(scale);
        clock.live = false;  // picking a speed leaves real time
    }
    tip("How fast time runs");
    if (stats.behind) ImGui::TextColored(WARN, "Running behind: try a lower speed");
    if (clock.live && stats.aheadDays > 1.0 / units::SECONDS_PER_DAY) {
        ImGui::PushStyleColor(ImGuiCol_Text, WARN);
        ImGui::TextWrapped("%s ahead of now, so real time is waiting. Reset goes back to today.",
                           formatDuration(stats.aheadDays).c_str());
        ImGui::PopStyleColor();
    }

    // The system, with Reset beside it
    static const char* const names[] = {"Solar system", "Alpha Centauri", "Sgr A* and the S-stars", "Empty space"};
    constexpr float RESET_WIDTH = 56.f;
    int current = static_cast<int>(preset);
    ImGui::SetNextItemWidth(-(RESET_WIDTH + style.ItemSpacing.x));
    if (ImGui::Combo("##preset", &current, names, IM_ARRAYSIZE(names)) && current != static_cast<int>(preset))
        st.presetRequested = current;
    tip("Pick a system");
    ImGui::SameLine();
    if (ImGui::Button("Reset", ImVec2(RESET_WIDTH, 0.f))) st.resetRequested = true;
    tip("Start this system again");
    switch (preset) {
    case Preset::SolarSystem:
        hint(stats.sourceNote.c_str());
        if (stats.canRetry && ImGui::Button("Try again")) st.retryFetchRequested = true;
        break;
    case Preset::AlphaCentauri: hint("Two Sun-like stars on an 80-year orbit."); break;
    case Preset::GalacticCenter: hint("Four stars around the Milky Way's black hole, from 2017."); break;
    case Preset::Empty: hint("Nothing yet. Click to add bodies."); break;
    }
    ImGui::Spacing();

    if (ImGui::BeginTabBar("##tabs")) {
        if (ImGui::BeginTabItem("Add")) {
            addTab(st);
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("View")) {
            viewTab(st, stats, preset, bodies);
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Physics")) {
            physicsTab(st, stats, integratorName);
            ImGui::EndTabItem();
        }
        if (anyHole && ImGui::BeginTabItem("Black holes")) {
            blackHoleTab(st, preset, holeName);
            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
    }

    ImGui::End();
}

void drawStatusBar(const Simulation& sim, const Camera& camera, const SceneFrame& frame, sf::Vector2f mouse,
                   const emma::Clock& clock, float y, float width, float height) {
    ImGui::SetNextWindowPos(ImVec2(0.f, y));
    ImGui::SetNextWindowSize(ImVec2(width, height));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(12.f, 5.f));
    ImGui::Begin("##statusbar", nullptr,
                 ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings |
                     ImGuiWindowFlags_NoBringToFrontOnFocus);
    ImGui::PopStyleVar();

    const double viewWidth = camera.viewport.size.x / camera.pxPerAU();
    ImGui::TextDisabled("View %s across", formatDistance(viewWidth).c_str());
    if (camera.viewport.contains(mouse) && !frame.galactic) {
        if (const std::optional<Vector3D> w = camera.pick(mouse)) {
            ImGui::SameLine(0.f, 28.f);
            if (viewWidth >= PARSEC_FROM_AU) {  // same unit as the view width
                const bool kpc = viewWidth >= AU_PER_KPC;
                const double unit = kpc ? AU_PER_KPC : units::AU_PER_PARSEC;
                ImGui::TextDisabled("Cursor %.3f, %.3f %s", w->x / unit, w->y / unit, kpc ? "kpc" : "pc");
            } else {
                ImGui::TextDisabled("Cursor %.4f, %.4f AU", w->x, w->y);
            }
        }
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

void drawLabels(const Camera& camera, const SceneFrame& frame, const std::vector<Body>& bodies) {
    ImDrawList* dl = ImGui::GetBackgroundDrawList();
    const ImU32 color = ImGui::GetColorU32(ImVec4(0.78f, 0.78f, 0.84f, 0.85f));
    for (const Body& b : bodies) {
        if (!b.isNamed() || b.isBlackHole()) continue;  // black holes get their labels elsewhere
        const Vector3D at = frame.place(b.position);
        const std::optional<sf::Vector2f> p = camera.toScreen(at);
        if (!p || !camera.viewport.contains(*p)) continue;

        // Skip the label if a heavier body covers this one (like the Moon when zoomed out)
        bool covered = false;
        for (const Body& o : bodies) {
            if (&o == &b || !o.isNamed() || o.mass <= b.mass) continue;
            const Vector3D other = frame.place(o.position);
            const std::optional<sf::Vector2f> q = camera.toScreen(other);
            if (!q) continue;
            const float reach = Renderer::screenRadius(o.mass, o.radius, camera.pxPerAUAt(other)) + 8.f;
            const float dx = q->x - p->x, dy = q->y - p->y;
            if (dx * dx + dy * dy < reach * reach) {
                covered = true;
                break;
            }
        }
        if (covered) continue;

        const float r = Renderer::screenRadius(b.mass, b.radius, camera.pxPerAUAt(at));
        dl->AddText(ImVec2(p->x + r + 4.f, p->y - r - 14.f), color, b.name.c_str());
    }
}

bool overBlackHole(const Renderer::BlackHoleShot& shot, sf::Vector2f mouse) {
    if (!shot.visible) return false;
    const float reach = std::max(shot.radius, 8.f);
    const float dx = shot.at.x - mouse.x, dy = shot.at.y - mouse.y;
    return dx * dx + dy * dy <= reach * reach;
}

void drawBlackHoleLabel(const Camera& camera, const Renderer::BlackHoleShot& shot) {
    if (!shot.visible || shot.hole.name.empty() || !camera.viewport.contains(shot.at)) return;
    ImGui::GetBackgroundDrawList()->AddText(ImVec2(shot.at.x + shot.radius + 4.f, shot.at.y - shot.radius - 14.f),
                                            ImGui::GetColorU32(ImVec4(0.78f, 0.78f, 0.84f, 0.85f)),
                                            shot.hole.name.c_str());
}

void drawBlackHoleTooltip(const Renderer::BlackHoleShot& shot, const std::string& where) {
    const Renderer::BlackHoleSight& h = shot.hole;
    const double rs = blackhole::schwarzschildRadiusAU(h.mass);
    ImGui::BeginTooltip();
    ImGui::TextColored(ACCENT, "%s", h.name.empty() ? h.kind.c_str() : h.name.c_str());
    if (!h.name.empty()) ImGui::TextDisabled("%s", h.kind.c_str());
    ImGui::Text("Mass: %s", formatMass(h.mass).c_str());
    ImGui::Text("Horizon radius: %.0f km", blackhole::horizonRadiusAU(h.mass, h.spin) * units::KM_PER_AU);
    if (h.spin > 0.0)
        ImGui::Text("Spin: %.3f", h.spin);
    else
        ImGui::Text("Spin: none");
    ImGui::Text("Shadow radius: %s", formatDistance(blackhole::SHADOW_RADIUS * rs).c_str());
    if (!where.empty()) ImGui::TextUnformatted(where.c_str());
    if (shot.rayTraced) ImGui::TextDisabled("Ray traced from %.0f Rs away", shot.eyeDistance / rs);
    if (h.spin > 0.0) ImGui::TextDisabled("Drawn as a hole that doesn't spin");
    ImGui::EndTooltip();
}

void drawTag(const Camera& camera, const Vector3D& at, const char* text) {
    const std::optional<sf::Vector2f> p = camera.toScreen(at);
    if (!p || !camera.viewport.contains(*p)) return;
    ImGui::GetBackgroundDrawList()->AddText(ImVec2(p->x + 6.f, p->y - 8.f), ImGui::GetColorU32(MUTED), text);
}

int bodyAt(const std::vector<Body>& bodies, const Camera& camera, const SceneFrame& frame, sf::Vector2f at) {
    struct Hit {
        std::size_t index;
        sf::Vector2f p;
        float r;
        float d2;
    };
    std::vector<Hit> hits;
    for (std::size_t i = 0; i < bodies.size(); ++i) {
        if (bodies[i].isBlackHole()) continue;
        const Vector3D w = frame.place(bodies[i].position);
        const std::optional<sf::Vector2f> p = camera.toScreen(w);
        if (!p) continue;
        const float r = std::max(Renderer::screenRadius(bodies[i].mass, bodies[i].radius, camera.pxPerAUAt(w)), 6.f);
        const float dx = p->x - at.x, dy = p->y - at.y, d2 = dx * dx + dy * dy;
        if (d2 <= r * r) hits.push_back(Hit{i, *p, r, d2});
    }

    int best = -1;
    float bestD2 = 1e30f;
    for (const Hit& h : hits) {
        bool hidden = false;
        for (const Hit& o : hits) {
            if (bodies[o.index].mass <= bodies[h.index].mass) continue;
            const float dx = o.p.x - h.p.x, dy = o.p.y - h.p.y;
            if (dx * dx + dy * dy < o.r * o.r) hidden = true;
        }
        if (!hidden && h.d2 < bestD2) {
            best = static_cast<int>(h.index);
            bestD2 = h.d2;
        }
    }
    return best;
}

void drawBodyTooltip(const Simulation& sim, const Camera& camera, const SceneFrame& frame, sf::Vector2f mouse) {
    const std::vector<Body>& bodies = sim.getBodies();
    const int hit = bodyAt(bodies, camera, frame, mouse);
    if (hit < 0) return;

    const Body& b = bodies[static_cast<std::size_t>(hit)];
    const std::string kind = b.description();
    ImGui::BeginTooltip();
    ImGui::TextColored(ACCENT, "%s", b.isNamed() ? b.name.c_str() : kind.c_str());
    if (b.isNamed()) ImGui::TextDisabled("%s", kind.c_str());
    if (b.mass > 0.0)
        ImGui::Text("Mass: %s", formatMass(b.mass).c_str());
    else
        ImGui::Text("Mass: not measured");
    if (b.radius > 0.0) ImGui::Text("Radius: %.0f km", b.radius * units::KM_PER_AU);

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
    if (au >= AU_PER_KPC) return printf_string("%.2f kpc (%s)", au / AU_PER_KPC, lightYears(au).c_str());
    if (au >= PARSEC_FROM_AU)
        return printf_string("%.2f pc (%s)", au / units::AU_PER_PARSEC, lightYears(au).c_str());
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