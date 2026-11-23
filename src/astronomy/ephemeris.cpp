#include "ephemeris.hpp"
#include "coordinates.hpp"
#include "time.hpp"

#include <algorithm>
#include <bit>
#include <cmath>
#include <fstream>
#include <iterator>
#include <stdexcept>

namespace solar {

    const char* bodyName(Body body) {
        switch (body) {
        case Body::Sun:
            return "Sun";
        case Body::Mercury:
            return "Mercury";
        case Body::Venus:
            return "Venus";
        case Body::Earth:
            return "Earth";
        case Body::Moon:
            return "Moon";
        case Body::MarsSystem:
            return "Mars";
        case Body::JupiterSystem:
            return "Jupiter";
        case Body::SaturnSystem:
            return "Saturn";
        case Body::UranusSystem:
            return "Uranus";
        case Body::NeptuneSystem:
            return "Neptune";
        case Body::PlutoSystem:
            return "Pluto";
        }
        return "Unknown";
    }

    // Read a 32-bit little-endian unsigned integer, rejecting incomplete data.
    std::uint32_t readUint32(const std::vector<std::uint8_t>& data, std::size_t offset) {
        // Check parameters
        if (offset > data.size() || data.size() - offset < 4) {
            throw std::runtime_error("Invalid ephemeris file");
        }

        std::uint32_t value = 0;

        // Loop through the 4 bytes and construct the integer in little-endian order
        for (int i = 0; i < 4; ++i) {
            value |= std::uint32_t(data[offset + i]) << (8 * i);
        }

        return value;
    }

    // Read a double stored in the file’s little-endian byte order.
    double Ephemeris::readDouble(std::size_t offset) const {
        // Check parameters
        if (offset > ephemerisBytes_.size() || ephemerisBytes_.size() - offset < 8)
            throw std::runtime_error("Invalid ephemeris file");

        // Read the 8 bytes and construct a 64-bit integer in little-endian order
        std::uint64_t bits = 0;
        for (int i = 0; i < 8; ++i) {
            bits |= std::uint64_t(ephemerisBytes_[offset + i]) << (8 * i);
        }

        return std::bit_cast<double>(bits);
    }

    Ephemeris::Ephemeris(const std::string& path) {
        std::ifstream file(path, std::ios::binary);
        if (!file)
            throw std::runtime_error("Cannot open ephemeris: " + path);

        // Populate the vector with the file’s bytes. This is faster than reading one number at a time.
        ephemerisBytes_ = {std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>()};

        if (ephemerisBytes_.size() < 1024 ||
            std::string(ephemerisBytes_.begin(), ephemerisBytes_.begin() + 8) != "DAF/SPK " ||
            std::string(ephemerisBytes_.begin() + 88, ephemerisBytes_.begin() + 96) != "LTL-IEEE" ||
            readUint32(ephemerisBytes_, 8) != 2 || readUint32(ephemerisBytes_, 12) != 6)

            throw std::runtime_error("Invalid ephemeris header");

        // Read the directory so each body’s data can be found without searching the file.
        auto record = readUint32(ephemerisBytes_, 76);

        while (record != 0) {
            const std::size_t offset = (std::size_t(record) - 1) * 1024;
            if (offset > ephemerisBytes_.size() || ephemerisBytes_.size() - offset < 1024)
                throw std::runtime_error("Invalid ephemeris record");
            const auto next = static_cast<std::uint32_t>(readDouble(offset));
            const int count = static_cast<int>(readDouble(offset + 16));
            if (count < 0 || count > 25 || segments_.size() + count > 32 || next == record)
                throw std::runtime_error("Invalid ephemeris summaries");
            for (int i = 0; i < count; ++i) {
                const auto summary = offset + 24 + i * 40;
                const auto first = readUint32(ephemerisBytes_, summary + 32),
                           last = readUint32(ephemerisBytes_, summary + 36);
                if (readUint32(ephemerisBytes_, summary + 24) != 1 || readUint32(ephemerisBytes_, summary + 28) != 2 ||
                    first < 1 || last < first + 4 || last > ephemerisBytes_.size() / 8)
                    throw std::runtime_error("Unsupported ephemeris segment");
                const auto directory = (std::size_t(last) - 4) * 8;
                Segment s{readDouble(summary),
                          readDouble(summary + 8),
                          readDouble(directory),
                          readDouble(directory + 8),
                          static_cast<int>(readUint32(ephemerisBytes_, summary + 16)),
                          static_cast<int>(readUint32(ephemerisBytes_, summary + 20)),
                          static_cast<int>(first),
                          static_cast<int>(readDouble(directory + 16)),
                          static_cast<int>(readDouble(directory + 24))};
                if (s.interval <= 0 || s.recordCount < 1 || s.recordSize < 5 || (s.recordSize - 2) % 3 != 0 ||
                    std::int64_t(s.recordCount) * s.recordSize + 4 != last - first + 1)
                    throw std::runtime_error("Invalid ephemeris segment");
                segments_.push_back(s);
            }
            record = next;
        }
        if (segments_.empty())
            throw std::runtime_error("Empty ephemeris");
    }

    // Find the target's position relative to the solar system barycentre, its centre of mass.
    glm::dvec3 Ephemeris::barycentricPosition(int target, double seconds) const {
        glm::dvec3 result{0};
        for (std::size_t depth = 0; depth <= segments_.size(); ++depth) {
            if (target == 0)
                return result;
            auto it = std::find_if(segments_.rbegin(), segments_.rend(), [&](const Segment& s) {
                return s.target == target && seconds >= s.start && seconds <= s.end;
            });
            if (it == segments_.rend())
                throw std::out_of_range("Time is outside ephemeris coverage");
            const Segment& s = *it;
            const int index =
                std::clamp(static_cast<int>(std::floor((seconds - s.initial) / s.interval)), 0, s.recordCount - 1);
            const std::size_t offset = (std::size_t(s.first - 1) + std::size_t(index) * s.recordSize) * 8;
            const double t = (seconds - readDouble(offset)) / readDouble(offset + 8);
            const int n = (s.recordSize - 2) / 3;
            // The file stores coefficients, which we evaluate for the requested time.
            for (int axis = 0; axis < 3; ++axis) {
                const auto coeff = offset + (2 + axis * n) * 8;
                double b1 = 0, b2 = 0;
                for (int i = n - 1; i >= 1; --i) {
                    const double b = 2 * t * b1 - b2 + readDouble(coeff + i * 8);
                    b2 = b1;
                    b1 = b;
                }
                result[axis] += t * b1 - b2 + readDouble(coeff);
            }
            target = s.centre;
        }
        throw std::runtime_error("Ephemeris has a cyclic centre chain");
    }

    glm::dvec3 Ephemeris::positionAtTdb(Body body, double seconds) const {
        if (!std::isfinite(seconds))
            throw std::invalid_argument("Invalid ephemeris time");
        // Subtract the Sun’s position to put it at the origin.
        return toScene(barycentricPosition(static_cast<int>(body), seconds) - barycentricPosition(10, seconds));
    }

    glm::dvec3 Ephemeris::position(Body body, double utcSeconds) const {
        return positionAtTdb(body, ephemerisTime(utcSeconds));
    }

} // namespace solar
