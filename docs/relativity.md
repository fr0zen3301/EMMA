# General relativity, black holes as bodies, and the S-stars

This document explains what relativity is and why it makes orbits turn, what a black hole needs to be a body in a simulation, and how astronomers measure the stars that orbit Sgr A*. Then it covers the problems that shaped how EMMA adds all three, and how I built them. `black-hole.md` explains what a black hole is and how EMMA ray traces one; this document builds on it.

## 1. Relativity, black holes and the S-stars: what they are and why we need them

### What relativity is

Relativity is Albert Einstein's theory of space, time and gravity. He published it in two parts, ten years apart.

Special relativity (1905). Light in empty space always travels at 299,792.458 km/s, whoever measures it and however fast they're moving. Einstein took that as a law of nature and worked out what follows from it. If everyone measures the same speed for light, they can't all measure the same times and distances. A clock moving past you runs slow by your measure, and the faster it moves, the slower it runs. At everyday speeds the effect is far too small to notice. Near Sgr A* it isn't: S2, a star that passes the hole at 7,700 km/s, moves at 2.6% of the speed of light, and at that speed its clock runs slow by 3 parts in 10,000.

General relativity (1915). Special relativity leaves out gravity, and Einstein spent the next ten years adding it. In Newton's picture, gravity is a force: the Sun reaches across empty space and pulls on Earth. In Einstein's, mass and energy curve space and time, which together are called spacetime, and a body that nothing pushes or pulls moves along the straightest path it can through that curved spacetime. Earth goes round the Sun because near the Sun, that's the straightest path there is.

For anything moving much slower than light, the curvature that matters is in time: clocks run slower close to a mass. A clock at distance r from a mass M loses about this fraction of its time, compared with a clock far away:

```
ψ = G M / (c² r)
```

G is the gravitational constant and c the speed of light. ψ (the Greek letter psi) also measures how strong gravity is. Where it's tiny, Newton's gravity is almost exactly right, and relativity adds only small corrections. As it approaches 1/2, Newton's gravity fails completely.

| Where | ψ |
|---|---|
| On Earth's surface, from Earth's gravity | 7 × 10⁻¹⁰ |
| At Earth's orbit, from the Sun's | 1 × 10⁻⁸ |
| At Mercury's orbit | 2.5 × 10⁻⁸ |
| On the Sun's surface | 2 × 10⁻⁶ |
| S2 at its closest to Sgr A* | 3.6 × 10⁻⁴ |
| On a neutron star's surface | about 0.2 |
| At a black hole's event horizon | 0.5 |

`spacetime-grid.md` draws ψ as a sheet.

The effect on clocks shows up in everyday technology. GPS satellites orbit 20,000 km up, where gravity is weaker, so their clocks gain about 45 microseconds a day on clocks on the ground. Their speed slows them by about 7, so they gain about 38 microseconds a day in all, and the system corrects for it. Uncorrected, GPS positions would drift by kilometers a day.

General relativity has passed every test it has been put to. It explained Mercury's orbit as soon as it was finished. It predicted that starlight passing the edge of the Sun bends by 1.75″, twice what Newton's gravity gives, and the eclipse expeditions of 1919 measured it. In 1959 Robert Pound and Glen Rebka sent gamma rays up and down a 22.5 m tower at Harvard and measured the tiny shift in their frequency that slower clocks at the bottom predict. Every test since, GRAVITY's measurements of S2 included, has agreed with it.

### What Newton couldn't explain about Mercury

Under Newton's gravity, one planet alone around the Sun traces the same ellipse forever. With several planets, each one's pull slowly disturbs the others, and one result is that every planet's ellipse turns in its own plane. The point where the planet comes closest to the Sun, the perihelion, creeps forward around the Sun. Around any body that point is called the pericenter, and around a star the periastron.

In 1859 Urbain Le Verrier added up the other planets' pulls on Mercury and found that its perihelion moved a little faster than they could account for. The modern figures are about 575″ a century in all, of which the planets explain about 532″. Astronomers looked for an unseen planet between Mercury and the Sun to make up the rest, and named it Vulcan. It doesn't exist. In November 1915 Einstein worked out what his new theory did to Mercury's orbit and got the missing 43″ a century.

### Why relativity turns orbits

Close to a mass, relativity's gravity is a little stronger than Newton's, and the extra grows faster than Newton's pull as the distance shrinks. A planet swinging through its perihelion gets slightly more pull than Newton's gravity would give it, so it curves round a little more than half a turn before heading back out. Its next perihelion comes slightly further round than the last. Lap after lap, the whole ellipse turns, in the same direction the planet goes.

Each lap it turns by

```
Δω = 6π G M / (c² a (1 − e²))
```

M is both masses together. a is the semi-major axis, half the ellipse's longest width. e is the eccentricity: 0 for a circle, close to 1 for a long, thin ellipse. a (1 − e²) is the semi-latus rectum, the orbit's distance from the Sun a quarter turn after perihelion. With ψ measured there, the turn is 6π ψ, about 19 ψ, each lap, so orbits that pass close to a lot of mass turn fastest.

| Orbit | Turn per lap | Turn per century |
|---|---|---|
| Mercury around the Sun | 0.1035″ | 43.0″ |
| Venus | 0.053″ | 8.6″ |
| Earth | 0.038″ | 3.8″ |
| S2 around Sgr A* | 12.2′ | 76′ |
| S29 | 13.8′ | 15′ |
| S38 | 6.9′ | 35′ |
| S55 | 6.7′ | 55′ |

″ is an arcsecond, 1/3600 of a degree, and ′ an arcminute, 1/60 of a degree. S2's turn is 7,000 times Mercury's per lap. Stronger fields turn orbits faster still: the binary pulsar PSR B1913+16, two neutron stars on an 8-hour orbit, turns 4.2° a year. The turn doesn't depend on how the mass is shared: two equal stars turn by the same formula, with M their total mass.

### S2 and GRAVITY

The S-stars are a few dozen stars within about 0.04 parsecs (8,000 AU) of Sgr A*. S2 is the brightest. It goes round in 16 years on an orbit with an eccentricity of 0.88, and at its closest it passes 120 AU from the hole at 7,700 km/s. Its ψ there is more than 10,000 times Mercury's, which makes its orbit a test of relativity in a much stronger field than the solar system has.

GRAVITY is an instrument at ESO's Very Large Telescope in Chile. It combines the light of the four 8.2 m telescopes as if they were parts of one telescope 130 m across, and it measures positions near Sgr A* to 30 to 100 microarcseconds. A microarcsecond is a millionth of an arcsecond: a 2 mm grain of sand on the Moon, seen from Earth.

With it, the GRAVITY team measured three things as S2 swung past the hole:

- In 2018, at S2's pericenter, its light arrived redder than Newton's gravity allows, because S2's clock slows as it dips into Sgr A*'s gravity and as it speeds up.
- In 2020, the turn of S2's orbit, to 5σ: a fluke that large would happen less than once in a million tries.
- In 2022, adding three more stars that passed their pericenters between 2018 and 2023, S29, S38 and S55, the turn to 7σ.

GRAVITY tests relativity by multiplying its terms by a factor f_SP in their orbit fits and letting the fit choose f_SP along with everything else: 0 would mean Newton was right, 1 that Einstein was. Their four-star fit gives f_SP = 0.997 ± 0.144.

### What a black hole has

According to relativity, a black hole is completely described by its mass, its spin and its electric charge, and real black holes have almost no charge. Nothing else about what fell in survives.

Roy Kerr found the description of a spinning black hole in 1963. Its spin is measured by a* = cJ/(GM²), where J is its angular momentum. a* runs from 0, for a hole that doesn't spin (a Schwarzschild hole), to just under 1. Spin shrinks the event horizon:

```
r₊ = GM/c² (1 + √(1 − a*²))
```

At a* = 0 that's 2GM/c², the Schwarzschild radius Rs. At a* = 0.998 it's 0.53 Rs.

Gas spiraling in from a disk spins a hole up, but the hole also swallows some of the disk's light, which carries angular momentum against the spin. Kip Thorne worked out in 1974 that the two balance at a* = 0.998, the fastest a real hole is expected to spin.

Black holes are called stellar-mass under about 100 M☉, supermassive from about 10⁵ M☉, and intermediate-mass between. The lines are conventions.

### Orbits measured on the sky

Telescopes see an S-star's orbit projected on the sky, and astronomers describe it with six numbers, the orbital elements:

| | a (″) | e | i (°) | Ω (°) | ω (°) | Pericenter |
|---|---|---|---|---|---|---|
| S2 | 0.12495 | 0.88441 | 134.70 | 228.19 | 66.25 | 2018.3789 |
| S29 | 0.3975 | 0.9693 | 144.37 | 7.00 | 205.79 | 2021.4104 |
| S38 | 0.14254 | 0.8145 | 166.65 | 109.45 | 27.17 | 2022.7044 |
| S55 | 0.10440 | 0.7267 | 158.52 | 314.94 | 322.78 | 2021.6940 |

a and e give the ellipse's size and shape. The three angles turn it into place: i is its tilt to the sky, Ω where it crosses the sky's plane, measured from north through east, and ω where its pericenter lies within its own plane. The last column is when the star was closest to the hole.

The semi-major axis comes as an angle on the sky: S2's is 0.12495″. Turning that into a distance needs the distance to the Galactic center. 1″ seen from 1 pc away is 1 AU, by the definition of the parsec, so at 8,277 pc S2's semi-major axis is 0.12495 × 8,277 = 1,034 AU.

Kepler's third law then gives the mass from the orbit's size and period: M = a³/P², in solar masses, AU and years. For S2 that's 1,034³ / 16.05², about 4.3 million M☉. The mass grows as the distance cubed, so a fit gives the mass and the distance together.

The elements describe each orbit in three directions at Sgr A*: north on the sky, east on the sky, and the line of sight. A star at angle u = ω + ν past its node, where ν is its angle past pericenter, at distance r, is at

```
north = r (cos Ω cos u − sin Ω sin u cos i)
east  = r (sin Ω cos u + cos Ω sin u cos i)
z     = r sin u sin i
```

With i under 90°, a star moves counterclockwise on the sky, from north toward east. All four have i above 90°, so all four go clockwise.

## 2. Problems and decisions

### Adding relativity's turn to Newton's gravity

Problem: EMMA's gravity is Newton's. Relativity's first correction to it, the first post-Newtonian or 1PN correction, depends on how fast each body moves as well as where it is. Velocity Verlet works out each new velocity from the force at the new position; if the force also depends on the new velocity, every step becomes an equation to solve, and the simple way of solving it makes Verlet stop being symplectic, so its energy error would grow. And the energy readout would need relativity's own version of the energy, with the velocities in it.

Decision: keep Newton's gravity and add one extra potential energy between every pair of bodies, which depends only on positions:

```
U = −3 G² m₁ m₂ (m₁ + m₂) / (c² r²)
```

It pulls each body toward the other. On body 1 the extra pull is

```
a₁ = 6 G² m₂ (m₁ + m₂) / (c² r³)
```

Newton's potential falls off as 1/r and this one as 1/r², so it matters only close to a mass, which is where relativity matters too. Because it depends only on positions, Verlet stays symplectic, and the energy readout adds it to Newton's potential and stays exact. Anna Nobili and Ian Roxburgh found this potential in 1986.

It gives exactly relativity's turn. Take a light body orbiting a heavy one. Per unit of the light body's mass, the extra potential is −β/r², with β = 3 (GM)²/c². The shape of an orbit follows Binet's equation, written in u = 1/r against the angle around the orbit. With Newton's gravity alone it's u″ + u = GM/h², where h is the orbit's angular momentum per unit mass, and its solutions are ellipses that close after exactly one turn. Adding −β/r² changes it to

```
u″ + u (1 − 2β/h²) = GM / h²
```

The solutions are still ellipses, but they close after an angle of 2π/√(1 − 2β/h²) instead of 2π. To first order the extra is 2π β/h². With h² = GM a (1 − e²) and β = 3 (GM)²/c², that's 6π GM / (c² a (1 − e²)): relativity's turn. For two bodies of any masses, the same works for their relative orbit with M = m₁ + m₂, which is why the potential has m₁ m₂ (m₁ + m₂) in it. The same equation is how EMMA's ray tracer turns a pull into the path of a light ray (`black-hole.md`).

### A turn too small to see

Problem: the real turn is too small to see on screen: S2's 12′ a lap is a fifth of a degree.

Decision: an Exaggerate slider in the Physics tab that multiplies the extra potential by up to 100, and the turn with it. At 100, S2's orbit turns 20° a lap and draws a rosette within a few laps. At 1 it's general relativity, and unticking General relativity sets it to 0, Newton's gravity alone. It's GRAVITY's f_SP used the other way round: GRAVITY measures the factor, and EMMA sets it. Changing it changes the potential energy, so the drift readout starts from a new baseline.

### Many bodies

Problem: above 64 bodies, Newton's pull comes from the Barnes-Hut tree, and adding the extra pull for every pair would bring back the cost of checking every pair.

Decision: the extra pull is a short-range effect of heavy bodies, so above 64 bodies EMMA adds it only for pairs that include one of the 16 heaviest bodies. Ties go to the body earlier in the list. The set changes only when bodies are added or merged, which starts a new drift baseline anyway.

### Keeping momentum exact

Problem: Newton's third law, equal and opposite pulls, is what keeps total momentum constant. The extra pull has to keep it too.

Decision: both bodies of a pair use the same strength for the extra pull, as for Newton's. It uses the same softened distance as Newton's pull, √(r² + ε²) with ε = 15 km, which never matters in practice, since bodies merge at their surfaces long before.

### What a black hole needs as a body

Problem: a black hole has to pull like any body, merge like one, and be drawn as a hole.

Decision: a body type of its own, with a mass, a spin and the direction it spins around. Its radius is its event horizon, worked out from its mass and spin. The spin is held at 0.998 at most, Thorne's limit. Adding one: switch the Add tab to Black hole, and the mass slider runs from 1 to 10¹⁰ M☉ with a Spin slider beside it. A new hole spins around the grid's pole, the axis of the orbits launched on the grid. Tooltips use the stellar-mass, intermediate-mass and supermassive classes.

### What a merge leaves

Problem: when a star touches a black hole's horizon, a plain merge would keep the heavier body's name and type, and a 20 M☉ star swallowing a 5 M☉ hole would turn the hole into a star.

Decision: when anything touches a black hole's horizon, the hole survives, whatever the mass of what fell in. A 5 M☉ hole that meets a 20 M☉ star is a 25 M☉ hole afterward, with the hole's name and spin. Mass and momentum are conserved, as in every merge, and the horizon grows with the new mass.

### Drawing more than one hole

Problem: EMMA's ray tracer draws the whole view, the sky included, so only one hole can be ray traced at a time.

Decision: each frame, the hole whose shadow looks biggest is ray traced, and the others are drawn as markers.

### Where the S-stars come from

Problem: the S-stars' orbits, Sgr A*'s mass and its distance all come from fits, and different papers give different values.

Decision: GRAVITY Collaboration (2022), A&A 657, L12, table B.1, their fit of four stars' orbits together with Sgr A*'s mass and distance: 4.297 million M☉ at 8,277 pc. Since the mass grows as the distance cubed, the two have to be used as a pair. The solar system's view of Sgr A* uses `Galaxy.hpp`'s own values: 8.15 kpc, from Reid et al. (2019), and 4.3 million M☉. Each system stays consistent with its own sources.

### From the sky to EMMA's frame

Problem: the elements are measured in sky directions, north, east and the line of sight, but EMMA's bodies live in the ecliptic frame.

Decision: build the three sky directions at Sgr A*'s position on the sky, 17h 45m 40.0409s, −29° 00′ 28.118″ (Reid & Brunthaler 2004), in ecliptic coordinates, and place each star with the same formulas EMMA uses for the planets, using north, east and the line of sight as the three axes. The line of sight points away from Earth. (Flipping that sign would mirror each orbit through the plane of the sky. The picture from Earth would stay the same; only which side of that plane each star is on, and the sign of its speed toward or away from us, would change.)

### When to start

Problem: GRAVITY's elements are osculating elements: the ellipse each star would follow from a given moment if relativity were switched off. To place the stars, the preset needs one starting date.

Decision: start all four on 2017.0, a year and a half before S2's pericenter. Each star's mean anomaly then, how far through its period it is since pericenter, as an angle, is 2π (2017.0 − t_peri) / P, with the period P from Kepler's third law and GRAVITY's mass.

### Stars without a measured mass

Problem: only S2 has a measured mass, radius and temperature: 13.6 M☉, 5.53 R☉ and 28,513 K, an early B-type star (Habibi et al. 2017). The other three are fainter, and their masses haven't been measured.

Decision: a star's mass is a few millionths of Sgr A*'s, far too little to change these orbits, so S29, S38 and S55 move as test particles: mass 0, radius 0, and the color of a 20,000 K star, typical of the S-stars measured so far. Their tooltips say "Mass: not measured". Sgr A* is a black-hole body with spin 0, the same hole the ray tracer draws.

### Seeing the orbits the way GRAVITY does

Problem: GRAVITY sees the orbits from Earth, and a top-down view would look nothing like their figures.

Decision: the camera starts out looking from Earth along the line of sight, so the orbits look the way GRAVITY sees them, with east to the left. The camera can't roll, so it keeps the ecliptic's north up, which at Sgr A* is within 1.5° of the sky's north. The view is 3,000 AU across, about 0.36″, and time runs at half a year a second, so S2 goes round in about 32 seconds.

## 3. How I built it

### The extra pull

`Gravity::accelerate` adds the extra pull to Newton's in the same loop over pairs. For each pair, s is how strongly the two pull each other per unit of the other's mass:

```cpp
const Vector3D d = bodies[j].position - bodies[i].position;
const double r2 = d.lengthSquared() + eps2;
double s = units::G / (r2 * std::sqrt(r2));
if (gr != 0.0) s += gr * (bodies[i].mass + bodies[j].mass) / (r2 * r2);
bodies[i].acceleration += d * (s * bodies[j].mass);
bodies[j].acceleration -= d * (s * bodies[i].mass);
```

`gr` is `relativity * RELATIVITY_PULL`, with `RELATIVITY_PULL` = 6 G²/c² and `relativity` the Exaggerate factor. The same `s` goes into both bodies, with opposite signs, so Newton's third law holds. Above 64 bodies, `markHosts` picks the 16 heaviest with `std::partial_sort`, which sorts only the first 16 places of a list, and a second loop adds the extra pull for pairs that include one.

### The energy

`Gravity::potentialEnergy` adds the matching term, for the same pairs:

```cpp
potential -= units::G * mi * mj / std::sqrt(r2);
if (gr != 0.0 && (everyPair || marks[i] || marks[j])) potential -= gr * mi * mj * (mi + mj) / r2;
```

Here `gr` is half of `relativity * RELATIVITY_PULL`, so the term is −3 G² m₁ m₂ (m₁ + m₂)/(c² r²). The force is exactly this energy's slope. `Simulation::predictPath` uses the same extra pull, so the dashed preview of a new body's path turns too.

### Black holes

`Body` has a type, `BodyType::Ordinary` or `BodyType::BlackHole`, and for black holes a spin and a spin axis. `Body::blackHole` makes one:

```cpp
const double a = std::clamp(s, 0.0, blackhole::MAX_SPIN);
Body hole(std::move(n), std::move(k), m, blackhole::horizonRadiusAU(m, a), HOLE_COLOR, pos, vel);
hole.type = BodyType::BlackHole;
hole.spin = a;
```

`std::move(n)` hands the name string over to the new body instead of copying it. `horizonRadiusAU` is the formula for r₊:

```cpp
inline double horizonRadiusAU(double mass, double spin) {
    const double a = std::clamp(spin, 0.0, MAX_SPIN);
    return 0.5 * schwarzschildRadiusAU(mass) * (1.0 + std::sqrt(1.0 - a * a));
}
```

When two bodies merge, `mergePair` decides which one survives:

```cpp
const bool holeA = bodies[a].isBlackHole(), holeB = bodies[b].isBlackHole();
const std::size_t keepIndex = holeA != holeB ? (holeA ? a : b) : (bodies[a].mass >= bodies[b].mass ? a : b);
```

If exactly one is a hole, it's kept; otherwise the heavier is. If the survivor is a hole, its radius becomes the horizon for the new mass.

### The preset

`Presets.cpp` builds each star from its sky elements and turns it into the ecliptic frame with `galaxy::skyFrame`:

```cpp
const double longPeri = star.node + star.argPeri;
const OrbitState o = keplerState(mu, a, star.e, longPeri, longPeri + meanAnomaly, star.incl, star.node);
const Vector3D position = sky.north * o.position.x + sky.east * o.position.y + sky.away * o.position.z;
```

`keplerState` takes longitudes measured from the node's reference direction, which here is north, so the longitude of pericenter is Ω + ω. The scenario's `lookDirection` is `sky.away`, and the app turns the camera to look along it when the preset loads.

`skyFrame` builds the three directions from right ascension and declination:

```cpp
inline SkyFrame skyFrame(double ra, double dec) {
    const double ca = std::cos(ra), sa = std::sin(ra), cd = std::cos(dec), sd = std::sin(dec);
    return SkyFrame{equatorialToEcliptic(Vector3D(-sd * ca, -sd * sa, cd)),
                    equatorialToEcliptic(Vector3D(-sa, ca, 0.0)),
                    equatorialToEcliptic(Vector3D(cd * ca, cd * sa, sd))};
}
```

The third direction is the star's own direction on the sky; north and east are at right angles to it, along the lines of declination and right ascension.

### The app

The Physics tab has General relativity and Exaggerate, the Add tab the Black hole switch and Spin, and the Black holes tab appears whenever a hole is on show. The renderer draws any hole, given its position, mass and axis.

### Code map

| File | What's there |
|---|---|
| `engine/include/emma/Constants.hpp` | `SPEED_OF_LIGHT` in AU per day |
| `engine/include/emma/BlackHole.hpp` | `MAX_SPIN`, `horizonRadiusAU` |
| `engine/include/emma/Body.hpp`, `engine/src/Body.cpp` | `BodyType`, `spin`, `spinAxis`, `Body::blackHole`, `describeBlackHole`, `HOLE_COLOR` |
| `engine/src/Collisions.cpp` | a black hole survives every merge, and its horizon grows |
| `engine/include/emma/Gravity.hpp`, `engine/src/Gravity.cpp` | `relativity`, `RELATIVITY_PULL`, `RELATIVITY_HOSTS`, the extra pull and potential |
| `engine/include/emma/Simulation.hpp`, `engine/src/Simulation.cpp` | `addBody(const Body&)`, `setRelativity`, `lookDirection`; the preview turns too |
| `engine/include/emma/Galaxy.hpp` | `SGR_A_RA`, `SGR_A_DEC`, `SkyFrame`, `skyFrame` |
| `engine/include/emma/Presets.hpp`, `engine/src/Presets.cpp` | `Preset::GalacticCenter`, `Scenario::lookDirection`, the S-stars |
| `app/Renderer.hpp`, `app/Renderer.cpp` | `BlackHoleSight`; `drawBlackHole` takes any hole; `shadowPixels` |
| `app/Ui.hpp`, `app/Ui.cpp` | General relativity and Exaggerate; the Black hole switch and Spin; the Black holes tab; tooltips |
| `app/main.cpp` | every hole drawn, the biggest-looking one ray traced; Go to for named holes; the preset's view |

### Tests

The turn of an orbit is measured with its eccentricity vector, which points from the central body toward the pericenter, and whose length is the eccentricity:

```
e = ((v² − μ/r) r − (r·v) v) / μ
```

r and v are the orbiting body's position and velocity relative to the central one, and μ = G (M + m). Relativity makes this vector wobble a little along the orbit, but the wobble is zero at both ends of the ellipse, the pericenter and the apocenter. So the tests compare it at successive apocenters, or perihelia for Mercury. They find those moments where r·v changes sign, and interpolate to them.

1. `black_hole_horizon`: a hole that doesn't spin has its horizon at Rs, and at spin 0.998 at 0.531607 Rs. A spin above 0.998 is held there. `Body::blackHole` sets the type, the radius and a unit axis.
2. `black_hole_swallow`: a 5 M☉ hole and a 20 M☉ star fall together. The hole survives with its name and spin, 25 M☉, a horizon for 25 M☉, and no momentum.
3. `mercury_precession`: the Sun and Mercury alone, for a century, with IAS15. With relativity the perihelion turns 42.98″ a century, to within 1% of the formula; without it, it doesn't turn. The other planets are left out so their 532″ a century doesn't mix in.
4. `s2_precession`: the preset itself, with IAS15. S2 passes pericenter at 2018.38 ± 0.01, 120 AU from Sgr A*, at 7,500 to 7,900 km/s, which checks the sky frame and the starting date. Between its 2026 and 2042 apocenters its orbit turns by relativity's 12.2′, to within 1%, and by nothing without relativity. The energy, relativity's term included, drifts by less than 10⁻¹⁰ over the 25 years. The test then runs Verlet at the preset's 1-day step and prints how far Verlet's own error turns the orbit, with and without relativity.

## Things to try

1. Load Sgr A* and the S-stars and let it run. S2 swings through pericenter in May 2018, about three seconds in, and S29 and S55 follow in 2021.
2. Set Exaggerate to 100 and wait a few laps. S2's orbit becomes a rosette, turning 20° a lap. Untick General relativity and the ellipses close again.
3. With Exaggerate at 100, compare Verlet and IAS15. Then set it back to 1 and run `physics_tests s2_precession` to see how big Verlet's own turn is next to relativity's.
4. Press Go to Sgr A*, and the hole is ray traced from 28 Rs, with the S-stars hundreds of AU away.
5. In empty space, switch the Add tab to Black hole, add a 10⁶ M☉ hole, switch back and launch stars around the hole on circular orbits. Then drop a second hole onto the first.

## Glossary

### Relativity

| Term | Meaning |
|---|---|
| Special relativity | Einstein's 1905 theory: light's speed is the same for everyone, so moving clocks run slow |
| General relativity | Einstein's 1915 theory of gravity: mass and energy curve spacetime, and bodies move along its straightest paths |
| Spacetime | Space and time together, as one four-dimensional whole |
| ψ | G M/(c² r): the fraction of its time a clock loses near a mass, and a measure of how strong gravity is there |
| Post-Newtonian (1PN) | Relativity written as Newton's gravity plus corrections in powers of ψ and (v/c)²; 1PN is the first |
| f_SP | GRAVITY's factor on the relativistic terms of their fits: 0 is Newton, 1 is Einstein |
| Gravitational redshift | Light losing energy as it climbs away from a mass, and arriving redder |

### Orbits

| Term | Meaning |
|---|---|
| Semi-major axis (a) | Half the longest width of an ellipse |
| Eccentricity (e) | How stretched an ellipse is: 0 for a circle, close to 1 for a long, thin one |
| Semi-latus rectum | a (1 − e²): the distance from the central body a quarter turn after pericenter |
| Pericenter, apocenter | Where an orbit comes closest to its central body, and where it's farthest. Perihelion around the Sun, periastron around a star |
| Inclination (i) | An orbit's tilt to a reference plane: the ecliptic for the planets, the sky for the S-stars |
| Ascending node (Ω) | Where an orbit crosses the reference plane going up. For the S-stars, a position angle measured from north through east |
| Argument of pericenter (ω) | The angle from the node to the pericenter, within the orbit's plane |
| Mean anomaly | How far through its period a body is since pericenter, as an angle |
| Osculating elements | The ellipse a body would follow from a given moment if every other effect were switched off |
| Eccentricity vector | A vector pointing at an orbit's pericenter, as long as its eccentricity |
| Kepler's third law | M = a³/P² in solar masses, AU and years |
| Test particle | A body with no mass of its own: other bodies pull it, and it pulls nothing |

### Black holes

| Term | Meaning |
|---|---|
| Spin (a*) | cJ/(GM²): 0 for a hole that doesn't spin, just under 1 for the fastest |
| Kerr black hole | A spinning black hole |
| Horizon radius (r₊) | GM/c² (1 + √(1 − a*²)): Rs without spin, 0.53 Rs at a* = 0.998 |
| Stellar-mass, intermediate-mass, supermassive | Under about 100 M☉, between, and from about 10⁵ M☉ |

### Measurement and numerics

| Term | Meaning |
|---|---|
| Arcsecond (″), arcminute (′) | 1/3600 and 1/60 of a degree |
| Microarcsecond | A millionth of an arcsecond |
| σ (sigma) | How many standard errors a measured effect is from zero. At 5σ, chance would produce it less than once in a million tries |
| Interferometer | Several telescopes combined to see as sharply as one as wide as the distance between them |
| Binet's equation | The equation for the shape of an orbit under a pull toward a center, in u = 1/r |
| Symplectic | An integrator whose energy error rises and falls around a fixed value instead of growing |

### C++

- `enum class BodyType { Ordinary, BlackHole };` is a list of named values. `enum class` keeps the names inside the type, so you write `BodyType::BlackHole`, and it can't be mixed up with plain numbers.
- `std::clamp(s, 0.0, MAX_SPIN)` keeps the spin within 0 and 0.998.
- `std::move` on a `std::string` hands its text over to a new owner instead of copying it, which is cheaper.
- `std::partial_sort` sorts only the first k places of a list, enough to find the 16 heaviest bodies without sorting them all.
- The ternary operator nests: `holeA != holeB ? (holeA ? a : b) : (...)` reads "if exactly one is a hole, pick it; otherwise pick the heavier".

## Sources

- A. Einstein (1915), Sitzungsberichte der Preussischen Akademie der Wissenschaften, 831: Mercury's perihelion explained.
- U. Le Verrier (1859), Comptes Rendus 49, 379: Mercury's unexplained perihelion advance.
- F. W. Dyson, A. S. Eddington & C. Davidson (1920), Philosophical Transactions of the Royal Society A 220, 291: starlight bent by the Sun, measured at the 1919 eclipse.
- R. V. Pound & G. A. Rebka (1960), Physical Review Letters 4, 337: clocks running slower lower down, measured in a 22.5 m tower.
- N. Ashby (2003), Living Reviews in Relativity 6, 1: relativity in GPS.
- R. A. Hulse & J. H. Taylor (1975), ApJ 195, L51: the binary pulsar.
- R. P. Kerr (1963), Physical Review Letters 11, 237: the spinning black hole.
- K. S. Thorne (1974), ApJ 191, 507: the 0.998 limit on a hole's spin.
- A. M. Nobili & I. W. Roxburgh (1986), in Relativity in Celestial Mechanics and Astrometry, IAU Symposium 114, 105: the 1/r² potential that turns orbits exactly as relativity does.
- GRAVITY Collaboration (2018), A&A 615, L15: S2's gravitational redshift.
- GRAVITY Collaboration (2020), A&A 636, L5: S2's orbit turning.
- GRAVITY Collaboration (2022), A&A 657, L12, table B.1: the four-star fit, Sgr A*'s mass and distance, and f_SP = 0.997 ± 0.144.
- M. Habibi et al. (2017), ApJ 847, 120, table 2: S2's mass, radius and temperature.
- M. J. Reid & A. Brunthaler (2004), ApJ 616, 872: Sgr A*'s position on the sky.
- M. J. Reid et al. (2019), ApJ 885, 131: R₀ = 8.15 kpc.
