# Roadmap

This is the plan for turning EMMA from a simulator into an engine. The physics already lives in its own library, `emma_core`, which has no graphics. The steps below build on that.

## Done

### Phase 1: engine and app split

- The physics, time, the Horizons download and the calendar moved into `engine/`, built as the `emma_core` library in `namespace emma`. Its public headers are in `engine/include/emma/`.
- The window, the drawing and the sidebar stayed in `app/`, built on the engine with SFML and Dear ImGui.
- Time moved out of the app into `emma::Clock`: speed, pause, single steps and real time.
- `Simulation` was split into parts that it now ties together: `World`, `Gravity`, `VerletIntegrator` and collisions.

## Planned

### An integrator interface

`VerletIntegrator` becomes one implementation of an `Integrator` interface, so other integrators can be swapped in without changing the rest of the engine.

### A higher-order integrator

An integrator such as IAS15, for systems with many close passes. Velocity Verlet gets through a close pass by splitting the step into as many as 4,096 pieces, and a split step isn't symplectic, so each close pass leaves a small permanent offset in the energy drift.

### 3D

Inclinations and a 3D camera. The simulation is 2D today: orbits lie flat in the ecliptic plane and their tilts are dropped. The largest, Mercury's, is 7°.

### More real systems

More presets built from measured data, such as TRAPPIST-1.
