#pragma once

#include <chrono>
#include <cmath>

namespace solar {
    const double j2000UnixSeconds = 946728000.0;
    const double secondsPerDay = 86400.0;

    // UTC effective dates, as Unix seconds, from NAIF naif0012.tls, starting with 1972-07-01.
    const double leapDates[] = {78796800,  94694400,   126230400,  157766400,  189302400,  220924800, 252460800,
                                283996800, 315532800,  362793600,  394329600,  425865600,  489024000, 567993600,
                                631152000, 662688000,  709948800,  741484800,  773020800,  820454400, 867715200,
                                915148800, 1136073600, 1230768000, 1341100800, 1435708800, 1483228800};

    // Terrestrial time is a steady clock used for calculations near Earth.
    // Convert UTC to that clock by accounting for leap seconds, measured in seconds since J2000.
    inline double terrestrialTime(double utc) {
        constexpr double initialTaiMinusUtcSeconds = 10.0;
        constexpr double ttMinusTaiSeconds = 32.184;
        double taiMinusUtcSeconds = initialTaiMinusUtcSeconds;
        for (double date : leapDates) {
            if (utc >= date) {
                ++taiMinusUtcSeconds;
            }
        }
        return utc - j2000UnixSeconds + taiMinusUtcSeconds + ttMinusTaiSeconds;
    }

    // The JPL ephemeris uses a clock that accounts for how gravity and motion affect time.
    // Convert UTC to an approximation of that clock, measured in seconds since J2000.
    inline double ephemerisTime(double utc) {
        const double meanAnomalyAtJ2000Radians = 6.239996;
        const double meanAnomalyRateRadiansPerSecond = 1.99096871e-7;
        const double tdbCorrectionAmplitudeSeconds = 0.001657;
        const double anomalyCorrectionRadians = 0.01671;
        const int correctionIterations = 3;
        const double tt = terrestrialTime(utc);
        double tdb = tt;
        for (int i = 0; i < correctionIterations; ++i) {
            const double anomaly = meanAnomalyAtJ2000Radians + meanAnomalyRateRadiansPerSecond * tdb;
            tdb = tt + tdbCorrectionAmplitudeSeconds * std::sin(anomaly + anomalyCorrectionRadians * std::sin(anomaly));
        }
        return tdb;
    }

    // Read the current UTC time once; later clock corrections won't make the simulation jump.
    inline double currentUtcSeconds() {
        return std::chrono::duration<double>(std::chrono::system_clock::now().time_since_epoch()).count();
    }

    // A steady clock measures elapsed time without jumping when the system clock changes.
    inline double elapsedSeconds(std::chrono::steady_clock::time_point start) {
        return std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count();
    }

} // namespace solar
