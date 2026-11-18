# The galaxy view

This document explains how the solar system moves through the Milky Way and how EMMA shows it, the problems that shaped the galaxy view, and how I built it. Tick Galaxy view in the View tab: the picture turns so the grid becomes the plane of the Galaxy, the camera rides along with the Sun, and every planet's trail stretches out into a long helix behind it. A slider sets how fast the Sun moves, from 0 up to 300 km/s, with the real value, 247 km/s, one click away.

The whole feature rests on one line:

```
scene position = R · p + D(t)
```

p is where the simulation had a body at time t, R is a rotation from ecliptic coordinates into Galactic ones, and D(t) is how far the Sun has moved through the Galaxy since the start.

## 1. The solar system in the Galaxy: what it is and why we need it

### Why look at it this way

The planets go around the Sun, but the Sun isn't standing still: it carries the whole solar system around the center of the Milky Way at about 247 km/s. Seen from the Galaxy, no planet traces a closed ellipse. Each one traces a long coil, stretched out behind the moving Sun. The galaxy view shows the same orbits from that point of view.

### Two frames

A frame, or reference frame, is a choice of origin, axes and units to measure positions in.

The simulation frame is what the physics uses. Its origin is the solar system's barycenter, its center of mass, held at rest. Its axes are the J2000 ecliptic, and its units are AU and days.

The scene frame is what the camera looks at. In the normal view it's the same as the simulation frame. In the galaxy view its axes are Galactic, and everything slides along the Sun's path.

Newton's laws work the same in any frame moving at a steady velocity. That's Galilean invariance: add the same drift to every position, and every distance, every force and every orbit stays the same. The helix is the ordinary orbit, seen by someone the Sun is flying past.

### Three sets of axes

Three sets of axes are involved. Each is a right-handed x, y, z, and each is a rotation of the others.

| Frame | Its flat plane | x points to | z points to |
|---|---|---|---|
| Ecliptic (J2000) | Earth's orbit | the March equinox | the north ecliptic pole |
| Equatorial (ICRS) | Earth's equator | the March equinox | the north celestial pole |
| Galactic | the Milky Way's disk | the Galactic center, in Sagittarius | the north Galactic pole |

Ecliptic and equatorial share their x axis, the equinox, which is where their planes cross. They differ by a tilt about it: the obliquity, 84,381.448″ = 23.4392911°, Earth's axial tilt at J2000.

Equatorial to Galactic is a fixed table from the Hipparcos Catalogue (ESA 1997). Each row of the table is one Galactic axis, written as a direction in equatorial coordinates. Row 0, turned into right ascension and declination, is RA 266.40°, Dec −28.94°: the Galactic center. Row 2 is RA 192.86°, Dec +27.13°: the north Galactic pole. Multiplying the table by a direction takes its dot product with each row, which measures how far it reaches along each Galactic axis, exactly what "its Galactic x, y and z" means.

The two turns multiply into one rotation. Rounded, it is:

```
Galactic x = (-0.0549, -0.9938, -0.0965) · ecliptic
Galactic y = ( 0.4941, -0.1110,  0.8623) · ecliptic
Galactic z = (-0.8677, -0.0004,  0.4971) · ecliptic
```

Two things can be read off it. The ecliptic is tilted 60.2° to the Galactic plane: the angle between their poles is acos(0.4971). And the north ecliptic pole sits at Galactic longitude 96.38°, latitude 29.81°: that's the third column.

A rotation's rows are unit vectors at right angles to each other, and for such a matrix the inverse is just the transpose, the matrix flipped across its diagonal. Going back from Galactic to ecliptic costs nothing extra.

### How fast the Sun moves, and which way

The Sun's velocity relative to the Galactic center has three parts, in Galactic axes:

```
V = (U, Θ₀ + V☉, W) = (11.1, 247, 7.25) km/s
```

- Θ₀ + V☉ = 247 km/s, along the Galaxy's rotation. Θ₀ is the speed of a perfectly circular orbit at the Sun's distance, called the local standard of rest. V☉ is how much faster than that the Sun goes. Reid et al. (2019) measure the sum directly, 247 ± 4 km/s, more precisely than either part on its own. The same paper puts the Galactic center 8.15 kpc away.
- U = 11.1 km/s toward the Galactic center and W = 7.25 km/s toward the north Galactic pole. These are the Sun's peculiar motion, its small drift relative to the local standard of rest (Schönrich, Binney & Dehnen 2010).

Altogether that's 247.36 km/s, or 0.14286 AU a day, or 52.18 AU a year. Every year the Sun covers about the distance from itself to the outer edge of the Kuiper belt.

Turned into ecliptic coordinates, the Sun's velocity points 60.6° out of the ecliptic plane, roughly toward the constellation Cygnus. Only cos 60.6° ≈ 0.49 of the speed lies in the plane of the planets' orbits, and the larger part, sin 60.6° ≈ 0.87, carries them out of it. That tilt decides the shape of every trail.

### Helix or loops?

Each planet's path in the scene is its orbit plus a steady drift. Split the Sun's velocity into two parts relative to the planet's orbital plane:

- The part perpendicular to the plane pulls each circle out into a coil, a helix. The distance along the axis per orbit is the helix's pitch.
- The part in the plane slides each circle sideways as it's drawn. Seen face-on, the curve is a trochoid, the curve a point on a rolling wheel traces.

A trochoid has loops only when the planet's own speed beats the in-plane drift. Otherwise it's a smooth wave.

For Earth, the in-plane drift is 247.36 × cos 60.6° ≈ 121 km/s, against Earth's 29.8 km/s. That's no loops by a factor of four, so Earth's real path is a long, gently twisted helix that advances about 52 AU per orbit. Slow the Sun down and loops appear:

- Earth starts looping once the Sun is below about 29.8 / 0.49 ≈ 61 km/s.
- Neptune, at 5.4 km/s, needs the Sun below about 11 km/s.
- Mercury moves between 39 and 59 km/s along its eccentric orbit, and its plane is tilted 7°, so its threshold is fuzzier, somewhere around 80 to 120 km/s.

The Sun's own path isn't perfectly straight either. The Sun circles the barycenter, mostly because of Jupiter, by up to about two solar radii, which shows as a tiny wiggle along a nearly straight line.

### The Sun's path around the Galactic center

Over a session the Sun's path is all but straight, but really it goes around the Galactic center, close to a circle of radius R₀ = 8.15 kpc, about 1.68 billion AU. At 247 km/s one lap takes 2π R₀ / 247 km/s ≈ 203 million years.

The Galactic center sits R₀ away along Galactic +x. The part of the Sun's velocity along the Galaxy's rotation carries it around that center: after a time t it has turned through an angle θ = v t / R₀, so it has moved R₀ sin θ along its starting direction and R₀ (1 − cos θ) toward the center.

A straight line misses an arc of radius R by about its sagitta, R θ² / 2:

| Simulated time | Distance traveled | How far the circle bends from a straight line |
|---|---|---|
| 1,000 years | 52,000 AU | under 1 AU |
| 10,000 years | 522,000 AU | about 80 AU |
| 36,000 years (an hour at 10 years/s) | 1.9 million AU | about 1,000 AU |

### The Milky Way band

Seen from inside the Galaxy, its disk is a band of stars around the whole sky, the Milky Way. In Galactic coordinates the band lies along latitude 0, and from Earth it crosses the ecliptic at about 60°.

To scatter stars evenly over a sphere, a simple trick works: take three independent random numbers from a normal distribution, a bell curve, and scale the result to length 1. The 3D normal distribution looks the same from every direction, so scaling it can't favor any of them. Picking random angles instead would bunch the stars at the poles.

## 2. Problems and decisions

### Moving the Sun in the physics or in the picture

Problem: the obvious way to show the Sun's motion would be to give every body an extra 247 km/s in the simulation itself. But that changes nothing physical, by Galilean invariance. It would also ruin the drift readout: at 247 km/s the Sun's whole mass would be moving, about 300,000 times the kinetic energy of all the planets' orbits, and since the readout divides by the total energy, it would shrink every real error by that factor and hide it (`energy-drift.md`). And every preset moves the system to its barycenter at rest when it loads, which would undo it anyway.

Decision: the galaxy view is purely a way of drawing. The physics stays exactly as it is, and one function turns each simulation position into a scene position.

### Data in one frame, the Galaxy in another

Problem: EMMA's data is ecliptic, because it asks JPL Horizons for the ecliptic plane and its fallback orbits are ecliptic too. Astronomers publish the Galactic frame relative to the equatorial one. And turning thousands of trail points through two rotations every frame would waste time.

Decision: two hops, ecliptic to equatorial to Galactic, multiplied into one rotation once, the first time it's needed.

### A table that's almost a rotation

Problem: the Hipparcos table is written to 10 digits, so it's a rotation only to about 10⁻¹⁰, and its transpose undoes it only that well. Going there and back would drift.

Decision: straighten the combined matrix once, when it's built. The pole row keeps its direction and is scaled to length 1; the x row loses the tiny part of it that lies along the pole, then is scaled to length 1; and the y row is rebuilt as z × x. This is the Gram-Schmidt process. Each axis moves by about 10⁻¹⁰, under half a kilometer at Neptune's distance, and afterward the transpose is the inverse to the last digit of a double.

### A slider for the Sun's speed

Problem: animations of the solar system flying through the Galaxy don't always agree with their own numbers. This feature started from a clip I'd seen whose readout says 18.7k AU in 403.4 years, which is 46 AU a year, or about 220 km/s, close to the real speed. Yet it draws the planets in tight coils near the Sun. At that speed no planet can loop, so the drawing must use a slower speed than its readout says.

Decision: a slider from 0 to 300 km/s with a Real button, so you can see both honestly: the real speed, and the speeds where loops appear. The slider sets only the length of the Sun's velocity; its direction always comes from the measured vector.

### Where to draw a trail

Problem: a trail point recorded at time t₁ has to be drawn where the system was then, R · p₁ + D(t₁). Drawn at D(now), the whole trail would shift along with the Sun, and you'd see an ordinary closed orbit riding along instead of a helix left behind.

Decision: every trail point stores the moment it was recorded, and the picture places it with that time. Storing the time instead of the finished Galactic position means the slider can change the speed at any moment and every trail redraws at once as if the Sun had always moved that fast, switching the view on or off needs no recalculation, and the simulation stays unaware of views.

### Trail points between samples

Problem: a body records a trail point each time its direction around the barycenter turns 2°, and the picture draws straight segments between them. In the galaxy view the true path between two samples is curved.

Decision: keep the same rule. The steady drift grows in a straight line with time, so a straight segment gets it exactly right. The Galactic circle bends between two samples by under 30 km, even for Neptune, whose samples are about a year apart. So the segment's only error is the same small chord as in the normal view. Each trail keeps 540 points, three turns of its orbit: about 38 AU long for Mercury, 157 AU for Earth and 25,800 AU for Neptune at 247 km/s.

### A straight line or the real circle

Problem: inside a session the Sun's path bends a few parts in 10,000, invisible next to the solar system. But the big-scale view (`big-scale-view.md`) puts the Galactic center in the picture, and there the Sun has to go around it.

Decision: follow the circle. The small drifts toward the center and the pole are added as plain velocity × time: they're really slow oscillations, tens of millions of years long, which no run gets near. The circle uses the Sun's actual speed, and keeps the Sun exactly R₀ from the center, which keeps the big picture consistent. (Strictly, the Sun moves a little faster than a circular orbit at its distance, about 236 km/s according to Reid et al., so its real path bends slightly less and slowly drifts outward. Over thousands of years that's about a tenth of the sagitta.) Lowering the speed with the slider moves the Sun around the same center more slowly.

### The stars in two frames

Problem: the starfield has to look right in both views, and turning 21,000 stars every frame would waste time.

Decision: make the stars in Galactic coordinates, 12,000 spread evenly over the sky and 9,000 dimmer ones in the Milky Way band, with latitudes scattered by about 5° above and below the plane. Store each star in both frames, and pick one when drawing. In the galaxy view the band lies flat along the grid. In the normal view it crosses the ecliptic at about 60°, as in the real sky.

### The camera

Problem: in the galaxy view the solar system moves 52 AU a year, so a camera left behind would lose it at once.

Decision: when the galaxy view switches on, the camera follows the Sun, turns so the Sun travels from left to right across the screen, along the Galaxy's rotation, and looks from 25° above the Galactic plane. The grid then lies on the Galactic plane through the Sun. F fits the view and goes back to following the Sun, since the point where the system started is far behind. The status bar hides the cursor's position, because grid coordinates in a moving frame wouldn't mean much.

After 10,000 years the scene's positions are around 500,000 AU from the origin. A double holds 15 to 16 significant digits, so positions there are still good to about 10⁻¹⁰ AU, roughly 15 m, and since the camera works relative to its target, zooming in on the Earth and Moon still works anywhere along the path.

### Adding bodies

Problem: turning a click in the galaxy view back into a simulation position would be easy, R⁻¹ · (q − D(t)), but the launch direction would need turning back too, and the dashed preview would need its own frame.

Decision: the galaxy view is for watching. Bodies are added in the normal view, and switching back is one click.

## 3. How I built it

### The scene frame

`SceneFrame::place` in `app/SceneFrame.hpp` is the only function that goes from the simulation frame to the scene:

```cpp
emma::Vector3D place(const emma::Vector3D& p, double t) const {
    if (!galactic) return p;
    return emma::galaxy::fromEcliptic(p) + emma::galaxy::sunDisplacement(velocity, t);
}
```

`main.cpp` updates the frame every frame:

```cpp
frame.galactic = state.galaxyView && sim.preset() == Preset::SolarSystem;
frame.velocity = galaxy::sunVelocityKms().normalized() * (state.sunSpeedKms * units::KMS_TO_AU_PER_DAY);
frame.now = sim.time();
```

### The rotation

The first hop is a turn about x:

```cpp
inline Vector3D eclipticToEquatorial(const Vector3D& e) {
    const double c = std::cos(OBLIQUITY), s = std::sin(OBLIQUITY);
    return Vector3D(e.x, c * e.y - s * e.z, s * e.y + c * e.z);
}
```

x is untouched, and y and z mix through cos ε and sin ε. JPL's "Approximate Positions of the Planets" page uses the same formula.

The second is the Hipparcos table:

```cpp
constexpr double ICRS_TO_GALACTIC[3][3] = {
    {-0.0548755604, -0.8734370902, -0.4838350155},
    {+0.4941094279, -0.4448296300, +0.7469822445},
    {-0.8676661490, -0.1980763734, +0.4559837762},
};
```

`eclipticToGalactic()` multiplies the two together and straightens the result, once, on the first call:

```cpp
inline const Rotation& eclipticToGalactic() {
    static const Rotation rotation = [] {
        const double c = std::cos(OBLIQUITY), s = std::sin(OBLIQUITY);
        Vector3D row[3];
        for (int i = 0; i < 3; ++i) {
            const double a = ICRS_TO_GALACTIC[i][0], b = ICRS_TO_GALACTIC[i][1], d = ICRS_TO_GALACTIC[i][2];
            row[i] = Vector3D(a, b * c + d * s, -b * s + d * c);
        }
        const Vector3D z = row[2].normalized();
        const Vector3D x = (row[0] - z * row[0].dot(z)).normalized();
        const Vector3D axes[3] = {x, z.cross(x), z};
        ...
    }();
    return rotation;
}
```

The loop turns each row of the table into ecliptic coordinates. Then `z` is the pole row scaled to length 1, `x` is the first row minus its part along `z`, scaled, and the middle axis is `z.cross(x)`.

`Rotation::apply` uses the matrix's rows and `applyInverse` its columns, so `toEcliptic` is as cheap as `fromEcliptic`.

### The Sun's circle

```cpp
inline Vector3D sunDisplacement(const Vector3D& v, double t) {
    const double angle = v.y * t / R0_AU;
    const double half = std::sin(0.5 * angle);
    return Vector3D(2.0 * R0_AU * half * half + v.x * t, R0_AU * std::sin(angle), v.z * t);
}
```

R₀ (1 − cos θ) is written as 2 R₀ sin²(θ/2). The two are equal, but for the tiny angles here cos θ is so close to 1 that subtracting it would throw away about half of a double's digits. The sine form keeps them all. `sunVelocityAt` turns the velocity through the same angle, and `SceneFrame::heading()` uses it for the dashed arrow showing which way the solar system is heading.

### Trails with times

```cpp
struct TrailPoint {
    Vector3D position;
    double time;  // days since time 0
};
```

`World::recordTrails()` stores `TrailPoint{b.position, time}`, and `Renderer::drawTrails` places each point with `frame.place(point.position, point.time)`.

### The stars

```cpp
struct Star {
    emma::Vector3D galactic;
    emma::Vector3D ecliptic;
    sf::Color color;
    float size;
};
```

Each frame, `drawStarfield` picks `frame.galactic ? star.galactic : star.ecliptic`, so the stars never get turned while drawing. A fixed random seed, 1337, gives the same sky every run.

### Switching the camera

When the galaxy view turns on, `main.cpp` sets Follow to the Sun and calls `camera.setAngles(90°, 25°)`. A yaw of 90° points the camera's right axis along Galactic +y, the Galaxy's rotation. When it turns off, Follow clears and the view fits the system again. Clicks only add bodies when `frame.galactic` is false.

### One frame

```
clock.run(sim)                                    physics, around the barycenter, unchanged
frame.galactic = galaxy view ticked and the preset is the solar system
frame.velocity = the measured direction × the slider's speed
frame.now      = sim.time()
if the view just switched: follow the Sun and turn the camera (or fit, going back)
camera.target  = frame.place(Sun's position)        follow
draw stars       with star.galactic or star.ecliptic
draw grid        in scene coordinates
draw trails      frame.place(point.position, point.time)
draw bodies      frame.place(body.position)          at time now
draw heading     dashed arrow from frame.place(barycenter) along frame.heading()
```

### Code map

| File | What's there |
|---|---|
| `engine/include/emma/Galaxy.hpp` | `OBLIQUITY`, `ICRS_TO_GALACTIC`, `R0_KPC`, `R0_AU`, `ORBIT_SPEED_KMS`, `U_SUN_KMS`, `W_SUN_KMS`; `Rotation`; `eclipticToEquatorial`, `equatorialToEcliptic`, `eclipticToGalactic()`, `fromEcliptic`, `toEcliptic`, `sunVelocityKms()`, `sunDisplacement`, `sunVelocityAt` |
| `engine/include/emma/Body.hpp` | `TrailPoint`; `trail` is a `std::deque<TrailPoint>` |
| `engine/src/World.cpp` | `recordTrails()` stores the time with each point |
| `app/SceneFrame.hpp` | `galactic`, `velocity`, `now`; `place`; `heading` |
| `app/Camera.hpp` | `setAngles(yaw, pitch)` |
| `app/Renderer.hpp/.cpp` | stars in both frames and the Milky Way band; drawing through the frame; `drawRay` for the heading arrow |
| `app/Ui.hpp/.cpp` | Galaxy view, the Sun's speed and Real in the View tab; how far the Sun has moved; `drawTag`; no cursor readout in the galaxy view |
| `app/main.cpp` | builds the frame every frame, switches the camera, follows through `frame.place`, adds bodies only in the normal view, draws the heading |

### Tests

`galactic_frame` checks five things, each against the mistake it would catch:

1. The Hipparcos table is a rotation to 10⁻⁹, and the matrix EMMA uses is one to 10⁻¹⁴. The first catches a typo in any of the table's 9 numbers, which straightening would otherwise hide. The second checks the straightening.
2. The north Galactic pole lands on (0, 0, 1). The test starts from its defining position, RA 192.85948°, Dec 27.12825°, and takes it through both hops. That catches a wrong obliquity, swapped hops, or the transpose used by mistake.
3. The north ecliptic pole lands at l = 96.38°, b = 29.81°. That's the sharpest check on the obliquity's sign: flipped, the pole would land near b = 19.9°.
4. There and back returns the same point to 10⁻¹², so `toEcliptic` really is the inverse.
5. The Sun heads about 60.6° out of the ecliptic, which guards the velocity's parts and their order.

`trail_smoothness` checks that trail times keep increasing and never run past the clock. `sun_path` in `view_tests` checks the circle: after a year the path is still within 10⁻⁵ AU of a straight line, after 10,000 years it has bent about 80 AU toward the Galactic center, a quarter of the way round the Sun is R₀ from the center and heading the right way, and the heading matches the path.

## Things to try

1. Turn on the galaxy view at the real speed and set time to 10 years/s. Zoom out to a few hundred AU, and Neptune's helix appears. Zoom out further to see the whole bundle.
2. Drag the Sun's speed down to about 50 km/s and watch Earth's trail start to loop. Below about 10 km/s, Neptune loops too.
3. Set the speed to 0. You see the solar system as it is, tilted about 60° to the Galactic grid, with the Milky Way band lying along the grid.
4. Turn the galaxy view off and look around. The band now crosses the ecliptic at about 60°, as in the real sky.

## Glossary

### Astronomy and physics

| Term | Meaning |
|---|---|
| Barycenter | The center of mass of a system. The Sun and the planets all orbit it |
| Ecliptic | The plane of Earth's orbit. J2000 pins it to its position on 1 January 2000, 12:00, because it slowly drifts |
| March equinox | Where the ecliptic and the celestial equator cross: the x axis both frames share |
| Equatorial frame, ICRS | Axes based on Earth's equator and pole. The ICRS is its modern version, fixed by distant quasars |
| Obliquity | The angle between the ecliptic and the equator, 23.44° |
| Galactic coordinates (l, b) | Longitude along the Milky Way's plane, measured from the Galactic center, and latitude above or below it |
| Local standard of rest | An imaginary point on a perfectly circular orbit at the Sun's distance |
| Peculiar motion | The Sun's velocity relative to the local standard of rest |
| Galilean invariance | Newton's laws work the same in every frame moving at a steady velocity |
| Helix | A circle stretched along its axis into a coil. Its pitch is the distance along the axis per turn |
| Trochoid | The path of a point on a rolling wheel: loops when the point moves faster than the wheel rolls, waves when slower |
| Sagitta | How far an arc bulges from the straight line between its ends, about R θ² / 2 for a small angle |

### Math

| Term | Meaning |
|---|---|
| Rotation matrix | A 3 × 3 matrix that turns vectors without stretching them |
| Orthonormal | Rows (and columns) one unit long and at right angles to each other. Every rotation matrix is |
| Transpose | The matrix flipped across its diagonal. For a rotation it's also the inverse |
| Dot product as projection | a · b, with b one unit long, is how far a reaches along b. A matrix times a vector is three dot products, one per row |
| Gram-Schmidt | Making a set of directions exactly unit length and at right angles, one at a time |

### C++

- `namespace emma::galaxy` is a namespace inside a namespace, written in one line (C++17). Everything in `Galaxy.hpp` is `emma::galaxy::something`.
- `constexpr` values, like the Hipparcos table and the Sun's speeds, are fixed while compiling, so the compiler can bake them into the program.
- An `inline` function in a header can be included by many `.cpp` files without the linker complaining that it's "defined twice". `Galaxy.hpp` is all `inline` functions, so it has no `.cpp` of its own.
- A function-local `static`, like the rotation inside `eclipticToGalactic()`, is built the first time the function runs and reused after that. Since C++11 that first build is thread-safe.
- `[] { ... }()` is a lambda that's called right away. It lets a `const` value be computed with loops or branches, right where it's declared.
- A lambda's capture list says what it may use from around it: `[&]` uses the surrounding variables by reference, `[n]` copies one variable, and `[this]` lets a lambda inside a member function use the object's members.
- Aggregate initialization fills a plain `struct` in field order: `TrailPoint{b.position, time}`. `Rotation r{}` fills it with zeros.
- `std::deque` is a sequence with fast adds and removals at both ends. That's why a trail can drop its oldest point cheaply when it adds a new one.
- `std::mt19937` is a random number engine, and `std::normal_distribution` and `std::uniform_real_distribution` shape its numbers into a bell curve or an even spread. The fixed seed, 1337, gives the same sky every run.
- The ternary operator, `frame.galactic ? star.galactic : star.ecliptic`, picks one of two values inside an expression: the first if the condition is true, the second if not.

## Sources

- ESA (1997), The Hipparcos and Tycho Catalogues, vol. 1, p. 92: the ICRS to Galactic matrix.
- JPL Solar System Dynamics, "Approximate Positions of the Planets": the ecliptic to equatorial turn and the J2000 obliquity.
- M. J. Reid et al. (2019), ApJ 885, 131: R₀ = 8.15 kpc and Θ₀ + V☉ = 247 km/s.
- R. Schönrich, J. Binney & W. Dehnen (2010), MNRAS 403, 1829: the Sun's peculiar motion, (11.1, 12.24, 7.25) km/s.
