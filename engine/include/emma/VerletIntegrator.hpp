#pragma once
#include "emma/Integrator.hpp"
#include "emma/Vector3D.hpp"
#include <vector>

namespace emma {

// Velocity Verlet: fast, and its energy error doesn't build up. During a close pass it
// splits a step into smaller pieces
class VerletIntegrator : public Integrator {
public:
    static constexpr double ETA = 0.03;        // a piece lasts at most 3% of the fastest pair's swing time
    static constexpr int MAX_SUBSTEPS = 4096;  // most pieces one step can be split into

    int step(World& world, Gravity& gravity, double dt, const std::function<void()>& afterPiece) override;

    void bodiesChanged() override { stale = true; }
    void forgetHistory() override { jerkValid = false; }
    const char* name() const override { return "Velocity Verlet"; }

private:
    void piece(World& world, Gravity& gravity, double h);
    double dynamicalTime(const std::vector<Body>& bodies) const;

    std::vector<Vector3D> oldAcceleration;
    bool stale = true;       // accelerations need working out before the next step
    bool jerkValid = false;  // oldAcceleration holds the last piece's accelerations
    double lastPiece = 0.0;  // days, the previous piece's length
};

}
