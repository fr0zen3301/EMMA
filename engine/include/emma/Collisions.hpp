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

// Merges bodies that touched during the last piece of a step. Mass and momentum are
// kept, and each merge leaves a flash. Returns true if anything merged
bool resolveCollisions(std::vector<Body>& bodies, std::vector<MergeFlash>& flashes);

}
