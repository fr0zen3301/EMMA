#ifndef SIMULATION_HPP
#define SIMULATION_HPP

#include "Body.hpp"
#include "QuadTree.hpp"
#include <cstddef>
#include <vector>

enum class Preset { SolarSystem = 0, AlphaCentauri = 1, Empty = 2 };

struct MergeFlash {
    Vector2D position;
    float age = 0.f;  // seconds of real time
};

// The physics. No rendering in here: units are AU, solar masses and days throughout.
class Simulation {
public:
    static constexpr float FLASH_LIFETIME = 0.5f;    // seconds of real time
    static constexpr double SOFTENING = 1e-7;        // AU (15 km), far below any body's radius
    static constexpr double THETA = 0.5;             // Barnes-Hut opening angle
    static constexpr double ETA = 0.03;              // longest piece of a step, as a fraction of the tightest dynamical time
    static constexpr int MAX_SUBSTEPS = 4096;        // most pieces one step can be split into
    static constexpr std::size_t DIRECT_LIMIT = 64;  // up to this many bodies, exact pairwise forces
    static constexpr std::size_t MAX_TRAIL = 720;    // samples per trail (about two orbits)

    void load(Preset preset);
    void clear();

    void step();                       // one fixed step of timeStep() days
    void updateEffects(float realDt);  // merge flashes, in real time

    void addBody(double mass, const Vector2D& position, const Vector2D& velocity);

    // The body whose gravity dominates at p: the smallest Laplace sphere of
    // influence containing p, otherwise the strongest pull. -1 with no bodies.
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
    bool hasCalendar() const { return calendar; }  // true when time 0 is J2000
    double viewRadius() const { return viewRadiusAU; }
    double suggestedTimeScale() const { return timeScaleHint; }  // days per real second
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
    double orbitalPeriodEstimate(std::size_t index) const;
    double trailIntervalFor(std::size_t index) const;
    void moveToBarycenter();
    void buildSolarSystem();
    void buildAlphaCentauri();

    std::vector<Body> bodies;
    std::vector<Vector2D> oldAcceleration;
    QuadTree tree;
    Preset currentPreset = Preset::SolarSystem;
    double simTime = 0.0;
    double dt = 0.1;
    double viewRadiusAU = 2.0;
    double timeScaleHint = 30.0;
    bool calendar = false;
    bool accelerationsStale = true;
    bool jerkValid = false;   // oldAcceleration holds the previous piece's accelerations
    double lastSubstep = 0.0;
    int lastSubsteps = 1;
    unsigned long long changes = 0;
};

#endif
