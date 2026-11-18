#pragma once
#include "emma/Constants.hpp"
#include "emma/Vector3D.hpp"
#include <algorithm>
#include <cmath>

// Black hole math. Lengths are in Schwarzschild radii (Rs), so the horizon is at r = 1.
// The ray step matches the shader's, so the tests check what the shader draws
namespace emma::blackhole {

constexpr double PHOTON_SPHERE = 1.5;               // light can travel in a circle here
constexpr double SHADOW_RADIUS = 2.598076211353316;  // sqrt(27)/2: rays aimed inside it fall in
constexpr double ISCO = 3.0;                       // innermost stable circular orbit
constexpr double STEP_FRACTION = 0.06;             // a step is 6% of the distance to the hole

constexpr double SPEED_OF_LIGHT = units::SPEED_OF_LIGHT;  // AU per day

// The fastest a real black hole can spin (Thorne 1974)
constexpr double MAX_SPIN = 0.998;

// Horizon radius in AU of a hole that doesn't spin: 2GM/c^2
constexpr double schwarzschildRadiusAU(double mass) {
    return 2.0 * units::G * mass / (SPEED_OF_LIGHT * SPEED_OF_LIGHT);
}

// Horizon radius in AU of a spinning hole. More spin, smaller horizon: down to about half of Rs
inline double horizonRadiusAU(double mass, double spin) {
    const double a = std::clamp(spin, 0.0, MAX_SPIN);
    return 0.5 * schwarzschildRadiusAU(mass) * (1.0 + std::sqrt(1.0 - a * a));
}

// A light ray, followed backward from the eye. A made-up pull bends it into the exact
// shape of a real light ray (docs/black-hole.md, section 9)
struct Ray {
    Vector3D position;
    Vector3D velocity;
    Vector3D acceleration;
    double h2 = 0.0;  // the ray's angular momentum, squared
};

inline Vector3D lightPull(const Vector3D& x, double h2) {
    const double r2 = x.lengthSquared();
    return x * (-1.5 * h2 / (r2 * r2 * std::sqrt(r2)));
}

inline Ray startRay(const Vector3D& eye, const Vector3D& direction) {
    Ray ray;
    ray.position = eye;
    ray.velocity = direction.normalized();
    ray.h2 = eye.cross(ray.velocity).lengthSquared();
    ray.acceleration = lightPull(eye, ray.h2);
    return ray;
}

inline double stepLength(double r) { return std::max(STEP_FRACTION * r, 0.01); }

// One velocity Verlet step, the same method EMMA uses for bodies
inline void stepRay(Ray& ray, double dt) {
    const Vector3D next = ray.position + ray.velocity * dt + ray.acceleration * (0.5 * dt * dt);
    const Vector3D a = lightPull(next, ray.h2);
    ray.velocity += (ray.acceleration + a) * (0.5 * dt);
    ray.position = next;
    ray.acceleration = a;
}

enum class RayEnd { Horizon, Escaped, Unfinished };

// Follows a ray until it falls in or escapes. Its final direction is in ray.velocity
inline RayEnd traceRay(Ray& ray, double escapeRadius, int maxSteps = 600) {
    for (int i = 0; i < maxSteps; ++i) {
        stepRay(ray, stepLength(ray.position.magnitude()));
        const double r = ray.position.magnitude();
        if (r < 1.0) return RayEnd::Horizon;
        if (r > escapeRadius && ray.position.dot(ray.velocity) > 0.0) return RayEnd::Escaped;
    }
    return RayEnd::Unfinished;
}

// Speed of a circular orbit at radius r, as a fraction of c
inline double orbitSpeed(double r) { return std::sqrt(0.5 / (r - 1.0)); }

// How much the gas's light shifts on its way to the eye (arriving / sent frequency).
// Colors scale by this, brightness by its fourth power
inline double frequencyShift(double r, double v, double cosAngle, double eyeR) {
    const double gamma = 1.0 / std::sqrt(1.0 - v * v);
    return std::sqrt(1.0 - 1.0 / r) / (gamma * (1.0 - v * cosAngle)) / std::sqrt(1.0 - 1.0 / eyeR);
}

}
