# Integrators: how EMMA moves time forward

This document explains what an integrator is and why a simulation needs one, the problems that shaped how EMMA's two integrators fit together, and how I built that in C++. It's also where EMMA's code uses inheritance and virtual functions, so the last part explains each C++ feature from the beginning. The two integrators themselves are explained in `energy-drift.md`, which compares how accurate they are, and `ias15.md`, which explains how IAS15 works.

## 1. Integrators: what they are and why we need them

### Moving forward in small steps

Newton's laws give each body's acceleration from the forces on it at one moment. They don't directly say where the bodies will be a day later, because the forces change as the bodies move. So a simulation goes forward in small steps: use the acceleration to change each body's velocity a little, use the velocity to change its position a little, work out the new forces, and repeat. This is integration, and the method that does it is an integrator.

Every simulation of moving bodies needs one. Without it you'd only know the forces at the start, never where anything goes.

### Speed or accuracy

Shorter steps follow the real motion more closely but take more work. Different integrators make that trade-off differently: some take one cheap guess per step, others work much harder per step to be far more accurate.

### EMMA's two integrators

Velocity Verlet is fast. It works out the forces once per piece of a step, and its energy error wobbles around a fixed value instead of growing, so it's good for long runs and many bodies.

IAS15 is accurate. It works out the forces 15 to 25 times per piece and keeps its errors near the rounding limit of a double. That's worth it for a few bodies, close encounters, and checking results, such as relativity's turn of S2's orbit (`relativity.md`).

## 2. Problems and decisions

### Two integrators, one simulation

Problem: the rest of the simulation shouldn't care which integrator is running. If it did, every place that touches the integrator would need an "if Verlet, do this, else if IAS15, do that", and adding a third method would mean finding and changing all of them.

Decision: I split the work into three roles.

- `Integrator` is a job description: whoever takes the job must be able to step the world forward, be told when the bodies change, and give its name.
- `VerletIntegrator` and `IAS15Integrator` are two workers who take that job.
- `Simulation` hires someone who fits the job description. It doesn't know or care which one it has.

That makes three things possible: the sidebar can swap integrators at any moment, the tests can run the same scenarios through both, and a third integrator would need no change to `Simulation` at all.

### Swapping while the simulation runs

Problem: the Physics tab can switch integrators at any moment, and a new integrator knows nothing about the bodies it's given.

Decision: every swap goes through one function, `setIntegrator`, which hands the new integrator over and tells it the bodies are new to it.

### Many bodies

Problem: above 64 bodies, EMMA gets the forces from a Barnes-Hut tree, which approximates far-away groups of bodies. The approximation shifts a little each time the tree is rebuilt, and IAS15 reads those small jumps as fast-changing motion and makes its pieces very short.

Decision: each integrator says whether it needs exact forces. IAS15 says yes, and above 64 bodies `Simulation` switches it to Verlet by itself. The sidebar says so.

### Collisions and trails inside a step

Problem: both integrators split a step into smaller pieces when they need to, through a close pass for example. Collisions have to be checked and trail points recorded after every piece, or a fast body could skip through another between two pieces. But collisions and trails aren't the integrator's job.

Decision: `Simulation` passes the integrator a small function, `afterPiece`, and the integrator runs it after every piece.

### Bodies that disappear in the middle of a step

Problem: each integrator keeps some data per body, stored by the body's place in the list. A merge removes a body and shifts every body after it, so that data no longer belongs to the right bodies.

Decision: after a merge, `Simulation` calls the integrator's `forgetHistory`, and the integrator throws that data away.

### A name that can't be misused

Problem: `Simulation` holds its integrator in a `std::unique_ptr`, and `unique_ptr` has a function of its own called `reset()`, which deletes the object it owns. If the integrator's "the bodies changed" function were also called `reset`, then `integrator.reset()`, with a dot instead of an arrow, would still compile. It would delete the integrator, and the next step would crash.

Decision: I named the function `bodiesChanged()`. Now `integrator.bodiesChanged()` with a dot is a compile error, "unique_ptr has no member bodiesChanged", which takes seconds to fix.

### The path preview

Problem: the dashed preview of a new body's path runs every frame while you hover, 60 times a second, so it has to be cheap. The real Verlet looks at every pair of bodies, Earth and Moon included, so in the solar system its pieces are never longer than about 0.13 day. Hovering out near Neptune, the preview looks one Neptune orbit ahead, about 165 years: with the real integrator that's hundreds of thousands of pieces per frame.

Decision: `Simulation::predictPath` keeps its own small copy of Verlet. Its step rule only looks at how close the new body is to each of the heaviest bodies, and ignores the Moon circling Earth, which the preview doesn't need. That's at most 8,000 pieces per frame. It uses the same forces as the bodies feel, relativity included, so the preview shows the path the body will really take. Same method, different job.

### Reorganizing without changing anything

Problem: EMMA started with a single Verlet class built into `Simulation`. Turning that into an interface had to leave the physics exactly as it was: the same steps, in the same order, with the same numbers. Tests that pass show nothing broke, but not that nothing changed.

Decision: I compared the tests' printed output before and after the change, line by line. Identical output means the same numbers down to the last digit.

### VS Code and new headers

Problem: when I added `VerletIntegrator.hpp`, VS Code underlined an include in red, `cannot open source file "emma/Vector2D.hpp"` (EMMA was still 2D then), even though the code built. VS Code's helper, IntelliSense, learns the include folders from `build/compile_commands.json`, a list CMake writes with the settings for every `.cpp` file. Headers aren't in that list; for a header, IntelliSense borrows the settings of a `.cpp` that includes it. Nothing included the new header yet, so it had nothing to borrow.

Decision: `Simulation.hpp` includes `VerletIntegrator.hpp`, so IntelliSense can borrow settings after the next build, and `.vscode/c_cpp_properties.json` has `"includePath": ["${workspaceFolder}/engine/include"]` as a fallback for any future header. If a red line stays after building, Developer: Reload Window in the command palette (Ctrl+Shift+P) refreshes it.

## 3. How I built it

### The interface

`engine/include/emma/Integrator.hpp` holds the job description and nothing else:

```cpp
class Integrator {
public:
    virtual ~Integrator() = default;

    // Moves the world forward dt days. afterPiece runs after each piece (collisions, trails).
    // Returns how many pieces it took
    virtual int step(World& world, Gravity& gravity, double dt, const std::function<void()>& afterPiece) = 0;

    // Bodies were added, loaded or cleared
    virtual void bodiesChanged() = 0;
    // Bodies were removed mid-step, so anything stored per body is out of date
    virtual void forgetHistory() = 0;

    virtual const char* name() const = 0;

    // True if it can't work with the tree's approximate forces. Simulation then switches to
    // Verlet above Gravity::DIRECT_LIMIT bodies
    virtual bool needsExactForces() const { return false; }
};
```

`bodiesChanged` makes Verlet work out the accelerations before its next move, and makes IAS15 throw away its stored curves. `needsExactForces` is the one question with a default answer, no; IAS15 answers yes. There's no `Integrator.cpp`, because there's no code to write: every function is either left to the workers or has a one-line default.

### The first worker

```cpp
class VerletIntegrator : public Integrator {
public:
    static constexpr double ETA = 0.03;        // a piece lasts at most 3% of the fastest pair's swing time
    static constexpr int MAX_SUBSTEPS = 4096;  // most pieces one step can be split into

    int step(World& world, Gravity& gravity, double dt, const std::function<void()>& afterPiece) override;

    void bodiesChanged() override { stale = true; }
    void forgetHistory() override { jerkValid = false; }
    const char* name() const override { return "Velocity Verlet"; }

private:
    ...
};
```

`step` is only declared here: the line ends with `;`, and its code is in `VerletIntegrator.cpp`. The three short functions have their code right here, between the `{ }`. The `.cpp` file writes `int VerletIntegrator::step(...)` without `virtual` or `override`: those words go only where the function is first declared, in the header.

### How Simulation uses it

`Simulation` holds whichever integrator it was given, and starts with Verlet:

```cpp
std::unique_ptr<Integrator> integrator = std::make_unique<VerletIntegrator>();
```

The sidebar's switch ends up in `setIntegrator`:

```cpp
void Simulation::setIntegrator(std::unique_ptr<Integrator> next) {
    if (!next) return;            // keep the current one
    integrator = std::move(next);
    integrator->bodiesChanged();
    checkIntegrator();
}

void Simulation::checkIntegrator() {
    // IAS15 can't handle the tree's approximate forces, so above DIRECT_LIMIT bodies use Verlet
    if (world.bodies.size() > DIRECT_LIMIT && integrator->needsExactForces()) {
        integrator = std::make_unique<VerletIntegrator>();
        integrator->bodiesChanged();
    }
}
```

`checkIntegrator` also runs after every load and every added body, so the 65th body switches IAS15 to Verlet the moment it arrives.

The rest of the app gets a read-only look:

```cpp
const Integrator& getIntegrator() const { return *integrator; }
```

`*integrator` means "the object the pointer points at": the `*` follows the pointer.

Each simulation step passes collisions and trails in as `afterPiece`:

```cpp
void Simulation::step() {
    lastSubsteps = integrator->step(world, gravity, dt, [this] {
        if (resolveCollisions(world.bodies, flashes)) {
            integrator->forgetHistory();  // bodies were removed, so stored data no longer lines up
            ++world.changes;
        }
        world.recordTrails();  // after every piece, so trails stay smooth
    });
}
```

The `[this] { ... }` is a lambda, a small function written in place. `[this]` lets it use the `Simulation`'s own members: `world`, `flashes` and `integrator`.

### From one class to an interface

Before the change, `Simulation` had `VerletIntegrator integrator;`. Here's how the files changed:

| File | What happened |
|---|---|
| `engine/include/emma/Integrator.hpp` | Holds only the interface, the list of things every integrator must be able to do |
| `engine/include/emma/VerletIntegrator.hpp` | New: the Verlet class, moved here from `Integrator.hpp` |
| `engine/src/Integrator.cpp` → `engine/src/VerletIntegrator.cpp` | Renamed; only the first `#include` line changed |
| `CMakeLists.txt` | Uses the new `.cpp` file name. CMake notices and reconfigures by itself on the next build |
| `engine/include/emma/Simulation.hpp` | Holds "some integrator" instead of a Verlet, with `setIntegrator()` and `getIntegrator()` |
| `engine/src/Simulation.cpp` | `integrator.` became `integrator->`, `reset()` became `bodiesChanged()`, and `setIntegrator()` was added |
| `.vscode/c_cpp_properties.json` | The `includePath` fallback for VS Code |

In `Simulation.cpp` there were five places to change, all the same way:

| Where | Before | After |
|---|---|---|
| `load()` | `integrator.reset();` | `integrator->bodiesChanged();` |
| `clear()` | `integrator.reset();` | `integrator->bodiesChanged();` |
| `addBody()` | `integrator.reset();` | `integrator->bodiesChanged();` |
| `step()` | `integrator.step(...)` | `integrator->step(...)` |
| `step()`, inside the lambda | `integrator.forgetHistory();` | `integrator->forgetHistory();` |

To compare the tests' output before and after:

```powershell
git stash -u                                   # put the changes aside for a moment
cmake --build build --config Release
build\Release\physics_tests > before.txt
git stash pop                                  # bring them back
cmake --build build --config Release
build\Release\physics_tests > after.txt
fc.exe before.txt after.txt                    # "no differences encountered" means identical
```

In PowerShell, `fc` on its own means a different command, Format-Custom, so the file comparer needs its full name, `fc.exe`.

I renamed `Integrator.cpp` with a normal move, not `git mv`. That's fine: when you `git add -A`, git notices that `VerletIntegrator.cpp` is almost the same as the old file and records it as a rename.

### The C++ behind it

#### .hpp and .cpp files, and what #include really does

- A `.hpp` file, a header, describes things: "there is a class called X, and it has these functions."
- A `.cpp` file holds the actual code of those functions.
- `#include "file.hpp"` is copy and paste. Before compiling, the compiler pastes the whole header's text in place of that line.
- Headers are never compiled on their own. Only `.cpp` files are. A header only gets compiled when some `.cpp` includes it, directly or through another header.

That last point is why IntelliSense needs a `.cpp` that includes a new header.

#### class Worker : public Job means "inherits from"

```cpp
class VerletIntegrator : public Integrator {
```

Read this as "a VerletIntegrator is an Integrator". It takes on the job description and promises to do everything in it.

`::` means something else, "the thing inside":

| You write | It means |
|---|---|
| `class VerletIntegrator : public Integrator` | VerletIntegrator inherits from Integrator |
| `VerletIntegrator::step` | the `step` function inside VerletIntegrator |
| `emma::Body` | `Body` inside the `emma` namespace |
| `units::G` | `G` inside `units` |

One colon means "inherits from". Two colons mean "inside".

#### virtual: each worker does it their own way

```cpp
virtual int step(...) = 0;
```

`Simulation` only knows it holds "an Integrator". When it calls `integrator->step(...)`, `virtual` tells C++: don't decide which `step` to run now; check what the object really is while the program runs. If it's a Verlet, run Verlet's `step`; if it's IAS15, run IAS15's.

Without `virtual`, C++ would look only at the type it was told, `Integrator`, and never reach Verlet's version.

A virtual call costs one extra lookup. That's nothing next to the force calculation, which loops over every pair of bodies.

#### = 0: the job description doesn't do this, workers must

`= 0` makes a function pure virtual: it has no code in `Integrator`, and every worker has to provide its own.

A class with at least one `= 0` function is abstract: you can't create one by itself. `Integrator i;` won't compile, on purpose. A job description isn't a worker.

C++ has no `interface` keyword, as Java and C# do. A class where everything is `virtual ... = 0` is how C++ writes an interface.

#### override: "I'm filling a slot. Compiler, check me."

```cpp
const char* name() const override { return "Velocity Verlet"; }
```

`override` goes in the worker class. It tells the compiler: this must match a `virtual` function in the job description exactly, with the same name, the same parameters and the same `const`. A typo gives a clear error right on that line. Without `override`, a typo would quietly create a brand-new, unrelated function, and the error would show up somewhere else.

#### const after the (): this function only looks

```cpp
const char* name() const ...
//                 ^^^^^ this one
```

This `const` promises the function won't change the object. Saying your own name doesn't need to change anything, so the promise is free. And only `const` functions can be called through a `const` reference: `getIntegrator()` returns a `const Integrator&`, so the user interface can call `name()` and `needsExactForces()`, but not `step()`.

The project uses this everywhere: `double time() const`, `bool isNamed() const`.

#### const char*: a pointer to text that lives somewhere else

`const char*` is not a string object. It's a pointer to the first letter of some text, and the text ends with a hidden `'\0'` character. It has no length and no methods, and it doesn't own the text; it just points at it.

Returning `"Velocity Verlet"` is safe because text written in quotes, a string literal, is stored inside the program and exists the whole time the program runs. Returning the text of a `std::string` made inside the function would not be safe:

```cpp
const char* name() const {
    std::string s = "Velocity Verlet";
    return s.c_str();   // s is destroyed when the function ends, so the pointer points at nothing
}
```

The tests use `const char*` the safe way: `struct Test { const char* name; ... }`.

#### virtual ~Integrator() = default: clean up the worker's things too

`~Integrator()` is the destructor, the function that runs when an object is destroyed.

`Simulation` holds its integrator through an `Integrator` pointer, and destroys it through that pointer. With `virtual` on the destructor, C++ runs the worker's own cleanup too, so Verlet's `oldAcceleration` vector is freed. Without it, only `Integrator`'s cleanup would run, which C++ calls undefined behavior.

The rule: if a class is meant to be used through a pointer to its base, the base needs a `virtual` destructor. `= default` means the normal cleanup is fine, with no code to write.

#### std::unique_ptr: a pointer that owns what it points to

```cpp
std::unique_ptr<Integrator> integrator = std::make_unique<VerletIntegrator>();
```

- `std::make_unique<VerletIntegrator>()` creates a new Verlet and hands back a `unique_ptr` to it.
- The `unique_ptr` owns the Verlet. When the `unique_ptr` is destroyed, when `Simulation` is, it deletes the Verlet automatically. There's no `delete` to write.
- Unique means only one owner at a time, so a `unique_ptr` can't be copied: two owners would both try to delete the same thing.

A pointer is needed at all because a variable of type `Integrator` can't hold a Verlet: `Integrator` is abstract, and a Verlet is bigger. A pointer to `Integrator` can point at any worker.

Since `Simulation` contains a `unique_ptr`, a `Simulation` can't be copied either. Nothing in EMMA copies one.

`std::make_unique<IAS15Integrator>()` gives a `unique_ptr<IAS15Integrator>`, but `setIntegrator` wants a `unique_ptr<Integrator>`. C++ converts it automatically, because an IAS15Integrator is an Integrator: a pointer to an IAS15 worker is also a pointer to "some worker".

#### . versus ->

| You have | To reach a member, write |
|---|---|
| an object: `World world;` | `world.time` |
| a pointer: `std::unique_ptr<Integrator> integrator;` | `integrator->step(...)` |

`integrator->step()` means "go to the object the pointer points at, and call its `step`". `integrator.something()` means "call `something` on the pointer itself". That's the difference the name `bodiesChanged` guards against.

#### std::move: hand it over, don't copy it

```cpp
integrator = std::move(next);
```

A `unique_ptr` can't be copied, only handed over. `std::move(next)` says "take what `next` owns and give it to `integrator`". Afterward `next` is empty and `integrator` owns the new worker. The worker it held before is deleted automatically.

### Tests

The scenarios that test an integrator take a `Simulation` from outside, so each runs once with Verlet and once with IAS15:

- `earth_year` and `ias15_earth_year`: one Earth year takes 365.2564 days, to within 0.05.
- `close_pass` and `ias15_close_pass`: two planets miss each other narrowly, and the energy barely changes (`energy-drift.md`).
- `head_on_merge` and `ias15_head_on_merge`: a head-on merge keeps mass and momentum exactly.

`ias15_guard` checks the switch: IAS15 still runs with 64 bodies, the 65th body switches it to Verlet, and picking IAS15 again with 65 bodies keeps Verlet.

## Things to try

These take a minute each. Read the compiler's message, then undo the change.

1. Abstract classes. Add `Integrator test;` inside any function and build. The compiler refuses, because `Integrator` has `= 0` functions, so it can't exist on its own.
2. What `override` does. In `VerletIntegrator.hpp`, delete the `const` after `name()` but keep `override`. Build, and you get a clear error on that line. Now delete `override` too and build again. The error moves to where the Verlet is created, `make_unique<VerletIntegrator>()`, saying VerletIntegrator is abstract: `name()` without `const` became a different function, so the real `name() const` slot is still empty.
3. Why the name matters. In `Simulation::load`, change `integrator->bodiesChanged();` to `integrator.reset();`. It compiles. Run the tests and watch them stop at the first `step()`. Change it back.
4. A virtual call. In `physics_tests.cpp`, print `sim.getIntegrator().name()` in one of the tests, once after `setIntegrator(std::make_unique<IAS15Integrator>())` and once without. The same line of code calls two different functions.

## Glossary

| Term | Meaning |
|---|---|
| Integrator | The method that moves bodies forward in time |
| Interface | A list of functions a class promises to provide, with no code of its own |
| Inheritance | A class taking on another class's interface: `class A : public B` |
| Virtual function | A function whose version is chosen while the program runs, from what the object really is |
| Pure virtual, abstract | A function with no code (`= 0`), and a class with at least one, which can't be created on its own |
| Destructor | The function that runs when an object is destroyed |
| Smart pointer | A pointer that deletes what it owns when it's done, like `std::unique_ptr` |
| Lambda | A small function written in place |
| IntelliSense | VS Code's code helper, separate from the real compiler |
