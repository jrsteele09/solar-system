#include "mesh.hpp"

#include <cmath>
#include <numbers>
#include <stdexcept>
#include <utility>
#include <vector>

namespace solar {
    Mesh Mesh::createSphere(float radius, int rings, int slices) {
        if (!std::isfinite(radius) || radius <= 0.0f || rings < 2 || slices < 3) {
            throw std::invalid_argument{"Sphere requires a positive finite radius, at least 2 rings and 3 slices"};
        }

        std::vector<glm::vec3> vertices;
        std::vector<glm::vec3> normals;
        std::vector<glm::vec2> texcoords;

        auto addCorner = [&](int y, int x) {
            glm::vec3 normal;
            if (y == 0) {
                normal = {0.0f, 1.0f, 0.0f};
            } else if (y == rings) {
                normal = {0.0f, -1.0f, 0.0f};
            } else {
                const float theta = static_cast<float>(y) / rings * std::numbers::pi_v<float>;
                const float phi = (static_cast<float>(x % slices) / slices * 2.0f - 1.0f) * std::numbers::pi_v<float>;
                normal = {std::sin(theta) * std::cos(phi), std::cos(theta), std::sin(theta) * std::sin(phi)};
            }
            vertices.push_back(normal * radius);
            normals.push_back(normal);

            // Put the centre of the map on +X, with the right side facing -Z.
            texcoords.emplace_back(1.0f - static_cast<float>(x) / slices, static_cast<float>(y) / rings);
        };

        for (int y = 0; y < rings; ++y) {
            for (int x = 0; x < slices; ++x) {
                if (y > 0) {
                    addCorner(y, x);
                    addCorner(y, x + 1);
                    addCorner(y + 1, x);
                }
                if (y < rings - 1) {
                    addCorner(y, x + 1);
                    addCorner(y + 1, x + 1);
                    addCorner(y + 1, x);
                }
            }
        }

        Mesh mesh{std::move(vertices)};
        mesh.normals_ = std::move(normals);
        mesh.texcoords_ = std::move(texcoords);
        return mesh;
    }
} // namespace solar
