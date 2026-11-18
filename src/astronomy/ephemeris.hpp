#pragma once

#include <glm/vec3.hpp>

#include <cstdint>
#include <string>
#include <vector>

namespace solar {
    // JPL uses the centres of Earth, Moon, Mercury and Venus, but the combined
    // planet-and-moons centre of mass for the outer planets.
    enum class Body {
        Sun = 10,
        Mercury = 199,
        Venus = 299,
        Earth = 399,
        Moon = 301,
        MarsSystem = 4,
        JupiterSystem = 5,
        SaturnSystem = 6,
        UranusSystem = 7,
        NeptuneSystem = 8,
        PlutoSystem = 9
    };

    // Display name for an ephemeris body.
    const char* bodyName(Body body);

    // Reads NASA JPL's pre-calculated solar system data and finds a body's
    // position in kilometres from the Sun at a given time.
    class Ephemeris {
      public:
        // Open the JPL file so it is ready to answer questions about positions.
        explicit Ephemeris(const std::string& path);

        // Find where a body is relative to the Sun at a given UTC clock time.
        glm::dvec3 position(Body body, double utcSeconds) const;

        // Find the same position using the time scale used inside the JPL file.
        glm::dvec3 positionAtTdb(Body body, double tdbSeconds) const;

      private:
        // A segment describes where to find one body's positions in the file.
        struct Segment {
            double start, end, initial, interval;
            int target, centre, first, recordSize, recordCount;
        };

        // The original JPL file, kept in memory for quick lookups.
        std::vector<std::uint8_t> ephemerisBytes_;
        std::vector<Segment> segments_;

        // Read one number from the JPL file at the given location.
        double readDouble(std::size_t offset) const;

        // Find a body's position relative to the solar system's centre of mass.
        glm::dvec3 barycentricPosition(int target, double seconds) const;
    };
} // namespace solar
