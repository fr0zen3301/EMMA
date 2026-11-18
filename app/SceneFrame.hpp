#pragma once
#include "emma/Galaxy.hpp"
#include "emma/Vector3D.hpp"

// Where things are drawn. Normally that's where the simulation has them. In the galaxy
// view, everything is also carried along the Sun's path around the Galaxy (drawing only)
struct SceneFrame {
    bool galactic = false;
    emma::Vector3D velocity;  // the Sun's velocity at time 0 (AU per day, Galactic)
    double now = 0.0;         // the simulation's time, in days

    // Where to draw a point the simulation had at time t
    emma::Vector3D place(const emma::Vector3D& p, double t) const {
        if (!galactic) return p;
        return emma::galaxy::fromEcliptic(p) + emma::galaxy::sunDisplacement(velocity, t);
    }
    emma::Vector3D place(const emma::Vector3D& p) const { return place(p, now); }

    // The Sun's velocity now (AU per day, Galactic)
    emma::Vector3D heading() const { return emma::galaxy::sunVelocityAt(velocity, now); }

    // Where to draw Sgr A*, at the Galactic center
    emma::Vector3D galacticCenter() const { return fromGalactic(emma::galaxy::galacticCenter()); }

    // Turns a simulation direction the way the picture is turned
    emma::Vector3D direction(const emma::Vector3D& d) const {
        return galactic ? emma::galaxy::fromEcliptic(d) : d;
    }

    // Converts directions to Galactic coordinates and back
    emma::Vector3D toGalactic(const emma::Vector3D& d) const {
        return galactic ? d : emma::galaxy::fromEcliptic(d);
    }
    emma::Vector3D fromGalactic(const emma::Vector3D& d) const {
        return galactic ? d : emma::galaxy::toEcliptic(d);
    }
};
