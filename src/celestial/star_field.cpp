#include "star_field.hpp"

#include "astronomy/coordinates.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdlib>
#include <fstream>
#include <glm/geometric.hpp>
#include <numbers>
#include <stdexcept>
#include <string>
#include <string_view>

namespace solar {
    constexpr double radiansPerDegree = std::numbers::pi_v<double> / 180.0;
    constexpr float faintestMagnitude = 6.5f;

    static bool number(std::string_view field, double& value) {
        const std::string text{field};
        char* end = nullptr;
        const double parsed = std::strtod(text.c_str(), &end);
        if (end == text.c_str() || !std::isfinite(parsed))
            return false;
        value = parsed;
        return true;
    }

    static Color starColor(double bv) {
        // Approximate the visible tint from the catalogue's blue-minus-visual colour index.
        const float warmth = std::clamp(static_cast<float>((bv + 0.2) / 2.0), 0.0f, 1.0f);
        return {static_cast<unsigned char>(210 + 45 * warmth), static_cast<unsigned char>(225 + 25 * warmth),
                static_cast<unsigned char>(255 - 65 * warmth), 255};
    }

    StarField::StarField(const char* cataloguePath) {
        std::ifstream file(cataloguePath);
        if (!file) {
            throw std::runtime_error{"Cannot open Hipparcos star catalogue"};
        }
        std::string line;
        while (std::getline(file, line)) {
            std::array<std::string_view, 38> fields{};
            std::size_t start = 0;
            bool complete = true;
            for (auto& field : fields) {
                const auto end = line.find('|', start);
                if (end == std::string::npos) {
                    complete = false;
                    break;
                }
                field = std::string_view{line}.substr(start, end - start);
                start = end + 1;
            }
            double magnitude, ra, dec;
            if (!complete || !number(fields[5], magnitude) || magnitude > faintestMagnitude || !number(fields[8], ra) ||
                !number(fields[9], dec) || ra < 0 || ra >= 360 || dec < -90 || dec > 90)
                continue;
            double bv = 0.65;
            number(fields[37], bv);
            const double alpha = ra * radiansPerDegree;
            const double delta = dec * radiansPerDegree;
            const glm::dvec3 equatorial{std::cos(delta) * std::cos(alpha), std::cos(delta) * std::sin(alpha),
                                        std::sin(delta)};
            stars_.push_back({glm::vec3(toScene(equatorial)), static_cast<float>(magnitude), starColor(bv)});
        }
        if (file.bad() || stars_.size() < 1000) {
            throw std::runtime_error{"Invalid Hipparcos star catalogue"};
        }
    }

    void StarField::draw(const Camera3D& camera) const {
        const glm::vec3 cameraPosition{camera.position.x, camera.position.y, camera.position.z};
        const glm::vec3 cameraTarget{camera.target.x, camera.target.y, camera.target.z};
        const glm::vec3 cameraUp{camera.up.x, camera.up.y, camera.up.z};
        const glm::vec3 forward = glm::normalize(cameraTarget - cameraPosition);
        const glm::vec3 right = glm::normalize(glm::cross(forward, cameraUp));
        const glm::vec3 up = glm::cross(right, forward);

        const float width = static_cast<float>(GetScreenWidth());
        const float height = static_cast<float>(GetScreenHeight());

        const float focal = height / (2.0f * std::tan(camera.fovy * static_cast<float>(radiansPerDegree) / 2.0f));

        for (const auto& star : stars_) {
            const glm::vec3& direction = star.direction;
            const float depth = glm::dot(direction, forward); // > 0 means the star is in front of the camera
            if (depth <= 0.0f) {
                continue;
            }

            const float x = width * 0.5f + focal * glm::dot(direction, right) / depth;
            const float y = height * 0.5f - focal * glm::dot(direction, up) / depth;

            if (x < -4 || x > width + 4 || y < -4 || y > height + 4) {
                continue;
            }
            const float brightness = std::clamp((faintestMagnitude - star.magnitude) / 7.0f, 0.0f, 1.0f);
            Color color = star.color;
            color.a = static_cast<unsigned char>(65 + 190 * std::sqrt(brightness));
            const float radius = 0.65f + 1.5f * brightness;
            DrawCircleV({x, y}, radius, color);
        }
    }
} // namespace solar
