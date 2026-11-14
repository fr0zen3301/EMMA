# EMMA

EMMA (Engine for Multi-body Mechanics and Astrophysics) is a real-time gravity simulator written in C++. Every body pulls on every other body through Newton's law of gravity, and EMMA works out where they all go, frame by frame. It draws the result with [SFML 3](https://www.sfml-dev.org/learn/) and puts the controls in a [Dear ImGui](https://github.com/ocornut/imgui) sidebar.

Everything runs in real units: distances in astronomical units (AU), masses in solar masses and time in days. The solar system starts from the planets' real positions today, downloaded from NASA JPL's Horizons service, so Earth takes a year to go around the Sun and the Moon takes 27.3 days to go around Earth. A drift readout shows how well the simulation is holding up. In a perfect simulation, total energy and angular momentum never change, and the readout shows how far they have moved.

## What you can do

- Watch the Sun, the eight planets and the Moon carry on from where they are today, or switch to Alpha Centauri A and B, a real pair of stars on an 80-year orbit.
- Add a body anywhere. A click places it. Pressing and dragging sets its speed and direction, and a dashed line shows where it will go before you let go.
- Tick "Circular orbit" and a click puts the new body on a circular orbit around whatever dominates gravity at that spot. That is the Sun almost everywhere, and Earth when you click close to Earth.
- Run in real time (one second per second, in step with your computer's clock) or at any speed from a few hours per second up to ten years per second. You can also pause, or step one frame at a time.
- Pick the integrator: velocity Verlet, which is fast and keeps energy steady over the long run, or IAS15, which aims for errors near the rounding limit of a double.
- Zoom from Neptune's orbit down to the Earth and Moon, turn the view to see how the orbits tilt, follow any planet, and hover over any body to see its mass, radius, distance and speed.
- Switch on the galaxy view to watch the planets trace stretched helices as the solar system moves through the Milky Way at 247 km/s, or slow the Sun down until their paths start to loop.

## Controls

| Input | Action |
|-------|--------|
| Left click | Place a body with the sidebar's settings |
| Left drag | Launch a body: the arrow's length is the speed and its angle the direction |
| Esc | Cancel a launch |
| Right drag | Turn the view around the point it looks at |
| Shift + right drag, or middle drag | Pan |
| Scroll | Zoom toward the cursor |
| Space | Pause or resume |
| N | Step one frame |
| F | Fit the view to the current system |
| T | Look straight down on the ecliptic |

Speeds are measured relative to the body that dominates gravity where you click, so a launch next to Earth moves along with Earth.

## The interface

The sidebar on the left has five sections:

- Simulation sets how fast time runs, has a Real time box and picks the integrator, velocity Verlet or IAS15. The solar system starts in real time, and Verlet is the default. Switching integrators starts the drift readout from a new baseline. This section also holds the Clear, Reset and Fit view buttons.
- System switches between the solar system, Alpha Centauri and empty space, and says where the solar system's starting positions came from.
- New body sets the mass, speed and direction of the next body you add. A swatch shows the color and class it will get, anywhere from a rocky body up to an O-type star.
- View turns trails, labels and the grid on and off, has a Top view button, and picks a named body for the camera to follow. For the solar system it also has the galaxy view, with a slider for the Sun's speed and how far the Sun has moved.
- Stats shows the body count, the frame rate, and how far total energy and angular momentum have drifted, with a graph of the last 30 seconds.

The status bar along the bottom shows how wide the view is at the point the camera looks at, the cursor's position on the grid in AU, the date and the size of the time step. In real time the date includes your local time of day; outside the solar system it shows the time elapsed instead. When a step is split into pieces, the status bar also shows how many.

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

### The galaxy view

The simulation always runs around the solar system's barycenter, which stays at rest. The galaxy view changes only the picture. It turns everything from ecliptic coordinates into Galactic coordinates, where the grid is the plane of the Milky Way, and slides each point along the Sun's path by the Sun's velocity times the moment the point was recorded. That is why every trail point carries a time. Newton's laws don't change when the whole system moves at a steady speed, so this is the same motion seen from the Galaxy's frame.

The Sun moves at 247 km/s around the Galactic center, plus a small drift toward the center and out of the plane, about 52 AU a year. Its path points about 61° out of the plane of the planets' orbits, so at the real speed every planet traces a long, stretched helix. A path only loops when the planet's own speed is larger than the part of the Sun's velocity that lies in its orbital plane, about half of it. The slider lets you lower the Sun's speed to see that.

### Collisions

Bodies merge when their surfaces touch. The check follows each pair along the path both moved during the step, so a fast body can't jump through another between two steps. To keep this quick, EMMA first sorts the bodies along the x axis and only runs the exact check on pairs whose paths overlap there. A merge keeps the total mass and momentum, places the new body at the pair's center of mass, and shows a short ring. The heavier body keeps its name and color.

### Sizes and colors

Bodies from the presets use their measured radius and color. A body you add gets its radius and color from its mass:

| Mass | Class | Radius | Color |
|------|-------|--------|-------|
| below 2 Earth masses | rocky body | R ∝ M^0.28 | grey |
| up to 0.41 Jupiter masses | Neptune-like planet | R ∝ M^0.59 | pale blue |
| up to 13 Jupiter masses | gas giant | about one Jupiter radius | tan |
| up to 0.08 solar masses | brown dwarf | about one Jupiter radius | blackbody, 800 to 2,300 K |
| 0.08 solar masses and up | main-sequence star | R ∝ M^0.8, or M^0.57 above 1 M☉ | blackbody at its surface temperature |

The planet and brown dwarf radii follow Chen & Kipping (2017). A star's surface temperature comes from the main-sequence mass-luminosity relation and the Stefan-Boltzmann law, and its color is that of a blackbody (an ideal glowing object) at that temperature. A 0.1 M☉ red dwarf comes out near 2,700 K and orange-red, the Sun at 5,772 K and nearly white, and a 10 M☉ star near 24,000 K and blue. The sidebar and tooltips name the spectral type.

At solar-system scale a planet is much smaller than a pixel, so every body is drawn at least as large as a marker that grows with the logarithm of its mass. Zoom in far enough and the true size takes over. The background stars turn with the view but stay put while you pan and zoom, as real stars would at their distance.

## Accuracy and limits

- A body you add starts on the grid, the plane parallel to the ecliptic through the point the camera looks at, and its launch velocity lies in that plane. When you look at the grid edge-on there is nothing to click on, so turn the view first.
- Gravity is Newtonian. General relativity makes Mercury's orbit turn an extra 43 arcseconds per century (its perihelion precession), and EMMA leaves that out.
- Offline, the planets start from mean Keplerian elements, good to a fraction of a degree around the year 2000.
- The galaxy view moves the Sun in a straight line. Its real path curves around the Galactic center once every 200 million years or so, which over 10,000 simulated years bends it by about 80 AU out of more than 500,000. Bodies can't be added in the galaxy view.
- With Verlet, a split step isn't symplectic, so each close pass leaves a small permanent offset in the energy drift. The planets and the Moon never need split steps. IAS15 isn't symplectic either, but its error through a close pass is far smaller.
- Merges are perfectly inelastic (the two bodies stick together), so they lower the total energy. The drift readout starts from a new baseline whenever bodies are added, merged or loaded.

## Building

EMMA needs a C++17 compiler, CMake 3.28 or newer, and Git. If SFML 3 is installed, CMake uses it. If not, CMake downloads SFML and builds it along with Dear ImGui and ImGui-SFML, so the first configure needs an internet connection and takes a few minutes. The build copies `fonts` next to the executable, so EMMA starts from any folder. Run a Release build: a Debug build runs the physics many times slower.

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

The tests build along with EMMA and don't need SFML. They check that:

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
- the calendar reads dates correctly, and the Horizons code reads tables, JSON replies and error messages
- the clock keeps the chosen speed, pauses and steps, and lands on the right moment in real time

Run them from the project folder:

```
ctest --test-dir build -C Release --output-on-failure
```

## Project structure

EMMA has two parts. The engine, in `engine/`, is built as the `emma_core` library and has no graphics: it holds the physics, time, the Horizons download and the calendar, all in `namespace emma`. Its public headers are in `engine/include/emma/`, included as `"emma/Simulation.hpp"` and so on, and its source files are in `engine/src/`. The app, in `app/`, is the window, the drawing and the sidebar, built on the engine with SFML and Dear ImGui.

### Engine

| File | Role |
|------|------|
| `Simulation.hpp` / `Simulation.cpp` | Ties the parts below together for the app: loading, stepping, adding bodies, switching integrators, and queries like the dominant body, circular velocity and the predicted path |
| `World.hpp` / `World.cpp` | The bodies and where the system is in time, plus trails, kinetic energy and angular momentum |
| `Gravity.hpp` / `Gravity.cpp` | The force: exact pairwise sum or Barnes-Hut, and the potential energy that matches it |
| `Integrator.hpp` | The interface every integrator implements, so `Simulation` can swap them |
| `VerletIntegrator.hpp` / `VerletIntegrator.cpp` | Velocity Verlet, split into pieces through close passes |
| `IAS15Integrator.hpp` / `IAS15Integrator.cpp` | IAS15, which picks its own piece length and aims for errors near the rounding limit |
| `Collisions.hpp` / `Collisions.cpp` | Swept contact checks and merges |
| `Presets.hpp` / `Presets.cpp` | The solar system and Alpha Centauri, each built as a scenario: bodies plus settings |
| `Clock.hpp` / `Clock.cpp` | Time: speed, pause, single steps, real time, and how many fixed steps each frame runs |
| `Body.hpp` / `Body.cpp` | A single body, plus the mass-radius, temperature and color relations |
| `Octree.hpp` / `Octree.cpp` | Barnes-Hut tree for systems above 64 bodies |
| `Galaxy.hpp` | The Galactic frame: the ecliptic to Galactic rotation and the Sun's velocity around the Galactic center |
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
| `app/Renderer.hpp` / `Renderer.cpp` | Starfield with the Milky Way, grid, trails, bodies, merge rings, predicted path, launch arrow and the Sun's heading |
| `app/Ui.hpp` / `Ui.cpp` | Sidebar, status bar, labels, tooltips and the color theme |

### Build and tests

| File | Role |
|------|------|
| `tests/physics_tests.cpp` | Engine tests, run through CTest |
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
