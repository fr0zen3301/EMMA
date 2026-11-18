#pragma once
#include "emma/Vector3D.hpp"
#include <cstdint>
#include <deque>
#include <string>

namespace emma {

struct Rgb {
    std::uint8_t r = 255;
    std::uint8_t g = 255;
    std::uint8_t b = 255;
};

enum class BodyClass { RockyBody, NeptuneLike, GasGiant, BrownDwarf, Star };

// Ordinary bodies have a surface. A black hole has only a horizon, and swallows what it touches
enum class BodyType { Ordinary, BlackHole };

// A point on a body's trail, and when the body was there
struct TrailPoint {
    Vector3D position;
    double time;  // days since time 0
};

class Body {
public:
    // Color of a black hole's trail and preview
    static constexpr Rgb HOLE_COLOR{255, 150, 70};

    // A body you add: its radius and color come from its mass
    Body(double mass, const Vector3D& position, const Vector3D& velocity);
    // A real body from a preset, with measured values
    Body(std::string name, std::string kind, double mass, double radius, Rgb color,
         const Vector3D& position, const Vector3D& velocity);
    // Makes a black hole. Its radius is its horizon, set by its mass and spin (0 to 0.998)
    static Body blackHole(std::string name, std::string kind, double mass, double spin, const Vector3D& spinAxis,
                          const Vector3D& position, const Vector3D& velocity);

    double mass;
    double radius;
    Vector3D position;
    Vector3D velocity;
    Vector3D acceleration;      // AU per day^2
    Vector3D previousPosition;  // where the body was at the start of the step
    Rgb color;
    std::string name;           // empty for bodies you add
    std::string kind;           // e.g. "G2V star"; empty for bodies you add
    bool alive = true;

    BodyType type = BodyType::Ordinary;
    // Black holes only
    double spin = 0.0;                 // 0 = no spin, up to 0.998
    Vector3D spinAxis{0.0, 0.0, 1.0};  // unit length, ecliptic frame

    std::deque<TrailPoint> trail;
    Vector3D trailHeading;
    
    bool isNamed() const { return !name.empty(); }
    bool isBlackHole() const { return type == BodyType::BlackHole; }
    std::string description() const {
        if (!kind.empty()) return kind;
        return isBlackHole() ? describeBlackHole(mass) : describe(mass);
    }

    static BodyClass classify(double mass);
    static std::string describe(double mass);       // class, with spectral type for stars
    static std::string describeBlackHole(double mass);  // stellar-mass, intermediate or supermassive
    static double radiusForMass(double mass);       // AU
    static double stellarTemperature(double mass);  // K, main-sequence estimate
    static Rgb colorForMass(double mass);
    static Rgb blackbody(double kelvin);
};

} // namespace emma
