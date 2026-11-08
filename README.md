# EMMA

EMMA (Engine for Multi-body Mechanics and Astrophysics) is a real-time gravity simulator in C++. It integrates Newton's law of gravitation for every body in the system, draws the result with [SFML 3](https://www.sfml-dev.org/learn/), and puts the controls in a [Dear ImGui](https://github.com/ocornut/imgui) sidebar.

It runs in real units. Distances are in astronomical units, masses in solar masses and time in days, and the solar system starts from where the planets actually were on 1 January 2000. Earth takes a year to go around the Sun and the Moon takes 27.3 days to go around Earth. A drift readout shows how far the integrator has wandered from exact conservation of energy and angular momentum.

## What you can do

- Watch the Sun, the eight planets and the Moon move on from their J2000 positions, or switch to Alpha Centauri A and B, a real pair of stars on an 80-year orbit.
- Add a body anywhere. A click places it; pressing and dragging sets its speed and direction, and a dashed line shows where it will go before you let go.
- Tick "Circular orbit" and a click puts the new body on a circular orbit around whatever dominates gravity at that spot. That is the Sun almost everywhere, and Earth when you click close to Earth.
- Run time from a few hours per second up to ten years per second, pause, or step one frame at a time.
- Zoom from Neptune's orbit down to the Earth and Moon, and hover over any body to see its mass, radius, distance and speed.

## Controls

| Input | Action |
|-------|--------|
| Left click | Place a body with the sidebar's settings |
| Left drag | Launch a body: the arrow's length is the speed and its angle the direction |
| Esc | Cancel a launch |
| Right or middle drag | Pan |
| Scroll | Zoom toward the cursor |
| Space | Pause or resume |
| N | Step one frame |
| F | Fit the view to the current system |

Speeds are measured relative to the body that dominates gravity where you click, so a launch next to Earth moves along with Earth.

## The interface

The sidebar on the left has five sections. Simulation sets how fast time runs and holds Clear, Reset and Fit view. System switches between the solar system, Alpha Centauri and empty space. New body sets the mass, speed and direction of the next body; a swatch shows the color and class it will get, from a rocky body up to an O-type star. View turns trails and labels on and off. Stats shows the body count, the frame rate, and the relative drift in total energy and angular momentum, with a graph of the last 30 seconds.

The status bar along the bottom shows how wide the view is, the cursor position in AU, the date (or the elapsed time outside the solar system) and the size of the time step, with the number of pieces it was split into during a close pass.

## The physics

### Units

EMMA uses the units of solar system dynamics. In AU, solar masses and days, the gravitational constant is k², where k = 0.01720209895 is the Gaussian gravitational constant, so G = 2.959 × 10⁻⁴ AU³ M☉⁻¹ day⁻². The interface converts to km/s and Earth, Jupiter or solar masses for display.

### Integration

Each step uses velocity Verlet:

1. Drift: move every body by v·dt + ½·a·dt².
2. Recompute the accelerations at the new positions.
3. Kick: update every velocity with the average of the old and new accelerations.
4. Merge any bodies that touched during the step, then record trail points.

Verlet is symplectic, so the energy error oscillates around a fixed value and doesn't build up the way it does with Euler steps, which make planets slowly spiral away. A body's acceleration is computed before its first step, so new bodies start with second-order accuracy too.

The base step is 0.1 day for the solar system (about 880 steps per orbit of Mercury) and 1 day for Alpha Centauri. Before each piece of a step, EMMA works out two times for every pair of bodies: its dynamical time, √(r³ / G(m₁ + m₂)), and the time the pair needs to close the gap at its current speed. A piece can last at most 3% of the shortest of these. Away from close passes that allows the whole base step in one piece, so Verlet keeps its long-run energy behavior. During a close pass the step splits into as many as 4,096 pieces, and the status bar shows the count. A single long step through a near miss would jump past the closest point and send the pair apart faster than it arrived, which looks like the bodies repelling each other. Above 64 bodies, checking every pair would cost more than the forces themselves, so EMMA sizes the pieces from how fast each body's acceleration changed over the previous piece. The predicted path uses the same rule.

Each frame runs as many base steps as the chosen speed needs. If that would take too long, EMMA drops the backlog and shows "Running behind" instead of freezing the window.

### Forces

Up to 64 bodies, EMMA sums every pair directly, once per pair. That is exact and faster than building a tree at this size. Above 64 it switches to a Barnes-Hut quadtree with an opening angle of 0.5, which treats a distant group of bodies as a single point mass and brings the cost down to O(N log N). The tree is stored in one flat array that gets reused every step.

Gravity uses Plummer softening with a length of 10⁻⁷ AU, about 15 km. That is smaller than any body's radius, so in practice the force is plain Newtonian gravity and the softening only keeps the division finite. The energy readout uses the potential whose gradient is exactly this force.

### Collisions

Bodies merge when their physical radii touch. The check follows each pair along the path both moved during the step, so a fast body can't jump through another between two steps. A sweep along the x axis limits the exact check to pairs that could have met. A merge conserves mass and momentum, places the result at the pair's center of mass, and shows a short ring. The heavier body keeps its name and color.

### Sizes and colors

Bodies from the presets use their measured radius and color. A body you add gets its radius and color from its mass:

| Mass | Class | Radius | Color |
|------|-------|--------|-------|
| below 2 Earth masses | rocky body | R ∝ M^0.28 | grey |
| up to 0.41 Jupiter masses | Neptune-like planet | R ∝ M^0.59 | pale blue |
| up to 13 Jupiter masses | gas giant | about one Jupiter radius | tan |
| up to 0.08 solar masses | brown dwarf | about one Jupiter radius | blackbody, 800 to 2,300 K |
| 0.08 solar masses and up | main-sequence star | R ∝ M^0.8, or M^0.57 above 1 M☉ | blackbody at its surface temperature |

The planet and brown dwarf radii follow Chen & Kipping (2017). A star's surface temperature comes from the main-sequence mass-luminosity relation and the Stefan-Boltzmann law. A 0.1 M☉ red dwarf comes out near 2,700 K and orange-red, the Sun at 5,772 K and nearly white, and a 10 M☉ star near 24,000 K and blue. The sidebar and tooltips name the spectral type.

At solar-system scale a planet is much smaller than a pixel, so every body is drawn at least as large as a marker that grows with the logarithm of its mass. Zoom in far enough and the true size takes over. The background stars stay fixed while you pan, as real stars would at this distance.

## Accuracy and limits

- The simulation is 2D. Orbits sit in the ecliptic plane and their inclinations are dropped; the largest, Mercury's, is 7°.
- Gravity is Newtonian. General relativity adds 43 arcseconds per century to Mercury's perihelion precession, and EMMA leaves that out.
- The planets start from mean Keplerian elements, which are good to a fraction of a degree around the year 2000. A full ephemeris such as JPL DE440 would be more precise.
- Split steps aren't symplectic, so each close pass leaves a small permanent offset in the energy drift. The planets and the Moon run unsplit.
- Merges are perfectly inelastic, so they lower the total energy. The drift readout starts a new baseline whenever bodies are added, merged or loaded.

## Building

EMMA needs a C++17 compiler, CMake 3.28 or newer, and Git. If SFML 3 is installed, CMake uses it. If not, CMake downloads SFML and builds it along with Dear ImGui and ImGui-SFML, so the first configure needs an internet connection and takes a few minutes. The build copies `fonts` next to the executable, so EMMA starts from any folder. Use a Release build to run it: a Debug build steps the physics many times slower.

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

If the font is missing, EMMA falls back to ImGui's built-in font.

## Project structure

| File | Role |
|------|------|
| `main.cpp` | Window, event loop, input (click, drag-launch, pan, zoom), fixed-step timing and the drift readout |
| `Simulation.hpp` / `Simulation.cpp` | The bodies and the physics: Verlet steps, forces, collisions, trails, presets, path prediction, energy |
| `Body.hpp` / `Body.cpp` | A single body, plus the mass-radius, temperature and color relations |
| `QuadTree.hpp` / `QuadTree.cpp` | Barnes-Hut tree for systems above 64 bodies |
| `Camera.hpp` | Converts between AU and pixels, with zoom and pan |
| `Renderer.hpp` / `Renderer.cpp` | Starfield, trails, bodies, merge rings, predicted path and launch arrow |
| `Ui.hpp` / `Ui.cpp` | Sidebar, status bar, labels, tooltips and the color theme |
| `Constants.hpp` | Units and physical constants |
| `Vector2D.hpp` | Header-only 2D vector |
| `CMakeLists.txt` | Build recipe: uses an installed SFML 3 or builds one, and fetches ImGui and ImGui-SFML |

## Data sources

- Planet orbits: E. M. Standish, "Keplerian Elements for Approximate Positions of the Major Planets", JPL Solar System Dynamics, table 1 (epoch J2000).
- Planet and Moon masses: IAU 2009 system of astronomical constants.
- Moon's starting position: J. Meeus, *Astronomical Algorithms*, mean elements at J2000.
- Radii and the Sun's temperature: IAU 2015 nominal values and the NASA planetary fact sheets.
- Alpha Centauri: period and eccentricity from Pourbaix et al. (2002), masses from Akeson et al. (2021), radii from Kervella et al. (2017).
- Mass-radius relation for planets and brown dwarfs: Chen & Kipping (2017), ApJ 834, 17.

## Roadmap

- 3D, with inclinations and a 3D camera.
- A higher-order integrator, such as IAS15, for systems with many close passes.
- More real systems, such as TRAPPIST-1.
