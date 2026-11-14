#pragma once
#include "emma/Constants.hpp"
#include "emma/Vector3D.hpp"
#include <SFML/Graphics/Rect.hpp>
#include <SFML/System/Vector2.hpp>
#include <algorithm>
#include <cmath>
#include <optional>

// An orbit camera. It looks at a target point from some distance away, turned by
// yaw (around the ecliptic pole) and pitch (how high above the ecliptic plane it
// sits: 90 degrees looks straight down). World points are AU in the ecliptic frame,
// window pixels have y pointing down. Everything is worked out in double precision
// relative to the target, so you can zoom from Neptune's orbit down to the Moon
// without the picture shaking
class Camera {
public:
    static constexpr double MIN_DISTANCE = 1e-7;  // AU
    static constexpr double MAX_DISTANCE = 1e5;   // AU
    static constexpr double TAN_HALF_FOV = 0.41421356237309515;  // tan(22.5 deg): a 45 degree field of view
    static constexpr double NEAR = 1e-4;          // near plane, as a fraction of the distance
    static constexpr double MAX_PICK = 1e3;       // farthest pick along a view ray, as a multiple of the distance
    static constexpr double MAX_PITCH = 0.5 * emma::units::PI;

    emma::Vector3D target;   // AU, the point the camera circles and looks at
    double distance = 10.0;  // AU, from the eye to the target
    sf::FloatRect viewport;  // pixels: the area right of the sidebar, above the status bar

    Camera() { aim(); }

    double yaw() const { return yawAngle; }
    double pitch() const { return pitchAngle; }

    sf::Vector2f middle() const { return viewport.position + viewport.size * 0.5f; }
    double focal() const { return 0.5 * viewport.size.y / TAN_HALF_FOV; }  // pixels
    double pxPerAU() const { return focal() / distance; }                  // at the target's depth

    // A world point in camera coordinates: x to the right, y up, z the depth in front of the eye
    emma::Vector3D toCamera(const emma::Vector3D& w) const {
        const emma::Vector3D d = (w - target) + forward * distance;
        return emma::Vector3D(d.dot(right), d.dot(up), d.dot(forward));
    }

    bool inFront(const emma::Vector3D& c) const { return c.z > NEAR * distance; }

    // Camera coordinates to pixels: the perspective divide. c must be in front of the near plane
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

    // A direction rather than a point, such as a background star's: it turns with
    // the view but doesn't move when the camera does
    std::optional<sf::Vector2f> toScreenDirection(const emma::Vector3D& d) const {
        const emma::Vector3D c(d.dot(right), d.dot(up), d.dot(forward));
        if (c.z <= 0.5) return std::nullopt;
        return project(c);
    }

    // How many pixels one AU covers at w's depth. 0 behind the eye
    double pxPerAUAt(const emma::Vector3D& w) const {
        const double depth = toCamera(w).z;
        return depth > NEAR * distance ? focal() / depth : 0.0;
    }

    // Cuts off the part of a segment (in camera coordinates) behind the near plane.
    // False when nothing is left
    bool clip(emma::Vector3D& a, emma::Vector3D& b) const {
        const double near = NEAR * distance;
        if (a.z <= near && b.z <= near) return false;
        if (a.z < near) a = a + (b - a) * ((near - a.z) / (b.z - a.z));
        else if (b.z < near) b = b + (a - b) * ((near - b.z) / (a.z - b.z));
        return true;
    }

    // The point on the plane z = planeZ under a pixel, if the view ray reaches it
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

    // On the grid: the plane through the target, parallel to the ecliptic
    std::optional<emma::Vector3D> pick(sf::Vector2f s) const { return pick(s, target.z); }

    void zoom(double factor) { distance = std::clamp(distance / factor, MIN_DISTANCE, MAX_DISTANCE); }

    // Zoom keeping the grid point under screenPoint fixed, or toward the target when
    // there is none nearby
    void zoomAt(sf::Vector2f screenPoint, double factor) {
        const std::optional<emma::Vector3D> before = pick(screenPoint);
        zoom(factor);
        const std::optional<emma::Vector3D> after = pick(screenPoint);
        if (before && after && (*before - target).magnitude() < 20.0 * distance) target += *before - *after;
    }

    // Slides the grid under the cursor from one pixel to another. Near the horizon,
    // where that would fling the view far away, it slides by the drag instead
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

    // Center on the origin (the barycenter) with radiusAU visible on the short side
    void fit(double radiusAU) {
        const double half = 0.5 * std::min(viewport.size.x, viewport.size.y);
        distance = std::clamp(focal() * radiusAU / half, MIN_DISTANCE, MAX_DISTANCE);
        target = emma::Vector3D();
    }

private:
    // The camera's axes in world coordinates, from yaw and pitch
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
