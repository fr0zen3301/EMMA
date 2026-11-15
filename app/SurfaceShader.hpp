#pragma once

// The fragment shader that draws a body's real surface. It runs on a square around the
// body: every pixel's view ray is followed to the body, a sphere flattened along its
// pole, and the texture is looked up at the latitude and longitude it hits. The Sun's
// direction lights it. Everything is in the body's own axes (x toward its prime
// meridian, z toward its north pole) with lengths in its equatorial radius.
//
// The extras each body may have: a night map (Earth's city lights), a cloud map, a
// height map (the Moon's relief, which shades its craters) and a ring strip (Saturn).
// The Sun gives its own light, a little dimmer toward its edge.
// Written for GLSL 1.10, which SFML uses: no version line and no precision qualifiers
inline constexpr const char* SURFACE_SHADER = R"(
uniform vec2 uCenter;      // the middle of the view, in pixels from the window's bottom left
uniform float uFocal;      // focal length, in pixels
uniform vec3 uEye;         // the eye, relative to the body
uniform vec3 uRight;       // the camera's axes
uniform vec3 uUp;
uniform vec3 uForward;
uniform vec3 uLight;       // toward the Sun
uniform float uPolar;      // polar radius over equatorial radius
uniform float uPixel;      // equatorial radii per pixel at the body's distance
uniform float uSun;        // 1 for the Sun itself
uniform sampler2D uColor;
uniform float uHasNight;
uniform sampler2D uNight;
uniform float uHasClouds;
uniform sampler2D uClouds;
uniform float uHasHeight;
uniform sampler2D uHeight;
uniform vec2 uHeightTexel; // one texel of the height map
uniform float uRelief;     // the height map's full range, in equatorial radii
uniform float uHasRings;
uniform sampler2D uRings;  // a strip: inner edge on the left, outer edge on the right
uniform float uRingIn;     // in equatorial radii
uniform float uRingOut;

const float PI = 3.14159265;

// Where a point on the surface falls on an equirectangular map centered on 0 degrees
// longitude. Longitude jumps from +180 to -180 degrees on the far side, and a texture
// lookup takes its mipmap level from how fast the coordinates change between pixels, so
// across that jump it would pick the blurriest level and draw a seam. A second
// longitude, with its jump at 0 degrees, is used wherever it changes more slowly
vec2 mapCoords(vec3 p) {
    float lon = atan(p.y, p.x) / (2.0 * PI);
    float lat = asin(clamp(p.z / length(p), -1.0, 1.0)) / PI;
    float u1 = lon + 0.5;
    float u2 = fract(lon) + 0.5;
    float u = fwidth(u1) <= fwidth(u2) + 0.000001 ? u1 : u2;
    return vec2(u, 0.5 - lat);
}

vec3 surfaceNormal(vec3 p) { return normalize(vec3(p.xy, p.z / (uPolar * uPolar))); }

// Whether the planet blocks the Sun from p: in space squeezed along the pole the
// planet is a unit sphere
bool inPlanetShadow(vec3 p) {
    vec3 so = vec3(p.xy, p.z / uPolar);
    vec3 sl = normalize(vec3(uLight.xy, uLight.z / uPolar));
    float along = -dot(so, sl);
    if (along < 0.0) return false;
    vec3 closest = so + sl * along;
    return dot(closest, closest) < 1.0;
}

// How much of the sunlight reaching p gets through the rings
float ringShadow(vec3 p) {
    if (uHasRings < 0.5 || abs(uLight.z) < 0.000001) return 1.0;
    float s = -p.z / uLight.z;
    if (s <= 0.0) return 1.0;
    float r = length(p.xy + uLight.xy * s);
    if (r < uRingIn || r > uRingOut) return 1.0;
    float a = texture2D(uRings, vec2((r - uRingIn) / (uRingOut - uRingIn), 0.5)).a;
    return 1.0 - 0.85 * a;
}

vec3 shadePlanet(vec3 p, vec3 d) {
    vec3 n = surfaceNormal(p);
    vec2 uv = mapCoords(p);
    vec3 albedo = texture2D(uColor, uv).rgb;
    if (uSun > 0.5) return albedo * (0.4 + 0.6 * max(dot(n, -d), 0.0));

    // Relief: the height map's slope east and north tips the normal, so slopes facing
    // the Sun are brighter, which shows most along the line between day and night
    vec3 shaded = n;
    if (uHasHeight > 0.5) {
        float lat = asin(clamp(p.z / length(p), -1.0, 1.0));
        float east = texture2D(uHeight, uv + vec2(uHeightTexel.x, 0.0)).r -
                     texture2D(uHeight, uv - vec2(uHeightTexel.x, 0.0)).r;
        float north = texture2D(uHeight, uv - vec2(0.0, uHeightTexel.y)).r -
                      texture2D(uHeight, uv + vec2(0.0, uHeightTexel.y)).r;
        float eastStep = 4.0 * PI * uHeightTexel.x * max(cos(lat), 0.05);  // the samples' distance apart
        float northStep = 2.0 * PI * uHeightTexel.y;
        vec3 e = normalize(vec3(-p.y, p.x, 0.0) + vec3(0.000001, 0.0, 0.0));
        vec3 nn = cross(n, e);
        shaded = normalize(n - e * (east * uRelief / eastStep) - nn * (north * uRelief / northStep));
    }

    float light = max(dot(shaded, uLight), 0.0) * ringShadow(p);
    vec3 color = albedo * light;
    float cloud = 0.0;
    if (uHasClouds > 0.5) {
        cloud = texture2D(uClouds, uv).r;
        color = mix(color, vec3(light), cloud * 0.9);
    }
    if (uHasNight > 0.5) {
        // City lights fade in past the line between day and night
        float dark = 1.0 - smoothstep(-0.1, 0.05, dot(n, uLight));
        color += texture2D(uNight, uv).rgb * dark * (1.0 - cloud) * 0.8;
    }
    return color + albedo * 0.01;
}

void main() {
    vec2 q = gl_FragCoord.xy - uCenter;
    vec3 d = normalize(uForward * uFocal + uRight * q.x + uUp * q.y);
    vec3 o = uEye;

    // The planet. Squeezed along the pole it's a unit sphere. The ray's closest approach
    // to the center is found first, which stays accurate when the eye is far away
    vec3 so = vec3(o.xy, o.z / uPolar);
    vec3 sd = vec3(d.xy, d.z / uPolar);
    float stretch = length(sd);
    vec3 sn = sd / stretch;
    float along = -dot(so, sn);
    vec3 closest = so + sn * along;
    float miss = length(closest);
    float s = along - sqrt(max(1.0 - miss * miss, 0.0));
    // On the surface where the ray hits it. Just outside the edge, the nearest edge
    // point, so the edge can fade over a pixel instead of being jagged
    vec3 sp = so + sn * s;
    sp = sp / max(length(sp), 0.000001);
    vec3 p = vec3(sp.xy, sp.z * uPolar);
    float tPlanet = s / stretch;
    float cover = clamp((1.0 - miss) / uPixel + 0.5, 0.0, 1.0);
    if (tPlanet <= 0.0) cover = 0.0;
    vec4 body = vec4(shadePlanet(p, d), cover);

    // The rings, in the equator's plane
    vec4 ring = vec4(0.0);
    float tRing = -1.0;
    if (uHasRings > 0.5 && abs(d.z) > 0.000001) {
        float t = -o.z / d.z;
        vec3 rp = o + d * t;
        float r = length(rp.xy);
        if (t > 0.0 && r > uRingIn && r < uRingOut) {
            vec4 c = texture2D(uRings, vec2((r - uRingIn) / (uRingOut - uRingIn), 0.5));
            // Brighter from the side the Sun lights, dark where the planet's shadow falls
            float lit = o.z * uLight.z > 0.0 ? 1.0 : 0.3;
            if (inPlanetShadow(rp)) lit *= 0.05;
            ring = vec4(c.rgb * lit, c.a);
            tRing = t;
        }
    }

    // Whichever is nearer goes in front
    vec4 front = body;
    vec4 back = ring;
    if (tRing > 0.0 && (cover <= 0.0 || tRing < tPlanet)) {
        front = ring;
        back = body;
    }
    float a = front.a + back.a * (1.0 - front.a);
    if (a <= 0.0) discard;
    vec3 rgb = (front.rgb * front.a + back.rgb * back.a * (1.0 - front.a)) / a;
    gl_FragColor = vec4(rgb, a);
}
)";
