#pragma once
#include "emma/Gravity.hpp"
#include "emma/World.hpp"
#include <functional>

namespace emma {

// The interface every integrator follows, so Simulation can swap them freely
class Integrator {
public:
    virtual ~Integrator() = default;

    // Moves the world forward dt days. afterPiece runs after each piece (collisions, trails).
    // Returns how many pieces it took
    virtual int step(World& world, Gravity& gravity, double dt, const std::function<void()>& afterPiece) = 0;

    // Bodies were added, loaded or cleared
    virtual void bodiesChanged() = 0;
    // Bodies were removed mid-step, so anything stored per body is out of date
    virtual void forgetHistory() = 0;

    virtual const char* name() const = 0;

    // True if it can't work with the tree's approximate forces. Simulation then switches to
    // Verlet above Gravity::DIRECT_LIMIT bodies
    virtual bool needsExactForces() const { return false; }
};

}
