# EMMA: Roadmap to an Engine

From an n-body simulator to an actual engine, slowly but surely.

## Where EMMA is today

The build already splits `emma_core` (physics, no graphics) from the `nbody` app. That split is the start of an engine. The whole path is growing that boundary until the app is just one program built on top of the engine.

**The rule for the whole journey:** refactor, don't rewrite. Every step should leave EMMA running and the tests passing. Finish each phase on its own git branch, and tag it when it works.

## Phase 1: Turn the core into a library

- [ ] Put everything in `namespace emma`
- [ ] Separate the public headers from the internals:

  ```
  EMMA/
    engine/
      include/emma/   World.hpp  Integrator.hpp  Forces.hpp  Clock.hpp  Scenario.hpp
      src/
    app/              main.cpp  Ui.cpp  Renderer.cpp   (SFML lives only here)
    scenarios/        solar_system.json  alpha_centauri.json
    tests/
  ```

- [ ] Move time control out of `main.cpp` into an `emma::Clock`: the time scale, pause, step, real-time sync and the accumulator. That's engine logic currently living in the app
- [ ] Split up `Simulation`, which does five jobs at once:
  - [ ] storing the bodies → `World`
  - [ ] stepping → `Integrator`
  - [ ] forces → direct sum or Barnes-Hut
  - [ ] collisions
  - [ ] presets → scenarios (Phase 2)

**Done when:** `main.cpp` only handles the window, input and drawing, and calls something like `engine.update(dt)`.

## Phase 2: Make it data-driven

- [ ] Move presets out of C++ into files (JSON with nlohmann/json, for example): bodies, masses, radii, colors, epoch. Adding TRAPPIST-1 then means writing a file, not recompiling
- [ ] Save and load the full state. That gives you reproducibility, rewind and shareable scenarios
- [ ] Put Horizons behind a "data source" interface, so other sources (a file, an exoplanet catalog) plug in the same way

**Done when:** a new system can be added and loaded without touching any C++.

## Phase 3: Physics worth calling an engine

- [ ] **Go 3D.** Horizons already sends the z coordinate, and EMMA throws it away. Switch to `Vector3D`, and turn the quadtree into an octree. Draw the 3D physics with a simple top-down view first, before touching the renderer
- [ ] **Integrator interface** with several options:
  - [ ] keep velocity Verlet
  - [ ] WHFast (symplectic, built for planetary systems)
  - [ ] IAS15 (adaptive, 15th order, handles close encounters)
- [ ] **Composable forces:** Newtonian gravity plus optional extra terms, like the general relativity correction that gives back Mercury's 43″ per century
- [ ] **Measure accuracy against reality.** Start today, integrate 365 days, and compare with what Horizons says for that date. That error is the engine's accuracy, and it makes a great post

**Done when:** you can pick an integrator and force terms per scenario, and a test reports the position error against Horizons after one year.

## Phase 4: Rendering engine

- [ ] Leave SFML (it's 2D) for OpenGL directly (GLFW + glad, keeping ImGui), or raylib if you'd rather go fast than have full control
- [ ] The problems you'll hit, roughly in order:
  1. [ ] A 3D camera
  2. [ ] Float precision at solar-system distances. Fix it by drawing everything relative to the camera ("floating origin")
  3. [ ] Depth over enormous ranges: a logarithmic depth buffer
  4. [ ] Bloom, so stars glow
  5. [ ] Instanced rendering, for thousands of bodies
- [ ] The renderer only reads the world. It never changes it

**Done when:** you can fly a 3D camera from the Sun to the Earth–Moon system without jitter or flicker.

## Phase 5: Scale and tooling

- [ ] **Speed:** profile first (Tracy is great), then speed up the hot spots: a better data layout (struct of arrays), multithreaded forces, later the GPU
- [ ] **Python bindings** (nanobind): `import emma`. This is what makes a science engine useful to other people. REBOUND, the best-known open-source n-body code, is used mostly from Python
- [ ] **Editor tools:** an inspector, a timeline, and video export (handy for IG posts)

## What to read

- *Game Programming Patterns* by Robert Nystrom (free online): the game loop and how to structure updates
- "Fix Your Timestep!" by Glenn Fiedler: EMMA already uses this pattern, and it explains why it works
- *Game Engine Architecture* by Jason Gregory: the standard reference for how engines are organized
- LearnOpenGL.com: for Phase 4
- REBOUND's source on GitHub (hannorein/rebound), plus the IAS15 (Rein & Spiegel 2015) and WHFast (Rein & Tamayo 2015) papers: the best real n-body engine to learn from

## Open decision

Which comes first after Phase 2, physics (Phase 3) or rendering (Phase 4), depends on what EMMA should mainly be:

- **Science engine:** accurate, scriptable, used for experiments → Phase 3, then Phase 5's Python bindings
- **Visual engine:** 3D, beautiful, interactive → Phase 4 early, with just enough of Phase 3 (3D) to feed it

Either way, Phase 1 comes first. A good first step: move time control into an `emma::Clock`.
