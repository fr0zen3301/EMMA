# The big-scale view

This document explains how far the Galaxy's distances reach and how they're measured, the problems that came with fitting the solar system and the Galactic center into one picture at their true distances, and how I built it. Nothing in the picture is drawn closer than it really is: the Galactic center is 8.15 kpc away, about 1.68 billion AU, and the camera can back out to about 48 kpc. `galaxy-view.md` explains the Sun's path around the Galactic center, and `black-hole.md` explains Sgr A*, the black hole there.

## 1. Galactic distances: what they are and why we need them

### How far is far

| | AU | Parsecs | Light-years |
|---|---|---|---|
| Neptune's orbit | 30 | | |
| Half a parsec | 10⁵ | 0.48 | 1.6 |
| Proxima Centauri, the nearest star | 268,000 | 1.30 | 4.2 |
| The Galactic center | 1.68 × 10⁹ | 8,150 | 26,600 |
| EMMA's zoom limit | 10¹⁰ | 48,500 | 158,000 |

The solar system fits inside a few dozen AU. The Galaxy needs numbers a hundred million times bigger, and to see where the solar system sits in it, you need both in one picture.

### Parsecs and light-years

Past the solar system, AU stop being readable: "41421356237 AU" means nothing at a glance. Astronomers use two other units.

A parsec is the distance at which 1 AU looks one arcsecond wide. One radian is 648,000/π arcseconds, and for a small angle the distance is the size divided by the angle in radians, so 1 parsec is 648,000/π AU, about 206,265 AU. Since 2015 the IAU defines it that way, exactly. A kiloparsec is 1,000 parsecs. Astronomers measure the Galaxy in parsecs.

A light-year is how far light travels in a Julian year of 365.25 days: 299,792.458 km/s × 365.25 days × 86,400 s, about 63,241 AU. It's the unit most people already have a feel for.

### How fine a double is

Every position in EMMA is a double, a number with 53 binary digits, about 16 decimal ones. Near a value P, the gap between one double and the next, called an ulp (unit in the last place), is between 1.1 × 10⁻¹⁶ and 2.2 × 10⁻¹⁶ times P. No position can be stored more finely than that gap. Near 1 AU the gap is about 3 cm. Near the Galactic center, 1.68 × 10⁹ AU from the solar system, it's about 55 km.

## 2. Problems and decisions

### How far out the camera goes

Problem: at first the camera stopped at 10⁵ AU, about half a parsec: plenty for the solar system, and nowhere near the Galactic center.

Decision: 10¹⁰ AU. With a 45° field of view, the short side of the view covers the distance × tan 22.5° on each side of the target, so at 10¹⁰ AU that's about 20 kpc each way. From the Sun, that shows the Galactic center with plenty of room around it. From the Galactic center, it shows the Milky Way's disk, which is roughly 30 kpc across.

Zoom multiplies the distance by a factor for each wheel notch, and one notch roughly halves or doubles it (`3d-camera.md`). From half a parsec out to 48 kpc is five factors of ten, about 17 doublings, so about 17 notches. The whole range, from 15 km to 48 kpc, is about 56 notches.

### Everything else that depends on the zoom

Problem: the near plane, the farthest a click can land, and the grid's spacing all have to suit a range of 15 km to 48 kpc.

Decision: each is set as a fraction or multiple of the viewing distance, so each scales by itself. The near plane is 10⁻⁴ of the distance, 10⁶ AU in front of the eye at 10¹⁰ AU. The farthest a click can land is 1,000 times the distance. The grid's spacing is about a fifth of the distance, rounded to 1, 2 or 5 times a power of ten in AU.

### Readable distances

Problem: one unit can't cover 384,400 km and 8.15 kpc.

Decision: every distance the interface shows switches unit with its size:

| Distance | Shown as | Example |
|---|---|---|
| under 0.01 AU | km | 384400 km |
| 0.01 to 1,000 AU | AU, three decimals | 5.203 AU |
| 1,000 to 100,000 AU | whole AU | 18692 AU |
| 100,000 AU to 1 kpc | parsecs, with light-years | 1.30 pc (4.2 ly) |
| 1 kpc and up | kiloparsecs, with light-years | 8.15 kpc (26582 ly) |

The switch to parsecs is at 100,000 AU, about half a parsec. Below that, whole AU are still easy to read, and the galaxy view's own numbers, tens of thousands of AU over a session, stay in AU. Light-years come in brackets, because they're the unit most people picture.

The status bar's cursor readout follows the view's width: once the view is wider than 100,000 AU, the cursor's grid position is in parsecs or kiloparsecs too.

### Precision far from the origin

Problem: the picture jitters once the gap between doubles is bigger than a pixel. A pixel covers the distance / f AU at the target, where f, the focal length, is about 1,000 pixels in the default window. So the closest you can zoom before positions jump between pixels is about

```
distance  >  P × 2.2 × 10⁻¹⁶ × f  ≈  P × 2 × 10⁻¹³
```

where P is how far the target is from the origin.

| Looking at | Its distance from the origin, P | Gap between doubles there | Closest zoom with a steady picture |
|---|---|---|---|
| Earth | 1 AU | 3 cm | far below the 15 km zoom limit |
| The Sun, after an hour in the galaxy view at 10 years/s | 1.9 × 10⁶ AU | about 60 m | about 60 km |
| The Galactic center | 1.68 × 10⁹ AU | about 55 km | about 55,000 km, or 4 × 10⁻⁴ AU |

Decision: the camera subtracts its target from every point first, in double precision, before anything turns into pixels (`3d-camera.md`), so only small numbers reach the graphics card. Zooming out is never a concern: at 10¹⁰ AU a pixel covers about 10⁷ AU, so any rounding is billions of times too small to see. Near the Galactic center, 4 × 10⁻⁴ AU is still far finer than needed for Sgr A*: its event horizon is about 0.085 AU in radius, and Go to Sgr A* puts you 28 of its Schwarzschild radii away, about 2.4 AU. In the Sgr A* and S-stars preset, Sgr A* sits at that preset's own origin, so the question doesn't come up there.

## 3. How I built it

### The units

`Constants.hpp` keeps the new units next to the AU:

```cpp
constexpr double SPEED_OF_LIGHT_KMS = 299792.458;  // exact
constexpr double AU_PER_PARSEC = 648000.0 / PI;    // about 206,265 AU
constexpr double AU_PER_LIGHT_YEAR = SPEED_OF_LIGHT_KMS * SECONDS_PER_DAY * DAYS_PER_YEAR / KM_PER_AU;  // about 63,241 AU
```

`AU_PER_LIGHT_YEAR` is built from four other constants, and the compiler works out the product once, while compiling. `Galaxy.hpp` turns its `R0_KPC` into AU with `AU_PER_PARSEC`.

### The zoom limit

```cpp
static constexpr double MAX_DISTANCE = 1e10;  // AU: far enough to see the whole Milky Way
```

### Showing distances

Every distance the interface shows goes through `formatDistance` in `Ui.cpp`: the status bar's "View ... across", how far the Sun has moved in the galaxy view, and the distances in tooltips.

```cpp
std::string formatDistance(double au) {
    if (au >= AU_PER_KPC) return printf_string("%.2f kpc (%s)", au / AU_PER_KPC, lightYears(au).c_str());
    if (au >= PARSEC_FROM_AU)
        return printf_string("%.2f pc (%s)", au / units::AU_PER_PARSEC, lightYears(au).c_str());
    if (au >= 1000.0) return printf_string("%.0f AU", au);
    if (au >= 0.01) return printf_string("%.3f AU", au);
    return printf_string("%.0f km", au * units::KM_PER_AU);
}
```

The checks run from the biggest unit down, so the first one that fits wins. `lightYears` gives one decimal under 10 light-years and none above.

### Code map

| File | What's there |
|---|---|
| `engine/include/emma/Constants.hpp` | `SPEED_OF_LIGHT_KMS`, `AU_PER_PARSEC`, `AU_PER_LIGHT_YEAR` |
| `engine/include/emma/Galaxy.hpp` | `R0_AU` from `units::AU_PER_PARSEC` |
| `app/Camera.hpp` | `MAX_DISTANCE`, 10¹⁰ AU |
| `app/Ui.cpp` | `formatDistance` and `lightYears`; the cursor readout in the status bar |
| `tests/view_tests.cpp` | `camera_galactic_scale` |

### Tests

`camera_galactic_scale` checks three things, each against the mistake it would catch:

1. Zoomed all the way out from the Sun, the Galactic center is in view. That fails if `MAX_DISTANCE` is too small.
2. Projecting a grid point and picking it again lands within 10⁻⁵ of the distance at 10¹⁰ AU. That fails if any part of the camera's math breaks with huge numbers.
3. Zoomed in to 0.01 AU around the Galactic center, the same round trip lands within 10⁻³ of the distance. At that zoom a pixel covers about 10⁻⁵ AU and doubles are 2.4 × 10⁻⁷ AU apart, so it should land within a small part of a pixel. The test prints both errors.

`camera_zoom` checks that the distance stops at `MAX_DISTANCE`. It compares against the constant itself, so the limit can change without editing the test.

## Things to try

1. Turn on the galaxy view and scroll out. Watch "View ... across" in the status bar switch from AU to parsecs at about half a parsec, then to kiloparsecs. Keep going until it reads about 48 kpc.
2. At the same zoom, look at the Sun's dashed heading arrow. It points along the Galactic plane, toward Galactic rotation.
3. Turn the galaxy view off and zoom out past half a parsec. The cursor readout switches to parsecs too.
4. From the solar system, open the Black holes tab and press Go to Sgr A*. The view jumps 8.15 kpc and lands next to the hole with a steady picture.

## Glossary

| Term | Meaning |
|---|---|
| Arcsecond (″) | 1/3,600 of a degree |
| Parsec (pc) | The distance at which 1 AU looks 1″ wide: about 206,265 AU, or 3.26 light-years |
| Kiloparsec (kpc) | 1,000 pc, about 206 million AU |
| Light-year (ly) | How far light goes in a Julian year, about 63,241 AU |
| ulp | The gap between a double and the next one up: 1.1 to 2.2 × 10⁻¹⁶ of the value |

### C++

- A `constexpr` built from other `constexpr`s: `AU_PER_LIGHT_YEAR` is computed from four other constants. The compiler works out the product once, while compiling, so the program never multiplies it again.
- A `static constexpr` limit in a test: `camera_zoom` compares against `Camera::MAX_DISTANCE` itself, not a copied number, so the limit can change without editing the test.
- `printf` formats like `%.2f` and `%.0f` print a number with 2 or 0 digits after the decimal point. `%.0f` rounds to a whole number. `printf_string` in `Ui.cpp` is a small helper that does the same into a `std::string`.

## Sources

- IAU 2015 Resolution B2: the parsec as exactly 648,000/π au.
- IAU 2012 Resolution B2: the astronomical unit, 149,597,870,700 m.
- M. J. Reid et al. (2019), ApJ 885, 131: R₀ = 8.15 kpc.
