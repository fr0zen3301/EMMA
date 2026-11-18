#pragma once
#include "emma/Vector3D.hpp"
#include <string>

// Which way the Sun, the planets and the Moon point and how fast they spin, so their
// surfaces face the right way at any date. Values from the IAU (2015 report)
namespace emma::spin {

struct Model {
    const char* name;
    double poleRa, poleRaRate;      // degrees, degrees per Julian century
    double poleDec, poleDecRate;    // degrees, degrees per Julian century
    double meridian, meridianRate;  // how far it has turned: degrees, degrees per day
    double flattening;              // 1 - polar radius / equatorial radius
    // A small wobble, for the Moon, Mars and Neptune: amplitude and angle, in degrees
    double raAmplitude, raAngle, raAngleRate;
    double decAmplitude, decAngle, decAngleRate;
    double meridianAmplitude, meridianAngle, meridianAngleRate;
};

// A body's axes: x toward longitude 0, z toward its north pole, y toward 90 degrees east
struct Axes {
    Vector3D x, y, z;
};

// The model for a body by name, or nullptr when there isn't one
const Model* find(const std::string& name);

// The body's axes at a Julian day, in the equatorial frame
Axes equatorialAxes(const Model& model, double julianDay);

// The same axes in EMMA's ecliptic frame
Axes eclipticAxes(const Model& model, double julianDay);

}
