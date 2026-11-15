#pragma once
#include "emma/Constants.hpp"
#include "emma/Vector3D.hpp"
#include <algorithm>
#include <cmath>

// Light around a black hole that doesn't spin (Schwarzschild). Lengths are in
// Schwarzschild radii, Rs = 2GM/c^2, so the event horizon is at r = 1 and nothing here
// depends on the mass. The ray step is the same as the one in the app's shader
// (app/BlackHoleShader.hpp), so the tests check what the shader draws
namespace emma::blackhole {

constexpr double PHOTON_SPHERE = 1.5;               // light can travel in a circle here
constexpr double SHADOW_RADIUS = 2.598076211353316;  // sqrt(27)/2: rays aimed inside it fall in
constexpr double ISCO = 3.0;                       // innermost stable circular orbit
constexpr double STEP_FRACTION = 0.06;             // a step is 6% of the distance to the hole

constexpr double SPEED_OF_LIGHT = units::SPEED_OF_LIGHT_KMS * units::KMS_TO_AU_PER_DAY;  // AU per day

// The event horizon's radius in AU for a mass in solar masses: 2GM/c^2
constexpr double schwarzschildRadiusAU(double mass) {
    return 2.0 * units::G * mass / (SPEED_OF_LIGHT * SPEED_OF_LIGHT);
}

// A light ray, traced backward from the eye. It moves under a central pull of
// (3/2) h^2 / r^4, with h = |x cross v| fixed along the ray. Binet's equation turns
// that into u'' + u = (3/2) u^2 with u = 1/r, which is the equation of a light ray
// around the hole. So the path has exactly a light ray's shape. The speed along it
// means nothing
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

// One velocity Verlet step, the same scheme as EMMA's default integrator
inline void stepRay(Ray& ray, double dt) {
    const Vector3D next = ray.position + ray.velocity * dt + ray.acceleration * (0.5 * dt * dt);
    const Vector3D a = lightPull(next, ray.h2);
    ray.velocity += (ray.acceleration + a) * (0.5 * dt);
    ray.position = next;
    ray.acceleration = a;
}

enum class RayEnd { Horizon, Escaped, Unfinished };

// Follows a ray until it falls through the horizon or gets beyond escapeRadius moving
// outward. Its final direction is then in ray.velocity
inline RayEnd traceRay(Ray& ray, double escapeRadius, int maxSteps = 600) {
    for (int i = 0; i < maxSteps; ++i) {
        stepRay(ray, stepLength(ray.position.magnitude()));
        const double r = ray.position.magnitude();
        if (r < 1.0) return RayEnd::Horizon;
        if (r > escapeRadius && ray.position.dot(ray.velocity) > 0.0) return RayEnd::Escaped;
    }
    return RayEnd::Unfinished;
}

// The speed of a circular orbit at radius r, measured by someone hovering there, in units of c
inline double orbitSpeed(double r) { return std::sqrt(0.5 / (r - 1.0)); }

// How much light from gas at radius r is shifted on its way to an eye at radius eyeR:
// the frequency that arrives over the one sent. The gas moves at speed v (units of c),
// and cosAngle is the cosine of the angle between its motion and the direction toward
// the eye. Colors scale by this factor and brightness by its fourth power
inline double frequencyShift(double r, double v, double cosAngle, double eyeR) {
    const double gamma = 1.0 / std::sqrt(1.0 - v * v);
    return std::sqrt(1.0 - 1.0 / r) / (gamma * (1.0 - v * cosAngle)) / std::sqrt(1.0 - 1.0 / eyeR);
}

}
