#include "mesh.hpp"

#include <glm/geometric.hpp>

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
} // namespace solar
