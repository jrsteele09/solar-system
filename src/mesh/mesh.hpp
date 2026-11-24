#pragma once

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

#include <span>
#include <vector>

namespace solar {
    class Mesh {
      public:
        static Mesh fromTriangles(std::span<const glm::vec3> vertices);
        static Mesh createSphere(float radius = 1.0f, int rings = 192, int slices = 320);
        static Mesh createRings(double innerRadius, double outerRadius, int segments = 256);

        const std::vector<glm::vec3>& vertices() const { return vertices_; }
        const std::vector<glm::vec3>& normals() const { return normals_; }
        const std::vector<glm::vec2>& texcoords() const { return texcoords_; }

      private:
        explicit Mesh(std::vector<glm::vec3> vertices);

        std::vector<glm::vec3> vertices_;
        std::vector<glm::vec3> normals_;
        std::vector<glm::vec2> texcoords_;
    };
} // namespace solar
