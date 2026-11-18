#pragma once
#include "emma/Constants.hpp"
#include "emma/Vector3D.hpp"
#include <SFML/Graphics/Rect.hpp>
#include <SFML/System/Vector2.hpp>
#include <algorithm>
#include <cmath>
#include <optional>

// A camera that circles a target point. yaw turns it around, pitch tilts it up and down
// (90 degrees looks straight down). Math is done relative to the target, so zooming way
// in doesn't make the picture shake
class Camera {
public:
    static constexpr double MIN_DISTANCE = 1e-7;  // AU
    static constexpr double MAX_DISTANCE = 1e10;  // AU: far enough to see the whole Milky Way
    static constexpr double TAN_HALF_FOV = 0.41421356237309515;  // a 45 degree field of view
    static constexpr double NEAR = 1e-4;          // nothing closer than this is drawn (fraction of distance)
    static constexpr double MAX_PICK = 1e3;       // farthest a click can land (multiple of distance)
    static constexpr double MAX_PITCH = 0.5 * emma::units::PI;

    emma::Vector3D target;   // AU, the point the camera circles and looks at
    double distance = 10.0;  // AU, from the eye to the target
    sf::FloatRect viewport;  // pixels: the area between the sidebar and the status bar

    Camera() { aim(); }

    double yaw() const { return yawAngle; }
    double pitch() const { return pitchAngle; }

    // Which way the camera faces, and where the eye is relative to the target
    const emma::Vector3D& rightAxis() const { return right; }
    const emma::Vector3D& upAxis() const { return up; }
    const emma::Vector3D& forwardAxis() const { return forward; }
    emma::Vector3D eyeOffset() const { return forward * -distance; }

    sf::Vector2f middle() const { return viewport.position + viewport.size * 0.5f; }
    double focal() const { return 0.5 * viewport.size.y / TAN_HALF_FOV; }  // pixels
    double pxPerAU() const { return focal() / distance; }                  // at the target

    // A point as seen by the camera: x right, y up, z how far in front
    emma::Vector3D toCamera(const emma::Vector3D& w) const {
        const emma::Vector3D d = (w - target) + forward * distance;
        return emma::Vector3D(d.dot(right), d.dot(up), d.dot(forward));
    }

    bool inFront(const emma::Vector3D& c) const { return c.z > NEAR * distance; }

    // Camera coordinates to pixels. c must be in front of the camera
    sf::Vector2f project(const emma::Vector3D& c) const {
        const sf::Vector2f m = middle();
        const double s = focal() / c.z;
        return sf::Vector2f(static_cast<float>(m.x + c.x * s), static_cast<float>(m.y - c.y * s));
    }

    std::optional<sf::Vector2f> toScreen(const emma::Vector3D& w) const {
        const emma::Vector3D c = toCamera(w);
        if (!inFront(c)) return std::nullopt;
        return project(c);
    }

    // A direction, like a background star's: it turns with the view but never moves
    std::optional<sf::Vector2f> toScreenDirection(const emma::Vector3D& d) const {
        const emma::Vector3D c(d.dot(right), d.dot(up), d.dot(forward));
        if (c.z <= 0.5) return std::nullopt;
        return project(c);
    }

    // How many pixels one AU covers at w's distance. 0 if w is behind the camera
    double pxPerAUAt(const emma::Vector3D& w) const {
        const double depth = toCamera(w).z;
        return depth > NEAR * distance ? focal() / depth : 0.0;
    }

    // Cuts off the part of a line that's behind the camera. False if nothing is left
    bool clip(emma::Vector3D& a, emma::Vector3D& b) const {
        const double near = NEAR * distance;
        if (a.z <= near && b.z <= near) return false;
        if (a.z < near) a = a + (b - a) * ((near - a.z) / (b.z - a.z));
        else if (b.z < near) b = b + (a - b) * ((near - b.z) / (a.z - b.z));
        return true;
    }

    // The point on a flat plane (height planeZ) under a pixel, if there is one
    std::optional<emma::Vector3D> pick(sf::Vector2f s, double planeZ) const {
        const sf::Vector2f m = middle();
        const double f = focal();
        const emma::Vector3D ray = forward + right * ((s.x - m.x) / f) - up * ((s.y - m.y) / f);
        const emma::Vector3D eye = forward * -distance;  // relative to the target
        if (ray.z == 0.0) return std::nullopt;
        const double t = (planeZ - target.z - eye.z) / ray.z;
        if (t <= 0.0 || t > MAX_PICK * distance) return std::nullopt;
        return target + (eye + ray * t);
    }

    // The point on the grid under a pixel
    std::optional<emma::Vector3D> pick(sf::Vector2f s) const { return pick(s, target.z); }

    void zoom(double factor) { distance = std::clamp(distance / factor, MIN_DISTANCE, MAX_DISTANCE); }

    // Zooms toward the point under the cursor
    void zoomAt(sf::Vector2f screenPoint, double factor) {
        const std::optional<emma::Vector3D> before = pick(screenPoint);
        zoom(factor);
        const std::optional<emma::Vector3D> after = pick(screenPoint);
        if (before && after && (*before - target).magnitude() < 20.0 * distance) target += *before - *after;
    }

    // Drags the view so the grid follows the cursor
    void pan(sf::Vector2f from, sf::Vector2f to) {
        const std::optional<emma::Vector3D> a = pick(from), b = pick(to);
        if (a && b && (*a - *b).magnitude() < 2.0 * distance) {
            target += *a - *b;
            return;
        }
        const double auPerPx = distance / focal();
        const emma::Vector3D ahead(-std::sin(yawAngle), std::cos(yawAngle), 0.0);
        target += right * (-(to.x - from.x) * auPerPx) + ahead * ((to.y - from.y) * auPerPx);
    }

    void orbit(double dYaw, double dPitch) {
        yawAngle = std::remainder(yawAngle + dYaw, 2.0 * emma::units::PI);
        pitchAngle = std::clamp(pitchAngle + dPitch, -MAX_PITCH, MAX_PITCH);
        aim();
    }

    void setAngles(double yaw, double pitch) {
        yawAngle = std::remainder(yaw, 2.0 * emma::units::PI);
        pitchAngle = std::clamp(pitch, -MAX_PITCH, MAX_PITCH);
        aim();
    }

    void topView() { setAngles(0.0, MAX_PITCH); }

    // Centers the view and zooms so radiusAU fits
    void fit(double radiusAU) {
        const double half = 0.5 * std::min(viewport.size.x, viewport.size.y);
        distance = std::clamp(focal() * radiusAU / half, MIN_DISTANCE, MAX_DISTANCE);
        target = emma::Vector3D();
    }

private:
    // Works out which way the camera faces from yaw and pitch
    void aim() {
        const double cy = std::cos(yawAngle), sy = std::sin(yawAngle);
        const double cp = std::cos(pitchAngle), sp = std::sin(pitchAngle);
        right = emma::Vector3D(cy, sy, 0.0);
        up = emma::Vector3D(-sy * sp, cy * sp, cp);
        forward = emma::Vector3D(-sy * cp, cy * cp, -sp);
    }

    double yawAngle = 0.0;
    double pitchAngle = MAX_PITCH;
    emma::Vector3D right, up, forward;
};
