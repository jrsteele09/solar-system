#pragma once

#include <glm/mat4x4.hpp>
#include "ephemeris.hpp"

namespace solar {
    // Find the direction of Earth's north pole and Greenwich at the given UTC time.
    glm::mat4 earthOrientation(double utcSeconds, double elapsedSeconds);

    // Orient the Moon's pole and rotate it from its starting angle over time.
    glm::mat4 moonOrientation(double utcSeconds, double elapsedSeconds);

    // Approximate orientation for the other planets; their starting longitude is illustrative.
    glm::mat4 planetOrientation(Body body, double elapsedSeconds);
} // namespace solar
