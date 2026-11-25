#pragma once

#include <glm/vec3.hpp>

#include <cstdint>
#include <string>
#include <vector>

namespace solar {
    // NAIF IDs distinguish planets and moons from their system barycentres.
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
        PlutoSystem = 9,
        Mars = 499,
        Jupiter = 599,
        Saturn = 699,
        Uranus = 799,
        Neptune = 899,
        Pluto = 999,
        Phobos = 401,
        Deimos = 402,
        Io = 501,
        Europa = 502,
        Ganymede = 503,
        Callisto = 504,
        Mimas = 601,
        Enceladus = 602,
        Tethys = 603,
        Dione = 604,
        Rhea = 605,
        Titan = 606,
        Iapetus = 608,
        Miranda = 705,
        Ariel = 701,
        Umbriel = 702,
        Titania = 703,
        Oberon = 704,
        Triton = 801,
        Charon = 901
    };

    // Resolve a system ID when a caller wants the rendered planet or its IAU frame.
    inline Body planetCentre(Body body) {
        const int id = static_cast<int>(body);
        return (id >= 4 && id <= 9) ? static_cast<Body>(100 * id + 99) : body;
    }

    // Display name for an ephemeris body.
    const char* bodyName(Body body);

    // An ephemeris is data used to find where celestial bodies are at a given time.
    // This class reads NASA JPL's pre-calculated data and returns positions in
    // kilometres relative to the Sun.
    class Ephemeris {
      public:
        // Open the JPL file so it is ready to answer questions about positions.
        explicit Ephemeris(const std::string& path);
        Ephemeris(const std::string& path, const char* satellitePath);
        ~Ephemeris();
        Ephemeris(const Ephemeris&) = delete;
        Ephemeris& operator=(const Ephemeris&) = delete;

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
        int satelliteHandle_ = 0;

        // Read one number from the JPL file at the given location.
        double readDouble(std::size_t offset) const;

        // Find a body's position relative to the solar system's centre of mass.
        glm::dvec3 barycentricPosition(int target, double seconds) const;
    };
} // namespace solar
