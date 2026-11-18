#pragma once

#include <glm/mat4x4.hpp>

namespace solar {
    // Find the direction of Earth's north pole and Greenwich at the given UTC time.
    glm::mat4 earthOrientation(double utcSeconds, double elapsedSeconds);
} // namespace solar
