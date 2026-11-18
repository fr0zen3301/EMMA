# IAS15: the accurate integrator

This document explains IAS15, EMMA's second integrator: what it is, how it works and why EMMA needs it, the problems that shaped how I set it up, and how I built it in C++. IAS15 (Rein & Spiegel 2015) is built for accuracy: its errors stay near the rounding limit of a double, about 16 digits. It plugs into the same job description as Verlet (`integrator-interface.md`), and `energy-drift.md` compares the two.

## 1. IAS15: what it is and why we need it

### Why a second integrator

Verlet looks at the forces at the start and the end of each piece and assumes the acceleration changes in a straight line in between. It's cheap and good enough for a nice-looking simulation, and its errors stay bounded, but they're not tiny.

Some things need much more. Through a close pass the forces change fast, and Verlet's straight-line guess is far off. And some effects are tiny: relativity turns Mercury's orbit by only 43″ a century. An integrator's own error can turn an orbit too, so to measure an effect like that, the simulation's errors have to be far smaller than the effect.

IAS15 is built to be as accurate as a computer can be. The price is work: it works out the forces 15 to 25 times per piece, where Verlet does it once. For the solar system's 10 bodies that's nothing. With hundreds of bodies you'd feel it. So IAS15 is the precise option, not the fast one.

The name says what it is: an Integrator with Adaptive Step-size control, 15th order. 15th order means halving the length of a piece divides the method's error by 2¹⁵, about 33,000.

### The idea: follow the curve

Think of Earth during one piece. Its acceleration isn't constant: it changes smoothly as Earth moves. IAS15 describes that change with a curve, a polynomial in time:

```
a(t) = a0 + b0·t + b1·t² + b2·t³ + ... + b6·t⁷        (t runs from 0 to 1 across the piece)
```

If you know this curve, you know exactly where the body ends up: integrate it once for the velocity and twice for the position. The whole job is finding the right curve, and IAS15 does it in five moves:

1. Guess a curve. The first guess is the last piece's curve, moved forward in time.
2. Check the guess at 7 moments inside the piece: move every body to where the curve says it would be then, and work out the real forces there.
3. Fix the curve so it matches those forces.
4. Repeat 2 and 3 until the curve stops changing, usually 2 or 3 rounds.
5. Move every body along the final curve to the end of the piece.

Moves 1 to 4 are called a predictor-corrector: predict, correct, and again.

### Why those 7 moments

The moments are at 5.6%, 18%, 35%, 55%, 73%, 89% and 98% of the piece, plus the start. That's Gauss-Radau spacing. With the start fixed as one of the samples, it's the spacing that fits a curve most accurately for this many samples. The numbers never change.

### Two ways to write the same curve

A curve can be written in more than one form, and each is good for something different:

| Form | Good for |
|---|---|
| Powers of t: b0·t + b1·t² + ... | Integrating, to get positions and velocities |
| Newton form: g0·t + g1·t(t − h1) + g2·t(t − h1)(t − h2) + ... | Filling in from samples, one at a time |

The Newton form has a handy property: when the sample at moment n comes in, it decides one coefficient, g(n−1), and changes nothing else.

### The warm start, and when to stop

The curve for the next piece is probably close to the last one. So IAS15 can start from the last curve, moved to where the last piece ended and stretched if the new piece is a different length. With that guess, the predictor-corrector usually needs only 2 or 3 rounds instead of 6 or more.

After each round, IAS15 looks at how much the curve's highest term changed, compared with the biggest acceleration. When it changes by less than about 10⁻¹⁶, the curve has settled.

### Choosing the length of a piece

After the curve settles, its last term, the t⁷ part, shows how hard the curve had to work:

- In calm motion, a planet far from everything, the last term is tiny. The curve fits easily, so the piece could be longer.
- When forces change fast, in a close pass, the last term is big. The curve is struggling, so the piece should be shorter.

The last term, compared with the acceleration, grows like the piece length to the 7th power. So IAS15 can work backward: how long a piece would make that ratio exactly 10⁻⁹? That length is its suggestion for the next piece.

In the solar system at 0.1 day per step, IAS15 is happy with longer pieces, so each step is a single piece. Through a close pass the pieces shrink as the bodies approach, and grow back after.

## 2. Problems and decisions

### How accurate to aim for

Problem: IAS15 needs a target: how small should the curve's last term be, compared with the acceleration? Smaller means shorter pieces and more accuracy, larger means the opposite.

Decision: 10⁻⁹, the value REBOUND, the reference code for IAS15, uses by default. It's the one knob, `EPSILON`.

### Turning samples into the curve

Problem: the samples fill in the Newton form one coefficient at a time, but positions and velocities come from the powers-of-t form. Converting between them needs a 7 × 7 table of long numbers, and typing in 49 long numbers by hand is an easy way to get one wrong.

Decision: IAS15 keeps both forms, updates the Newton form as each sample comes in, and turns each change into a change of the powers-of-t form with the table. The table is computed once, when the program starts, by multiplying out t(t − h1)(t − h2)... the way you would by hand. There's nothing to copy.

### Starting each piece well

Problem: from a cold start, with no guess, the predictor-corrector needs 6 rounds or more per piece. But each body's curve is stored at the body's place in the list, so when bodies are added, cleared or merged, the stored curves would belong to the wrong bodies.

Decision: the warm start. Each piece starts from the last piece's curve, and the stored curves are thrown away whenever the list of bodies changes.

### When to stop refining

Problem: the curve settles, but never perfectly: at some point rounding noise in the last digits keeps it from improving.

Decision: three rules. Stop when the highest term changes by less than 10⁻¹⁶ of the biggest acceleration. From the third round on, stop when it's no smaller than last round, because that's rounding noise. And stop after 12 rounds whatever happens.

### A piece that turns out far too long

Problem: IAS15 only learns how long a piece should have been after it has done it.

Decision: if the suggestion is under a quarter of the piece just taken, the piece is thrown away and redone shorter. The bodies haven't really moved yet, because their positions are always worked out from where the piece started, so redoing it is safe. The failed curve, stretched to the shorter piece, makes a good first guess.

### Growing too fast

Problem: one calm moment could suggest a giant next piece, right before things get busy again.

Decision: the next piece can be at most 4 times as long as this one.

### Landing on the step

Problem: the simulation asks for exactly one step's worth of days, because the clock and the trails depend on it, but IAS15 chooses its own piece lengths.

Decision: the last piece is cut to land exactly on the step. A piece cut short that way wasn't IAS15's choice, so it doesn't pull the next piece's length down.

### One piece length for every body

Problem: different bodies' forces change at different rates. Mercury's acceleration changes much faster than Neptune's.

Decision: one piece length for every body, set by comparing the biggest last term with the biggest acceleration across all of them. It's simple, and it's enough for EMMA's systems.

### Many bodies

Problem: above 64 bodies, the forces come from the Barnes-Hut tree, an approximation that shifts slightly between rounds. IAS15 reads those small jumps as fast-changing forces and makes its pieces very short.

Decision: IAS15 says it needs exact forces, and above 64 bodies EMMA switches to Verlet (`integrator-interface.md`).

### Switching from the sidebar

Problem: the sidebar needs to switch integrators, but if it changed the simulation directly, changes could happen in the middle of drawing a frame, in any order.

Decision: the same pattern as every other choice in the sidebar: the sidebar only asks, by setting a request, and `main.cpp` carries it out between frames, in one place.

## 3. How I built it

### In three parts

I built IAS15 in three parts, testing each before the next:

1. The math, taking exactly one IAS15 piece per simulation step: the curve, the samples and the predictor-corrector. That was enough to measure IAS15's accuracy against Verlet's in the tests.
2. Choosing the piece length, with the redo, the growth limit and the cut at the end of the step. With this, IAS15 shortens its pieces through a close pass and grows them back after.
3. The switch in the sidebar, and the integrator's name in the status bar.

### The constants

```cpp
static constexpr int MAX_ITERATIONS = 12;   // most refinement passes per piece
static constexpr double CONVERGED = 1e-16;  // stop refining when the curve changes less than this
static constexpr double EPSILON = 1e-9;     // target accuracy: sets the piece length
static constexpr double SAFETY = 0.25;      // redo a piece that was far too long; grow at most 4x
static constexpr int MAX_SUBSTEPS = 4096;   // most pieces one step can be split into
```

### A tour of IAS15Integrator.cpp

- `NODES`: the 8 moments, the start plus the 7 Gauss-Radau ones.
- `makeNewtonToPower()` and `NEWTON_TO_POWER`: the table that turns a change in the Newton form, `g`, into a change in the powers-of-t form, `b`.
- `choose(n, k)`: n choose k, used when the curve is moved forward to the next piece.
- `positionAt` and `velocityAtEnd`: where a body is, and how fast it moves, following the curve.
- `step`: splits a simulation step into pieces of the length IAS15 asks for, cuts the last one to land on the step, and runs `afterPiece` after each.
- `piece`: one piece. It makes the first guess, calls `converge`, redoes the piece shorter if needed, then moves the bodies.
- `converge`: the predictor-corrector loop, with the stopping rules.
- `suggestStep`: the next piece's length.
- `rescale`: stretches the curve to a shorter piece, for a redo.
- `predictNextStep`: the warm start.

### Choosing the next piece

The redo, the growth limit and the cut at the end of the step meet in three lines of `step`:

```cpp
const double taken = piece(world, gravity, h, minPiece);
// Next piece: IAS15's suggestion, but grow at most 4x at a time
const double chosen = taken < h ? taken : std::max(taken, wanted);
nextStep = std::min(suggested, chosen / SAFETY);
```

`taken` is shorter than `h` only when the piece was redone, which is IAS15's own choice. A piece cut to land on the step counts as the length IAS15 wanted.

### The switch in the sidebar

1. `UiState` in `app/Ui.hpp` has `int integratorRequested = -1;`, next to `presetRequested`.
2. The Physics tab's switch sets `integratorRequested` to 0 or 1 when the choice changes. To show which one is active, `drawSidebar` gets `sim.getIntegrator().name()`, the same way it gets `sim.preset()`.
3. Each frame, `main.cpp` checks the request, swaps the integrator, and sets the request back to −1 so it only happens once:

```cpp
if (state.integratorRequested >= 0) {
    if (state.integratorRequested == 1) sim.setIntegrator(std::make_unique<IAS15Integrator>());
    else sim.setIntegrator(std::make_unique<VerletIntegrator>());
    state.integratorRequested = -1;
    baselineChange = ~0ull;  // start the drift readout from a new baseline
}
```

### The C++ in it

#### std::array<T, N>: a list with a fixed size

```cpp
using Coefficients = std::array<Vector3D, 7>;   // always exactly 7 vectors
```

`std::vector` can grow and shrink. `std::array` has a size fixed while compiling. Every body always has exactly 7 coefficients, so `std::array` fits, and it's a bit faster because it needs no extra memory of its own.

`std::array<std::array<double, 7>, 7>` is an array of arrays: a 7 × 7 table, like `NEWTON_TO_POWER`.

#### using Name = Type: a nickname for a type

```cpp
using Coefficients = std::array<Vector3D, 7>;
std::vector<Coefficients> b;      // reads much better than std::vector<std::array<Vector3D, 7>>
```

It doesn't create a new type; it just gives a long one a short name.

#### namespace { ... }: private to this file

Everything inside the unnamed `namespace { }` at the top of the `.cpp`, `NODES`, `choose`, `positionAt` and the rest, can only be used in this one file. Other files can't see it, so names can't clash. EMMA's other `.cpp` files do the same.

#### constexpr: known while compiling

```cpp
constexpr std::array<double, 8> NODES = { ... };
```

`const` means "never changes". `constexpr` goes further: "the value is known while compiling", so the compiler can bake it straight into the program.

`NEWTON_TO_POWER` is only `const`, not `constexpr`, because a function, `makeNewtonToPower()`, computes it. It runs once, when the program starts, before `main`.

#### const Vector3D&: look without copying

```cpp
Vector3D positionAt(const Vector3D& x0, const Vector3D& v0, ...)
```

- `&` means pass a reference: the function looks at the caller's original instead of getting its own copy.
- `const` promises not to change it.

For a small `Vector3D` a copy would be cheap anyway. For a `std::array` of 7 vectors it starts to matter. It's the normal way to pass anything bigger than a number.

#### for (Coefficients& c : b): why the & matters

```cpp
for (Coefficients& c : b) {
    ...
    c = shifted;     // changes the real b[i]
}
```

This loop goes through every element of `b`. With the `&`, `c` is each element, so `c = shifted` changes `b` itself. Without the `&`, `c` would be a copy: the loop would change the copy and throw it away, and `b` would never change, with no error and no warning.

#### static_cast<double>(k): an explicit conversion

`k` is a `std::size_t`, the unsigned whole-number type used for sizes and indices. `static_cast<double>(k)` says "turn this into a double, I know what I'm doing". The rest of EMMA's code does the same.

### Tests

1. `ias15_earth_year`: one Earth year takes 365.2564 days, as with Verlet.
2. `ias15_conservation`: ten years of the solar system; the energy and the angular momentum drift by less than 10⁻¹⁰. When I wrote this, it measured 1.85 × 10⁻¹⁴, against Verlet's 2.61 × 10⁻⁸.
3. `ias15_close_pass`: two planets miss each other narrowly, and the energy changes by less than 10⁻⁷ (Verlet: 1.92 × 10⁻⁵).
4. `ias15_head_on_merge`: a merge keeps mass and momentum exactly.
5. `ias15_guard`: above 64 bodies, Verlet takes over.
6. `mercury_precession` and `s2_precession` use IAS15 to measure relativity's turn of an orbit.

To see one test's numbers, run `build\Release\physics_tests ias15_conservation`. `ias15_conservation` prints "10 years with IAS15: worst energy drift ...": compare it with Verlet's line in `conservation`. `ias15_close_pass` prints "close pass: steps split into up to N pieces, energy change ...": compare it with Verlet's `close_pass` line. What these numbers mean is in `energy-drift.md`.

## Things to try

1. Load the solar system, open the Physics tab and switch between Verlet and IAS15 at a few years per second. Watch the energy drift readout and its graph each time.
2. Run `build\Release\physics_tests close_pass` and then `build\Release\physics_tests ias15_close_pass`, and compare how many pieces each takes and how much the energy changes.
3. Load Sgr A* and the S-stars, set Exaggerate to 100, and compare the two integrators through S2's close pass.

## Glossary

| Term | Meaning |
|---|---|
| Polynomial | A curve made of powers of t: a + b t + c t² + ... |
| Predictor-corrector | Guess an answer, check it, fix it, repeat until it settles |
| Gauss-Radau spacing | The moments to sample at for the most accurate fit, with the start as one of them |
| Newton form | A way of writing a polynomial that's easy to fill in from samples |
| Warm start | Starting from the last answer instead of from nothing |
| Order | How fast the error shrinks with shorter pieces |
| Machine precision | The limit set by rounding: a double keeps about 16 digits |

## Sources

- H. Rein & D. S. Spiegel (2015), MNRAS 446, 1424: IAS15.
- REBOUND, https://github.com/hannorein/rebound: the reference code IAS15 comes from.
