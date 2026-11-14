#pragma once
#include "emma/Galaxy.hpp"
#include "emma/Vector3D.hpp"

// Where the picture is drawn from. The simulation always runs in the barycentric
// ecliptic frame. In the galaxy view the picture is turned into Galactic coordinates
// and slid along the Sun's motion, so a point the simulation had at time t is drawn
// at R p + V t. The physics never sees V
struct SceneFrame {
    bool galactic = false;
    emma::Vector3D velocity;  // AU per day, in Galactic coordinates
    double now = 0.0;         // days, the simulation's time

    // A simulation point at time t, in scene coordinates
    emma::Vector3D place(const emma::Vector3D& p, double t) const {
        if (!galactic) return p;
        return emma::galaxy::fromEcliptic(p) + velocity * t;
    }
    emma::Vector3D place(const emma::Vector3D& p) const { return place(p, now); }
};
