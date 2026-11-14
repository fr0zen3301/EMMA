#pragma once
#include "emma/Body.hpp"
#include "emma/Vector3D.hpp"
#include <vector>

namespace emma {

// The ring shown where two bodies merged
struct MergeFlash {
    Vector3D position;
    float age = 0.f;  // seconds of real time
};

// Merges every pair of bodies that touched during the last piece of a step and
// removes the absorbed ones. Mass and momentum are conserved, the heavier body
// keeps its name and color, and each merge adds a flash
bool resolveCollisions(std::vector<Body>& bodies, std::vector<MergeFlash>& flashes);

}
