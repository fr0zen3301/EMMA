#ifndef SIMULATION_HPP
#define SIMULATION_HPP

#include "Body.hpp"
#include "Constants.hpp"
#include "QuadTree.hpp"
#include <array>
#include <cstddef>
#include <vector>

enum class Preset { SolarSystem = 0, AlphaCentauri = 1, Empty = 2 };

// The solar system preset's bodies, in this order:
// Sun, Mercury, Venus, Earth, Moon, Mars, Jupiter, Saturn, Uranus, Neptune
constexpr std::size_t SOLAR_BODY_COUNT = 10;

// Measured starting state for the solar system (AU and AU/day, ecliptic plane)
// Any origin works: load() moves everything to the barycenter.
struct SolarSnapshot {
    double julianDay = 0.0;  // UTC
    std::array<Vector2D, SOLAR_BODY_COUNT> position;
    std::array<Vector2D, SOLAR_BODY_COUNT> velocity;
};

struct MergeFlash {
    Vector2D position;
    float age = 0.f;  // seconds of real time
};

// The physics. No rendering in here
class Simulation {
public:
    static constexpr float FLASH_LIFETIME = 0.5f;    // seconds of real time
    static constexpr double SOFTENING = 1e-7;        // AU (15 km), far below any body's radius
    static constexpr double THETA = 0.5;             // Barnes-Hut opening angle
    static constexpr double ETA = 0.03;              // longest piece of a step, as a fraction of the tightest dynamical time
    static constexpr int MAX_SUBSTEPS = 4096;        // most pieces one step can be split into
    static constexpr std::size_t DIRECT_LIMIT = 64;  // up to this many bodies, exact pairwise forces
    static constexpr std::size_t MAX_TRAIL = 540;    // points per trail: three full turns
    static constexpr double TRAIL_TURN = 2.0 * units::DEG;  // a new trail point after this much turning
    static constexpr double MIN_STEP = 1e-9;          // days (under 0.1 ms), the shortest step limitStep() allows

    // With a snapshot, the solar system starts from it; without, from the J2000 orbital elements
    void load(Preset preset, const SolarSnapshot* snapshot = nullptr);
    void clear();

    void step();                       // one fixed step of timeStep() days
    // Shortens the step to at most maxDays, for slow speeds and real time. The preset's
    // own step stays the upper limit
    void limitStep(double maxDays);
    void updateEffects(float realDt);  // merge flashes, in real time

    void addBody(double mass, const Vector2D& position, const Vector2D& velocity);

    // The body whose gravity dominates at p: the smallest Laplace sphere of
    // influence containing p, otherwise the strongest pull. -1 with no bodies
    int dominantBody(const Vector2D& p, int exclude = -1) const;
    // Velocity for a circular, prograde orbit around the dominant body at p
    Vector2D circularVelocity(const Vector2D& p) const;
    // Where a massless body starting at p with velocity v would go
    std::vector<Vector2D> predictPath(const Vector2D& p, const Vector2D& v) const;

    double totalEnergy() const;      // Msun AU^2 / day^2
    double angularMomentum() const;  // Msun AU^2 / day, about the origin

    const std::vector<Body>& getBodies() const { return bodies; }
    std::size_t getBodyCount() const { return bodies.size(); }
    double time() const { return simTime; }      // days since the preset's start
    double timeStep() const { return dt; }       // days
    int lastSubstepCount() const { return lastSubsteps; }  // pieces the last step was split into
    bool hasCalendar() const { return calendar; }  // true when time 0 is a real date
    double epoch() const { return epochJD; }       // Julian day at time 0
    double viewRadius() const { return viewRadiusAU; }
    double suggestedTimeScale() const { return timeScaleHint; }
    Preset preset() const { return currentPreset; }
    unsigned long long changeCount() const { return changes; }  // bumps on add, merge, load, clear

    std::vector<MergeFlash> flashes;

private:
    void verletStep(double h);
    double dynamicalTime() const;
    void computeAccelerations();
    void resolveCollisions();
    void recordTrails();
    void merge(std::size_t a, std::size_t b);
    int strongestPull(const Vector2D& p, int exclude) const;
    void moveToBarycenter();
    void buildSolarSystem(const SolarSnapshot* snapshot);
    void buildAlphaCentauri();

    std::vector<Body> bodies;
    std::vector<Vector2D> oldAcceleration;
    QuadTree tree;
    Preset currentPreset = Preset::SolarSystem;
    double simTime = 0.0;
    double dt = 0.1;
    double baseDt = 0.1;  // the preset's step, before limitStep()
    double viewRadiusAU = 2.0;
    double timeScaleHint = 30.0;
    bool calendar = false;
    double epochJD = units::J2000_JD;
    bool accelerationsStale = true;
    bool jerkValid = false;   // oldAcceleration holds the previous piece's accelerations
    double lastSubstep = 0.0;
    int lastSubsteps = 1;
    unsigned long long changes = 0;
};

#endif
