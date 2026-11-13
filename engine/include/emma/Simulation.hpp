#ifndef SIMULATION_HPP
#define SIMULATION_HPP

#include "emma/Body.hpp"
#include "emma/Collisions.hpp"
#include "emma/Constants.hpp"
#include "emma/Gravity.hpp"
#include "emma/Integrator.hpp"
#include "emma/Presets.hpp"
#include "emma/VerletIntegrator.hpp"
#include "emma/World.hpp"
#include <cstddef>
#include <memory>
#include <vector>

namespace emma {

// A running system: a World, the gravity and integrator that move it, and the
// collisions between its bodies. The app talks to the engine through this class.
// No rendering in here
class Simulation {
public:
    static constexpr float FLASH_LIFETIME = 0.5f;  // seconds of real time
    static constexpr double MIN_STEP = 1e-9;       // days (under 0.1 ms), the shortest step limitStep() allows

    // The parts' settings, under the names the app and tests already use
    static constexpr double SOFTENING = Gravity::SOFTENING;
    static constexpr double THETA = Gravity::THETA;
    static constexpr std::size_t DIRECT_LIMIT = Gravity::DIRECT_LIMIT;
    static constexpr double ETA = VerletIntegrator::ETA;
    static constexpr int MAX_SUBSTEPS = VerletIntegrator::MAX_SUBSTEPS;
    static constexpr std::size_t MAX_TRAIL = World::MAX_TRAIL;
    static constexpr double TRAIL_TURN = World::TRAIL_TURN;

    // With a snapshot, the solar system starts from it. without, from the J2000 orbital elements
    void load(Preset preset, const SolarSnapshot* snapshot = nullptr);
    void clear();

    void step();                       // one fixed step of timeStep() days
    // Shortens the step to at most maxDays, for slow speeds and real time. The preset's
    // own step stays the upper limit
    void limitStep(double maxDays);
    void updateEffects(float realDt);  // merge flashes

    void addBody(double mass, const Vector2D& position, const Vector2D& velocity);

    // Swaps the integrator. The new one starts knowing nothing about the bodies.
    // Above DIRECT_LIMIT bodies, one that needs exact forces (IAS15) gives way to Verlet
    void setIntegrator(std::unique_ptr<Integrator> next);
    const Integrator& getIntegrator() const { return *integrator; }

    // The body whose gravity dominates at p: the smallest Laplace sphere of
    // influence containing p, otherwise the strongest pull. -1 with no bodies
    int dominantBody(const Vector2D& p, int exclude = -1) const;
    // Velocity for a circular, prograde orbit around the dominant body at p
    Vector2D circularVelocity(const Vector2D& p) const;
    // Where a massless body starting at p with velocity v would go
    std::vector<Vector2D> predictPath(const Vector2D& p, const Vector2D& v) const;

    double totalEnergy() const;      // Msun AU^2 / day^2
    double angularMomentum() const;  // Msun AU^2 / day, about the origin

    const World& getWorld() const { return world; }
    const std::vector<Body>& getBodies() const { return world.bodies; }
    std::size_t getBodyCount() const { return world.bodies.size(); }
    double time() const { return world.time; }        // days since the preset's start
    double timeStep() const { return dt; }            // days
    int lastSubstepCount() const { return lastSubsteps; }  // pieces the last step was split into
    bool hasCalendar() const { return world.hasCalendar; }  // true when time 0 is a real date
    double epoch() const { return world.epoch; }      // Julian day at time 0
    double viewRadius() const { return viewRadiusAU; }
    double suggestedTimeScale() const { return timeScaleHint; }
    Preset preset() const { return currentPreset; }
    unsigned long long changeCount() const { return world.changes; }  // bumps on add, merge, load, clear

    std::vector<MergeFlash> flashes;

private:
    int strongestPull(const Vector2D& p, int exclude) const;
    void checkIntegrator();  // Verlet takes over from IAS15 above DIRECT_LIMIT bodies

    World world;
    Gravity gravity;
    std::unique_ptr<Integrator> integrator = std::make_unique<VerletIntegrator>();
    Preset currentPreset = Preset::SolarSystem;
    double dt = 0.1;
    double baseDt = 0.1;  // the preset's step, before limitStep()
    double viewRadiusAU = 2.0;
    double timeScaleHint = 30.0;
    int lastSubsteps = 1;
};

} // namespace emma

#endif
