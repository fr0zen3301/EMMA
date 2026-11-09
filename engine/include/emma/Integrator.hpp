#pragma once
#include "emma/Gravity.hpp"
#include "emma/Vector2D.hpp"
#include "emma/World.hpp"
#include <functional>
#include <vector>

namespace emma {

// Velocity Verlet with adaptive splitting. In quiet stretches a step runs as a
// single piece, so Verlet stays symplectic; through a close pass it splits into
// as many as MAX_SUBSTEPS pieces. (Phase 3 turns this into one implementation
// of an Integrator interface.)
class VerletIntegrator {
public:
    static constexpr double ETA = 0.03;        // longest piece, as a fraction of the tightest dynamical time
    static constexpr int MAX_SUBSTEPS = 4096;  // most pieces one step can be split into

    // Advances world by dt days. afterPiece runs after every piece: collisions
    // and trails go there. Returns how many pieces the step took
    int step(World& world, Gravity& gravity, double dt, const std::function<void()>& afterPiece);

    // Bodies were added, loaded or cleared: compute accelerations before the next piece
    void reset() { stale = true; }
    // Bodies were removed mid-step. the last piece's accelerations no longer line up
    void forgetHistory() { jerkValid = false; }

private:
    void piece(World& world, Gravity& gravity, double h);
    double dynamicalTime(const std::vector<Body>& bodies) const;

    std::vector<Vector2D> oldAcceleration;
    bool stale = true;       // accelerations need computing before the next drift
    bool jerkValid = false;  // oldAcceleration holds the previous piece's accelerations
    double lastPiece = 0.0;  // days, the previous piece's length
};

}
