# Planet surfaces

This document explains how a planet's real surface can be drawn from a map, turned the right way and lit by the Sun, the problems that shaped how EMMA does it, and how I built it. Zoom in on the Sun, a planet or the Moon, and EMMA draws it turned the way it faced at the simulated moment. Earth shows city lights on its night side and clouds over the day side. The Moon's craters show along the line between day and night, and Saturn has its rings, with the planet's shadow on them and their shadow on the planet.

## 1. Planet surfaces: what they are and why we need them

### Why draw real surfaces

At solar-system scale a planet is far smaller than a pixel, so a dot is all you can see. Zoom in, and a dot no longer tells you anything: which side faces the Sun, how Earth's day moves across its continents, which way Saturn's rings lean, why Uranus looks so strange. Drawing the real surface, turned and lit the way it really was at that moment, shows all of that.

### Maps of a sphere

A planet's surface is stored as an equirectangular map: an image with longitude running across, from 180° west at the left edge to 180° east at the right with 0° in the middle, and latitude running down, from the north pole to the south pole. Any point on the planet has a latitude and a longitude, so it has a place on the map.

### From a pixel to a point on the planet

To draw a planet, you follow each pixel's line of sight from the eye until it meets the planet, turn that point into a latitude and a longitude, and look up the map there. Then you light it by how directly it faces the Sun: a point facing the Sun straight on is fully lit, and the light falls off toward the line between day and night.

Planets aren't perfect spheres. A spinning body bulges at its equator: Saturn is about 10% flatter from pole to pole than across its equator, and Jupiter about 6.5%. A sphere flattened along its pole is still easy to work with, because squeezing space along the pole turns it back into a plain sphere.

### Which way a body faces

A body's surface only looks right if it's turned the right way. Earth should show daylight over the right continents at the right time of day, and Saturn's rings should lean the way they really do.

The IAU's Working Group on Cartographic Coordinates and Rotational Elements publishes, for each body, where its north pole points and how its prime meridian, its line of 0° longitude, turns:

- The north pole points to a right ascension α₀ and declination δ₀ in the equatorial J2000 frame. Both drift slowly, by a few hundredths of a degree per century.
- The prime meridian has turned W degrees east from the point where the body's equator crosses Earth's equator of J2000. W grows steadily: by 360.9856235° a day for Earth, which is one turn every 23 hours 56 minutes measured against the stars.

From those, the direction of the prime meridian is

```
x = Q cos W + (P × Q) sin W
```

where P is the pole and Q is the crossing point, at right ascension α₀ + 90°. That's a turn of Q by the angle W around the pole. The body's z axis is P, and its y axis is P × x.

| Body | Pole tilt | Turns once in |
|---|---|---|
| Sun | 7.25° from the ecliptic's pole | 25.4 days |
| Earth | 23.44° from the ecliptic's pole | 23.93 hours |
| Moon | about 1.5° from the ecliptic's pole | 27.32 days, so the same side always faces Earth |
| Mars | 25.2° from its orbit's axis | 24.62 hours |
| Jupiter | 3.1° from its orbit's axis | 9.93 hours |
| Saturn | 28.05° from the ecliptic's pole | 10.66 hours |
| Uranus | 97.8° from its orbit's axis, so it rolls on its side | 17.24 hours, backward |
| Venus | 2.6° from its orbit's axis, nearly upright | 243 days, backward |

### Light and shadow

Past the line between day and night a planet is dark. A few bodies have more going on:

- Earth's cities glow at night, and its clouds are white where the Sun lights them.
- The Moon's surface is rough. Slopes facing the Sun look brighter, which shows best near the line between day and night, where crater rims cast the strongest contrast.
- Saturn's rings lie in its equator's plane, from the C ring's inner edge at 74,658 km to the A ring's outer edge at 136,775 km. They're brighter seen from the side the Sun lights, dark where the planet's shadow falls on them, and they cast their own shadow on the planet.
- The Sun gives its own light, and it's a little dimmer toward its edge. That's limb darkening, and real photos of the Sun show it.

## 2. Problems and decisions

### When to switch from a marker to the surface

Problem: far away, a planet is drawn as a marker, a flat disc that grows with the logarithm of its mass, so it stays visible. Close up, its true size takes over. Switching to the surface too early would draw a detailed planet a pixel wide.

Decision: switch to the real surface once the body's true size is larger than its marker and at least 3 px in radius. Bodies without a map, the ones you add, the stars of Alpha Centauri and the S-stars, keep their flat disc.

### Which maps

Problem: the maps have to be good, free to use, and as true to the real bodies as possible.

Decision: the planet and Sun maps are from Solar System Scope, based on NASA imagery, under the Creative Commons Attribution 4.0 license. Their colors are a little more saturated than real photos, and parts of some planets that haven't been mapped yet are filled with terrain that matches the rest. The Moon's maps are from NASA's CGI Moon Kit, built from Lunar Reconnaissance Orbiter data. Venus uses its cloud map, since its surface is hidden under the clouds. All the maps put 0° longitude in the middle with east to the right, the convention Earth and Moon maps use.

| Body | Maps |
|---|---|
| Sun, Mercury, Venus, Mars, Jupiter, Uranus, Neptune | the surface |
| Earth | the surface, the city lights at night, and the clouds |
| Moon | the surface, and a height map for its relief |
| Saturn | the surface, and a strip running from the rings' inner edge to their outer edge |

### Flattened bodies

Problem: a flattened body makes finding where a line of sight meets it harder, and EMMA's radii are mean radii, the radius of a sphere with the same volume, not the equatorial radius the shape needs.

Decision: work in the body's own axes, x toward its prime meridian and z toward its north pole, with lengths in its equatorial radius, and squeeze space along the pole so the body becomes a plain sphere. The equatorial radius is the mean radius divided by the cube root of (polar radius / equatorial radius).

### A smooth edge

Problem: a pixel is either on the planet or off it, which makes a jagged edge.

Decision: fade the edge over one pixel, by how far the line of sight passes from the body's center.

### The seam at 180°

Problem: longitude jumps from +180° to −180° on the far side of the body. The graphics card picks how blurry a map lookup should be from how fast the map coordinates change between neighboring pixels, so at that jump it would pick the blurriest copy of the map and draw a thin seam.

Decision: keep a second longitude whose jump is at 0° instead, and use whichever of the two changes more slowly at each pixel. The method is Marco Tarini's.

### Which rotation models

Problem: the IAU's models include small periodic wobbles for some bodies, and they come from reports of different years.

Decision: the 2015 report's values, as NASA's NAIF gives them in `pck00011.tpc`, where Earth and the Moon come from the 2009 report. EMMA keeps the largest wobble for the Moon (up to 3.9°), Mars (up to 1.6°) and Neptune (under 1°). Every term left out is under 0.15°, and the IAU states its models are good to about 0.1°.

### Fast spinners

Problem: W grows by over a million degrees a decade for the fast spinners. Turning such a big number into radians and taking its sine wastes digits.

Decision: bring W back into one turn first, with the remainder after dividing by 360°, then turn it into radians.

### The Moon's relief

Problem: the Moon's height map stores heights as values from 0 to 255, not in kilometers.

Decision: assume the map spans the Moon's full relief, about 20 km from lowest to highest, and tip each point's surface by the slope between neighboring heights.

### Loading the maps

Problem: there are many large images, and most of them aren't needed at the start.

Decision: load a body's maps the first time it's drawn large enough, and only once, so the app starts fast. A missing file just leaves that part out. Maps that wrap around get smaller copies, called mipmaps, so a distant planet doesn't shimmer.

### How much of the screen to work on

Problem: running the surface program for every pixel of the view, for every planet, would waste most of the work on empty space.

Decision: run it on a square just around the body and its rings, or over the whole view when the eye is close.

## 3. How I built it

### Switching from the marker

`Renderer::drawBodies` tries the surface once the body's true radius on screen reaches 3 px and has caught up with its marker, and draws the flat disc otherwise:

```cpp
const double trueRadius = b.radius * camera.focal() / depth[i];
const bool surface = trueRadius >= SURFACE_FROM_PX && trueRadius + 1e-3 >= r &&
                     drawSurface(target, camera, frame, b, p, static_cast<float>(trueRadius), sun, julianDay);
if (!surface) drawDisc(target, p, r, toColor(b.color));
```

`drawSurface` returns false when the body has no rotation model or no map, or when shaders aren't available.

### The maps

The maps live in the `textures` folder, which the build copies next to the executable. They're named after the body in lowercase: `earth.jpg`, `earth_night.jpg`, `earth_clouds.jpg`, `moon_height.jpg`, `saturn_ring.png`, and so on. `Renderer::surfaceFor` loads them the first time:

```cpp
maps.color = loadTexture(base + ".jpg", true);
maps.night = loadTexture(base + "_night.jpg", true);
maps.clouds = loadTexture(base + "_clouds.jpg", true);
maps.height = loadTexture(base + "_height.jpg", true);
maps.rings = loadTexture(base + "_ring.png", false);
```

Each one is a `std::optional<sf::Texture>`, empty when the file isn't there.

### The body's axes

`engine/src/Spin.cpp` holds each body's IAU model and works out its axes for a date:

```cpp
const Vector3D pole(std::cos(dec) * std::cos(ra), std::cos(dec) * std::sin(ra), std::sin(dec));
// Where the body's equator crosses Earth's equator
const Vector3D node(-std::sin(ra), std::cos(ra), 0.0);
// Turn from there by w to find longitude 0
Axes axes;
axes.x = node * std::cos(w) + pole.cross(node) * std::sin(w);
axes.z = pole;
axes.y = pole.cross(axes.x);
```

Before that, W is brought back into one turn:

```cpp
const double wDegrees = std::fmod(m.meridian + m.meridianRate * d, 360.0) +
                        m.meridianAmplitude * std::sin(angle(m.meridianAngle, m.meridianAngleRate));
```

`eclipticAxes` turns all three axes into the ecliptic frame the simulation runs in, and `drawSurface` turns them again into the picture's frame, which matters in the galaxy view.

### The shader

`app/SurfaceShader.hpp` holds the program the graphics card runs for every pixel, written for GLSL 1.10, the version SFML uses. The renderer works out the equatorial radius from the mean one:

```cpp
const double polar = 1.0 - model->flattening;
const double equatorial = body.radius / std::cbrt(polar);
```

and passes the shader the eye, the camera's axes and the Sun's direction, all in the body's axes and in equatorial radii.

The seam trick is a few lines:

```glsl
vec2 mapCoords(vec3 p) {
    float lon = atan(p.y, p.x) / (2.0 * PI);
    float lat = asin(clamp(p.z / length(p), -1.0, 1.0)) / PI;
    float u1 = lon + 0.5;
    float u2 = fract(lon) + 0.5;
    float u = fwidth(u1) <= fwidth(u2) + 0.000001 ? u1 : u2;
    return vec2(u, 0.5 - lat);
}
```

`fwidth` measures how fast a value changes between neighboring pixels. `u1` jumps at ±180° and `u2` at 0°, and each pixel uses whichever is smooth there.

### Code map

| File | What's there |
|---|---|
| `engine/include/emma/Spin.hpp`, `engine/src/Spin.cpp` | The IAU models for the Sun, the planets and the Moon, and each body's axes at a date |
| `app/SurfaceShader.hpp` | The shader |
| `app/Renderer.hpp/.cpp` | `drawSurface`, `surfaceFor`, `loadTexture`; `drawBodies` switches from the marker to the surface |
| `app/SceneFrame.hpp` | `direction()` turns a simulation direction into the picture's frame, for the galaxy view |
| `app/main.cpp` | Passes the simulated date to `drawBodies`, and tells the renderer where the executable is |
| `CMakeLists.txt` | Copies `textures` next to the executable and builds `Spin.cpp` |

### Tests

`body_spin` checks three things:

1. At J2000, Greenwich points to right ascension 280.147°, 90° past the IAU's 190.147°. A wrong sign or a mixed-up axis would fail it.
2. Earth's axis tilts 23.4393° from the ecliptic's pole, Saturn's about 28.05° and the Moon's about 1.5°. These catch a mistake in turning equatorial coordinates into ecliptic ones.
3. The axes of Earth, the Moon, Mars, Jupiter and Neptune are still exactly at right angles and one unit long after 9,000 days, millions of degrees of spin later.

## Things to try

1. Follow Earth and zoom in until it's a few hundred pixels across. Turn the view to the night side and find the city lights, then run time at a few hours per second and watch the day move across the continents.
2. Follow the Moon and zoom in until it fills the view. The craters stand out most along the line between day and night.
3. Follow Saturn and turn the view to look across its rings, then from above them. Look for the planet's shadow on the rings behind it.
4. Follow Uranus. It spins lying on its side, with its pole tipped almost into the plane of its orbit.

## Glossary

| Term | Meaning |
|---|---|
| Equirectangular map | An image with longitude across and latitude down |
| Prime meridian | A body's line of 0° longitude |
| Right ascension, declination | Longitude and latitude on the sky, measured from Earth's equator |
| Flattening | How much shorter a body is pole to pole than across its equator, as a fraction |
| Mean radius | The radius of a sphere with the same volume |
| Limb darkening | A star looking dimmer toward its edge |
| Mipmaps | Smaller copies of a map, used when it's drawn small |
| Shader | A small program the graphics card runs once for every pixel |

### C++ and GLSL

- `std::optional<sf::Texture>` holds a texture or nothing. A missing map file leaves it empty, and `if (maps.night)` checks before using it.
- `std::fmod(a, 360.0)` is the remainder after dividing by 360, for doubles: it brings an angle back into one turn.
- `std::cbrt` is the cube root.
- In GLSL, `uniform` values are the same for every pixel, like the eye's position; the C++ side sets them with `setUniform` before drawing.
- `fwidth(x)` in GLSL measures how fast x changes between a pixel and its neighbors.

## Sources

- B. A. Archinal et al. (2018), "Report of the IAU Working Group on Cartographic Coordinates and Rotational Elements: 2015," Celestial Mechanics and Dynamical Astronomy 130, 22, with its 2019 correction.
- NASA NAIF, `pck00011.tpc` (2022), https://naif.jpl.nasa.gov/pub/naif/generic_kernels/pck/pck00011.tpc: the values used here, and the radii behind each flattening.
- Solar System Scope, https://www.solarsystemscope.com/textures/: the planet, Sun, ring and Earth maps, under the Creative Commons Attribution 4.0 International license.
- NASA's Scientific Visualization Studio, CGI Moon Kit, https://svs.gsfc.nasa.gov/4720: the Moon's color and height maps.
- M. Tarini (2012), "Cylindrical and Toroidal Parameterizations Without Vertex Seams," Journal of Graphics Tools 16, 144: the second-longitude method for the seam.
