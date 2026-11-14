#include "emma/Horizons.hpp"
#include "Http.hpp"
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <map>
#include <sstream>
#include <system_error>
#include <vector>

namespace emma::horizons {
namespace {

// Horizons IDs in SolarSnapshot order. Mars to Neptune are their system
// barycenters, which matches the system masses EMMA uses for them
const char* const COMMANDS[SOLAR_BODY_COUNT] = {"10", "199", "299", "399", "301", "4", "5", "6", "7", "8"};

const char* const CACHE_HEADER = "EMMA Horizons cache v2";

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

// Rows of = or - are banner lines, not messages
bool isRule(const std::string& t) {
    return t.find_first_not_of("=-") == std::string::npos;
}

// Something readable from Horizons text. Its error comes after the API header
// lines and a "Toolkit version" line, framed by rows of =, and can run over
// several lines
std::string firstMessage(const std::string& text) {
    std::istringstream lines(text);
    std::string line, message;
    while (std::getline(lines, line)) {
        const std::string t = trim(line);
        const bool header = t.empty() || t[0] == '*' || isRule(t) || t.rfind("API ", 0) == 0 ||
                            t.rfind("Toolkit version", 0) == 0;
        if (header) {
            if (!message.empty()) break;  // the message has ended
            continue;
        }
        if (!message.empty()) message += ' ';
        message += t;
    }
    while (!message.empty() && message.back() == '.') message.pop_back();  // the caller adds its own period
    if (message.empty()) return "Horizons sent no data";
    return message.size() > 160 ? message.substr(0, 160) + "..." : message;
}

// ---------------------------------------------------------------------------
// Just enough JSON to read a Horizons reply: the string fields of the top-level
// object ("result", "error", "message"). Everything else is skipped

void skipSpace(const std::string& s, std::size_t& i) {
    while (i < s.size() && (s[i] == ' ' || s[i] == '\t' || s[i] == '\n' || s[i] == '\r')) ++i;
}

void appendUtf8(std::string& out, unsigned long cp) {
    if (cp < 0x80) {
        out += static_cast<char>(cp);
    } else if (cp < 0x800) {
        out += static_cast<char>(0xC0 | (cp >> 6));
        out += static_cast<char>(0x80 | (cp & 0x3F));
    } else if (cp < 0x10000) {
        out += static_cast<char>(0xE0 | (cp >> 12));
        out += static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
        out += static_cast<char>(0x80 | (cp & 0x3F));
    } else {
        out += static_cast<char>(0xF0 | (cp >> 18));
        out += static_cast<char>(0x80 | ((cp >> 12) & 0x3F));
        out += static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
        out += static_cast<char>(0x80 | (cp & 0x3F));
    }
}

bool readHex4(const std::string& s, std::size_t i, unsigned long& value) {
    if (i + 4 > s.size()) return false;
    value = 0;
    for (std::size_t k = i; k < i + 4; ++k) {
        const char c = s[k];
        value <<= 4;
        if (c >= '0' && c <= '9') value |= static_cast<unsigned long>(c - '0');
        else if (c >= 'a' && c <= 'f') value |= static_cast<unsigned long>(c - 'a' + 10);
        else if (c >= 'A' && c <= 'F') value |= static_cast<unsigned long>(c - 'A' + 10);
        else return false;
    }
    return true;
}

// A JSON string starting at the quote s[i]; i ends just past the closing quote
bool readString(const std::string& s, std::size_t& i, std::string& out) {
    if (i >= s.size() || s[i] != '"') return false;
    out.clear();
    for (++i; i < s.size(); ++i) {
        const char c = s[i];
        if (c == '"') {
            ++i;
            return true;
        }
        if (c != '\\') {
            out += c;
            continue;
        }
        if (++i >= s.size()) return false;
        switch (s[i]) {
        case '"': out += '"'; break;
        case '\\': out += '\\'; break;
        case '/': out += '/'; break;
        case 'b': out += '\b'; break;
        case 'f': out += '\f'; break;
        case 'n': out += '\n'; break;
        case 'r': out += '\r'; break;
        case 't': out += '\t'; break;
        case 'u': {
            unsigned long cp = 0;
            if (!readHex4(s, i + 1, cp)) return false;
            i += 4;
            // A high surrogate followed by a low one makes a single code point
            unsigned long low = 0;
            if (cp >= 0xD800 && cp <= 0xDBFF && i + 6 < s.size() && s[i + 1] == '\\' && s[i + 2] == 'u' &&
                readHex4(s, i + 3, low) && low >= 0xDC00 && low <= 0xDFFF) {
                cp = 0x10000 + ((cp - 0xD800) << 10) + (low - 0xDC00);
                i += 6;
            }
            appendUtf8(out, cp);
            break;
        }
        default: return false;
        }
    }
    return false;  // no closing quote
}

// Skips one JSON value of any kind. Strings inside objects and arrays may hold brackets
bool skipValue(const std::string& s, std::size_t& i) {
    skipSpace(s, i);
    if (i >= s.size()) return false;
    std::string ignored;
    if (s[i] == '"') return readString(s, i, ignored);
    if (s[i] == '{' || s[i] == '[') {
        int depth = 0;
        while (i < s.size()) {
            const char c = s[i];
            if (c == '"') {
                if (!readString(s, i, ignored)) return false;
                continue;
            }
            if (c == '{' || c == '[') {
                ++depth;
            } else if (c == '}' || c == ']') {
                if (--depth == 0) {
                    ++i;
                    return true;
                }
            }
            ++i;
        }
        return false;
    }
    // A number, true, false or null
    while (i < s.size() && s[i] != ',' && s[i] != '}' && s[i] != ']' && s[i] != ' ' && s[i] != '\t' &&
           s[i] != '\n' && s[i] != '\r')
        ++i;
    return true;
}

// The string fields of the top-level object. False if the text isn't a JSON object
bool readStringFields(const std::string& s, std::map<std::string, std::string>& fields) {
    std::size_t i = 0;
    skipSpace(s, i);
    if (i >= s.size() || s[i] != '{') return false;
    ++i;
    skipSpace(s, i);
    if (i < s.size() && s[i] == '}') return true;
    while (i < s.size()) {
        std::string key;
        skipSpace(s, i);
        if (!readString(s, i, key)) return false;
        skipSpace(s, i);
        if (i >= s.size() || s[i] != ':') return false;
        ++i;
        skipSpace(s, i);
        if (i < s.size() && s[i] == '"') {
            std::string value;
            if (!readString(s, i, value)) return false;
            fields[key] = value;
        } else if (!skipValue(s, i)) {
            return false;
        }
        skipSpace(s, i);
        if (i < s.size() && s[i] == ',') {
            ++i;
            continue;
        }
        return i < s.size() && s[i] == '}';
    }
    return false;
}

}

std::string requestUrl(const char* command, double julianDay) {
    // A one-day span at a one-day step; EMMA reads the first row, the state at
    // julianDay. A single time in TLIST would be simpler, but in November 2026
    // Horizons read a TLIST Julian day as an epoch before 5879611 B.C.
    char start[32], stop[32];
    std::snprintf(start, sizeof(start), "%.6f", julianDay);
    std::snprintf(stop, sizeof(stop), "%.6f", julianDay + 1.0);

    // Values are wrapped in single quotes, sent as %27; '@' is %40 and a space %20
    std::string url = "https://ssd.jpl.nasa.gov/api/horizons.api?format=json";
    url += "&COMMAND=%27";
    url += command;
    url += "%27&OBJ_DATA=%27NO%27&MAKE_EPHEM=%27YES%27&EPHEM_TYPE=%27VECTORS%27";
    url += "&CENTER=%27500%400%27";  // solar system barycenter
    url += "&REF_PLANE=%27ECLIPTIC%27&REF_SYSTEM=%27ICRF%27";
    url += "&START_TIME=%27JD";
    url += start;
    url += "%27&STOP_TIME=%27JD";
    url += stop;
    url += "%27&STEP_SIZE=%271%20d%27&TIME_TYPE=%27UT%27";
    url += "&OUT_UNITS=%27AU-D%27&VEC_TABLE=%272%27&VEC_CORR=%27NONE%27";
    url += "&CSV_FORMAT=%27YES%27&VEC_LABELS=%27NO%27";
    return url;
}

bool parseReply(const std::string& json, long status, std::string& output, std::string& error) {
    std::map<std::string, std::string> fields;
    const bool isJson = readStringFields(json, fields);

    // Horizons puts the errors it detects in "error"; a 400 reply explains itself in "message"
    if (isJson) {
        for (const char* key : {"error", "message"}) {
            const auto it = fields.find(key);
            if (it != fields.end() && !trim(it->second).empty()) {
                error = firstMessage(it->second);
                return false;
            }
        }
    }
    if (status != 200) {
        error = status == 503 ? "Horizons is busy or down for maintenance (HTTP 503)"
                              : "the server answered HTTP " + std::to_string(status);
        return false;
    }
    if (!isJson) {
        error = "Horizons sent a reply EMMA couldn't read";
        return false;
    }
    const auto result = fields.find("result");
    if (result == fields.end()) {
        error = "Horizons sent no data";
        return false;
    }
    output = result->second;
    return true;
}

bool parseStateVector(const std::string& text, Vector3D& position, Vector3D& velocity, std::string& error) {
    const std::size_t soe = text.find("$$SOE");
    const std::size_t eoe = text.find("$$EOE");
    if (soe == std::string::npos || eoe == std::string::npos || eoe < soe) {
        // Horizons doesn't always move its error into the "error" field
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
        position = Vector3D(v[0], v[1], v[2]);
        velocity = Vector3D(v[3], v[4], v[5]);
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
        std::string body, output, error;
        long status = 0;
        if (!http::get(requestUrl(COMMANDS[i], julianDay), body, status, error) ||
            !parseReply(body, status, output, error) ||
            !parseStateVector(output, result.snapshot.position[i], result.snapshot.velocity[i], error)) {
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
        if (!(in >> s.position[i].x >> s.position[i].y >> s.position[i].z >> s.velocity[i].x >> s.velocity[i].y >>
              s.velocity[i].z))
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
        out << s.position[i].x << ' ' << s.position[i].y << ' ' << s.position[i].z << ' ' << s.velocity[i].x << ' '
            << s.velocity[i].y << ' ' << s.velocity[i].z << '\n';
}

}
