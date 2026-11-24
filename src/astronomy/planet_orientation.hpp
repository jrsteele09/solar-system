#pragma once

#include "ephemeris.hpp"
#include <glm/mat4x4.hpp>

namespace solar {
    // Load the planetary orientation models before calling orientation().
    void loadOrientationModels(const char* pckPath);

    // Rotate a body's local mesh into the scene frame at the given UTC time.
    glm::mat4 orientation(Body body, double utcSeconds);
} // namespace solar
