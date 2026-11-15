#pragma once
#include "emma/Galaxy.hpp"
#include "emma/Vector3D.hpp"

// Where the picture is drawn from. The simulation always runs in the barycentric
// ecliptic frame. In the galaxy view the picture is turned into Galactic coordinates
// and carried along the Sun's path around the Galactic center, so a point the
// simulation had at time t is drawn at R p + D(t), with D(t) how far the Sun has
// moved since time 0. The physics never sees D
struct SceneFrame {
    bool galactic = false;
    emma::Vector3D velocity;  // AU per day, in Galactic coordinates: the Sun's velocity at time 0
    double now = 0.0;         // days, the simulation's time

    // A simulation point at time t, in scene coordinates
    emma::Vector3D place(const emma::Vector3D& p, double t) const {
        if (!galactic) return p;
        return emma::galaxy::fromEcliptic(p) + emma::galaxy::sunDisplacement(velocity, t);
    }
    emma::Vector3D place(const emma::Vector3D& p) const { return place(p, now); }

    // The Sun's velocity now, turned along its path (AU per day, Galactic)
    emma::Vector3D heading() const { return emma::galaxy::sunVelocityAt(velocity, now); }

    // Sgr A*, at the Galactic center, in scene coordinates. It doesn't move: the Sun goes around it
    emma::Vector3D galacticCenter() const { return fromGalactic(emma::galaxy::galacticCenter()); }

    // A direction in the simulation (ecliptic) turned the way the picture is
    emma::Vector3D direction(const emma::Vector3D& d) const {
        return galactic ? emma::galaxy::fromEcliptic(d) : d;
    }

    // A direction in scene coordinates turned into Galactic ones, and back
    emma::Vector3D toGalactic(const emma::Vector3D& d) const {
        return galactic ? d : emma::galaxy::fromEcliptic(d);
    }
    emma::Vector3D fromGalactic(const emma::Vector3D& d) const {
        return galactic ? d : emma::galaxy::toEcliptic(d);
    }
};
