#include "earth.hpp"
#include "coordinates.hpp"

#include <cmath>
#include <glm/geometric.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace solar {
    using detail::arcseconds;
    using detail::day;
    using detail::j2000Unix;
    using detail::pi;
    using detail::terrestrialTime;
    using detail::toScene;

    // IAU 2006 precession and Greenwich mean sidereal time, adapted from ERFA;
    // see licenses/erfa.txt. UTC approximates UT1; nutation and polar motion are omitted.
    glm::mat4 earthOrientation(double utc, double /*elapsedSeconds*/) {
        const double days = (utc - j2000Unix) / day;
        const double t = terrestrialTime(utc) / (day * 36525.0);
        const double rotation =
            2 * pi * std::fmod(.7790572732640 + std::fmod(days, 1.0) + .00273781191135448 * days, 1.0);
        const double sidereal =
            rotation +
            (.014506 + (4612.156534 + (1.3915817 + (-.00000044 + (-.000029956 - .0000000368 * t) * t) * t) * t) * t) *
                arcseconds;
        // These angles account for the slow change in Earth’s axis over time.
        const double gamma =
            (-.052928 + (10.556378 + (.4932044 + (-.00031238 + (-.000002788 + .0000000260 * t) * t) * t) * t) * t) *
            arcseconds;
        const double phi =
            (84381.412819 + (-46.811016 + (.0511268 + (.00053289 + (-.000000440 - .0000000176 * t) * t) * t) * t) * t) *
            arcseconds;
        const double psi =
            (-.041775 + (5038.481484 + (1.5584175 + (-.00018522 + (-.000026452 - .0000000148 * t) * t) * t) * t) * t) *
            arcseconds;
        const double epsilon =
            (84381.406 + (-46.836769 + (-.0001831 + (.00200340 + (-.000000576 - .0000000434 * t) * t) * t) * t) * t) *
            arcseconds;
        const glm::dmat4 identity{1.0};
        const auto frame =
            glm::rotate(identity, gamma, glm::dvec3{0, 0, 1}) * glm::rotate(identity, phi, glm::dvec3{1, 0, 0}) *
            glm::rotate(identity, -psi, glm::dvec3{0, 0, 1}) * glm::rotate(identity, -epsilon, glm::dvec3{1, 0, 0});
        // Build the Earth’s local axes: Greenwich, north, and the third perpendicular axis.
        const glm::dvec3 x = toScene(glm::dvec3(frame * glm::dvec4{std::cos(sidereal), std::sin(sidereal), 0, 0}));
        const glm::dvec3 y = toScene(glm::dvec3(frame * glm::dvec4{0, 0, 1, 0}));
        const glm::dvec3 z = glm::cross(x, y);
        glm::mat4 result{1.0f};
        result[0] = glm::vec4(glm::vec3(x), 0);
        result[1] = glm::vec4(glm::vec3(y), 0);
        result[2] = glm::vec4(glm::vec3(z), 0);
        return result;
    }
} // namespace solar
