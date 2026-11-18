# Sgr A*, the black hole at the Galactic center

This document explains what a black hole is and what you see when you look at one, the problems that shaped how EMMA draws it, and how I built that. The black hole in EMMA is Sagittarius A* (Sgr A*, said "Sagittarius A-star"), at the center of the Milky Way, 8.15 kpc from the Sun. It has about 4.3 million times the Sun's mass. `relativity.md` explains how EMMA turns black holes into bodies that pull, and the preset with the stars that orbit Sgr A*.

## 1. Black holes: what they are and why we need them

### What a black hole is

Gravity gets stronger with more mass and with less distance. To leave the surface of a planet or a star, a rocket has to reach that body's escape speed: 11.2 km/s for Earth and 618 km/s for the Sun. If you squeeze the same mass into a smaller ball, the escape speed at its surface goes up. Squeeze it small enough and the escape speed reaches the speed of light. Nothing can go faster than light, so nothing can leave, and the squeezed object is a black hole.

The radius where this happens is the Schwarzschild radius:

```
Rs = 2GM / c²
```

G is the gravitational constant, M the mass and c the speed of light.

| If you squeezed | into a black hole, its radius would be |
|---|---|
| Earth | about 9 mm |
| The Sun | 2.95 km |
| Sgr A*, 4.3 million Suns | 12.7 million km, or 0.085 AU |

The sphere at that radius is the event horizon. It's a boundary in empty space with no surface. Anything that crosses it, light included, can only go further in, and nothing from inside ever comes back out.

From far away, a black hole pulls like anything else of the same mass. If the Sun turned into a black hole of the same mass, the planets would keep their orbits; they'd only get cold and dark. The strange effects in this document all happen close to the hole, within about 15 Rs, which for Sgr A* is about 1.3 AU.

### Why it belongs in EMMA

The Milky Way has a black hole at its center, and the solar system goes around it every 203 million years (`galaxy-view.md`). A simulator of the Galaxy that left it out would be missing its center. And a black hole is where gravity stops behaving the way Newton described: light bends, clocks slow, and the picture you see is shaped by gravity itself. Drawing it the way it would really look is a way to see general relativity at work.

### Where black holes come from

There are two main kinds.

Stellar black holes come from dying stars. A star holds itself up against its own gravity with the heat from burning its fuel. A star of roughly 20 Suns or more burns through its fuel in about ten million years or less, while the Sun will last ten billion. When the fuel runs out, the core collapses, often in a supernova that blows the outer layers away. If the collapsed core weighs more than about 2 to 3 Suns, nothing can stop the collapse, and it becomes a black hole. Black holes made this way weigh from a few Suns to a few tens of Suns. The Milky Way probably holds millions of them. Most have no gas nearby to light them up, so they're hard to find.

Supermassive black holes sit at the centers of large galaxies and weigh millions to billions of Suns. Sgr A* is one. The black hole in the galaxy M87, the first one ever photographed, weighs about 6.5 billion Suns. Nobody knows for sure how they started. They may have begun as black holes left by the first stars, or as giant gas clouds in the young universe that collapsed straight into black holes. Either way, they grew by swallowing gas and by merging with other black holes when their galaxies merged.

Radio astronomers found Sgr A* in 1974 as a small, bright source of radio waves. Proving it was a black hole took decades of watching stars move around it. The star S2 goes around it every 16 years and passes within 120 AU of it. The orbits of S2 and its neighbors put 4.3 million Suns' worth of mass in a space far too small to be anything but a black hole, and Reinhard Genzel and Andrea Ghez shared the 2020 Nobel Prize in Physics for that work. Later, the GRAVITY instrument followed bright flares of gas close to the hole and found the same mass inside a region smaller than Venus's orbit around the Sun. In 2022 the Event Horizon Telescope (EHT) published the first image of its ring.

### Sgr A* in numbers

| | Size | In AU |
|---|---|---|
| Mass | 4.3 million M☉ | |
| Distance from the Sun | 8.15 kpc | 1.68 × 10⁹ |
| Event horizon radius, Rs | 1 Rs | 0.0849 (12.7 million km) |
| Photon sphere | 1.5 Rs | 0.127 |
| Shadow radius | 2.6 Rs | 0.221 |
| Innermost stable orbit | 3 Rs | 0.255 |

Measuring every length in Schwarzschild radii, so Rs = 1, makes a black hole's shape independent of its mass: one ten times heavier looks the same, only ten times bigger. In EMMA's units, G = 2.959 × 10⁻⁴ AU³ M☉⁻¹ day⁻² and the speed of light is 299,792.458 km/s × 86,400 s / 149,597,870.7 km = 173.14 AU per day, so for Sgr A*:

```
Rs = 2 × 2.959e-4 × 4.3e6 / 173.14² = 0.0849 AU
```

That's about 18 times the Sun's radius, or a fifth of the way from the Sun to Mercury.

### The black disc and the ring

Light near a black hole travels along curved paths. Far from the hole the bending is small. It gets stronger closer in, and at 1.5 Rs light can travel in a full circle around the hole. That radius is the photon sphere. Light coming in from outside that gets closer than this can't get away again.

Now picture yourself looking at the hole from far away, and follow your line of sight backward from your eye. If it passes more than 2.6 Rs from the center, it bends a little and carries on to the stars behind. If it passes within 2.6 Rs (√27/2 Rs, to be exact), it curves in and ends at the horizon. Nothing comes out of the horizon, so no light reaches you along that line, and you see black. All those lines together make a black disc 2.6 times wider than the horizon itself, called the shadow. The hole looks bigger than it is because gravity bends the light around it.

Lines of sight that pass just outside 2.6 Rs skim the photon sphere. Some go around the hole once or more before they come back out. On the way they spend a long time in the hottest gas, the gas closest to the hole, and collect a lot of its light. Together they make a thin, bright ring right at the edge of the shadow, the photon ring.

A black disc with a bright rim and a soft glow around it looks like a solar eclipse. In an eclipse, though, the Moon is a solid object in front of the Sun, and the glow is the Sun's outer atmosphere behind it. Around a black hole nothing sits in front of anything. The black is where no light can come from, and the glow is gas all around the hole, bent into one ring.

The EHT's image of Sgr A* shows a ring 51.8 ± 2.3 microarcseconds (μas) across. EMMA's shadow, 2 × 0.221 AU wide seen from 8,150 pc, covers 0.441 / 8,150 = 54 μas, within about 5% of the measurement. (1 AU seen from 1 parsec is 1 arcsecond, by the definition of the parsec.)

### The light next to the black hole

Seen from just above the gas around it, the picture has these parts:

1. The black disc is the shadow.
2. The orange and yellow glow is hot gas circling the hole. As gas falls inward, friction and magnetic fields inside it turn the energy of the fall into heat, and the hot gas glows. Sgr A*'s gas is thin and sends out its light mostly as radio waves, which your eye can't see. EMMA shows how bright it is in false colors, the scale the EHT uses for its images: black for none, then red, orange, yellow and white as the gas gets brighter.
3. The glow is strongest above the shadow and toward one side, where three effects add up:
   - Light from gas behind the hole bends up over the top on its way to you, so you see the far side of the gas lifted above the shadow.
   - The gas on one side of the hole moves toward you, and on the other side it moves away. The side coming toward you looks brighter and bluer, an effect called Doppler beaming.
   - Gas very close to the horizon looks dimmer, because its light loses energy climbing out of the hole's gravity.
4. The thin bright line along the shadow's edge is the photon ring.
5. The streaks and specks scattered around the hole are background stars, their light bent around the hole and stretched into short arcs.

If you turn the view until you look down the gas's axis, the glow evens out into a ring all the way around, and the picture looks most like an eclipse.

### Why the stars behind it look pushed aside

A star behind the black hole is still visible. Its light passes the hole and bends around it, so you see the star, only in the wrong place.

The bending pushes the star outward. Its light bends toward the hole on the way past, so the star appears farther from the hole than it really is. A star exactly behind the hole spreads into a circle around it, the Einstein ring. Seen from a distance D, the ring's angular radius is

```
θE = √(2 Rs / D)
```

At 28 Rs that's √(2/28) = 0.27 rad, about 15°.

The stars get stretched into arcs, too. Light from a star's near and far edges bends by different amounts, so its image is smeared along the ring, and you see circular streaks around the hole.

There's also a second image of every star, on the opposite side of the hole, from light that went around the other way. These second images sit close to the shadow's edge and are squeezed thin and faint. As you turn the view, a star's second image slides toward the shadow and shrinks until it seems to fade into it. Nothing covers the star: its light has to bend more and more sharply to reach you, so its image gets thinner and thinner. Light that loops around the hole once or more makes still more images, packed even closer to the edge.

Far from the hole, light passing at a distance b bends by

```
α ≈ 2 Rs / b
```

At b = 100 Rs that's 0.02 rad, about 1.1°, so stars far from the hole barely move.

### Doppler beaming

An ambulance siren sounds higher while the ambulance comes toward you and lower once it has passed. The ambulance moves while it sends out each sound wave, so the waves in front of it get squeezed together and the ones behind it get spread apart. This change in pitch is called the Doppler effect.

Light shifts the same way. Light from something moving toward you arrives at a higher frequency, which looks bluer. Light from something moving away arrives at a lower frequency and looks redder. At everyday speeds the change is far too small to see. The gas near Sgr A* moves much faster: at 3 Rs, the inner edge of a thin disk, it orbits at half the speed of light.

At speeds like that, the side coming toward you also gets brighter, for three reasons:

- Each photon, a small packet of light, carries more energy at a higher frequency.
- The photons arrive closer together in time. The gas sends some, moves toward you, then sends more, so they pile up on the way to you.
- The gas throws its light forward, the way it's moving, like a car's headlights. More of the light comes toward you and less goes off to the side.

On the side moving away, all three work in reverse. They combine into one number, the shift factor g: the light's frequency when it reaches you divided by its frequency when it left the gas. Colors scale with g and brightness with g⁴, one g for the energy, one for the arrival rate and two for the forward throw. The brightening is what's called Doppler beaming.

At the inner edge of a thin disk, g runs from 0.47 on the side moving away to 1.41 on the side moving toward you. The near side is up to (1.41 / 0.47)⁴ ≈ 80 times brighter, and bluer: about 9,200 K against 3,000 K.

Gravity shifts the light too. Light loses energy as it climbs away from the hole, whichever way the gas moves, so it arrives redder and dimmer. This is gravitational redshift. Both effects go into g:

```
g = √(1 − 1/r) / (γ (1 − v cos θ)) × 1 / √(1 − 1/D)
```

- √(1 − 1/r) is the gravitational redshift. At 3 Rs it's 0.82.
- γ (1 − v cos θ) is the Doppler part. θ is the angle between the gas's motion and the direction toward you, and γ = 1 / √(1 − v²).
- 1 / √(1 − 1/D) is a small blueshift because the eye itself sits in the hole's pull. At 28 Rs it's 1.018.

The orbit speed of gas at radius r, measured by someone hovering there, is v = √(0.5 / (r − 1)) in units of the speed of light, with r in Rs.

The makers of Interstellar left the Doppler shift out on purpose, so their disk would look even and wouldn't confuse audiences.

### Real gas, a thin disk, or none

Sgr A* is starving. There's hot gas around it, but very little falls in, and what does is too hot and thin to settle into a flat disk. It forms a thick, puffy cloud that glows faintly, mostly in radio waves. That's why Sgr A* is so dim for its size, and why the EHT saw a soft ring.

An accretion disk is gas that has settled into a flat, spinning disk around a black hole, like Saturn's rings but glowing hot. Disks like this form when gas falls in steadily, as in quasars and in black holes that pull gas off a companion star. Sgr A* doesn't have one, but it's what most people picture as a black hole. Its inner edge is at 3 Rs, the innermost stable circular orbit (ISCO): closer in, no circular orbit is stable, and gas plunges straight into the hole. Gas heats up as it spirals inward, following the standard thin-disk profile (Shakura & Sunyaev 1973):

```
flux ∝ (r_in / r)³ × (1 − √(r_in / r))
T    ∝ flux^(1/4)
```

It's hottest at r = 49/36 × r_in ≈ 4.1 Rs. The far side of a disk, behind the hole, should be hidden by it. Light leaving the top of that far side bends over the hole and reaches you, so you see the far side lifted over the shadow like a halo. Light leaving its underside bends under the hole and makes the thin bright arc below the shadow. Jean-Pierre Luminet first worked out this picture in 1979, by hand.

With nothing glowing around the hole, only the background stars are left. What you see is pure gravitational lensing: the shadow, the Einstein ring, and the stretched and doubled stars. Most black holes in the Galaxy have no gas around them and would look like this.

### Ray tracing: following light backward

To draw what you'd see, follow the light. For each pixel, follow the light backward from the eye through the curved space and see where it came from:

- A line of sight that ends inside the horizon gives a black pixel.
- One that escapes gives the color of the sky in the direction it finally points.
- One that passes through glowing gas picks up that gas's light on the way.

Nothing is painted on. The black disc, the bright ring and the bent stars all come out of the paths. Light really travels the other way, toward the eye, but a light path is the same in both directions, and starting at the eye means only rays that reach the screen get traced.

Around a black hole that doesn't spin, general relativity gives the path of a light ray as, with lengths in Rs:

```
u'' + u = (3/2) u²
```

Here u = 1/r, and '' is the second derivative with respect to the angle around the hole. Without the right-hand side, this is the equation of a straight line.

## 2. Problems and decisions

### Drawing what gravity does to light

Problem: no flat drawing can show the lifted far side of a disk, the arc underneath, the doubled stars or the way all of them change as you turn the view.

Decision: ray trace it. Every pixel follows its own light ray backward from the eye, on the graphics card, through a fragment shader: a small program the graphics card runs once for every pixel.

### Solving the light-ray equation in a shader

Problem: the light-ray equation is written in u = 1/r against the angle around the hole, which is awkward to step through for a ray heading anywhere in 3D.

Decision: a trick from Riccardo Antonelli's ray tracer Starless. Move the ray through ordinary 3D space under a made-up pull toward the hole, (3/2) h² / r⁴, where h = |x × v| is the ray's angular momentum, which stays the same all along it. Any pull toward a center gives an orbit whose shape follows Binet's equation, u'' + u = f / (h² u²), the same equation that gives the planets their ellipses. With f = (3/2) h² u⁴ it becomes u'' + u = (3/2) u², the light-ray equation. So a plain Newtonian integrator traces paths with the exact shape of light rays around the hole. The speed along a path means nothing; only its shape matters.

As a check, a circular orbit needs the pull to equal v²/r. With h = r v the pull is (3/2) h² / r⁴ = (3/2) v² / r², and that equals v²/r at r = 1.5, the photon sphere.

### Stepping along the ray

Problem: far from the hole a ray barely bends, so short steps there waste time. Near the photon sphere it bends sharply, so long steps there go wrong.

Decision: velocity Verlet, the same method as EMMA's default integrator, with each step 6% of the distance to the hole, so steps are long far away and short near the photon sphere, and never under 0.01 Rs. A ray takes at most 600 steps. The step isn't capped at any maximum length, so the shader works from any distance: from 2,700 Rs, the farthest it runs, a ray needs at most about 230 steps. A ray stops when it's inside the horizon, when it's moving outward beyond 1.1 times the eye's distance (and at least 40 Rs), or when a thin disk has covered it completely.

### Checking a shader

Problem: a shader runs on the graphics card, and a test can't run it.

Decision: `BlackHole.hpp` has the same ray step in C++, line for line, so the tests check the math the shader uses.

### The sky in every direction

Problem: a bent ray can end up pointing anywhere, off screen or behind the camera included, so the shader needs the whole sky at once.

Decision: the first time it's needed, EMMA draws its background stars, the same 21,000 as the starfield, into one 4096 × 2048 image with Galactic longitude across and latitude down: an equirectangular map. The map stretches the sky toward the poles, so stars there are drawn wider, by 1/cos(latitude), and every star covers at least one pixel of the map so none is too small to show up.

### Precision 1.68 billion AU from the origin

Problem: in the solar system, Sgr A* is 1.68 × 10⁹ AU from the origin. A float can't hold positions that far out finely enough to draw.

Decision: work out the eye's position relative to the hole in double precision first, `(camera.target - center) + camera.eyeOffset()`. The difference between two big positions is small, and fine in a float. Then divide by Rs.

### When to ray trace

Problem: far away, the shadow is smaller than a pixel, and tracing every pixel of the view for it would waste the graphics card.

Decision: ray trace once the true shadow is at least 1 px in radius, which with the default window is within about 2,700 Rs, or 230 AU of Sgr A*. Farther out, draw a flat marker: a black disc in an orange ring, brighter on the side whose gas moves toward the eye, 9 px in radius with its ring.

### Keeping it smooth

Problem: tracing every pixel every frame is heavy, and most frames the view hasn't changed.

Decision: render into a picture and keep it. EMMA renders again only when the eye, the view, the window or a setting changes. While they keep changing it renders at half resolution, and 0.25 s after they stop, it renders once more at full resolution. If shaders aren't available, EMMA uses the flat drawing up close too, with the black disc at its true size.

### Which gas to show

Problem: the real gas around Sgr A* is a dim, thick cloud. It's what's observed, but a still picture of it is a glowing ring around a black disc, and it shows the bending of light less clearly than a thin disk does.

Decision: three looks, real gas, thin disk and no disk, with the real gas as the default, because that's what Sgr A* looks like. The thin disk shows the ray tracing at its clearest.

### Modeling the real gas

Problem: a full simulation of the gas around a black hole is a research project of its own.

Decision: a few simple rules with the right shape:

- At each point the gas glows with j = r^−2.5 × exp(−z² / 2H²), where z is the height above the cloud's midplane and H = 0.6 r. It's bright near the hole and fades outward, and it's thick enough to look round from every side.
- The gas is see-through, so the glow from every point along a line of sight adds up. Lines of sight that skim the photon sphere run longest through the brightest gas, which is why the photon ring stands out.
- The gas circles the hole at 80% of the circular orbit speed, so one side is brighter.
- The colors are the EHT's false colors.
- The gas's axis is tilted 35° from the line of sight from Earth. The EHT's comparisons with simulations rule out views more tilted than 50°, and the exact angle isn't known, so I chose 35°.

EMMA shows what a perfectly sharp camera would see. The EHT's ring is blurrier, 30 to 50% as wide as it is across, partly because even a telescope the size of Earth can't see finer detail.

### Modeling the thin disk

Problem: a real disk's temperature depends on how fast gas falls in, which an illustration doesn't have.

Decision: the inner edge at 3 Rs, the ISCO; the outer edge at 14 Rs, fading out from 9 Rs; the standard thin-disk profile; the hottest point set to 6,500 K so the colors read well on screen; and the disk opaque, covering 97% of whatever lies behind it where a line of sight crosses it.

### Getting there

Problem: Sgr A* is 8.15 kpc from the solar system, too far to scroll to.

Decision: a Go to Sgr A* button in the Black holes tab. It sets Follow to Sgr A*, puts the eye 28 Rs from the hole, close enough to see light bend, and turns the view so the gas's axis points up the screen. It then tips the view so the eye sits 7° above the disk. EMMA's camera can't roll, and this is how the disk ends up lying flat across the picture. While you follow Sgr A*, the eye can't get closer than 3 Rs.

### Seeing what gravity does

Problem: the picture is so different from a flat one that it's hard to tell which parts are gravity's doing.

Decision: two switches. Untick Bend light and the light travels in straight lines, as if gravity didn't affect it: the shadow shrinks to the horizon itself, the thin disk turns into a flat ellipse, and the stars stay where they are. Untick Doppler and the gas glows the same in every direction, with only the gravitational redshift left: the Interstellar version.

## 3. How I built it

### One ray per pixel

The camera is EMMA's `Camera`, with a 45° field of view and right, up and forward axes. For each pixel the shader builds a direction and starts a ray at the eye:

```glsl
vec2 q = gl_FragCoord.xy - uCenter;
vec3 v = normalize(uForward * uFocal + uRight * q.x + uUp * q.y);
vec3 x = uEye;
```

### The pull

```glsl
vec3 pull(vec3 x, float h2) {
    float r2 = dot(x, x);
    return -1.5 * h2 * x / (r2 * r2 * sqrt(r2)) * uBend;
}
```

`uBend` is 1 with Bend light on and 0 with it off, so unticking it sends light straight.

### Stepping

```glsl
float dt = max(0.06 * r, 0.01);
vec3 xn = x + v * dt + 0.5 * a * dt * dt;
vec3 an = pull(xn, h2);
vec3 vn = v + 0.5 * (a + an) * dt;
```

That's velocity Verlet: move with the current acceleration, work out the new one, then update the velocity with the average of the two.

### The sky

```glsl
float lon = atan(d.y, d.x);
float lat = asin(clamp(d.z, -1.0, 1.0));
return texture2D(uSky, vec2(lon / (2.0 * PI) + 0.5, 0.5 - lat / PI)).rgb;
```

`Renderer::bakeSky` draws the stars into the map once and copies it into a plain `sf::Texture`, which has its first row at the top, as this lookup expects.

### The real gas

For each step of a ray, the shader adds the glow of the gas at the step's midpoint:

```glsl
glow += j * g * g * g * g * length(xn - x);
```

`j` is the gas's glow there, `g⁴` the Doppler brightening, and `length(xn - x)` how far the ray went through it.

### Drawing it

`Renderer::drawBlackHole` runs right after the starfield:

1. The eye's position relative to the hole is worked out in double precision, then divided by Rs.
2. If the true shadow would be at least 1 px in radius, the shader runs over the whole view. Otherwise the hole is a flat ring marker.
3. The first time the shader is needed, `bakeSky()` draws the sky map.
4. `app/BlackHoleShader.hpp` holds the shader as a string, so there's no separate file to lose. It's written for GLSL 1.10, the version SFML uses. Every direction it gets is turned into Galactic coordinates first, the sky map's frame.
5. The picture is rendered into an `sf::RenderTexture` and kept, and rendered again only when a key of 19 numbers, the eye, the view, the window and every setting, changes.

`main.cpp` draws black holes before everything else, since a ray-traced one covers the whole view. Only the hole that looks biggest is ray traced; the others are drawn as markers.

### Code map

| File | What's there |
|---|---|
| `engine/include/emma/BlackHole.hpp` | `schwarzschildRadiusAU`, `PHOTON_SPHERE`, `SHADOW_RADIUS`, `ISCO`; the ray step (`Ray`, `startRay`, `stepRay`, `traceRay`), the same as the shader's; `orbitSpeed` and `frequencyShift` |
| `engine/include/emma/Galaxy.hpp` | `SGR_A_MASS`, `galacticCenter()`, `sgrASpinAxis()` |
| `app/SceneFrame.hpp` | `galacticCenter()` in scene coordinates; `toGalactic` and `fromGalactic` for directions |
| `app/Camera.hpp` | `rightAxis()`, `upAxis()`, `forwardAxis()`, `eyeOffset()` |
| `app/BlackHoleShader.hpp` | the shader |
| `app/Renderer.hpp/.cpp` | `drawBlackHole`, `traceBlackHole`, `bakeSky`, `drawFlatBlackHole`, `shadowPixels` |
| `app/Ui.hpp/.cpp` | the Black holes tab, Sgr A* in the Follow list, its label and tooltip |
| `app/main.cpp` | Go to, following a hole, drawing holes after the starfield, labels and tooltips |
| `tests/physics_tests.cpp` | the tests below |

### Tests

1. `black_hole_radius`: Rs for 4.3 million M☉ is 0.084887 AU, and the Sun's is 2.9533 km, from G and c in `Constants.hpp`. A wrong unit for the speed of light would fail it.
2. `black_hole_shadow`: a ray aimed 2.5 Rs off center falls in, and one aimed 2.7 Rs off center escapes. A wrong factor in the pull would fail it.
3. `black_hole_photon_sphere`: a ray started sideways at r = 1.5 stays within 2% of that radius for a quarter turn. The circle is unstable, so the test follows it for a quarter turn.
4. `black_hole_bending`: a ray passing 1,000 Rs away bends by 2/b to within 2%.
5. `black_hole_doppler`: at r = 3 the gas moves at 0.5 c, and g is √2 = 1.414 for gas moving straight toward you and √2/3 = 0.471 for gas moving straight away. A flipped sign would fail it.

## Things to try

1. Open the Black holes tab and press Go to Sgr A*. Pick No disk and right-drag slowly so a bright star passes behind the hole. It splits into two images, one outside the Einstein ring and one squeezed against the shadow. When the star is exactly behind the hole, the two join into a ring.
2. With the thin disk, untick Bend light. The lifted far side disappears, the disk turns into a flat ellipse, and the black disc shrinks to the horizon.
3. With the thin disk, untick Doppler to get the Interstellar look.
4. With the real gas, turn the view until you look down the gas's axis. The glow evens out into a ring, like an eclipse.
5. Scroll out from 3 Rs to a few thousand Rs. The Einstein ring shrinks toward the hole, and past about 230 AU the ray-traced picture hands over to the flat marker.

## Glossary

### Black holes

| Term | Meaning |
|---|---|
| Event horizon | The boundary nothing comes back out of, not even light |
| Schwarzschild radius (Rs) | The event horizon's radius for a black hole that doesn't spin, 2GM/c² |
| Photon sphere | 1.5 Rs, where light can travel in a circle |
| Shadow | The black disc you see, about 2.6 Rs in radius. Every line of sight aimed inside it ends at the horizon |
| Photon ring | The thin bright rim at the shadow's edge, from light that skims the photon sphere |
| Impact parameter (b) | How far from the center a ray is aimed, as if it went straight |
| Accretion disk | Gas spiraling into a black hole in a flat, spinning disk |
| ISCO | The innermost stable circular orbit, 3 Rs for a black hole that doesn't spin |
| Stellar black hole | One left by the collapse of a heavy star, a few to a few tens of Suns |
| Supermassive black hole | One at the center of a galaxy, millions to billions of Suns |
| Supernova | The explosion of a heavy star when its core collapses |
| Kerr black hole | A spinning black hole. A Schwarzschild black hole is one that doesn't spin |

### Light

| Term | Meaning |
|---|---|
| Photon | A small packet of light |
| Doppler effect | Light or sound shifted higher when its source comes toward you and lower when it moves away |
| Doppler beaming | Light from a fast source coming toward you arriving brighter, and from one moving away arriving dimmer |
| Gravitational redshift | Light losing energy as it climbs away from a mass |
| Gravitational lensing | Light bending around a mass, which moves, stretches and copies what's behind it |
| Einstein ring | The circle a source exactly behind a lens spreads into |
| Optically thin | See-through, so light from everything along the line of sight adds up. Optically thick is opaque, and you see only the surface |
| False color | Colors given to light the eye can't see, such as radio |
| Microarcsecond (μas) | A millionth of an arcsecond |

### Rendering and math

| Term | Meaning |
|---|---|
| Ray tracing | Making a picture by following light rays, here backward from the eye |
| Equirectangular map | An image of the whole sky with longitude across and latitude down, the same layout as the planet textures |
| Binet's equation | The equation for the shape of an orbit under a pull toward a center, written in u = 1/r |
| Fragment shader | A small program the graphics card runs once for every pixel |
| Uniform | A value the program passes to a shader that's the same for every pixel, such as the camera's position |

## Sources

- B. Balick & R. L. Brown (1974), ApJ 194, 265: the discovery of Sgr A* as a compact radio source.
- Event Horizon Telescope Collaboration (2019), ApJL 875, L1: the first image of a black hole, M87*.
- Event Horizon Telescope Collaboration (2022), ApJL 930, L12: the Sgr A* ring, 51.8 ± 2.3 μas across, and views more tilted than 50° disfavored.
- Event Horizon Telescope Collaboration (2022), ApJL 930, L15: the ring's width, 30 to 50% of its diameter.
- GRAVITY Collaboration (2023), A&A 677, L10: the mass of Sgr A* inside a region smaller than Venus's orbit, and flares near it.
- M. J. Reid et al. (2019), ApJ 885, 131: R₀ = 8.15 kpc.
- J.-P. Luminet (1979), A&A 75, 228: the first image of a thin disk around a black hole.
- N. I. Shakura & R. A. Sunyaev (1973), A&A 24, 337: the thin-disk temperature profile.
- O. James, E. von Tunzelmann, P. Franklin & K. S. Thorne (2015), Classical and Quantum Gravity 32, 065001: the black hole in Interstellar, and why its disk leaves out the Doppler shift.
- R. Antonelli, Starless, https://rantonels.github.io/starless/: the trick for tracing light rays with a pull toward the center.
