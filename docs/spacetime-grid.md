# The spacetime grid

This document explains what spacetime is, why clocks running slow near a mass is what holds orbits together, and what a picture of it can and can't show. Then it covers the problems that shaped EMMA's bent grid, and how I built it. Tick Bend the grid in the View tab and the flat grid becomes a sheet with a dip under every mass. It builds on `relativity.md`, which explains what relativity is.

## 1. Spacetime: what it is and why we need it

### Spacetime

Everything that happens, happens at some place and at some moment. Relativity treats the two together. A moment at a place is called an event, and the set of all events is spacetime: three directions of space and one of time.

Special relativity showed that space and time can't be cleanly separated. Two observers moving past each other disagree about how far apart two events are in space and how far apart they are in time, though they agree on a combination of the two. General relativity added that spacetime can be curved by mass and energy, and that the curvature is gravity.

Curved spacetime has four dimensions and we're inside it, so no picture can show all of it. Every picture of it shows one slice of one part. A picture is still worth having: it turns "gravity is curvature" from a phrase into something you can see.

### Gravity slows clocks

For anything much slower than light, the part of the curvature that matters most is in time: close to a mass, time runs slower. A clock at distance r from a mass M loses a fraction

```
ψ = G M / (c² r)
```

of its time compared with a clock far away, as long as ψ is small. With several masses, their ψs add.

| Where the clock is | It loses | In a year |
|---|---|---|
| On Earth's surface, from Earth's gravity | 6.96 × 10⁻¹⁰ | 22 ms |
| At Earth's distance from the Sun, from the Sun's | 9.87 × 10⁻⁹ | 0.31 s |
| On the Sun's surface | 2.12 × 10⁻⁶ | 67 s |
| On S2 at pericenter, 120 AU from Sgr A* | 3.6 × 10⁻⁴ | 3.1 hours |
| 3 Rs from a black hole | 18% | 67 days |
| At a black hole's horizon | all of it | seen from far away, the clock stops |

Close to a black hole ψ isn't small. For one mass that doesn't spin, a clock that stays at distance r runs at exactly √(1 − Rs/r) of the rate far away, where Rs = 2GM/c² is the Schwarzschild radius. Since ψ = Rs/(2r) for one mass, that's √(1 − 2ψ). At the horizon, r = Rs, the rate is 0.

These are measured effects. In 1959 Robert Pound and Glen Rebka measured clocks running slower at the bottom of a 22.5 m tower than at the top, and GPS satellites' clocks are corrected for the difference every day (`relativity.md`).

### Why slow clocks make things fall

General relativity's rule for a body that nothing pushes or pulls is that between two events, it follows the path along which its own clock ticks the most.

A ball thrown up and caught a second later shows how that rule becomes falling. Between the throw and the catch, the ball could stay low, where clocks run slightly slow. Or it could rise, where clocks run slightly faster; but then, to go up and come back within the same second, it has to move faster, and moving clocks run slow too. The path with the most ticks balances the two: it rises to gain from the faster clocks higher up, but not so far that its speed costs more than its height gains. That balance is the arc Newton's gravity predicts.

Worked out in full, the rule gives Newton's pull exactly: c² times the slope of ψ, pointing down into the well. For slow bodies, Newton's gravity is the slope of time's curvature, and ψ, which is just how much slower clocks run, says everything about the pull. The heavier the mass and the closer you are, the steeper the slope and the stronger the pull.

Space is curved too, but for a slow body its effect is smaller by about (v/c)², 10⁻⁸ for Earth going round the Sun. It matters for light, which moves at c: starlight passing the Sun bends twice as much as Newton's gravity gives, half from time's curvature and half from space's. It also contributes to the small relativistic turn of orbits in `relativity.md`.

### The rubber sheet

The familiar picture of gravity is a stretched rubber sheet with a heavy ball sagging the middle and a marble rolling around the dip. It's based on a real result, Flamm's paraboloid (1916). Around a single mass, space itself is curved, and Flamm's paraboloid draws that curvature for the plane of an orbit: the plane is bent up into an extra dimension so that distances along the surface are the true distances.

### Falling: the equivalence principle

In 1907 Einstein realized that someone falling freely doesn't feel their own weight. Everything around them falls at the same rate, so inside a falling elevator nothing shows that gravity is there at all. He later called it the happiest thought of his life, and it became the equivalence principle: in free fall, a uniform pull can't be felt or measured.

What a falling observer can measure is how the pull changes across their surroundings. Things nearer the mass are pulled a little harder and fall a little faster, things farther away fall a little slower, and things off to either side fall along lines that converge. Together that stretches a falling cloud of objects along the line toward the mass and squeezes it across. That's a tide, the same effect that raises the ocean tides on Earth.

A far mass's tide has a recognizable shape. At a distance d from the falling point, ψ minus its level and slope there dips along the line toward the mass by ψ (d/r)², and rises across that line by half as much.

### How big the wells are

A well's depth at distance r is G M/(c² r), so it scales with the mass. Earth is 1/333,000 of the Sun's mass, and the Moon is 81 times lighter again, 1/27 million of the Sun.

Right at its surface, Earth's own well is surprisingly deep: a clock there loses 6.96 × 10⁻¹⁰ to Earth's gravity, about 7% of the 9.87 × 10⁻⁹ it loses to the Sun's. But Earth's well is that deep only next to Earth. It halves every time the distance from Earth's center doubles, so 10 Earth radii out it's down to 0.7%, and a few hundred thousand km out it's lost in the Sun's. At the scale of the planets' orbits, the Sun's well is all there is. Earth's gravity wins only close to Earth, which is why the Moon, 384,000 km out, goes round Earth while both go round the Sun.

## 2. Problems and decisions

### What to draw

Problem: the rubber sheet is the picture everyone knows, but it's the wrong one for EMMA, for four reasons:

- Flamm's paraboloid shows space's curvature, which affects slow bodies only at (v/c)². The pull that holds planets in orbit comes from time's curvature.
- It's for one mass alone. There's no such shape for ten bodies.
- It never flattens out: it keeps rising as √r forever, so there's no flat sheet far away to compare against.
- On a rubber sheet the marble rolls into the dip because Earth's gravity pulls it down, so the picture uses gravity to explain gravity.

Decision: draw ψ, the time part of the curvature, downward. For several masses it's just the sum, as long as the field is weak, which it is everywhere in EMMA except within a few Rs of a black hole. It goes to zero far from every mass. And the dip is where clocks run slow, which is why bodies move toward it. The sheet isn't a place: bodies don't sit on it or sink into it. It's a map of one number over the grid's plane.

### Following a planet

Problem: following Earth, the plain sheet would show no dip at all. At 1 AU the Sun's well slopes so steeply that across the grid around Earth it falls much farther than Earth's own dip goes down, and the whole sheet would be one tilted plane.

Decision: draw the sheet as someone falling freely at the middle of the view, the camera's target, would see it. Take away ψ's level and slope at that point:

```
dip(p) = ψ(p) − ψ(p₀) − ∇ψ(p₀) · (p − p₀)
```

p₀ is the middle of the view and ∇ψ(p₀) is ψ's slope there. What's left is the dips of nearby masses and the tides of far ones. With the view on the Sun, or on the solar system's barycenter close to it, the slope is almost zero and the sheet is plain ψ.

### Making it visible

Problem: the real dips are tiny: at solar-system scale, 10⁻⁸ of anything.

Decision: scale the sheet so its deepest dip is a fixed fraction of the camera's distance from its target, 25% by default, set by the Depth slider from 5% to 50%. The scale changes gradually from frame to frame, so the sheet doesn't jump when a mass comes into view, and a change of more than ten times is taken at once. The readout says it's not to scale.

### Where the sheet hangs

Problem: a sheet that rose above the plane would cover the bodies moving on it.

Decision: put the sheet's least deep point on the grid's plane, the plane the bodies move in, and hang everything else below it. Clicks still land on the flat plane.

### Narrow wells

Problem: with the bodies' real sizes, a planet's well would be a needle far thinner than a pixel. It would vanish between two grid lines, or spike wherever a line happened to cross it.

Decision: for drawing, every well is at least three quarters of a grid cell wide. The readout uses the real sizes.

What that means for Earth: in the default solar-system view a cell is 1 AU, so Earth's dip is drawn 0.75 AU wide instead of 6,371 km, about 12,000 times shallower. The Sun gets the same treatment, so between the two only the mass ratio is left: Earth's dip is drawn 3 millionths as deep as the Sun's. With the Sun's well about 1.1 AU deep, that's about 500 km, a thousandth of a pixel; the Moon's is 81 times less, and Jupiter's about a quarter of a pixel. That's the true proportion between them. To see Earth's dip, follow Earth and zoom in until the Moon's orbit fills the view, about 800,000 km across: the cells are then about 300,000 km, Earth's dip fills the middle, and the Sun's tide bends the edges into a saddle. The Moon's own dip shows when you follow it and zoom in to about 10,000 km across, where Earth is outside the grid.

### Inside a body

Problem: G M/(c² r) goes to infinity at the center of a point mass.

Decision: inside a body, use the well of a ball of even density instead. It meets the outside value and slope at the surface, and is 1.5 times the surface value at the center. A black hole's ball is its horizon; inside it ψ passes 1/2, and the readout says a clock there stops.

### Which masses

Problem: summing every body's well at every point of the grid, every frame, gets slow with many bodies, and light bodies barely dent the sheet.

Decision: the 32 heaviest bodies, plus Sgr A* at the Galactic center in the solar system. Each line is cut into four pieces per cell so it can curve, which makes 13,202 points a frame, each summing up to 33 wells.

### The clock readout

Problem: the sheet isn't to scale, so the real numbers need to be somewhere.

Decision: a line under the Depth slider says how much slower a clock at the middle of the view runs, as a fraction and as time lost in a year. It uses √(1 − 2ψ), which is exact for one mass that doesn't spin, so it's exact next to a lone black hole.

## 3. How I built it

### One well

`sheet::well` in `app/SpacetimeSheet.hpp` is one mass's ψ at distance r, with s the ball's radius:

```cpp
inline double well(double mass, double r, double s) {
    if (r >= s && r > 0.0) return GM_PER_C2 * mass / r;
    if (s <= 0.0) return 0.0;
    return GM_PER_C2 * mass * (3.0 * s * s - r * r) / (2.0 * s * s * s);
}
```

`GM_PER_C2` is G/c² in AU per solar mass, about 1.48 km. `sheet::depthAt` adds every mass's well, with each ball at least as wide as the smoothing:

```cpp
for (const Mass& m : masses) psi += well(m.mass, (p - m.at).magnitude(), std::max(m.radius, smoothing));
```

### The falling view

```cpp
inline double dip(const std::vector<Mass>& masses, const FallingFrame& frame, const emma::Vector3D& p,
                  double smoothing = 0.0) {
    return depthAt(masses, p, smoothing) - frame.level - frame.slope.dot(p - frame.at);
}
```

`fallingAt` fills a `FallingFrame` with ψ's level and slope at the target, and `dip` subtracts them at each point.

### Drawing the sheet

`Renderer::drawGrid` works out every point's dip, finds the shallowest and the deepest, and sets the scale:

```cpp
const double range = high - low;
const double want = range > 0.0 ? sheetDepth * camera.distance / range : 0.0;
if (sheetScale <= 0.0 || want <= 0.0 || want > 10.0 * sheetScale || want < 0.1 * sheetScale)
    sheetScale = want;
else
    sheetScale += SHEET_EASE * (want - sheetScale);
```

Each frame the scale moves 15% of the way toward the value it wants, unless the change is more than ten times, which it takes at once. Then each point hangs below the plane:

```cpp
const std::size_t index = static_cast<std::size_t>((f * count + k) * points + j);
if (bend) p.z -= sheetScale * (sheetDips[index] - top);
```

`top` is the shallowest dip on the grid.

### The readout

`drawGrid` returns ψ at the middle of the view, with the bodies' real sizes, and `sheet::clockSlowdown` turns it into how much a clock there lags:

```cpp
inline double clockSlowdown(double psi) {
    const double rate = 1.0 - 2.0 * psi;
    return rate > 0.0 ? 1.0 - std::sqrt(rate) : 1.0;
}
```

The Depth slider's label, such as "25%", is written as `"25%%"`. ImGui reads a slider's label as a number format, the way `printf` does, so a plain `%` would start a format code; `%%` prints one percent sign. The slider also uses `ImGuiSliderFlags_NoRoundToFormat`, so ImGui keeps the value as it is instead of rounding it to the label's digits.

### Code map

| File | What's there |
|---|---|
| `app/SpacetimeSheet.hpp` | `sheet::Mass`, `well`, `wellSlope`, `depthAt`, `slopeAt`, `clockSlowdown`, `FallingFrame`, `fallingAt`, `dip` |
| `app/Renderer.hpp` / `Renderer.cpp` | `drawGrid` takes the masses and a depth, samples the dip along each line in four pieces per cell, and returns ψ at the target |
| `app/Ui.hpp` / `Ui.cpp` | Bend the grid and Depth in the View tab, and the clock readout |
| `app/main.cpp` | gathers the heaviest bodies, and Sgr A* in the solar system, for the sheet |
| `tests/view_tests.cpp`, `CMakeLists.txt` | the three tests below |

### Tests

The sheet's math is in a header of its own so `view_tests` can check it without opening a window.

1. `sheet_clocks`: a clock on the Sun's surface loses 2.1225 × 10⁻⁶, at 1 AU 9.8706 × 10⁻⁹, and on Earth's surface 6.961 × 10⁻¹⁰. At 1.5 Rs from a black hole it runs at exactly √(1/3) of the far rate. At the horizon and inside it, it stops.
2. `sheet_ball`: inside a body the well meets the outside with the same value and slope at the surface, and is 1.5 G M/(c² R) at the center. Its slope is its gradient, inside and out.
3. `sheet_falling`: falling at 1 AU from the Sun, the sheet is zero at the falling point. 0.005 AU away it dips along the line to the Sun by ψ (d/r)² and rises across it by half that, and the tide left over is under 1% of the slope it replaces.

## Things to try

1. In the solar system, tick Bend the grid and press T for the top view, then turn the view a little. The Sun's well is the only one you'll see at this scale. Follow Jupiter and zoom in to see its own.
2. Double-click Earth to follow it and zoom in until the Moon's orbit fills the view. The Sun's pull is gone, because you're falling with Earth. Earth's dip is in the middle, and toward the grid's edges the sheet takes the saddle shape of the Sun's tide: down toward the Sun and away from it, up to either side.
3. Load Sgr A* and the S-stars and tick Bend the grid. The middle of the view is Sgr A* itself, so the readout says a clock there stops. Double-click S2 to follow it, and the readout becomes S2's clock: it swings from 2.2 × 10⁻⁵ slow at apocenter to 3.6 × 10⁻⁴ at pericenter. That slowing is about half of the 200 km/s redshift GRAVITY measured in S2's light at its 2018 pericenter; the other half comes from S2's speed.
4. In empty space, add two black holes and launch stars between them. Their wells merge into one sheet, with a saddle between them where their pulls cancel.

## Glossary

### Spacetime and gravity

| Term | Meaning |
|---|---|
| Event | A place at a moment: a point in spacetime |
| Spacetime | Space and time together, three directions of space and one of time |
| ψ | G M/(c² r), summed over the masses: the fraction of its time a clock loses compared with one far away |
| Gravitational time dilation | Clocks running slower closer to a mass |
| Flamm's paraboloid | The shape of space's curvature around one mass, drawn as a surface: the usual rubber sheet |
| Equivalence principle | In free fall, a uniform pull can't be felt or measured |
| Tide | How a pull changes across a region: it stretches things along the line to the mass and squeezes them across |
| Weak field | Where ψ is small, so Newton's gravity is almost right and the ψs of several masses simply add |

### Drawing

| Term | Meaning |
|---|---|
| Dip | How much deeper than the middle of the view the sheet is at a point, once the uniform pull is taken away |
| Smoothing | Drawing each well at least three quarters of a grid cell wide, so lines can't miss it |
| Uniform ball | A body of the same density throughout: its well rounds off inside it instead of going to infinity |

### C++

- `namespace sheet { ... }` groups the sheet's functions under one name, so you call them as `sheet::well` and `sheet::dip`, and they can't clash with others.
- `inline` functions in a header can be included by several `.cpp` files, here the renderer and the tests, without "defined twice" errors from the linker.
- A default argument, `double smoothing = 0.0`, lets a caller leave the last argument out. The readout calls `depthAt(masses, t)` with the real sizes; the drawing passes a smoothing.
- `std::max(m.radius, smoothing)` picks the larger of two values: the body's radius, or the smoothing if that's wider.
- A `struct` like `FallingFrame` bundles values that belong together, so `dip` takes one argument instead of three.

## Sources

- A. Einstein (1907), Jahrbuch der Radioaktivität und Elektronik 4, 411: the equivalence principle.
- L. Flamm (1916), Physikalische Zeitschrift 17, 448: the paraboloid that shows space's curvature around one mass, the usual rubber-sheet shape.
- K. Schwarzschild (1916), Sitzungsberichte der Königlich Preußischen Akademie der Wissenschaften, 189: the metric whose time part gives a clock's rate, √(1 − Rs/r).
- R. V. Pound & G. A. Rebka (1960), Physical Review Letters 4, 337: clocks running slower lower down, measured in a 22.5 m tower.
- N. Ashby (2003), Living Reviews in Relativity 6, 1: relativity in GPS.
