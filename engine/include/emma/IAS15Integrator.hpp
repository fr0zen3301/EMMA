#pragma once
#include "emma/Integrator.hpp"
#include "emma/Vector2D.hpp"
#include <array>
#include <vector>

namespace emma {

// IAS15 (Rein & Spiegel). Within a step it
// writes each body's acceleration as a polynomial in time, samples the forces at
// the start and at 7 moments spread across the step (Gauss-Radau spacing), refits
// the polynomial and repeats until it stops changing. Then it moves every body
// along it. It aims for errors near the rounding limit of a double.
// It picks its own piece length from how fast the forces change, and splits a
// simulation step into as many pieces as that takes
class IAS15Integrator : public Integrator {
public:
    static constexpr int MAX_ITERATIONS = 12;   // predictor-corrector passes per step, at most
    static constexpr double CONVERGED = 1e-16;  // settled: the last coefficient moved less than this, relative to |a|
    static constexpr double EPSILON = 1e-9;     // how small b[6] should be relative to |a|: sets the piece length
    static constexpr double SAFETY = 0.25;      // redo a piece when IAS15 wants one under a quarter as long; grow at most 4x
    static constexpr int MAX_SUBSTEPS = 4096;   // most pieces one step can be split into

    int step(World& world, Gravity& gravity, double dt, const std::function<void()>& afterPiece) override;

    // A new set of bodies may need a very different piece length, so start over
    void bodiesChanged() override {
        hasHistory = false;
        nextStep = 0.0;
    }
    void forgetHistory() override { hasHistory = false; }
    const char* name() const override { return "IAS15"; }

private:
    // a(t) = a0 + b[0] t + b[1] t^2 + ... + b[6] t^7, with t running from 0 to 1 across the step
    using Coefficients = std::array<Vector2D, 7>;

    double piece(World& world, Gravity& gravity, double h, double minPiece);  // returns the length it took
    void converge(std::vector<Body>& bodies, Gravity& gravity, double h);    // the predictor-corrector
    double suggestStep(double h) const;  // how long the next piece should be, from b[6]
    void rescale(double ratio);          // the same curve over a piece ratio times as long
    void predictNextStep(double ratio);  // shifts b so it starts where the last step ended

    std::vector<Vector2D> x0, v0, a0;  // each body at the start of the step
    std::vector<Coefficients> b;       // the polynomial, in powers of t
    std::vector<Coefficients> g;       // the same polynomial in Newton form
    bool hasHistory = false;           // b holds the last step's polynomial
    double lastStep = 0.0;             // days
    double suggested = 0.0;            // days, the last piece's suggestion for the next one
    double nextStep = 0.0;             // days, the length to try next (0: none yet)
};

}
