#pragma once
#include "emma/Presets.hpp"
#include <atomic>
#include <filesystem>
#include <optional>
#include <string>

// Today's positions for the solar system preset, from NASA JPL's Horizons
// service (https://ssd.jpl.nasa.gov/horizons/), built on the DE441 ephemeris
namespace emma::horizons {

struct Result {
    bool ok = false;
    std::string error;
    SolarSnapshot snapshot;
};

// Downloads one state vector per body, one request at a time. Blocking, so run
// it on a worker thread. progress counts finished bodies
Result fetchSolarSystem(double julianDay, std::atomic<int>* progress, const std::atomic<bool>* cancel);

// The request for one body: barycentric, ecliptic plane, AU and days, a CSV
// table inside a JSON reply
std::string requestUrl(const char* command, double julianDay);

// Unpacks a JSON reply. On success, output holds the Horizons text. Otherwise
// error holds the "error" or "message" field, or a note on the HTTP status
bool parseReply(const std::string& json, long status, std::string& output, std::string& error);

// Reads the first row of a Horizons VECTORS table (CSV), in the J2000 ecliptic frame
bool parseStateVector(const std::string& text, Vector3D& position, Vector3D& velocity, std::string& error);

// The last download is saved, so later starts on the same day does not need network
std::filesystem::path defaultCachePath();
std::optional<SolarSnapshot> loadCache(const std::filesystem::path& file);
void saveCache(const std::filesystem::path& file, const SolarSnapshot& snapshot);

}