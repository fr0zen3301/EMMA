#include "Horizons.hpp"
#include "Http.hpp"
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <system_error>
#include <vector>

namespace horizons {
namespace {

// Horizons IDs in SolarSnapshot order. Mars to Neptune are their system
// barycenters, which matches the system masses EMMA uses for them
const char* const COMMANDS[SOLAR_BODY_COUNT] = {"10", "199", "299", "399", "301", "4", "5", "6", "7", "8"};

const char* const CACHE_HEADER = "EMMA Horizons cache v1";

std::string trim(const std::string& s) {
    const std::size_t first = s.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) return {};
    const std::size_t last = s.find_last_not_of(" \t\r\n");
    return s.substr(first, last - first + 1);
}

bool parseNumber(const std::string& field, double& out) {
    const std::string t = trim(field);
    if (t.empty()) return false;
    char* end = nullptr;
    out = std::strtod(t.c_str(), &end);
    return end == t.c_str() + t.size();
}

// Something readable from a response without a table: Horizons puts its
// error messages in the plain text, after the two API header lines
std::string firstMessage(const std::string& text) {
    std::istringstream lines(text);
    std::string line;
    while (std::getline(lines, line)) {
        const std::string t = trim(line);
        if (t.empty() || t[0] == '*' || t.rfind("API ", 0) == 0) continue;
        return t.size() > 120 ? t.substr(0, 120) + "..." : t;
    }
    return "Horizons sent no data";
}

}

std::string requestUrl(const char* command, double julianDay) {
    char time[32];
    std::snprintf(time, sizeof(time), "%.6f", julianDay);

    // Values are wrapped in single quotes, sent as %27; '@' is %40
    std::string url = "https://ssd.jpl.nasa.gov/api/horizons.api?format=text";
    url += "&COMMAND=%27";
    url += command;
    url += "%27&OBJ_DATA=%27NO%27&MAKE_EPHEM=%27YES%27&EPHEM_TYPE=%27VECTORS%27";
    url += "&CENTER=%27500%400%27";  // solar system barycenter
    url += "&REF_PLANE=%27ECLIPTIC%27&REF_SYSTEM=%27ICRF%27";
    url += "&TLIST=%27";
    url += time;
    url += "%27&TLIST_TYPE=%27JD%27&TIME_TYPE=%27UT%27";
    url += "&OUT_UNITS=%27AU-D%27&VEC_TABLE=%272%27&VEC_CORR=%27NONE%27";
    url += "&CSV_FORMAT=%27YES%27&VEC_LABELS=%27NO%27";
    return url;
}

bool parseStateVector(const std::string& text, Vector2D& position, Vector2D& velocity, std::string& error) {
    const std::size_t soe = text.find("$$SOE");
    const std::size_t eoe = text.find("$$EOE");
    if (soe == std::string::npos || eoe == std::string::npos || eoe < soe) {
        error = firstMessage(text);
        return false;
    }

    std::istringstream rows(text.substr(soe + 5, eoe - soe - 5));
    std::string row;
    while (std::getline(rows, row)) {
        if (trim(row).empty()) continue;

        // JD, calendar date, X, Y, Z, VX, VY, VZ
        std::vector<std::string> fields;
        std::istringstream cells(row);
        std::string cell;
        while (std::getline(cells, cell, ',')) fields.push_back(cell);

        double v[6] = {};
        bool ok = fields.size() >= 8;
        for (int i = 0; ok && i < 6; ++i) ok = parseNumber(fields[2 + static_cast<std::size_t>(i)], v[i]);
        if (!ok) {
            const std::string t = trim(row);
            error = "unexpected row in the Horizons table: " + t.substr(0, 80);
            return false;
        }
        position = Vector2D(v[0], v[1]);
        velocity = Vector2D(v[3], v[4]);
        return true;
    }
    error = "Horizons returned an empty table";
    return false;
}

Result fetchSolarSystem(double julianDay, std::atomic<int>* progress, const std::atomic<bool>* cancel) {
    Result result;
    result.snapshot.julianDay = julianDay;
    for (std::size_t i = 0; i < SOLAR_BODY_COUNT; ++i) {
        if (cancel && cancel->load()) {
            result.error = "cancelled";
            return result;
        }
        std::string body, error;
        if (!http::get(requestUrl(COMMANDS[i], julianDay), body, error) ||
            !parseStateVector(body, result.snapshot.position[i], result.snapshot.velocity[i], error)) {
            result.error = error;
            return result;
        }
        if (progress) progress->fetch_add(1);
    }
    result.ok = true;
    return result;
}

std::filesystem::path defaultCachePath() {
    std::error_code ec;
    const std::filesystem::path dir = std::filesystem::temp_directory_path(ec);
    return ec ? std::filesystem::path("emma_horizons_cache.txt") : dir / "emma_horizons_cache.txt";
}

std::optional<SolarSnapshot> loadCache(const std::filesystem::path& file) {
    std::ifstream in(file);
    if (!in) return std::nullopt;
    std::string header;
    if (!std::getline(in, header) || trim(header) != CACHE_HEADER) return std::nullopt;

    SolarSnapshot s;
    if (!(in >> s.julianDay)) return std::nullopt;
    for (std::size_t i = 0; i < SOLAR_BODY_COUNT; ++i) {
        if (!(in >> s.position[i].x >> s.position[i].y >> s.velocity[i].x >> s.velocity[i].y))
            return std::nullopt;
    }
    return s;
}

void saveCache(const std::filesystem::path& file, const SolarSnapshot& s) {
    std::ofstream out(file, std::ios::trunc);
    if (!out) return;  // a missing cache only costs a download next time
    out << CACHE_HEADER << '\n';
    out.precision(17);
    out << s.julianDay << '\n';
    for (std::size_t i = 0; i < SOLAR_BODY_COUNT; ++i)
        out << s.position[i].x << ' ' << s.position[i].y << ' ' << s.velocity[i].x << ' ' << s.velocity[i].y << '\n';
}

}
