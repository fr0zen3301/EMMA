#pragma once

// The fragment shader that ray traces Sgr A*. Every pixel follows its light ray backward
// from the eye through the curved space around a black hole that doesn't spin, with the
// same velocity Verlet step as emma::blackhole::stepRay. Lengths are in Schwarzschild
// radii and directions in Galactic coordinates
//
// uLook picks what glows around the hole: 0 the thick, see-through gas flow Sgr A*
// really has, 1 a thin, opaque disk, 2 nothing.
// Written for GLSL 1.10, which SFML uses: no version line and no precision qualifiers
inline constexpr const char* BLACK_HOLE_SHADER = R"(
uniform vec2 uCenter;      // the middle of the picture, in pixels from its bottom left
uniform float uFocal;      // focal length, in pixels
uniform vec3 uEye;         // the eye, relative to the hole
uniform vec3 uRight;       // the camera's axes
uniform vec3 uUp;
uniform vec3 uForward;
uniform vec3 uAxis;        // the axis the gas turns about
uniform vec3 uAxis1;       // two directions in the gas's midplane
uniform vec3 uAxis2;
uniform float uLook;
uniform float uBend;       // 1 bends light, 0 lets it go straight
uniform float uDoppler;    // 1 includes the gas's motion in its color and brightness
uniform float uExposure;
uniform float uEyeShift;   // 1 / sqrt(1 - 1/D): the blueshift from the eye sitting in the hole's pull
uniform float uEscape;     // a ray beyond this radius, moving outward, has escaped
uniform sampler2D uSky;    // the starfield by Galactic longitude and latitude

const float PI = 3.14159265;
const float DISK_IN = 3.0;        // the innermost stable orbit
const float DISK_OUT = 14.0;
const float FLUX_PEAK = 0.05665;  // the thin-disk flux profile's highest value, at 49/36 of DISK_IN
const float T_PEAK = 6500.0;      // kelvin, chosen so the colors read well

float hash(float n) { return fract(sin(n * 127.1) * 43758.5453); }

float noise(float x) {
    float i = floor(x);
    float f = fract(x);
    f = f * f * (3.0 - 2.0 * f);
    return mix(hash(i), hash(i + 1.0), f);
}

vec3 sky(vec3 d) {
    float lon = atan(d.y, d.x);
    float lat = asin(clamp(d.z, -1.0, 1.0));
    return texture2D(uSky, vec2(lon / (2.0 * PI) + 0.5, 0.5 - lat / PI)).rgb;
}

// The color of a blackbody at a temperature, in kelvin
vec3 blackbody(float kelvin) {
    float t = clamp(kelvin, 1000.0, 40000.0) / 100.0;
    float r;
    float g;
    float b;
    if (t <= 66.0) {
        r = 1.0;
        g = 0.39008157876 * log(t) - 0.63184144378;
    } else {
        r = 1.29293618606 * pow(t - 60.0, -0.1332047592);
        g = 1.12989086089 * pow(t - 60.0, -0.0755148492);
    }
    if (t >= 66.0) b = 1.0;
    else if (t <= 19.0) b = 0.0;
    else b = 0.54320678911 * log(t - 10.0) - 1.19625408914;
    return clamp(vec3(r, g, b), 0.0, 1.0);
}

// Black, red, orange, yellow, white: the false-color scale of the EHT's images
vec3 heat(float k) { return clamp(vec3(2.0 * k, 2.0 * k - 0.5, 2.0 * k - 1.0), 0.0, 1.0); }

// How much light from gas at p is shifted toward the eye, for a ray heading along dir.
// fraction is the gas's speed as a fraction of a circular orbit's
float shift(vec3 p, vec3 dir, float fraction) {
    float r = length(p);
    vec3 c = cross(uAxis, p);
    float l = length(c);
    vec3 along = l > 0.000001 ? c / l : vec3(0.0);
    float v = min(fraction * uDoppler * sqrt(0.5 / max(r - 1.0, 0.05)), 0.95);
    float gamma = 1.0 / sqrt(1.0 - v * v);
    return sqrt(max(1.0 - 1.0 / r, 0.0)) / (gamma * (1.0 - v * dot(along, -dir))) * uEyeShift;
}

// The pull that bends a ray into a light ray's path: -(3/2) h^2 x / r^5
vec3 pull(vec3 x, float h2) {
    float r2 = dot(x, x);
    return -1.5 * h2 * x / (r2 * r2 * sqrt(r2)) * uBend;
}

void main() {
    vec2 q = gl_FragCoord.xy - uCenter;
    vec3 v = normalize(uForward * uFocal + uRight * q.x + uUp * q.y);
    vec3 x = uEye;
    vec3 h = cross(x, v);
    float h2 = dot(h, h);
    vec3 a = pull(x, h2);

    vec3 light = vec3(0.0);  // from the thin disk
    float cover = 0.0;       // how much of what's behind the thin disk hides
    float glow = 0.0;        // from the gas flow
    int stop = 0;            // 1 fell in, 2 escaped, 3 hidden behind the disk

    for (int i = 0; i < 600; i++) {
        float r = length(x);
        float dt = max(0.06 * r, 0.01);
        vec3 xn = x + v * dt + 0.5 * a * dt * dt;
        vec3 an = pull(xn, h2);
        vec3 vn = v + 0.5 * (a + an) * dt;

        if (uLook > 0.5 && uLook < 1.5) {
            // Did the ray cross the disk's plane during this step?
            float s0 = dot(x, uAxis);
            float s1 = dot(xn, uAxis);
            if (s0 * s1 < 0.0) {
                float t = s0 / (s0 - s1);
                vec3 p = mix(x, xn, t);
                float rc = length(p);
                if (rc > DISK_IN && rc < DISK_OUT) {
                    vec3 d = normalize(mix(v, vn, t));
                    float s = DISK_IN / rc;
                    float flux = s * s * s * (1.0 - sqrt(s)) / FLUX_PEAK;
                    float g = shift(p, d, 1.0);
                    float angle = atan(dot(p, uAxis2), dot(p, uAxis1));
                    float streaks = 0.7 + 0.3 * noise(rc * 5.0 + 1.6 * sin(2.0 * angle + rc * 0.7));
                    vec3 c = blackbody(T_PEAK * pow(flux, 0.25) * g) * flux * g * g * g * g * streaks;
                    float alpha = smoothstep(DISK_IN, DISK_IN + 0.3, rc) *
                                  (1.0 - smoothstep(DISK_OUT - 5.0, DISK_OUT, rc)) * 0.97;
                    light += (1.0 - cover) * alpha * c;
                    cover += (1.0 - cover) * alpha;
                }
            }
        }

        if (uLook < 0.5) {
            // See-through gas: everything it gives off along the ray adds up
            vec3 m = 0.5 * (x + xn);
            float rm = length(m);
            if (rm > 1.0 && rm < 200.0) {
                float z = dot(m, uAxis);
                float thick = 0.6 * rm;
                float j = pow(rm, -2.5) * exp(-0.5 * z * z / (thick * thick)) * smoothstep(1.0, 1.8, rm);
                float g = shift(m, normalize(v), 0.8);
                glow += j * g * g * g * g * length(xn - x);
            }
        }

        x = xn;
        v = vn;
        a = an;
        float rn = length(x);
        if (rn < 1.0) { stop = 1; break; }
        if (rn > uEscape && dot(x, v) > 0.0) { stop = 2; break; }
        if (cover > 0.995) { stop = 3; break; }
    }

    vec3 background = stop == 2 ? sky(normalize(v)) : vec3(0.0);
    vec3 color;
    if (uLook < 0.5) {
        float k = 1.0 - exp(-glow * uExposure * 2.0);
        color = heat(k) + background * (1.0 - k);
    } else if (uLook < 1.5) {
        color = (vec3(1.0) - exp(-light * uExposure * 1.2)) + background * (1.0 - cover);
    } else {
        color = background;
    }
    gl_FragColor = vec4(color, 1.0);
}
)";
