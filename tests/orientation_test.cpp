#include "astronomy/planet_orientation.hpp"
#include "astronomy/ephemeris.hpp"
#include "mesh/mesh.hpp"
#include <glm/geometric.hpp>
#include <cmath>
#include <iostream>
#include <stdexcept>

int main() {
    try {
        const auto rings = solar::Mesh::createRings(1.3, 2.3, 12);
        if (rings.texcoords().size() != rings.vertices().size())
            throw std::runtime_error{"Ring texture coordinates are missing"};
        for (std::size_t i = 0; i < rings.vertices().size(); ++i) {
            const auto radius = glm::length(rings.vertices()[i]);
            const float expected = radius < 1.8f ? 0.0f : 1.0f;
            if (rings.texcoords()[i].x != expected)
                throw std::runtime_error{"Incorrect radial ring texture coordinate"};
        }
        solar::loadOrientationModels(SOLAR_PCK_PATH);
        solar::Ephemeris ephemeris{SOLAR_EPHEMERIS_PATH};
        constexpr solar::Body bodies[] = {solar::Body::Mercury, solar::Body::Venus, solar::Body::Earth,
            solar::Body::Moon, solar::Body::MarsSystem, solar::Body::JupiterSystem, solar::Body::SaturnSystem,
            solar::Body::UranusSystem, solar::Body::NeptuneSystem, solar::Body::PlutoSystem};
        for (double utc : {946728000.0, 1735689600.0, 1893456000.0}) {
            for (auto body : bodies) {
                const auto m = solar::orientation(body, utc);
                const glm::vec3 x{m[0]}, y{m[1]}, z{m[2]};
                if (!std::isfinite(x.x) || std::abs(glm::length(x) - 1) > 1e-5 ||
                    std::abs(glm::length(y) - 1) > 1e-5 || std::abs(glm::dot(x, y)) > 1e-5 ||
                    glm::dot(glm::cross(x, y), z) < 0.99999f)
                    throw std::runtime_error{"Invalid body orientation matrix"};
            }
            // The Moon's zero-longitude face should point roughly toward Earth.
            const auto moon = solar::orientation(solar::Body::Moon, utc);
            const auto towardEarth = glm::normalize(ephemeris.position(solar::Body::Earth, utc) -
                                                     ephemeris.position(solar::Body::Moon, utc));
            if (glm::dot(glm::dvec3{moon[0]}, towardEarth) < 0.95)
                throw std::runtime_error{"Lunar prime meridian does not face Earth"};
        }
        const auto first = solar::orientation(solar::Body::MarsSystem, 1735689600.0);
        solar::orientation(solar::Body::MarsSystem, 1735700000.0);
        const auto repeat = solar::orientation(solar::Body::MarsSystem, 1735689600.0);
        if (glm::length(glm::vec3(first[0] - repeat[0])) > 1e-7f)
            throw std::runtime_error{"Orientation depends on evaluation order"};
    } catch (const std::exception& e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
