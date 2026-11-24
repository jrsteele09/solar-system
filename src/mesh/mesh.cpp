#include "mesh.hpp"

#include <glm/geometric.hpp>

#include <numbers>
#include <stdexcept>
#include <utility>

namespace solar {
    Mesh::Mesh(std::vector<glm::vec3> vertices) : vertices_(std::move(vertices)) {
        normals_.resize(vertices_.size());

        for (std::size_t i = 0; i < vertices_.size(); i += 3) {
            const glm::vec3 edge1 = vertices_[i + 1] - vertices_[i];
            const glm::vec3 edge2 = vertices_[i + 2] - vertices_[i];
            const glm::vec3 cross = glm::cross(edge1, edge2);
            const float length = glm::length(cross);
            const glm::vec3 normal = length > 0.0f ? cross / length : glm::vec3{0.0f};

            normals_[i] = normal;
            normals_[i + 1] = normal;
            normals_[i + 2] = normal;
        }
    }

    Mesh Mesh::fromTriangles(std::span<const glm::vec3> vertices) {
        if (vertices.empty() || vertices.size() % 3 != 0) {
            throw std::invalid_argument{"Mesh vertices must contain complete triangles"};
        }

        return Mesh{std::vector<glm::vec3>{vertices.begin(), vertices.end()}};
    }

    Mesh Mesh::createRings(double innerRadius, double outerRadius, int segments) {
        std::vector<glm::vec3> vertices;
        vertices.reserve(segments * 6);

        for (int i = 0; i < segments; ++i) {
            const float a = 2.0f * std::numbers::pi_v<float> * i / segments;
            const float b = 2.0f * std::numbers::pi_v<float> * (i + 1) / segments;

            const float inner_a_x = std::cos(a) * static_cast<float>(innerRadius);
            const float inner_a_z = std::sin(a) * static_cast<float>(innerRadius);
            const float inner_b_x = std::cos(b) * static_cast<float>(innerRadius);
            const float inner_b_z = std::sin(b) * static_cast<float>(innerRadius);
            const float outer_a_x = std::cos(a) * static_cast<float>(outerRadius);
            const float outer_a_z = std::sin(a) * static_cast<float>(outerRadius);
            const float outer_b_x = std::cos(b) * static_cast<float>(outerRadius);
            const float outer_b_z = std::sin(b) * static_cast<float>(outerRadius);

            // Triangle 1: inner_a, outer_b, outer_a
            vertices.push_back({inner_a_x, 0.0f, inner_a_z});
            vertices.push_back({outer_b_x, 0.0f, outer_b_z});
            vertices.push_back({outer_a_x, 0.0f, outer_a_z});

            // Triangle 2: inner_a, inner_b, outer_b
            vertices.push_back({inner_a_x, 0.0f, inner_a_z});
            vertices.push_back({inner_b_x, 0.0f, inner_b_z});
            vertices.push_back({outer_b_x, 0.0f, outer_b_z});
        }

        return fromTriangles(vertices);
    }
} // namespace solar
