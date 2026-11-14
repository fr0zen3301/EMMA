#include "emma/World.hpp"
#include <cmath>

namespace emma {

void World::moveToBarycenter() {
    double m = 0.0;
    Vector3D r, p;
    for (const Body& b : bodies) {
        m += b.mass;
        r += b.position * b.mass;
        p += b.velocity * b.mass;
    }
    if (m <= 0.0) return;
    const Vector3D com = r / m, drift = p / m;
    for (Body& b : bodies) {
        b.position -= com;
        b.velocity -= drift;
        b.previousPosition = b.position;
    }
}

void World::recordTrails() {
    // A new point each time a body's direction of travel has turned by TRAIL_TURN
    // since the last one
    static const double cosTurn = std::cos(TRAIL_TURN);
    for (Body& b : bodies) {
        const double speed = b.velocity.magnitude();
        if (speed <= 0.0) continue;
        const Vector3D heading = b.velocity / speed;
        if (!b.trail.empty() && heading.dot(b.trailHeading) > cosTurn) continue;
        b.trail.push_back(b.position);
        if (b.trail.size() > MAX_TRAIL) b.trail.pop_front();
        b.trailHeading = heading;
    }
}

double World::kineticEnergy() const {
    double kinetic = 0.0;
    for (const Body& b : bodies) kinetic += 0.5 * b.mass * b.velocity.lengthSquared();
    return kinetic;
}

Vector3D World::angularMomentum() const {
    Vector3D l;
    for (const Body& b : bodies) l += b.position.cross(b.velocity) * b.mass;
    return l;
}

} // namespace emma
