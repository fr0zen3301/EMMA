#pragma once
#include "emma/Integrator.hpp"
#include "emma/Vector3D.hpp"
#include <vector>

namespace emma {

// Velocity Verlet with adaptive splitting. In quiet stretches a step runs as a
// single piece, so Verlet stays symplectic; through a close pass it splits into
// as many as MAX_SUBSTEPS pieces
class VerletIntegrator : public Integrator {
public:
    static constexpr double ETA = 0.03;        // longest piece, as a fraction of the tightest dynamical time
    static constexpr int MAX_SUBSTEPS = 4096;  // most pieces one step can be split into

    int step(World& world, Gravity& gravity, double dt, const std::function<void()>& afterPiece) override;

    void bodiesChanged() override { stale = true; }
    void forgetHistory() override { jerkValid = false; }
    const char* name() const override { return "Velocity Verlet"; }

private:
    void piece(World& world, Gravity& gravity, double h);
    double dynamicalTime(const std::vector<Body>& bodies) const;

    std::vector<Vector3D> oldAcceleration;
    bool stale = true;       // accelerations need computing before the next drift
    bool jerkValid = false;  // oldAcceleration holds the previous piece's accelerations
    double lastPiece = 0.0;  // days, the previous piece's length
};

}
