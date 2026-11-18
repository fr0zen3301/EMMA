# EMMA

EMMA (Engine for Multi-body Mechanics and Astrophysics) is a real-time gravity simulator written in C++. Every body pulls on every other body through Newton's law of gravity, and EMMA works out where they all go, frame by frame. It draws the result with [SFML 3](https://www.sfml-dev.org/learn/) and puts the controls in a [Dear ImGui](https://github.com/ocornut/imgui) sidebar.

Everything runs in real units: distances in astronomical units (AU), masses in solar masses and time in days. The solar system starts from the planets' real positions today, downloaded from NASA JPL's Horizons service, so Earth takes a year to go around the Sun and the Moon takes 27.3 days to go around Earth. A drift readout shows how well the simulation is holding up. In a perfect simulation, total energy and angular momentum never change, and the readout shows how far they have moved.

## What you can do

- Watch the Sun, the eight planets and the Moon carry on from where they are today, or switch to Alpha Centauri A and B, a real pair of stars on an 80-year orbit.
- Switch to Sgr A* and the S-stars. Sgr A*, the black hole at the center of the Milky Way, is a real body there, and it pulls S2, S29, S38 and S55 along the orbits the GRAVITY instrument measured. The view starts from Earth, so the orbits look the way GRAVITY sees them. S2 swings 120 AU from the hole at 7,700 km/s.
- Watch orbits turn a little every lap, as general relativity says they should: 43 arcseconds a century for Mercury and 12 arcminutes a lap for S2. Relativity is on by default, and the Exaggerate slider multiplies it up to 100 times, enough to turn an orbit into a rosette.
- Add a black hole of 1 to 10 billion solar masses, spinning or not. It swallows whatever touches its horizon and keeps its own name, even when what fell in was heavier.
- Bend the grid into a sheet that dips where clocks run slower. That's the time part of spacetime's curvature, which is what holds orbits. The sheet is drawn as someone falling freely with the middle of the view would see it, and the sidebar says how slow a clock there runs. Details in `docs/spacetime-grid.md`.
- Add a body anywhere. A click places it. Pressing and dragging sets its speed and direction, and a dashed line shows where it will go before you let go.
- Pick Circular orbit and a click puts the new body on a circular orbit around whatever dominates gravity at that spot. That is the Sun almost everywhere, and Earth when you click close to Earth.
- Run in real time (one second per second, in step with your computer's clock) or at any speed from a few hours per second up to ten years per second. You can also pause, or step one frame at a time.
- Pick the integrator: velocity Verlet, which is fast and keeps energy steady over the long run, or IAS15, which aims for errors near the rounding limit of a double.
- Zoom from the Earth and Moon out to about 48 kiloparsecs, far enough to fit the Galactic center in the view, turn the view to see how the orbits tilt, follow any planet, and hover over any body to see its mass, radius, distance and speed.
- Switch on the galaxy view to watch the planets trace stretched helices as the solar system moves through the Milky Way at 247 km/s, or slow the Sun down until their paths start to loop.
- Zoom in on the Sun, a planet or the Moon to see its real surface, turned the way it faces at the simulated moment and lit by the Sun: Earth's city lights on its night side and its clouds, the Moon's craters along the line between day and night, and Saturn's rings with their shadows.
- Go to Sgr A*, the black hole at the Galactic center, and see it ray traced: every pixel's light is followed backward through the curved space around the hole. Show it with the dim gas it really has, with a thin glowing disk, or with only the starfield bent around it, and switch the bending off to see what gravity does.

## Controls

| Input | Action |
|-------|--------|
| Left click on empty space | Place a body with the sidebar's settings |
| Double-click a body | Follow it: the view glides to it and stays on it |
| Left drag | Launch a body: the arrow's length is the speed and its angle the direction |
| Esc | Cancel a launch |
| Right drag | Turn the view around the point it looks at |
| Shift + right drag, or middle drag | Pan |
| Scroll | Zoom toward the cursor. Scrolling in over a body follows it, so the zoom heads for it and stops just outside its surface |
| Space | Pause or resume |
| N | Step one frame |
| F | Fit the view to the current system |
| T | Look straight down on the ecliptic |

Speeds are measured relative to the body that dominates gravity where you click, so a launch next to Earth moves along with Earth.

## The interface

The top of the sidebar is always on show. It has Pause and Step, a Real time switch for the solar system, the speed slider, and the system picker with a Reset button beside it. Below the picker, a line says where the solar system's starting positions came from. The ? button in the corner lists the keyboard and mouse shortcuts.

The rest of the sidebar is split into tabs:

- Add sets up the next body you add. A switch picks a planet or star, or a black hole of 1 to 10 billion solar masses with a spin from 0 to 0.998. A swatch shows the color and class the body will get, anywhere from a rocky body up to an O-type star. A second switch picks a circular orbit or a custom launch, and the speed and direction controls appear only for a custom launch.
- View has Follow, which keeps a named body in the middle of the view, the Fit view and Top view buttons, and switches for trails, labels and the grid. Bend the grid turns the grid into a spacetime sheet, with a Depth slider. For the solar system there's also the galaxy view, with a slider for the Sun's speed and how far the Sun has moved.
- Physics picks the integrator, velocity Verlet (the default) or IAS15, and switches general relativity on and off, with an Exaggerate slider that multiplies it by up to 100. Changing any of these starts the drift readout from a new baseline. Below them are the drift readouts, with a graph of the last 30 seconds, the body count and the frame rate.
- Black holes appears whenever a black hole is on show. Its Go to button jumps to the named one, which is Sgr A* in both the solar system and the S-stars preset. A switch picks how black holes look: Sgr A*'s gas as observed (the default), a thin disk, or no disk. There are also switches for light bending and the Doppler effect, and a brightness slider. Only the hole that looks biggest is ray traced; the others are drawn as markers.

To clear the scene, pick Empty space.

The status bar along the bottom shows how wide the view is at the point the camera looks at, the cursor's position on the grid, the date and the size of the time step. Distances are in AU, and in parsecs and light-years once the view is wider than 100,000 AU. In real time the date includes your local time of day; outside the solar system it shows the time elapsed instead. When a step is split into pieces, the status bar also shows how many.

## The physics

### Units

EMMA uses the units astronomers use for the solar system. One astronomical unit (AU) is 149,597,870.7 km, roughly the average distance between Earth and the Sun. Masses are in solar masses (M☉), so the Sun's mass is 1, and time is in days. In these units the numbers stay close to 1: Earth orbits at about 1 AU and Neptune at about 30, instead of hundreds of millions of kilometers.

The gravitational constant then becomes G = k², where k = 0.01720209895 is the Gaussian gravitational constant. That gives G = 2.959 × 10⁻⁴ AU³ M☉⁻¹ day⁻². The interface converts to km/s and to Earth, Jupiter or solar masses for display.

### Starting positions

When EMMA starts, it asks JPL Horizons for the position and velocity of the Sun, each planet and the Moon at 00:00 UTC today. The numbers are measured from the solar system's barycenter, which is the center of mass of the whole system, in the J2000 ecliptic frame: x and y lie in the ecliptic plane, the plane of Earth's orbit, with x pointing toward the March equinox, and z is the height above or below that plane. Horizons computes them from DE441, JPL's planetary ephemeris: a model that gives the position of each planet and moon at any moment. For Mars and the outer planets, EMMA asks for the barycenter of the planet together with its moons, which matches the masses it uses for them: each one includes the planet's moons.

The ten requests take a few seconds, and the solar system waits, paused, until they finish. EMMA saves the result in the system's temporary folder, so a second start on the same day needs no network.

Without a connection, EMMA starts from the last positions it saved. If it has never downloaded any, it starts from average orbits for 1 January 2000 (mean orbital elements). These keep each orbit's tilt (its inclination) and the line where it crosses the ecliptic (its ascending node), so Mercury's orbit sits 7° out of the plane and the Moon's about 5°. The sidebar says which start it used and offers to try the download again.

### Integration

EMMA moves time forward in fixed steps. The base step is 0.1 day for the solar system (about 880 steps per orbit of Mercury) and 1 day for Alpha Centauri. When time runs slowly, the step shrinks to one frame's worth of time, 10 minutes at 10 hours per second, so bodies glide instead of jumping. In real time it shrinks to a fraction of a second.

An integrator carries the bodies through each step, and the sidebar picks one of two: velocity Verlet, the default, or IAS15. Either one can split a step into as many as 4,096 smaller pieces. After every piece, EMMA merges any bodies that touched and records trail points. A body gets a new trail point each time its direction of travel has turned by 2°, so tight loops and wide orbits both draw as smooth curves.

Each frame runs as many base steps as the chosen speed needs. If that would take too long, EMMA drops the backlog and shows "Running behind" instead of freezing the window.

#### Velocity Verlet

Each piece does three things:

1. Drift: move every body by v·dt + ½·a·dt², using its velocity v and acceleration a.
2. Work out the accelerations again at the new positions.
3. Kick: update every velocity using the average of the old and new accelerations.

Verlet is symplectic. In practice that means its energy error rises and falls around a fixed value and never builds up. With simpler methods such as Euler steps, the error builds up and planets slowly spiral away. EMMA works out a new body's acceleration before its first step, so new bodies get Verlet's full (second-order) accuracy from their very first step.

Close passes need smaller steps. A single long step through a near miss would jump past the closest point and send the pair apart faster than it arrived, which looks like the bodies pushing each other away. So before each piece of a step, EMMA works out two times for every pair of bodies. One is the pair's dynamical time, √(r³ / G(m₁ + m₂)), which sets how quickly the two can swing around each other. The other is the time the pair needs to close the gap at its current speed. A piece can last at most 3% of the shortest of these times.

Away from close passes, that rule allows the whole base step in one piece, so Verlet keeps its long-run energy behavior. During a close pass the step splits into smaller pieces, and the status bar shows the count. Above 64 bodies, checking every pair would cost more than the forces themselves, so EMMA instead sizes the pieces from how fast each body's acceleration changed over the previous piece. Right after a load, a new body or a merge there is no previous piece, so that one piece uses the pairwise check. The dashed predicted path always uses Verlet with this rule, whichever integrator moves the bodies.

#### IAS15

IAS15 (Rein & Spiegel 2015) is a 15th-order integrator that aims for errors near the rounding limit of a double. Over each piece it writes every body's acceleration as a polynomial in time. It works out the forces at the start of the piece and at seven moments spread across it (Gauss-Radau spacing), fits the polynomial to them, and repeats until the fit stops changing. Then it moves every body along the polynomial to the end of the piece.

IAS15 picks its own piece length from the polynomial's highest term, which shows how quickly the forces are changing. When a piece turns out far too long, it redoes it shorter, and from one piece to the next it grows by at most four times. A piece never runs past the end of the base step. Each piece costs at least 8 force calculations instead of Verlet's one, so IAS15 is slower, but through a close pass its energy error stays far smaller than Verlet's.

Above 64 bodies, EMMA switches from IAS15 to Verlet by itself, and the sidebar says so. IAS15 is built for exact forces, and the approximate ones from the Barnes-Hut tree (see Forces) would make it take far shorter pieces.

### Forces

Up to 64 bodies, EMMA adds up the pull between every pair directly, once per pair. That is exact, and at this size it is faster than building a tree. Above 64 bodies it switches to a Barnes-Hut octree, which splits space into nested cubes and treats a distant group of bodies as a single point mass at the group's center of mass. With an opening angle of 0.5, a group counts as distant when it is less than half as wide as its distance from the body being pulled. That brings the cost down from N² to about N log N. The tree is stored in one flat array that gets reused every step.

Gravity is softened very slightly (Plummer softening, with a length of 10⁻⁷ AU, about 15 km). That is smaller than any body's radius, so in practice the force is plain Newtonian gravity, and the softening only keeps the force from dividing by zero when two bodies sit at the same point. The energy readout uses the potential energy that goes with exactly this softened force.

### General relativity

Newton's gravity gives closed ellipses. In Einstein's, each ellipse turns a little in its own plane every lap, by

```
Δω = 6π G M / (c² a (1 − e²))
```

where M is both masses together, a the semi-major axis and e the eccentricity. For Mercury that's 0.1035″ a lap, or 43″ a century, the anomaly Newton's gravity couldn't explain and Einstein's could. For S2 around Sgr A* it's 12.2′ a lap, which GRAVITY measured.

EMMA gets this turn from one extra potential between every pair of bodies:

```
U = −3 G² m₁ m₂ (m₁ + m₂) / (c² r²)
```

For a light body around a heavy one, this is Nobili & Roxburgh's (1986) potential. It turns orbits by exactly the amount above, to first order in GM/(c²r), whatever the two masses are. It doesn't reproduce relativity's other effects on an orbit, such as the change to its period. Because it depends only on positions (the full post-Newtonian force also depends on velocities), Verlet stays symplectic, and the energy readout, which includes this potential, stays exact.

The Exaggerate slider multiplies the potential. GRAVITY's fits scale their relativistic terms by a similar factor, f_SP, and their four-star fit gives 0.997 ± 0.144. Above 64 bodies the extra pull acts only between pairs that include one of the 16 heaviest bodies, which keeps the cost from going back up to N². Details in `docs/relativity.md`.

### Black holes

A black hole is a body of its own type, `BodyType::BlackHole`. Its radius is its event horizon, r₊ = GM/c² (1 + √(1 − a*²)). For a hole that doesn't spin (a* = 0) that's the Schwarzschild radius, 2GM/c², and it shrinks to 0.53 of that at a* = 0.998, the fastest a hole can spin up by swallowing a disk (Thorne 1974). A hole pulls like any other mass, relativistic term included. Only the hole that looks biggest on screen is ray traced, and the ray tracer treats every hole as one that doesn't spin.

### The galaxy view

The simulation always runs around the solar system's barycenter, which stays at rest. The galaxy view changes only the picture. It turns everything from ecliptic coordinates into Galactic coordinates, where the grid is the plane of the Milky Way, and slides each point along the Sun's path to where the Sun was at the moment the point was recorded. That path is a circle around the Galactic center, 8.15 kpc away, at the Sun's speed. That is why every trail point carries a time. The orbits themselves don't change: Newton's laws are the same when the whole system moves at a steady speed, and the Galaxy's pull, which bends that motion into a circle, is almost exactly the same on every body. So this is the same motion seen from the Galaxy's frame.

The Sun moves at 247 km/s around the Galactic center, plus a small drift toward the center and out of the plane, about 52 AU a year. Its path points about 61° out of the plane of the planets' orbits, so at the real speed every planet traces a long, stretched helix. A path only loops when the planet's own speed is larger than the part of the Sun's velocity that lies in its orbital plane, about half of it. The slider lets you lower the Sun's speed to see that.

### Collisions

Bodies merge when their surfaces touch. The check follows each pair along the path both moved during the step, so a fast body can't jump through another between two steps. To keep this quick, EMMA first sorts the bodies along the x axis and only runs the exact check on pairs whose paths overlap there. A merge keeps the total mass and momentum, places the new body at the pair's center of mass, and shows a short ring. The heavier body keeps its name and color.

A black hole merges when something touches its horizon. It always survives, even when what fell in was heavier: it keeps its name, type and spin, and its horizon grows with the new mass. The spin doesn't change, although in reality whatever falls in carries angular momentum, and two holes that merge leave one spinning at about 0.7.

### Sizes and colors

Bodies from the presets use their measured radius and color. A body you add gets its radius and color from its mass:

| Mass | Class | Radius | Color |
|------|-------|--------|-------|
| below 2 Earth masses | rocky body | R ∝ M^0.28 | grey |
| up to 0.41 Jupiter masses | Neptune-like planet | R ∝ M^0.59 | pale blue |
| up to 13 Jupiter masses | gas giant | about one Jupiter radius | tan |
| up to 0.08 solar masses | brown dwarf | about one Jupiter radius | blackbody, 800 to 2,300 K |
| 0.08 solar masses and up | main-sequence star | R ∝ M^0.8, or M^0.57 above 1 M☉ | blackbody at its surface temperature |
| any mass, with Black hole ticked | stellar-mass (under 100 M☉), intermediate-mass, or supermassive (10⁵ M☉ and up) | its event horizon | a shadow in a glowing ring |

The planet and brown dwarf radii follow Chen & Kipping (2017). A star's surface temperature comes from the main-sequence mass-luminosity relation and the Stefan-Boltzmann law, and its color is that of a blackbody (an ideal glowing object) at that temperature. A 0.1 M☉ red dwarf comes out near 2,700 K and orange-red, the Sun at 5,772 K and nearly white, and a 10 M☉ star near 24,000 K and blue. The sidebar and tooltips name the spectral type.

At solar-system scale a planet is much smaller than a pixel, so every body is drawn at least as large as a marker that grows with the logarithm of its mass. Zoom in far enough and the true size takes over. From there, once a body is at least 3 px in radius, EMMA draws its real surface from a map in `textures`, as a sphere flattened by the body's real flattening, turned by the IAU's model of where its pole points and how fast it spins, and lit from the Sun's direction. The background stars turn with the view but stay put while you pan and zoom, as real stars would at their distance.

## Accuracy and limits

- A body you add starts on the grid, the plane parallel to the ecliptic through the point the camera looks at, and its launch velocity lies in that plane. When you look at the grid edge-on there is nothing to click on, so turn the view first.
- Gravity is Newtonian plus general relativity's periastron precession, which turns Mercury's orbit 43 arcseconds a century and S2's 12 arcminutes a lap. Relativity's other effects are left out, including its small change to orbital periods, the redshift of S2's light, and the dragging of space around a spinning hole.
- Offline, the planets start from mean Keplerian elements, good to a fraction of a degree around the year 2000.
- The galaxy view moves the Sun on a circle around the Galactic center. The Sun's small drifts toward the center and out of the Galactic plane stay straight lines, though they are really slow oscillations tens of millions of years long. Bodies can't be added in the galaxy view.
- With Verlet, a split step isn't symplectic, so each close pass leaves a small permanent offset in the energy drift. The planets and the Moon never need split steps. IAS15 isn't symplectic either, but its error through a close pass is far smaller.
- Merges are perfectly inelastic (the two bodies stick together), so they lower the total energy. The drift readout starts from a new baseline whenever bodies are added, merged or loaded.
- In the solar system, Sgr A* is drawn but not simulated, so it pulls on nothing. Its pull would be almost the same on every body there and would only move the system as a whole, which the galaxy view already shows. In the Sgr A* and S-stars preset it's a real body. Neither version spins, though the EHT's results suggest the real one does, and its gas is a simple model of the right shape, not a simulation. The thin disk is an illustration: Sgr A* doesn't have one.
- A black hole's spin sets only its horizon. The ray tracer draws every hole as one that doesn't spin.
- GRAVITY gives the S-stars' orbits as osculating elements near each star's apocenter: 2010.35 for S2, 1977 for S29, 2000 for S38 and 2012 for S55. EMMA starts all four from these elements in 2017.0. S38 passed pericenter in 2003, after the date of its elements, so the 7′ its orbit turned then is missing. Only S2's mass has been measured, so S29, S38 and S55 move as test particles with no mass of their own. A star's mass is a few millionths of Sgr A*'s, so leaving it out doesn't change the orbits.

## Building

EMMA needs a C++17 compiler, CMake 3.28 or newer, and Git. If SFML 3 is installed, CMake uses it. If not, CMake downloads SFML and builds it along with Dear ImGui and ImGui-SFML, so the first configure needs an internet connection and takes a few minutes. The build copies `fonts` and `textures` next to the executable, so EMMA starts from any folder. Run a Release build: a Debug build runs the physics many times slower.

### macOS

With Homebrew:

```
brew install cmake sfml
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
./build/nbody
```

### Windows

Install the Build Tools for Visual Studio and tick the "Desktop development with C++" workload. That gives you the MSVC compiler, the Windows SDK, CMake and Ninja. Then open "Developer PowerShell for VS" in the project folder:

```
cmake -B build
cmake --build build --config Release
build\Release\nbody.exe
```

In VS Code, install the C/C++ and CMake Tools extensions and open the folder. When CMake Tools asks for a kit, pick the Visual Studio one ending in amd64. Set the build variant to Release, then press F7 to build and Shift+F5 to run.

Release builds on Windows open without a console window. If the font is missing, EMMA falls back to ImGui's built-in font.

### Tests

The tests build along with EMMA. The physics tests link only the engine and don't need SFML. They check that:

- the offline start puts Earth where it was on 1 January 2000
- the offline start tilts Mercury's orbit by 7° and the Moon's by about 5°, and Mercury's stays at 7° over ten years
- Earth's orbit takes one sidereal year
- energy and angular momentum stay conserved over ten simulated years
- a close pass doesn't speed bodies up
- a head-on collision keeps mass and momentum
- IAS15 passes the same four checks, and holds energy and angular momentum to 1 part in 10¹⁰ over the ten years
- IAS15 hands over to Verlet above 64 bodies
- above 64 bodies, a close pass right after bodies are added still splits the step
- the Barnes-Hut tree matches an exact sum
- each star of Alpha Centauri rules the space around it
- trails stay smooth, and their points keep their times in order
- the ecliptic to Galactic rotation is a true rotation, sends the north Galactic pole to straight up and the north ecliptic pole to l = 96.38°, b = 29.81°, and points the Sun's motion about 61° out of the ecliptic
- Greenwich points where the IAU says at J2000, Earth's axis tilts 23.44° from the ecliptic's pole, Saturn's 28.05° and the Moon's about 1.5°, and each body's axes stay at right angles after millions of degrees of spin
- Sgr A*'s horizon is 0.0849 AU; with the same ray step the shader uses, light aimed inside 2.6 Schwarzschild radii falls in, light circles the hole at 1.5, and light passing far away bends by the amount Einstein predicted; gas at the disk's inner edge moving toward you shifts its light by √2
- a black hole's horizon is Rs without spin and 0.5316 Rs at spin 0.998, and a black hole swallows a star four times its mass, keeping its name and spin while its horizon grows
- with relativity, Mercury's perihelion turns 42.98″ a century around the Sun alone, to within 1%; without relativity it doesn't turn
- in the Sgr A* preset, S2 passes pericenter in 2018.38, 120 AU from the hole at about 7,700 km/s; with IAS15, its orbit turns by relativity's 12.2′ between its 2026 and 2042 apocenters, to within 1%. The test also prints how far Verlet's own error turns the orbit at the same step
- the calendar reads dates correctly, and the Horizons code reads tables, JSON replies and error messages
- the clock keeps the chosen speed, pauses and steps, and lands on the right moment in real time

The view tests are a separate program, `view_tests`, because the camera and the galaxy view's frame are part of the app, and the camera measures pixels with SFML's vector and rectangle types. They use only those headers and never open a window. They check that:

- a grid point turned into a pixel and picked back lands where it started, from 20 view angles, and the target lands in the middle of the view
- clipping leaves a line in front of the eye untouched, cuts one that crosses the near plane exactly at it, and drops one that's entirely behind
- zooming at the cursor keeps the grid point under it fixed, and the distance stops at its limits
- panning keeps the grabbed grid point under the cursor, and near the horizon slides by the drag at the same height
- picking finds nothing when the view ray runs along the grid, when the grid is behind the eye, and when the ray would meet it past the pick limit
- Fit view shows the requested radius on the short side of the view
- zoomed all the way out from the Sun, the Galactic center is in view, and picking still works there and zoomed in close to the Galactic center
- in the galaxy view, the Sun's path bends toward the Galactic center by about 80 AU over 10,000 years, stays R₀ from the center a quarter of the way round, and its heading matches the path
- the bent grid's clocks lose 2.12 × 10⁻⁶ on the Sun's surface, 9.87 × 10⁻⁹ at 1 AU and 6.96 × 10⁻¹⁰ on Earth, run at exactly √(1/3) of the far rate at 1.5 Rs from a black hole, and stop at its horizon; a body's well joins its inside smoothly at the surface; and seen falling at 1 AU, the Sun's slope disappears, leaving a tide that dips along the line to the Sun and rises across it

Run them from the project folder:

```
ctest --test-dir build -C Release --output-on-failure
```

## Project structure

EMMA has two parts. The engine, in `engine/`, is built as the `emma_core` library and has no graphics: it holds the physics, time, the Horizons download and the calendar, all in `namespace emma`. Its public headers are in `engine/include/emma/`, included as `"emma/Simulation.hpp"` and so on, and its source files are in `engine/src/`. The app, in `app/`, is the window, the drawing and the sidebar, built on the engine with SFML and Dear ImGui.

### Engine

| File | Role |
|------|------|
| `Simulation.hpp` / `Simulation.cpp` | Ties the parts below together for the app: loading, stepping, adding bodies and black holes, switching integrators and relativity, and queries like the dominant body, circular velocity and the predicted path |
| `World.hpp` / `World.cpp` | The bodies and where the system is in time, plus trails, kinetic energy and angular momentum |
| `Gravity.hpp` / `Gravity.cpp` | The force: exact pairwise sum or Barnes-Hut, plus general relativity's precession term, and the potential energy that matches it |
| `Integrator.hpp` | The interface every integrator implements, so `Simulation` can swap them |
| `VerletIntegrator.hpp` / `VerletIntegrator.cpp` | Velocity Verlet, split into pieces through close passes |
| `IAS15Integrator.hpp` / `IAS15Integrator.cpp` | IAS15, which picks its own piece length and aims for errors near the rounding limit |
| `Collisions.hpp` / `Collisions.cpp` | Swept contact checks and merges |
| `Presets.hpp` / `Presets.cpp` | The solar system, Alpha Centauri, and Sgr A* with the S-stars, each built as a scenario: bodies plus settings |
| `Clock.hpp` / `Clock.cpp` | Time: speed, pause, single steps, real time, and how many fixed steps each frame runs |
| `Body.hpp` / `Body.cpp` | A single body and its type (ordinary or black hole), plus the mass-radius, temperature and color relations |
| `Octree.hpp` / `Octree.cpp` | Barnes-Hut tree for systems above 64 bodies |
| `Galaxy.hpp` | The Galactic frame: the ecliptic to Galactic rotation and the Sun's velocity around the Galactic center, where Sgr A* is, and the sky as seen from Earth for orbits measured on it |
| `BlackHole.hpp` | Black holes: the horizon with and without spin, and light around a hole: the ray step the shader uses, and the Doppler and gravitational shifts |
| `Spin.hpp` / `Spin.cpp` | Where the Sun's, the planets' and the Moon's poles point and how they spin, from the IAU, so their surfaces face the right way |
| `Horizons.hpp` / `Horizons.cpp` | Today's positions from JPL Horizons: the requests, the JSON replies and their error messages, the parser and the cache |
| `Calendar.hpp` / `Calendar.cpp` | Julian days, calendar dates and local time |
| `Constants.hpp` | Units and physical constants |
| `Vector3D.hpp` | Header-only 3D vector |
| `engine/src/Http.hpp` / `Http.cpp` | HTTPS requests, with WinHTTP on Windows and the system curl elsewhere. Private to the engine |

### App

| File | Role |
|------|------|
| `app/main.cpp` | Window, event loop, input (click, drag-launch, turn, pan, zoom, follow), the drift readout and the background Horizons download |
| `app/Camera.hpp` | Orbit camera: projects AU onto pixels with perspective and picks points on the grid under the cursor, with turn, pan and zoom |
| `app/SceneFrame.hpp` | Places simulation points in the picture: as they are, or in Galactic coordinates moving with the Sun |
| `app/SpacetimeSheet.hpp` | The bent grid's math: each mass's well, how much a clock lags, and the sheet seen falling with the middle of the view |
| `app/Renderer.hpp` / `Renderer.cpp` | Starfield with the Milky Way, grid, trails, bodies, merge rings, predicted path, launch arrow and the Sun's heading, and Sgr A*: ray traced up close, a ring marker from afar |
| `app/BlackHoleShader.hpp` | The fragment shader that ray traces Sgr A* |
| `app/SurfaceShader.hpp` | The fragment shader that draws a body's real surface, with Earth's night lights and clouds, the Moon's relief and Saturn's rings |
| `app/Ui.hpp` / `Ui.cpp` | Sidebar, status bar, labels, tooltips and the color theme |

### Build and tests

| File | Role |
|------|------|
| `tests/physics_tests.cpp` | Engine tests, run through CTest |
| `tests/view_tests.cpp` | Camera and galaxy-view frame tests, run through CTest. They use SFML's vector and rectangle headers but open no window |
| `CMakeLists.txt` | Build recipe: uses an installed SFML 3 or builds one, fetches ImGui and ImGui-SFML, and registers the tests |

## Data sources

- Today's positions: NASA JPL Horizons, https://ssd.jpl.nasa.gov/horizons/, computed from the DE441 ephemeris.
- Offline starting orbits: E. M. Standish, "Keplerian Elements for Approximate Positions of the Major Planets", JPL Solar System Dynamics, table 1 (epoch J2000).
- Planet and Moon masses: IAU 2009 system of astronomical constants.
- Moon's starting position: J. Meeus, *Astronomical Algorithms*, mean elements at J2000.
- Radii and the Sun's temperature: IAU 2015 nominal values and the NASA planetary fact sheets.
- Alpha Centauri: period and eccentricity from Pourbaix et al. (2002), masses from Akeson et al. (2021), radii from Kervella et al. (2017).
- Mass-radius relation for planets and brown dwarfs: Chen & Kipping (2017), ApJ 834, 17.
- IAS15: H. Rein & D. S. Spiegel (2015), MNRAS 446, 1424.
- Galactic coordinates: the ICRS to Galactic matrix from *The Hipparcos and Tycho Catalogues* (ESA 1997), vol. 1, p. 92, and the J2000 obliquity of 84,381.448 arcseconds.
- The Sun's speed around the Galactic center, 247 km/s: M. J. Reid et al. (2019), ApJ 885, 131.
- The Sun's drift toward the Galactic center and the north Galactic pole, 11.1 and 7.25 km/s: R. Schönrich, J. Binney & W. Dehnen (2010), MNRAS 403, 1829.
- Planet and Sun textures: Solar System Scope, https://www.solarsystemscope.com/textures/, distributed under the Creative Commons Attribution 4.0 International license. They're based on NASA imagery, with colors slightly more saturated than in real photos, and unmapped areas filled with matching made-up terrain.
- Moon color and height maps: NASA's Scientific Visualization Studio, CGI Moon Kit, https://svs.gsfc.nasa.gov/4720, from Lunar Reconnaissance Orbiter camera and laser altimeter data.
- Where each body's pole points and how it spins: B. A. Archinal et al. (2018), the IAU Working Group on Cartographic Coordinates and Rotational Elements' 2015 report, Celestial Mechanics and Dynamical Astronomy 130, 22, as given with the bodies' radii in NASA NAIF's pck00011.tpc.
- Sgr A*'s mass, 4.3 million solar masses: GRAVITY Collaboration (2023), A&A 677, L10. The size of its ring and the limits on its tilt: Event Horizon Telescope Collaboration (2022), ApJL 930, L12.
- The S-stars' orbits, with Sgr A*'s mass (4.297 million M☉) and distance (8,277 pc) fitted together with them: GRAVITY Collaboration (2022), A&A 657, L12, table B.1.
- S2's mass, radius and temperature (13.6 M☉, 5.53 R☉, 28,513 K): M. Habibi et al. (2017), ApJ 847, 120, table 2.
- Sgr A*'s position on the sky: M. J. Reid & A. Brunthaler (2004), ApJ 616, 872.
- The potential that turns orbits the way general relativity does: A. M. Nobili & I. W. Roxburgh (1986), IAU Symposium 114, 105.
- The most a black hole spins up, 0.998: K. S. Thorne (1974), ApJ 191, 507.
- The thin disk's temperature profile: N. I. Shakura & R. A. Sunyaev (1973), A&A 24, 337. The idea of tracing light rays with a central force: R. Antonelli, *Starless*, https://rantonels.github.io/starless/.
