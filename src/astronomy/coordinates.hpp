#pragma once

#include <cmath>
#include <glm/vec3.hpp>
#include <numbers>

namespace solar {
    constexpr double pi = std::numbers::pi_v<double>;
    constexpr double arcseconds = pi / (180.0 * 3600.0);

    // Turn JPL coordinates into our scene axes: Y is north of the orbital plane.
    inline glm::dvec3 toScene(glm::dvec3 v) {
        constexpr double obliquity = 84381.448 * arcseconds;
        return {v.x, -std::sin(obliquity) * v.y + std::cos(obliquity) * v.z,
                -(std::cos(obliquity) * v.y + std::sin(obliquity) * v.z)};
    }

} // namespace solar
