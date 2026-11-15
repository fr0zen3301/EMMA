#pragma once
#include "emma/Vector3D.hpp"
#include <string>

// Which way the Sun, the planets and the Moon point in space and how fast they spin,
// so their surfaces can be drawn turned the right way at any date. The values are the
// IAU's (Working Group on Cartographic Coordinates and Rotational Elements, 2015 report;
// the 2009 report for Earth and the Moon)
//
// Each body's north pole points to a right ascension and declination in the ICRF, the
// equatorial J2000 frame. Its prime meridian, the line of 0 degrees longitude, has
// turned W degrees east from where the body's equator crosses the ICRF equator. All
// three drift with time. For the Moon, Mars and Neptune the largest periodic term is
// kept; every term left out is under 0.15 degrees
namespace emma::spin {

struct Model {
    const char* name;
    double poleRa, poleRaRate;      // degrees, degrees per Julian century
    double poleDec, poleDecRate;    // degrees, degrees per Julian century
    double meridian, meridianRate;  // W: degrees, degrees per day
    double flattening;              // 1 - polar radius / equatorial radius
    // The largest periodic term, if any: an amplitude in degrees, times the sine (pole
    // right ascension, W) or cosine (pole declination) of an angle a0 + a1 T, in degrees
    // with T in Julian centuries since J2000
    double raAmplitude, raAngle, raAngleRate;
    double decAmplitude, decAngle, decAngleRate;
    double meridianAmplitude, meridianAngle, meridianAngleRate;
};

// A body's axes: x toward its prime meridian, z toward its north pole, and y = z cross x,
// toward 90 degrees east longitude
struct Axes {
    Vector3D x, y, z;
};

// The model for a body by name, or nullptr when there isn't one
const Model* find(const std::string& name);

// The body's axes at a Julian day, in the ICRF (equatorial J2000)
Axes equatorialAxes(const Model& model, double julianDay);

// The same axes in EMMA's J2000 ecliptic frame
Axes eclipticAxes(const Model& model, double julianDay);

}
