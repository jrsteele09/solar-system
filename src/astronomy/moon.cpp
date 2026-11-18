#include "moon.hpp"

#include <cmath>
#include <glm/geometric.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <numbers>

namespace solar {
    glm::mat4 moonOrientation(double /*utcSeconds*/, double elapsedSeconds) {
        constexpr double radians = std::numbers::pi_v<double> / 180.0;
        constexpr double poleRightAscension = 269.9949 * radians;
        constexpr double poleDeclination = 66.5392 * radians;
        constexpr double earthObliquity = 23.43928 * radians;
        constexpr double rotationDegreesPerDay = 13.17635815;
        constexpr double secondsPerDay = 86400.0;

        // Turn the Moon's rotation pole into the scene's coordinate system.
        const glm::dvec3 equatorial{std::cos(poleDeclination) * std::cos(poleRightAscension),
                                    std::cos(poleDeclination) * std::sin(poleRightAscension),
                                    std::sin(poleDeclination)};
        const glm::dvec3 pole{equatorial.x,
                              -std::sin(earthObliquity) * equatorial.y + std::cos(earthObliquity) * equatorial.z,
                              -(std::cos(earthObliquity) * equatorial.y + std::sin(earthObliquity) * equatorial.z)};
        const glm::dvec3 axis = glm::cross(glm::dvec3{0.0, 1.0, 0.0}, pole);
        const double axisLength = glm::length(axis);
        const double tilt = std::atan2(axisLength, pole.y);
        const glm::dvec3 tiltAxis = axisLength > 0.000001 ? axis / axisLength : glm::dvec3{1.0, 0.0, 0.0};

        // The starting surface angle is illustrative.
        const double period = secondsPerDay * 360.0 / rotationDegreesPerDay;
        const double spin = std::fmod(elapsedSeconds, period) * (2.0 * std::numbers::pi_v<double>) / period;
        const glm::dmat4 identity{1.0};
        return glm::mat4(glm::rotate(identity, tilt, tiltAxis) *
                         glm::rotate(identity, spin, glm::dvec3{0.0, 1.0, 0.0}));
    }
} // namespace solar
