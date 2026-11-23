#include "sun_glow.hpp"

#include <glm/geometric.hpp>
#include <glm/vec3.hpp>
#include <rlgl.h>

#include <algorithm>
#include <cmath>
#include <numbers>
#include <stdexcept>

namespace solar {
    constexpr float haloScale = 4.0f;
    constexpr float minimumDiameterPixels = 18.0f;
    constexpr float haloWidth = 2.7f;

    SunGlow::SunGlow(const char* fragmentPath) {
        shader_ = LoadShader(nullptr, fragmentPath);
        if (!IsShaderValid(shader_) || shader_.id == rlGetShaderIdDefault()) {
            throw std::runtime_error{"Could not load the Sun glow shader"};
        }
        timeLocation_ = GetShaderLocation(shader_, "animationTime");
        coreRadiusLocation_ = GetShaderLocation(shader_, "coreRadius");
        if (timeLocation_ < 0 || coreRadiusLocation_ < 0) {
            UnloadShader(shader_);
            throw std::runtime_error{"Sun glow shader is missing its settings"};
        }
        Image image = GenImageColor(1, 1, WHITE);
        texture_ = LoadTextureFromImage(image);
        UnloadImage(image);
        if (!IsTextureValid(texture_)) {
            UnloadShader(shader_);
            throw std::runtime_error{"Could not create the Sun glow texture"};
        }
    }

    SunGlow::~SunGlow() {
        UnloadTexture(texture_);
        UnloadShader(shader_);
    }

    float SunGlow::radius(Vector3 position, float sunRadius, float fov, int screenHeight) const {
        const float distance = glm::length(glm::vec3{position.x, position.y, position.z});
        const float radians = fov * std::numbers::pi_v<float> / 180.0f;
        const float minimumDiameter = 2.0f * distance * std::tan(radians * 0.5f) * minimumDiameterPixels /
                                      static_cast<float>(std::max(screenHeight, 1));
        return std::max(sunRadius * haloScale * haloWidth, minimumDiameter) * 0.5f;
    }

    void SunGlow::draw(Camera3D camera, Vector3 position, float sunRadius, float elapsedSeconds,
                       int screenHeight) const {
        const float glowRadius = radius(position, sunRadius, camera.fovy, screenHeight);
        const float diameter = glowRadius * 2.0f;
        const float coreRadius = sunRadius / glowRadius;
        const glm::vec3 cameraPosition{camera.position.x, camera.position.y, camera.position.z};
        const glm::vec3 cameraTarget{camera.target.x, camera.target.y, camera.target.z};
        const glm::vec3 cameraUp{camera.up.x, camera.up.y, camera.up.z};
        const glm::vec3 sunPosition{position.x, position.y, position.z};
        const glm::vec3 forward = glm::normalize(cameraTarget - cameraPosition);
        const glm::vec3 right = glm::normalize(glm::cross(forward, cameraUp));
        const glm::vec3 up = glm::normalize(glm::cross(right, forward));
        const glm::vec3 towardCamera = glm::normalize(-sunPosition);
        const glm::vec3 face = sunPosition + towardCamera * (sunRadius * 1.01f);

        SetShaderValue(shader_, timeLocation_, &elapsedSeconds, SHADER_UNIFORM_FLOAT);
        SetShaderValue(shader_, coreRadiusLocation_, &coreRadius, SHADER_UNIFORM_FLOAT);
        rlDrawRenderBatchActive();
        rlDisableDepthMask();
        BeginBlendMode(BLEND_ADDITIVE);
        BeginShaderMode(shader_);
        DrawBillboardPro(camera, texture_, {0, 0, 1, 1}, {face.x, face.y, face.z}, {up.x, up.y, up.z},
                         {diameter, diameter}, {glowRadius, glowRadius}, 0.0f, WHITE);
        EndShaderMode();
        EndBlendMode();
        rlDrawRenderBatchActive();
        rlEnableDepthMask();

        // Short rays keep the glow visible even close to the Sun.
        for (int i = 0; i < 8; ++i) {
            const float angle = i * std::numbers::pi_v<float> / 4.0f + elapsedSeconds * 0.008f;
            const glm::vec3 direction = right * std::cos(angle) + up * std::sin(angle);
            const float length = sunRadius * (1.23f + 0.07f * std::sin(elapsedSeconds * 1.4f + i * 2.7f));
            const glm::vec3 start = face + direction * (sunRadius * 0.98f);
            const glm::vec3 end = face + direction * length;
            DrawLine3D({start.x, start.y, start.z}, {end.x, end.y, end.z}, Color{255, 240, 187, 210});
        }
    }
} // namespace solar
