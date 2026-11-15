# Roadmap

This is the plan for turning EMMA from a simulator into an engine. The physics already lives in its own library, `emma_core`, which has no graphics. The steps below build on that.

## Done

### Phase 1: engine and app split

- The physics, time, the Horizons download and the calendar moved into `engine/`, built as the `emma_core` library in `namespace emma`. Its public headers are in `engine/include/emma/`.
- The window, the drawing and the sidebar stayed in `app/`, built on the engine with SFML and Dear ImGui.
- Time moved out of the app into `emma::Clock`: speed, pause, single steps and real time.
- `Simulation` was split into parts that it now ties together: `World`, `Gravity`, `VerletIntegrator` and collisions.

### Integrators: an interface and IAS15

- `Integrator` is an interface, and `Simulation` talks only to it, so integrators can be swapped without changing the rest of the engine. `VerletIntegrator` is one implementation.
- `IAS15Integrator` adds IAS15, which picks its own piece length and aims for errors near the rounding limit of a double. Through a close pass its energy error stays far smaller than Verlet's.
- The sidebar switches between the two.

### 3D physics

- `Vector2D` became `Vector3D`. `cross()` returns a vector, and angular momentum is a vector too.
- The offline start keeps each orbit's inclination and ascending node from the Standish table, and the Moon's orbit is tilted 5.145°. Today's positions from Horizons keep their z, and the cache moved to v2.
- The Barnes-Hut tree became an octree.
- The `inclination` test checks Mercury's 7° and the Moon's 5° tilts.

### 3D camera

- An orbit camera with perspective. Right-drag turns the view, Shift+right or middle drag pans, and scroll zooms toward the cursor.
- Clicks land on a grid through the point the camera looks at, by picking with a ray from the eye.
- Follow, Top view, the grid, and labels and bodies sorted by depth.
- Details in `docs/3d-camera.md`.

### Galaxy view

- The picture can turn into Galactic coordinates and move with the Sun at 247 km/s, so the planets trace helices. The physics doesn't change.
- `Galaxy.hpp` holds the ecliptic to Galactic rotation and the Sun's velocity. `SceneFrame` places points in the picture, and trail points carry their time.
- A slider sets the Sun's speed. The starfield has a Milky Way band, and a dashed ray shows the Sun's heading.
- The `galactic_frame` test checks the rotation and the Sun's direction.
- The Hipparcos matrix is straightened as it's built, so its transpose is its inverse to the last digit. The test checks the raw table separately, so a typo can't hide.
- Details in `docs/galaxy-view.md`.

### Camera tests

- `tests/view_tests.cpp` is a second test program, `view_tests`. The camera is app code and uses SFML's `Rect` and `Vector2` types, so this program links SFML for those headers, and `physics_tests` still links only the engine.
- Six tests: project then pick returns the same grid point from 20 view angles; clipping keeps exactly the visible part; zooming at the cursor keeps its grid point fixed and stops at the limits; panning keeps the grabbed point under the cursor and slides near the horizon; picks miss when they should; Fit view shows the requested radius.

### The Sun's real path around the Galactic center

- The galaxy view carries the Sun around a circle of radius R₀ = 8.15 kpc centered on the Galactic center, instead of along a straight line. The small drifts toward the center and the pole stay straight.
- `galaxy::sunDisplacement` and `sunVelocityAt` hold the math, `SceneFrame::place` uses it, and `SceneFrame::heading()` turns the heading ray with the Sun.
- The `sun_path` test checks the 80 AU bend over 10,000 years, the quarter-orbit position and the heading.
- Details in `docs/galaxy-view.md`, section 6.

### Planet surfaces

- The Sun, the planets and the Moon are drawn with their real surfaces once they're at least 3 px in radius: flattened spheres, turned by the IAU's pole and spin models at the simulated date, and lit by the Sun.
- Earth has city lights and clouds, the Moon its relief from a height map, and Saturn its rings, with shadows both ways.
- The `body_spin` test checks Greenwich at J2000, the tilts of Earth, Saturn and the Moon, and that the axes stay a rotation.
- Details in `docs/planet-surfaces.md`.

## Planned

### Adding bodies in the galaxy view

Clicking does nothing in the galaxy view today. To allow it:
- Turn the click back into the simulation frame. A scene point q at time t is the simulation point R⁻¹·(q − D(t)), with D(t) the Sun's displacement.
- Turn the launch velocity back with R⁻¹ only. It's measured relative to the dominant body, which moves with the Sun too, so the Sun's motion cancels.
- `predictPath` returns positions only, sampled unevenly through close passes. To draw the dashed prediction through the frame, each point at its own time, it needs to return times too, as `TrailPoint`s.
- Decide which plane a click lands on. The galaxy view's grid is the Galactic plane, 60° to the ecliptic, so new bodies would launch on orbits tilted 60° to the planets'. The alternative is picking on the ecliptic through the target, which needs `pick` to take any plane, and drawing that plane.

### The big-scale view

The view from the clip: the Galactic center and the eROSITA bubbles alongside the solar system, at true scale.
- Done: the Sun's real path, and zooming out to 10¹⁰ AU (about 48 kpc) with distances in parsecs and light-years. The `camera_galactic_scale` test checks the camera at both ends of the range.
- To do: Sgr A* at the Galactic center; the solar system as one labeled marker when it's too small to see; the starfield fading out when the view is far outside the solar system; the eROSITA bubbles, with their size and shape from the discovery paper (Predehl et al. 2020).
- Details in `docs/big-scale-view.md`.

### Sgr A* and the S-stars

The supermassive black hole at the Galactic center, about 4.3 million solar masses, as a landmark in the big-scale view and as a preset of its own with the S-stars orbiting it.
- A black hole isn't a mass class, so `Body` needs an explicit kind, which also decides how it's drawn and its size for collisions.
- S2 passes 120 AU from Sgr A* at 7,700 km/s on a 16-year orbit with an eccentricity of 0.88: a showcase for IAS15.
- Optionally a first post-Newtonian correction to the force. It would add S2's 12′ per orbit precession, measured by GRAVITY in 2020, and Mercury's 43″ per century.
- Pick one distance to the Galactic center: GRAVITY's fits use about 8.28 kpc, `Galaxy.hpp` uses Reid's 8.15 kpc.
- Done, first version: Sgr A* ray traced up close in the app, with three looks (gas flow, thin disk, no disk), a Go to Sgr A* button and five tests. Details in `docs/black-hole.md`.
- Still to do: a spinning (Kerr) hole, and Sgr A* as a real `Body` for the S-stars preset.

### More real systems

More presets built from measured data, such as TRAPPIST-1.
