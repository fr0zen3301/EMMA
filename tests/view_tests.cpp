// Tests for the app's view math: the orbit camera and the scene frame. The camera
// measures pixels with SFML's sf::Vector2f and sf::FloatRect, so these get their own
// test program instead of sharing physics_tests, which links only the engine. Those
// SFML types are header-only, and nothing here opens a window.
// Run them all:  ctest --test-dir build -C Release --output-on-failure
// Run one:       build/Release/view_tests camera_round_trip

#include "Camera.hpp"
#include "SceneFrame.hpp"
#include "emma/Constants.hpp"
#include "emma/Galaxy.hpp"
#include "emma/Vector3D.hpp"
#include <SFML/Graphics/Rect.hpp>
#include <SFML/System/Vector2.hpp>
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <optional>

using namespace emma;

namespace {

int failures = 0;

#define CHECK(condition, ...)                \
    do {                                     \
        if (!(condition)) {                  \
            std::printf("  FAILED: ");       \
            std::printf(__VA_ARGS__);        \
            std::printf("\n");               \
            ++failures;                      \
        }                                    \
    } while (0)

double relative(double value, double reference) {
    return std::abs((value - reference) / reference);
}

double relative(const Vector3D& value, const Vector3D& reference) {
    return (value - reference).magnitude() / reference.magnitude();
}

bool same(const Vector3D& a, const Vector3D& b) {
    return a.x == b.x && a.y == b.y && a.z == b.z;
}

double pixelsApart(sf::Vector2f a, sf::Vector2f b) {
    return std::hypot(static_cast<double>(a.x - b.x), static_cast<double>(a.y - b.y));
}

// A camera like the app's: the view sits right of a 300 pixel sidebar, and the
// target is off the origin and a little above the ecliptic
Camera makeCamera(double yawDeg, double pitchDeg) {
    Camera camera;
    camera.viewport = sf::FloatRect({300.f, 0.f}, {1300.f, 860.f});
    camera.target = Vector3D(1.0, -0.4, 0.02);
    camera.distance = 3.0;
    camera.setAngles(yawDeg * units::DEG, pitchDeg * units::DEG);
    return camera;
}

// The pixel dx, dy from the middle of the view. Positive dy is down the screen
sf::Vector2f pixel(const Camera& camera, double dx, double dy) {
    const sf::Vector2f m = camera.middle();
    return sf::Vector2f(static_cast<float>(m.x + dx), static_cast<float>(m.y + dy));
}

// ---------------------------------------------------------------------------

void testCameraRoundTrip() {
    // project() rounds pixels to float, about 7 digits, so a point can't come back
    // exactly. The error grows with depth and as the view ray grazes the grid. At
    // these angles it stays at a few millionths of the viewing distance, so the
    // check allows 1e-5 of it
    const double yaws[] = {0.0, 37.0, -120.0, 179.0};
    const double pitches[] = {90.0, 60.0, 25.0, 10.0, -40.0};
    const Vector3D offsets[] = {{0.0, 0.0}, {0.7, 0.2}, {-0.5, 0.9}, {0.3, -1.0}, {-1.0, -0.6}};

    double worst = 0.0;
    int count = 0;
    for (const double yaw : yaws) {
        for (const double pitch : pitches) {
            const Camera camera = makeCamera(yaw, pitch);

            const std::optional<sf::Vector2f> middle = camera.toScreen(camera.target);
            CHECK(middle && pixelsApart(*middle, camera.middle()) < 1e-3,
                  "yaw %.0f, pitch %.0f: the target isn't drawn in the middle of the view", yaw, pitch);

            for (const Vector3D& offset : offsets) {
                const Vector3D p = camera.target + offset;  // on the grid, the plane z = target.z
                const std::optional<sf::Vector2f> s = camera.toScreen(p);
                CHECK(s.has_value(), "yaw %.0f, pitch %.0f: grid point %+.1f, %+.1f is behind the eye", yaw, pitch,
                      offset.x, offset.y);
                if (!s) continue;
                const std::optional<Vector3D> q = camera.pick(*s);
                CHECK(q.has_value(), "yaw %.0f, pitch %.0f: picking missed grid point %+.1f, %+.1f", yaw, pitch,
                      offset.x, offset.y);
                if (!q) continue;
                const double error = (*q - p).magnitude() / camera.distance;
                worst = std::max(worst, error);
                ++count;
                CHECK(error < 1e-5, "yaw %.0f, pitch %.0f: grid point %+.1f, %+.1f came back %.2e distances away",
                      yaw, pitch, offset.x, offset.y, error);
            }
        }
    }
    std::printf("  %d grid points, worst round trip %.2e of the viewing distance\n", count, worst);
}

void testCameraClip() {
    const Camera camera = makeCamera(30.0, 45.0);
    const double nearPlane = Camera::NEAR * camera.distance;

    // All in front: left exactly as it was
    Vector3D a(0.1, 0.2, 2.0), b(-0.3, 0.5, 3.0);
    CHECK(camera.clip(a, b), "a segment in front of the eye was dropped");
    CHECK(same(a, Vector3D(0.1, 0.2, 2.0)) && same(b, Vector3D(-0.3, 0.5, 3.0)),
          "a segment in front of the eye was changed");

    // Crossing the near plane, either way round: the end behind moves onto the
    // plane, along the segment, and the end in front stays put
    const Vector3D behind(0.4, -0.2, -1.0), front(0.1, 0.3, 2.0);
    const Vector3D along = behind - front;
    for (int order = 0; order < 2; ++order) {
        Vector3D first = order == 0 ? behind : front;
        Vector3D second = order == 0 ? front : behind;
        CHECK(camera.clip(first, second), "a segment crossing the near plane was dropped");
        const Vector3D& cut = order == 0 ? first : second;
        const Vector3D& kept = order == 0 ? second : first;
        CHECK(same(kept, front), "the end in front of the eye moved");
        CHECK(std::abs(cut.z - nearPlane) < 1e-12 * camera.distance, "cut at depth %.6e, the near plane is at %.6e",
              cut.z, nearPlane);
        // How far the cut point is from the original line, as a fraction of the segment's length
        const double off = (cut - front).cross(along).magnitude() / along.lengthSquared();
        CHECK(off < 1e-12, "the cut point is %.2e of the segment's length off the line", off);
    }

    // All behind the near plane: nothing left to draw
    Vector3D c(0.2, 0.1, -0.5), d(-0.1, 0.4, 0.5 * nearPlane);
    CHECK(!camera.clip(c, d), "a segment entirely behind the near plane was kept");
}

void testCameraZoom() {
    Camera camera = makeCamera(30.0, 40.0);
    const sf::Vector2f cursor = pixel(camera, 210.0, -95.0);
    const std::optional<Vector3D> before = camera.pick(cursor);
    CHECK(before.has_value(), "the cursor isn't over the grid");
    if (!before) return;

    // In and back out: the grid point under the cursor stays under it
    const double factors[] = {2.0, 0.5};
    const double distances[] = {1.5, 3.0};
    for (int i = 0; i < 2; ++i) {
        camera.zoomAt(cursor, factors[i]);
        CHECK(relative(camera.distance, distances[i]) < 1e-15, "zooming by %.1f left the distance at %.17g",
              factors[i], camera.distance);
        const std::optional<Vector3D> after = camera.pick(cursor);
        CHECK(after && (*after - *before).magnitude() < 1e-9 * camera.distance,
              "zooming by %.1f moved the grid point under the cursor", factors[i]);
    }

    // The distance stops at its limits
    camera.zoom(1e30);
    CHECK(camera.distance == Camera::MIN_DISTANCE, "zoomed in to %.3e AU, the limit is %.3e", camera.distance,
          Camera::MIN_DISTANCE);
    camera.zoom(1e-30);
    CHECK(camera.distance == Camera::MAX_DISTANCE, "zoomed out to %.3e AU, the limit is %.3e", camera.distance,
          Camera::MAX_DISTANCE);
}

void testCameraPan() {
    // Grabbing the grid: the point under the cursor follows it
    Camera camera = makeCamera(-70.0, 35.0);
    const sf::Vector2f from = pixel(camera, -150.0, 60.0), to = pixel(camera, -110.0, 35.0);
    const std::optional<Vector3D> grabbed = camera.pick(from);
    CHECK(grabbed.has_value(), "the cursor isn't over the grid");
    if (!grabbed) return;
    camera.pan(from, to);
    const std::optional<Vector3D> under = camera.pick(to);
    CHECK(under && (*under - *grabbed).magnitude() < 1e-9 * camera.distance,
          "after panning, the grabbed grid point isn't under the cursor");
    CHECK(std::abs(camera.target.z - 0.02) < 1e-14, "panning moved the grid to height %.17g", camera.target.z);

    // Near the horizon a grab would fling the view far away, so it slides by the
    // drag instead: distance / focal AU per pixel, keeping the grid's height
    Camera low = makeCamera(-70.0, 3.0);
    const sf::Vector2f skyFrom = pixel(low, 0.0, -300.0), skyTo = pixel(low, 30.0, -260.0);
    CHECK(!low.pick(skyFrom), "a pixel above the horizon landed on the grid");
    const Vector3D start = low.target;
    low.pan(skyFrom, skyTo);
    const Vector3D moved = low.target - start;
    const double expected = 50.0 * low.distance / low.focal();  // a 30 by 40 pixel drag
    CHECK(moved.z == 0.0, "sliding changed the grid's height by %.3e AU", moved.z);
    CHECK(relative(moved.magnitude(), expected) < 1e-9, "slid %.6e AU for a 50 pixel drag, expected %.6e",
          moved.magnitude(), expected);
}

void testCameraPickMisses() {
    // Edge-on, the ray through the middle of the view runs along the grid
    const Camera edgeOn = makeCamera(25.0, 0.0);
    CHECK(!edgeOn.pick(edgeOn.middle()), "edge-on, the middle of the view landed on the grid");

    // Above the horizon the grid is behind the eye. The horizon sits
    // focal * tan(pitch) pixels above the middle
    const Camera tilted = makeCamera(25.0, 20.0);
    const double horizon = -tilted.focal() * std::tan(20.0 * units::DEG);
    CHECK(!tilted.pick(pixel(tilted, 0.0, 1.5 * horizon)), "a pixel above the horizon landed on the grid");

    // Just under the horizon the ray meets the grid about 10,000 distances away,
    // past MAX_PICK. Halfway from there to the middle, it lands one distance / cos(pitch) ahead
    const Camera low = makeCamera(25.0, 2.0);
    const double lowHorizon = -low.focal() * std::tan(2.0 * units::DEG);
    const std::optional<Vector3D> beyond = low.pick(pixel(low, 0.0, (1.0 - 1e-4) * lowHorizon));
    CHECK(!beyond, "a pick at the horizon landed %.0f distances away",
          beyond ? (*beyond - low.target).magnitude() / low.distance : 0.0);
    const std::optional<Vector3D> ahead = low.pick(pixel(low, 0.0, 0.5 * lowHorizon));
    const double expected = low.distance / std::cos(2.0 * units::DEG);
    CHECK(ahead && relative((*ahead - low.target).magnitude(), expected) < 1e-3,
          "halfway to the horizon, the pick didn't land one distance ahead");
}

void testCameraFit() {
    // Fit view centers on the barycenter and shows the radius on the short side of
    // the view. Here that's the height, so from the top view, 5 AU along +y
    // reaches the top edge
    Camera camera = makeCamera(0.0, 90.0);
    camera.fit(5.0);
    CHECK(same(camera.target, Vector3D()), "fit left the target at %.3g, %.3g, %.3g", camera.target.x,
          camera.target.y, camera.target.z);
    const double half = 0.5 * std::min(camera.viewport.size.x, camera.viewport.size.y);
    CHECK(relative(camera.pxPerAU() * 5.0, half) < 1e-12, "5 AU covers %.6f pixels, half the short side is %.1f",
          camera.pxPerAU() * 5.0, half);

    const sf::Vector2f m = camera.middle();
    const std::optional<sf::Vector2f> edge = camera.toScreen(Vector3D(0.0, 5.0, 0.0));
    CHECK(edge && std::abs(edge->x - m.x) < 0.01 && std::abs((m.y - edge->y) - half) < 0.01,
          "5 AU up the screen didn't reach the top edge of the view");
}

// The worst project-then-pick round trip over a few grid points around the target,
// as a fraction of the viewing distance. -1 if a point fails to come back at all
double worstRoundTrip(const Camera& camera) {
    const Vector3D offsets[] = {{0.0, 0.0}, {0.3, 0.2}, {-0.25, 0.1}, {0.1, -0.3}};
    double worst = 0.0;
    for (const Vector3D& offset : offsets) {
        const Vector3D p = camera.target + offset * camera.distance;
        const std::optional<sf::Vector2f> s = camera.toScreen(p);
        const std::optional<Vector3D> q = s ? camera.pick(*s) : std::nullopt;
        if (!q) return -1.0;
        worst = std::max(worst, (*q - p).magnitude() / camera.distance);
    }
    return worst;
}

void testCameraGalacticScale() {
    // Zoomed all the way out from the Sun, the Galactic center is in view: the short
    // side of the view reaches about 20 kpc each way
    Camera wide = makeCamera(0.0, 90.0);
    wide.target = Vector3D();
    wide.zoom(1e-30);  // as far out as it goes
    const Vector3D center(galaxy::R0_AU, 0.0, 0.0);
    const std::optional<sf::Vector2f> at = wide.toScreen(center);
    CHECK(at && wide.viewport.contains(*at), "zoomed all the way out, the Galactic center is out of view");
    const double wideError = worstRoundTrip(wide);
    CHECK(wideError >= 0.0 && wideError < 1e-5, "zoomed all the way out, the round trip is off by %.2e of the distance",
          wideError);

    // Zoomed in close to the Galactic center, 1.7e9 AU from the origin. Doubles there
    // are about 2.4e-7 AU apart, and at 0.01 AU a pixel covers about 1e-5 AU, so
    // picking still lands within a small part of a pixel
    Camera close = makeCamera(30.0, 50.0);
    close.target = center;
    close.distance = 0.01;
    const double closeError = worstRoundTrip(close);
    std::printf("  round trip off by %.1e of the distance at 48 kpc, %.1e at 0.01 AU near the Galactic center\n",
                wideError, closeError);
    CHECK(closeError >= 0.0 && closeError < 1e-3, "0.01 AU from the Galactic center, the round trip is off by %.2e",
          closeError);
}

void testSunPath() {
    const Vector3D p(0.4, -1.1, 0.03);  // AU, ecliptic: a point somewhere in the solar system
    const double year = units::DAYS_PER_YEAR;
    SceneFrame frame;
    frame.velocity = galaxy::sunVelocityKms() * units::KMS_TO_AU_PER_DAY;

    // Outside the galaxy view the frame changes nothing
    CHECK(same(frame.place(p, 1e6 * year), p), "the normal view moved a point");

    // A year in, the path hasn't yet bent away from a straight line by 1e-5 AU
    frame.galactic = true;
    const Vector3D r = galaxy::fromEcliptic(p);
    const double offLine = (frame.place(p, year) - (r + frame.velocity * year)).magnitude();
    CHECK(offLine < 1e-5, "a year in, the path is %.2e AU off the straight line", offLine);

    // 10,000 years in, it has bent toward the Galactic center by the sagitta of the
    // arc, R0 theta^2 / 2: about 80 AU
    const double t = 1e4 * year;
    const Vector3D bend = frame.place(p, t) - (r + frame.velocity * t);
    const double theta = frame.velocity.y * t / galaxy::R0_AU;
    const double sagitta = 0.5 * galaxy::R0_AU * theta * theta;
    std::printf("  after 10,000 years the path has bent %.1f AU toward the Galactic center\n", bend.x);
    CHECK(bend.x > 0.0 && relative(bend.magnitude(), sagitta) < 1e-3,
          "after 10,000 years the path bent %.1f AU, expected %.1f AU toward the center", bend.magnitude(), sagitta);

    // Only the circling part: a quarter of the way round, the Sun has moved R0 along
    // x and R0 along y. It's still R0 from the center, and moving along +x
    SceneFrame circling;
    circling.galactic = true;
    circling.velocity = Vector3D(0.0, frame.velocity.y, 0.0);
    circling.now = 0.5 * units::PI * galaxy::R0_AU / circling.velocity.y;
    const Vector3D sun = circling.place(Vector3D());
    CHECK(relative(sun, Vector3D(galaxy::R0_AU, galaxy::R0_AU, 0.0)) < 1e-12,
          "a quarter of the way round, the Sun is at %.6e, %.6e, %.6e AU", sun.x, sun.y, sun.z);
    CHECK(relative(circling.heading(), Vector3D(circling.velocity.y, 0.0, 0.0)) < 1e-12,
          "a quarter of the way round, the Sun isn't heading along +x");

    // heading() is the path's own velocity: compare it with a centered difference
    frame.now = 1e6 * year;
    const double h = 1.0;  // day
    const Vector3D slope = (frame.place(Vector3D(), frame.now + h) - frame.place(Vector3D(), frame.now - h)) / (2.0 * h);
    CHECK(relative(slope, frame.heading()) < 1e-6, "a million years in, heading() is %.2e off the path's direction",
          relative(slope, frame.heading()));
}

struct Test {
    const char* name;
    void (*run)();
};

const Test TESTS[] = {
    {"camera_round_trip", testCameraRoundTrip},
    {"camera_clip", testCameraClip},
    {"camera_zoom", testCameraZoom},
    {"camera_pan", testCameraPan},
    {"camera_pick_misses", testCameraPickMisses},
    {"camera_fit", testCameraFit},
    {"camera_galactic_scale", testCameraGalacticScale},
    {"sun_path", testSunPath},
};

}

int main(int argc, char** argv) {
    const char* only = argc > 1 ? argv[1] : nullptr;
    bool ran = false;
    for (const Test& t : TESTS) {
        if (only && std::strcmp(only, t.name) != 0) continue;
        std::printf("%s\n", t.name);
        t.run();
        ran = true;
    }
    if (!ran) {
        std::printf("no test named %s\n", only);
        return 1;
    }
    if (failures) std::printf("%d check(s) failed\n", failures);
    else std::printf("all checks passed\n");
    return failures ? 1 : 0;
}
