#include "planet_orientation.hpp"
#include "coordinates.hpp"
#include "time.hpp"

#include <cmath>
#include <glm/geometric.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <numbers>

namespace solar {
    using detail::arcseconds;
    using detail::j2000UnixSeconds;
    using detail::pi;
    using detail::secondsPerDay;
    using detail::terrestrialTime;
    using detail::toScene;

    // IAU 2006 precession and Greenwich mean sidereal time, adapted from ERFA;
    // see licenses/erfa.txt. UTC approximates UT1; nutation and polar motion are omitted.
    glm::mat4 earthOrientation(double utc, double /*elapsedSeconds*/) {
        const double days = (utc - j2000UnixSeconds) / secondsPerDay;
        const double t = terrestrialTime(utc) / (secondsPerDay * 36525.0);
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

    glm::mat4 planetOrientation(Body body, double elapsedSeconds) {
        // IAU pole directions and signed sidereal rotation rates, from NAIF pck00011.tpc.
        struct Spin { Body body; double poleRa, poleDec, degreesPerDay; };
        constexpr Spin spins[] = {
            {Body::Mercury, 281.0103, 61.4155, 6.1385108},
            {Body::Venus, 272.76, 67.16, -1.4813688},
            {Body::MarsSystem, 317.269202, 54.432516, 350.891982443297},
            {Body::JupiterSystem, 268.056595, 64.495303, 870.536},
            {Body::SaturnSystem, 40.589, 83.537, 810.7939024},
            {Body::UranusSystem, 257.311, -15.175, -501.1600928},
            {Body::NeptuneSystem, 299.36, 43.46, 541.1397757},
            {Body::PlutoSystem, 132.993, -6.163, 56.3625225},
        };
        for (const auto& spin : spins) {
            if (spin.body != body) continue;
            constexpr double radiansPerDegree = std::numbers::pi_v<double> / 180.0;
            const double ra = spin.poleRa * radiansPerDegree;
            const double dec = spin.poleDec * radiansPerDegree;
            const glm::dvec3 pole = detail::toScene({std::cos(dec) * std::cos(ra), std::cos(dec) * std::sin(ra),
                                                     std::sin(dec)});
            const glm::dvec3 axis = glm::cross(glm::dvec3{0, 1, 0}, pole);
            const double axisLength = glm::length(axis);
            const double tilt = std::atan2(axisLength, pole.y);
            const glm::dvec3 tiltAxis = axisLength > 1e-6 ? axis / axisLength : glm::dvec3{1, 0, 0};
            const double periodSeconds = secondsPerDay * 360.0 / spin.degreesPerDay;
            const double angle = std::fmod(elapsedSeconds, std::abs(periodSeconds)) *
                                 (2.0 * std::numbers::pi_v<double>) / periodSeconds;
            const glm::dmat4 identity{1.0};
            return glm::mat4(glm::rotate(identity, tilt, tiltAxis) * glm::rotate(identity, angle, glm::dvec3{0, 1, 0}));
        }
        return glm::mat4{1.0f};
    }
} // namespace solar
