#pragma once

#include <cmath>
#include <glm/vec3.hpp>
#include <numbers>

namespace solar::detail {
    constexpr double pi = std::numbers::pi_v<double>;
    constexpr double j2000Unix = 946728000.0;
    constexpr double day = 86400.0;
    constexpr double arcseconds = pi / (180.0 * 3600.0);

    // UTC effective dates from NAIF naif0012.tls, starting with 1972-07-01.
    constexpr double leapDates[] = {78796800,  94694400,   126230400,  157766400,  189302400,  220924800, 252460800,
                                    283996800, 315532800,  362793600,  394329600,  425865600,  489024000, 567993600,
                                    631152000, 662688000,  709948800,  741484800,  773020800,  820454400, 867715200,
                                    915148800, 1136073600, 1230768000, 1341100800, 1435708800, 1483228800};

    // Convert UTC to the continuous time scale used for Earth’s orientation.
    inline double terrestrialTime(double utc) {
        double offset = 10;
        for (double date : leapDates)
            if (utc >= date)
                ++offset;
        return utc - j2000Unix + offset + 32.184;
    }

    // Convert UTC to the time scale used by the JPL file.
    inline double ephemerisTime(double utc) {
        const double tt = terrestrialTime(utc);
        double tdb = tt;
        for (int i = 0; i < 3; ++i) {
            const double anomaly = 6.239996 + 1.99096871e-7 * tdb;
            tdb = tt + .001657 * std::sin(anomaly + .01671 * std::sin(anomaly));
        }
        return tdb;
    }

    // Turn JPL coordinates into our scene axes: Y is north of the orbital plane.
    inline glm::dvec3 toScene(glm::dvec3 v) {
        constexpr double obliquity = 84381.448 * arcseconds;
        return {v.x, -std::sin(obliquity) * v.y + std::cos(obliquity) * v.z,
                -(std::cos(obliquity) * v.y + std::sin(obliquity) * v.z)};
    }

} // namespace solar::detail
