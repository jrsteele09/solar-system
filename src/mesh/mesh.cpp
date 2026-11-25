#include "mesh.hpp"

#include <glm/geometric.hpp>

#include <algorithm>
#include <array>
#include <cmath>
#include <fstream>
#include <numbers>
#include <sstream>
#include <stdexcept>
#include <string>
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

    Mesh Mesh::loadSbmtObj(const std::filesystem::path& path) {
        std::ifstream input(path);
        if (!input)
            throw std::runtime_error{"Cannot open moon shape model: " + path.string()};

        // SBMT publishes body-fixed vertices in kilometres and triangular plates
        // with one-based vertex indices. The files contain no normals or UVs.
        struct Vertex {
            glm::vec3 position;
            glm::vec2 uv;
            glm::vec3 normal{0.0f};
        };
        using Face = std::array<std::size_t, 3>;
        std::vector<Vertex> points;
        std::vector<Face> faces;
        const auto invalid = [&] { return std::runtime_error{"Invalid SBMT moon shape model: " + path.string()}; };
        std::string line;
        while (std::getline(input, line)) {
            if (line.empty() || line.front() == '#')
                continue;
            std::istringstream row(line);
            char type;
            if (!(row >> type))
                continue;
            if (type == 'v') {
                float x, y, z;
                if (!(row >> x >> y >> z) || !std::isfinite(x) || !std::isfinite(y) || !std::isfinite(z))
                    throw invalid();
                // Model +Z is north and +Y is east. The scene uses +Y and -Z respectively.
                constexpr float pi = std::numbers::pi_v<float>;
                points.push_back(
                    {{x, z, -y}, {0.5f + std::atan2(y, x) / (2 * pi), std::atan2(std::hypot(x, y), z) / pi}});
            } else if (type == 'f') {
                Face face;
                for (auto& index : face) {
                    if (!(row >> index) || index == 0 || index > points.size())
                        throw invalid();
                    --index;
                }
                faces.push_back(face);
                // Accumulate area-weighted normals while vertices are still shared.
                const auto& a = points[face[0]].position;
                const auto normal = glm::cross(points[face[1]].position - a, points[face[2]].position - a);
                for (auto index : face)
                    points[index].normal += normal;
            } else {
                throw invalid();
            }
            row >> std::ws;
            if (!row.eof())
                throw invalid();
        }
        if (input.bad() || faces.empty())
            throw invalid();
        for (auto& point : points) {
            if (!(glm::length(point.normal) > 0))
                throw invalid();
            point.normal = glm::normalize(point.normal);
        }

        Mesh mesh;
        mesh.vertices_.reserve(faces.size() * 3);
        mesh.normals_.reserve(faces.size() * 3);
        mesh.texcoords_.reserve(faces.size() * 3);
        for (const auto& face : faces) {
            const float maxU = std::max({points[face[0]].uv.x, points[face[1]].uv.x, points[face[2]].uv.x});
            for (auto index : face) {
                const auto& point = points[index];
                auto uv = point.uv;
                // Interpolate across the wrapped map edge at 180 degrees.
                if (maxU - uv.x > 0.5f)
                    uv.x += 1.0f;
                mesh.vertices_.push_back(point.position);
                mesh.normals_.push_back(point.normal);
                mesh.texcoords_.push_back(uv);
            }
        }
        return mesh;
    }

    Mesh Mesh::createRings(double innerRadius, double outerRadius, int segments) {
        std::vector<glm::vec3> vertices;
        std::vector<glm::vec2> texcoords;
        vertices.reserve(segments * 6);
        texcoords.reserve(segments * 6);

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

            // The ring image is a radial strip: left is inner, right is outer.
            texcoords.insert(texcoords.end(), {{0.0f, 0.5f}, {1.0f, 0.5f}, {1.0f, 0.5f},
                                               {0.0f, 0.5f}, {0.0f, 0.5f}, {1.0f, 0.5f}});
        }

        Mesh mesh = fromTriangles(vertices);
        mesh.texcoords_ = std::move(texcoords);
        return mesh;
    }
} // namespace solar
