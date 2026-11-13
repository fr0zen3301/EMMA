#pragma once
#include "emma/Gravity.hpp"
#include "emma/World.hpp"
#include <functional>

namespace emma {

// What every integrator can do. Simulation talks only to this, so Verlet,
// IAS15 or anything later can be swapped in without Simulation changing
class Integrator {
public:
    virtual ~Integrator() = default;

    // Advances world by dt days. afterPiece runs after every piece: collisions
    // and trails go there. Returns how many pieces the step took
    virtual int step(World& world, Gravity& gravity, double dt, const std::function<void()>& afterPiece) = 0;

    // Bodies were added, loaded or cleared: compute accelerations before the next piece
    virtual void bodiesChanged() = 0;
    // Bodies were removed mid-step. anything remembered per body no longer lines up
    virtual void forgetHistory() = 0;

    virtual const char* name() const = 0;

    // True for an integrator built for exact forces. Above Gravity::DIRECT_LIMIT bodies,
    // where the forces come from the Barnes-Hut tree, Simulation swaps it for Verlet
    virtual bool needsExactForces() const { return false; }
};

}
