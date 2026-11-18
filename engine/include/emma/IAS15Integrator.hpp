#pragma once
#include "emma/Integrator.hpp"
#include "emma/Vector3D.hpp"
#include <array>
#include <vector>

namespace emma {

// IAS15 (Rein & Spiegel 2015): a very accurate integrator. It fits each body's acceleration
// with a smooth curve and picks its own piece length, keeping errors near the rounding limit
class IAS15Integrator : public Integrator {
public:
    static constexpr int MAX_ITERATIONS = 12;   // most refinement passes per piece
    static constexpr double CONVERGED = 1e-16;  // stop refining when the curve changes less than this
    static constexpr double EPSILON = 1e-9;     // target accuracy: sets the piece length
    static constexpr double SAFETY = 0.25;      // redo a piece that was far too long; grow at most 4x
    static constexpr int MAX_SUBSTEPS = 4096;   // most pieces one step can be split into

    int step(World& world, Gravity& gravity, double dt, const std::function<void()>& afterPiece) override;

    // New bodies may need a very different piece length, so start fresh
    void bodiesChanged() override {
        hasHistory = false;
        nextStep = 0.0;
    }
    void forgetHistory() override { hasHistory = false; }
    const char* name() const override { return "IAS15"; }
    // The tree's forces are approximate, which would make IAS15 take tiny pieces
    bool needsExactForces() const override { return true; }

private:
    // The acceleration curve: a(t) = a0 + b[0] t + ... + b[6] t^7, with t from 0 to 1
    using Coefficients = std::array<Vector3D, 7>;

    double piece(World& world, Gravity& gravity, double h, double minPiece);  // returns the length it took
    void converge(std::vector<Body>& bodies, Gravity& gravity, double h);    // refines the curve until it settles
    double suggestStep(double h) const;  // how long the next piece should be
    void rescale(double ratio);          // fits the curve to a longer or shorter piece
    void predictNextStep(double ratio);  // starts the next curve where this one ended

    std::vector<Vector3D> x0, v0, a0;  // each body at the start of the piece
    std::vector<Coefficients> b;       // the curve's coefficients
    std::vector<Coefficients> g;       // the same curve in the form used while fitting
    bool hasHistory = false;           // b holds the last piece's curve
    double lastStep = 0.0;             // days
    double suggested = 0.0;            // days, suggested length for the next piece
    double nextStep = 0.0;             // days, the length to try next (0: none yet)
};

}
