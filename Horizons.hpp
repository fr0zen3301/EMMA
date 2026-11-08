#pragma once
#include "Simulation.hpp"
#include <atomic>
#include <filesystem>
#include <optional>
#include <string>

// Today's positions for the solar system preset, from NASA JPL's Horizons
// service (https://ssd.jpl.nasa.gov/horizons/), built on the DE441 ephemeris.
namespace horizons {

struct Result {
    bool ok = false;
    std::string error;
    SolarSnapshot snapshot;
};

// Downloads one state vector per body, one request at a time. Blocking, so run
// it on a worker thread. progress counts finished bodies; cancel stops it
// between requests.
Result fetchSolarSystem(double julianDay, std::atomic<int>* progress, const std::atomic<bool>* cancel);

// The request for one body: barycentric, ecliptic plane, AU and days, CSV
std::string requestUrl(const char* command, double julianDay);

// Reads the first row of a Horizons VECTORS table (CSV) into the ecliptic plane
bool parseStateVector(const std::string& text, Vector2D& position, Vector2D& velocity, std::string& error);

// The last download is saved, so later starts on the same day need no network
std::filesystem::path defaultCachePath();
std::optional<SolarSnapshot> loadCache(const std::filesystem::path& file);
void saveCache(const std::filesystem::path& file, const SolarSnapshot& snapshot);

} // namespace horizons
