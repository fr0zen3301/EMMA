# Roadmap

This is the plan for turning EMMA from a simulator into an engine. The physics lives in its own library, `emma_core`, with no graphics, and each step below builds on that.

## Done

### Phase 1: engine and app split

- The physics, time, the Horizons download and the calendar moved into `engine/`, built as the `emma_core` library in `namespace emma`. Its public headers are in `engine/include/emma/`.
- The window, the drawing and the sidebar stayed in `app/`, built on the engine with SFML and Dear ImGui.
- Time moved out of the app into `emma::Clock`: speed, pause, single steps and real time.
- `Simulation` was split into parts it ties together: `World`, `Gravity`, the integrator and collisions.

### Integrators: an interface and IAS15

- `Integrator` is an interface, and `Simulation` talks only to it, so integrators can be swapped without changing the rest of the engine. `VerletIntegrator` is one; `IAS15Integrator` is the other.
- IAS15 picks its own piece length and keeps its errors near the rounding limit of a double. Through a close pass its energy error stays far smaller than Verlet's.
- The Physics tab switches between the two, and above 64 bodies EMMA switches to Verlet by itself.
- Details in `docs/integrator-interface.md` and `docs/ias15.md`.

### 3D physics

- `Vector2D` became `Vector3D`. `cross()` returns a vector, and angular momentum is a vector too.
- The offline start keeps each orbit's inclination and ascending node from the Standish table, and the Moon's orbit is tilted 5.145°. Today's positions from Horizons keep their z, and the cache moved to v2.
- The Barnes-Hut tree became an octree.
- The `inclination` test checks Mercury's 7° and the Moon's 5° tilts.

### 3D camera

- An orbit camera with perspective. Right-drag turns the view, Shift+right or middle drag pans, and scroll zooms toward the cursor.
- Clicks land on a grid through the point the camera looks at, by picking with a ray from the eye.
- Follow, Top view, the grid, and labels and bodies sorted by depth.
- `tests/view_tests.cpp` is a second test program, so `physics_tests` still links only the engine. Its camera tests check that projecting and picking return the same grid point from 20 view angles, that clipping keeps exactly the visible part of a line, that zoom and pan keep the grid point under the cursor, that picks miss when they should, and that Fit view shows the requested radius.
- Details in `docs/3d-camera.md`.

### Galaxy view

- The picture can turn into Galactic coordinates and move with the Sun at 247 km/s, so the planets trace helices. The physics doesn't change.
- `Galaxy.hpp` holds the ecliptic to Galactic rotation and the Sun's velocity. `SceneFrame` places points in the picture, and trail points carry their time.
- A slider sets the Sun's speed. The starfield has a Milky Way band, and a dashed arrow shows the Sun's heading.
- The Hipparcos matrix is straightened as it's built, so its transpose is its inverse to the last digit. The `galactic_frame` test checks the raw table separately, so a typo can't hide.
- The Sun goes around a circle of radius R₀ = 8.15 kpc centered on the Galactic center. The `sun_path` test checks the 80 AU bend over 10,000 years, the quarter-orbit position and the heading.
- Details in `docs/galaxy-view.md`.

### Planet surfaces

- The Sun, the planets and the Moon are drawn with their real surfaces once they're at least 3 px in radius: flattened spheres, turned by the IAU's pole and spin models at the simulated date, and lit by the Sun.
- Earth has city lights and clouds, the Moon its relief from a height map, and Saturn its rings, with shadows both ways.
- The `body_spin` test checks Greenwich at J2000, the tilts of Earth, Saturn and the Moon, and that the axes stay a rotation.
- Details in `docs/planet-surfaces.md`.

### Sgr A*, ray traced

- The black hole at the Galactic center, drawn up close by following each pixel's light backward through curved space, with three looks: the real gas (the default), a thin disk, and no disk.
- Go to Sgr A* jumps 8.15 kpc to it from the solar system, and five tests check the ray tracer's math.
- Details in `docs/black-hole.md`.

### Relativity, black holes as bodies, and the S-stars

- General relativity's turn of every orbit, from one extra potential between every pair, −3 G² m₁ m₂ (m₁ + m₂) / (c² r²) (Nobili & Roxburgh 1986). It turns orbits by exactly relativity's 6π G M / (c² a (1 − e²)) a lap, depends only on positions so Verlet stays symplectic, and the energy readout includes it. It's on by default, with an Exaggerate factor up to 100.
- `BodyType::BlackHole`, with a spin and a spin axis. Its radius is its horizon, and it survives every merge with its horizon growing. Holes can be added from the Add tab.
- Any hole can be drawn. The one that looks biggest is ray traced; the others are markers.
- The Sgr A* and S-stars preset: Sgr A* as a body, and S2, S29, S38 and S55 from GRAVITY's 2022 four-star fit, seen from Earth from 2017.0.
- Four tests: `black_hole_horizon`, `black_hole_swallow`, `mercury_precession` (42.98″ a century) and `s2_precession` (12.2′ a lap, and S2's 2018 pericenter).
- Details in `docs/relativity.md`.

### The spacetime grid

- Bend the grid turns the grid into a sheet that dips where clocks run slower, ψ = G M / (c² r) summed over the 32 heaviest bodies, plus Sgr A* in the solar system. That's the time part of spacetime's curvature, the part that holds orbits.
- The sheet is drawn as someone falling freely with the middle of the view would see it, so only nearby wells and far masses' tides show. It's exaggerated to a share of the view set by the Depth slider.
- A readout says how much slower a clock at the middle of the view runs.
- Three tests: `sheet_clocks`, `sheet_ball`, `sheet_falling`.
- Details in `docs/spacetime-grid.md`.

### The sidebar

- A fixed top with Pause, Step, Real time, the speed and the system picker, and four tabs below it: Add, View, Physics and Black holes.
- One-line tooltips, and the keyboard and mouse shortcuts behind a ? button.
- Leaving real time no longer carries over the gap to the clock, and real time says so when it has to wait because the simulation is ahead of now. The `clock` test checks both.

## Planned

### Adding bodies in the galaxy view

Clicks don't add bodies in the galaxy view yet. To allow it:

- Turn the click back into the simulation frame. A scene point q at time t is the simulation point R⁻¹ · (q − D(t)), with D(t) the Sun's displacement.
- Turn the launch velocity back with R⁻¹ only. It's measured relative to the dominant body, which moves with the Sun too, so the Sun's motion cancels.
- `predictPath` returns positions only, sampled unevenly through close passes. To draw the dashed preview through the frame, each point at its own time, it needs to return times too, as `TrailPoint`s.
- Decide which plane a click lands on. The galaxy view's grid is the Galactic plane, 60° to the ecliptic, so new bodies would launch on orbits tilted 60° to the planets'. The alternative is picking on the ecliptic through the target, which needs `pick` to take any plane, and drawing that plane.

### The big-scale view

The Galactic center and the eROSITA bubbles alongside the solar system, at true scale.

- Done: the Sun's real path, zooming out to 10¹⁰ AU (about 48 kpc) with distances in parsecs and light-years, and Sgr A* at the Galactic center. The `camera_galactic_scale` test checks the camera at both ends of the range.
- To do: the solar system as one labeled marker when it's too small to see; the starfield fading out when the view is far outside the solar system; the eROSITA bubbles, with their size and shape from the discovery paper (Predehl et al. 2020).
- Details in `docs/big-scale-view.md`.

### Black holes: what's left

- A spinning (Kerr) hole's picture: its shadow is flattened on one side and shifted.
- Lense-Thirring precession, the turn a spinning hole gives the orbits near it.
- The full post-Newtonian force as an IAS15 option.
- The spin a merger leaves, and the spin what falls in adds.
- Each S-star started from the date of its own elements, which would put back the 7′ S38's orbit turned in 2003.
- One distance to the Galactic center for both the preset (GRAVITY's 8,277 pc, with 4.297 million M☉) and the solar system's landmark (`Galaxy.hpp`'s 8.15 kpc, with 4.3 million M☉).

### More real systems

More presets built from measured data, such as TRAPPIST-1.
