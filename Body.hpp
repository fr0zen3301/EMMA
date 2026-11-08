#pragma once
#include "Vector2D.hpp"
#include <cstdint>
#include <deque>
#include <string>

struct Rgb {
    std::uint8_t r = 255;
    std::uint8_t g = 255;
    std::uint8_t b = 255;
};

enum class BodyClass { RockyBody, NeptuneLike, GasGiant, BrownDwarf, Star };

class Body {
public:
    // A spawned body: its radius and color follow from its mass
    Body(double mass, const Vector2D& position, const Vector2D& velocity);
    // A real body from a preset, with its measured radius, color and type
    Body(std::string name, std::string kind, double mass, double radius, Rgb color,
         const Vector2D& position, const Vector2D& velocity);

    double mass;                // solar masses
    double radius;              // AU, physical radius (used for collisions)
    Vector2D position;          // AU
    Vector2D velocity;          // AU per day
    Vector2D acceleration;      // AU per day^2
    Vector2D previousPosition;  // where the body started the current step
    Rgb color;
    std::string name;           // empty for spawned bodies
    std::string kind;           // "G2V star", "Gas giant", empty for spawned bodies
    bool alive = true;

    std::deque<Vector2D> trail;
    Vector2D trailHeading;  // direction of travel when the last trail point was recorded

    bool isNamed() const { return !name.empty(); }
    std::string description() const { return kind.empty() ? describe(mass) : kind; }

    static BodyClass classify(double mass);
    static std::string describe(double mass);       // class, with spectral type for stars
    static double radiusForMass(double mass);       // AU
    static double stellarTemperature(double mass);  // K, main-sequence estimate
    static Rgb colorForMass(double mass);
    static Rgb blackbody(double kelvin);
};
