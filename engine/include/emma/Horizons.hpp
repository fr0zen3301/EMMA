#pragma once
#include "emma/Presets.hpp"
#include <atomic>
#include <filesystem>
#include <optional>
#include <string>

// Downloads today's solar system positions from NASA JPL's Horizons service
// (https://ssd.jpl.nasa.gov/horizons/)
namespace emma::horizons {

struct Result {
    bool ok = false;
    std::string error;
    SolarSnapshot snapshot;
};

// Downloads every body, one at a time. Slow, so run it on a worker thread.
// progress counts finished bodies
Result fetchSolarSystem(double julianDay, std::atomic<int>* progress, const std::atomic<bool>* cancel);

// The web address that asks Horizons for one body's position and velocity
std::string requestUrl(const char* command, double julianDay);

// Unpacks Horizons' JSON reply into output. On failure, error says why
bool parseReply(const std::string& json, long status, std::string& output, std::string& error);

// Reads a position and velocity from Horizons' table
bool parseStateVector(const std::string& text, Vector3D& position, Vector3D& velocity, std::string& error);

// The last download is saved, so a second start on the same day needs no network
std::filesystem::path defaultCachePath();
std::optional<SolarSnapshot> loadCache(const std::filesystem::path& file);
void saveCache(const std::filesystem::path& file, const SolarSnapshot& snapshot);

}