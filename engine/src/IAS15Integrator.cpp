#include "emma/IAS15Integrator.hpp"
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <limits>

namespace emma {

namespace {

// Gauss-Radau spacing: the moments within a step where IAS15 samples the forces,
// as fractions of the step. 0 is the start; the end (1) is never sampled
constexpr std::array<double, 8> NODES = {
    0.0,
    0.0562625605369221464656521910318,
    0.180240691736892364987579942780,
    0.352624717113169637373907769648,
    0.547153626330555383001448554766,
    0.734210177215410531523210605558,
    0.885320946839095768090359771030,
    0.977520613561287501891174488626,
};

// The Newton basis N_k(t) = t (t - h1) (t - h2) ... (t - hk), k = 0..6, written out
// in powers of t: N_k(t) = sum over j of NEWTON_TO_POWER[k][j] t^(j+1). This turns
// the Newton-form coefficients g into the power-form b
std::array<std::array<double, 7>, 7> makeNewtonToPower() {
    std::array<std::array<double, 7>, 7> rows{};
    std::array<double, 7> poly{};
    poly[0] = 1.0;  // N_0(t) = t
    for (std::size_t k = 0; k < 7; ++k) {
        if (k > 0) {
            // Multiply by (t - h_k): every power moves up by one, minus h_k times itself
            for (std::size_t j = k; j > 0; --j) poly[j] = poly[j - 1] - NODES[k] * poly[j];
            poly[0] = -NODES[k] * poly[0];
        }
        rows[k] = poly;
    }
    return rows;
}

const std::array<std::array<double, 7>, 7> NEWTON_TO_POWER = makeNewtonToPower();

// n choose k
double choose(std::size_t n, std::size_t k) {
    double r = 1.0;
    for (std::size_t i = 1; i <= k; ++i) r = r * static_cast<double>(n - k + i) / static_cast<double>(i);
    return r;
}

// Where a body is at fraction s of a step of h days: its acceleration polynomial
// integrated twice. With t = s h:
// x = x0 + v0 t + t^2 (a0/2 + b0 s/6 + b1 s^2/12 + b2 s^3/20 + ... + b6 s^7/72)
Vector3D positionAt(const Vector3D& x0, const Vector3D& v0, const Vector3D& a0,
                    const std::array<Vector3D, 7>& b, double s, double h) {
    Vector3D p;
    for (std::size_t r = 0; r < 7; ++r) {
        const std::size_t k = 6 - r;  // innermost term first (Horner's rule)
        const double kk = static_cast<double>(k);
        p = b[k] / ((kk + 2.0) * (kk + 3.0)) + p * s;
    }
    p = a0 / 2.0 + p * s;
    const double t = s * h;
    return x0 + v0 * t + p * (t * t);
}

// Velocity at the end of a step of h days: the polynomial integrated once.
// v = v0 + h (a0 + b0/2 + b1/3 + ... + b6/8)
Vector3D velocityAtEnd(const Vector3D& v0, const Vector3D& a0, const std::array<Vector3D, 7>& b, double h) {
    Vector3D sum = a0;
    for (std::size_t k = 0; k < 7; ++k) sum += b[k] / (static_cast<double>(k) + 2.0);
    return v0 + sum * h;
}

} // namespace

int IAS15Integrator::step(World& world, Gravity& gravity, double dt, const std::function<void()>& afterPiece) {
    // Pieces as long as IAS15 asks for, cut so the last one lands exactly on dt
    const double minPiece = dt / MAX_SUBSTEPS;
    double remaining = dt;
    int pieces = 0;
    while (remaining > 1e-12 * dt) {
        const double wanted = nextStep > 0.0 ? nextStep : dt;
        double h = std::max(wanted, minPiece);
        if (h >= remaining) h = remaining;
        else if (remaining < 1.5 * h) h = 0.5 * remaining;  // no sliver left over at the end

        const double taken = piece(world, gravity, h, minPiece);
        // The next piece: IAS15's suggestion, at most 1/SAFETY times as long as the
        // last piece it chose itself. A piece cut short to land on dt wasn't chosen
        const double chosen = taken < h ? taken : std::max(taken, wanted);
        nextStep = std::min(suggested, chosen / SAFETY);

        if (afterPiece) afterPiece();
        remaining -= taken;
        ++pieces;
    }
    return pieces;
}

double IAS15Integrator::piece(World& world, Gravity& gravity, double h, double minPiece) {
    std::vector<Body>& bodies = world.bodies;
    const std::size_t n = bodies.size();

    // First guess for this step's polynomial: the last step's, shifted to start
    // where it ended. Without one (at the start, after bodies were added or merged,
    // or after a big jump in step size) the guess is constant acceleration
    if (hasHistory && b.size() == n && lastStep > 0.0 && h <= 4.0 * lastStep) predictNextStep(h / lastStep);
    else b.assign(n, Coefficients{});
    g.resize(n);
    x0.resize(n);
    v0.resize(n);
    a0.resize(n);

    gravity.accelerate(bodies);  // the forces at the start of the step
    for (std::size_t i = 0; i < n; ++i) {
        Body& body = bodies[i];
        body.previousPosition = body.position;  // collisions sweep from here
        x0[i] = body.position;
        v0[i] = body.velocity;
        a0[i] = body.acceleration;
    }

    // Converge, and redo it shorter while IAS15 finds the piece far too long for
    // how fast the forces change
    for (;;) {
        converge(bodies, gravity, h);
        suggested = suggestStep(h);
        if (suggested >= SAFETY * h || h <= minPiece) break;
        const double shorter = std::max(suggested, minPiece);
        rescale(shorter / h);  // the same curve over the shorter piece: a good first guess
        h = shorter;
    }

    // Move every body along the converged polynomial to the end of the step
    for (std::size_t i = 0; i < n; ++i) {
        bodies[i].position = positionAt(x0[i], v0[i], a0[i], b[i], 1.0, h);
        bodies[i].velocity = velocityAtEnd(v0[i], a0[i], b[i], h);
    }
    world.time += h;
    lastStep = h;
    hasHistory = true;
    return h;
}

void IAS15Integrator::converge(std::vector<Body>& bodies, Gravity& gravity, double h) {
    const std::size_t n = bodies.size();

    // The current guess in Newton form: solve b = NEWTON_TO_POWER g from the top down
    for (std::size_t i = 0; i < n; ++i) {
        for (std::size_t r = 0; r < 7; ++r) {
            const std::size_t j = 6 - r;
            Vector3D value = b[i][j];
            for (std::size_t k = j + 1; k < 7; ++k) value -= g[i][k] * NEWTON_TO_POWER[k][j];
            g[i][j] = value;
        }
    }

    // Predictor-corrector: sample the forces along the current guess, refit, repeat
    double lastError = std::numeric_limits<double>::infinity();
    for (int pass = 0; pass < MAX_ITERATIONS; ++pass) {
        double change = 0.0, scale = 0.0;
        for (std::size_t node = 1; node < NODES.size(); ++node) {
            const double s = NODES[node];
            for (std::size_t i = 0; i < n; ++i) bodies[i].position = positionAt(x0[i], v0[i], a0[i], b[i], s, h);
            gravity.accelerate(bodies);

            for (std::size_t i = 0; i < n; ++i) {
                const Vector3D& a = bodies[i].acceleration;
                // Divided differences: the Newton coefficient this sample pins down
                Vector3D value = (a - a0[i]) / s;
                for (std::size_t m = 1; m < node; ++m) value = (value - g[i][m - 1]) / (s - NODES[m]);
                const Vector3D delta = value - g[i][node - 1];
                g[i][node - 1] = value;
                // ... and the power-form coefficients it feeds into
                for (std::size_t j = 0; j < node; ++j) b[i][j] += delta * NEWTON_TO_POWER[node - 1][j];

                if (node == 7) {
                    change = std::max(change, delta.magnitude());
                    scale = std::max(scale, a.magnitude());
                }
            }
        }
        // Done once the highest coefficient has settled, or when rounding noise
        // keeps it from settling any further
        const double error = scale > 0.0 ? change / scale : 0.0;
        if (error < CONVERGED) break;
        if (pass >= 2 && error >= lastError) break;
        lastError = error;
    }
}

double IAS15Integrator::suggestStep(double h) const {
    // b[6], the t^7 term, is the part of the motion the polynomial only just
    // catches. Relative to the acceleration it grows like h^7, so scale h until
    // that ratio would be EPSILON
    double highest = 0.0, scale = 0.0;
    for (std::size_t i = 0; i < b.size(); ++i) {
        highest = std::max(highest, b[i][6].magnitude());
        scale = std::max(scale, a0[i].magnitude());
    }
    if (highest <= 0.0 || scale <= 0.0) return h / SAFETY;  // nothing changing: grow as fast as allowed
    return h * std::pow(EPSILON / (highest / scale), 1.0 / 7.0);
}

void IAS15Integrator::rescale(double ratio) {
    // The same curve a(t) over a piece ratio times as long, from the same start:
    // the t^(j+1) term picks up ratio^(j+1)
    for (Coefficients& c : b) {
        double power = ratio;
        for (std::size_t j = 0; j < 7; ++j) {
            c[j] = c[j] * power;
            power *= ratio;
        }
    }
}

void IAS15Integrator::predictNextStep(double ratio) {
    // The last step's polynomial a(t), re-centered on its end and stretched to the
    // new step: a(1 + ratio s), written out in powers of s. Its constant term is
    // the new a0, which gets computed fresh, so only the b's are kept
    for (Coefficients& c : b) {
        Coefficients shifted{};
        double power = ratio;  // ratio^(j+1)
        for (std::size_t j = 0; j < 7; ++j) {
            Vector3D sum;
            for (std::size_t k = j; k < 7; ++k) sum += c[k] * choose(k + 1, j + 1);
            shifted[j] = sum * power;
            power *= ratio;
        }
        c = shifted;
    }
}

} // namespace emma
