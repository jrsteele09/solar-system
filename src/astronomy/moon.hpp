#pragma once

#include <glm/mat4x4.hpp>

namespace solar {
    // Orient the Moon's pole and rotate it from its starting angle over time.
    glm::mat4 moonOrientation(double utcSeconds, double elapsedSeconds);
} // namespace solar
