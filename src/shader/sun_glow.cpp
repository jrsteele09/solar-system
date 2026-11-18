#include "sun_glow.hpp"

#include <raymath.h>
#include <rlgl.h>

#include <algorithm>
#include <cmath>
#include <numbers>
#include <stdexcept>

namespace solar {
    namespace {
        constexpr float haloScale = 4.0f;
        constexpr float minimumDiameterPixels = 18.0f;
        constexpr float haloWidth = 2.7f;
    } // namespace

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
        const float distance = Vector3Length(position);
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
        const Vector3 forward = Vector3Normalize(Vector3Subtract(camera.target, camera.position));
        const Vector3 right = Vector3Normalize(Vector3CrossProduct(forward, camera.up));
        const Vector3 up = Vector3Normalize(Vector3CrossProduct(right, forward));
        const Vector3 towardCamera = Vector3Normalize(Vector3Negate(position));
        const Vector3 face = Vector3Add(position, Vector3Scale(towardCamera, sunRadius * 1.01f));

        SetShaderValue(shader_, timeLocation_, &elapsedSeconds, SHADER_UNIFORM_FLOAT);
        SetShaderValue(shader_, coreRadiusLocation_, &coreRadius, SHADER_UNIFORM_FLOAT);
        rlDrawRenderBatchActive();
        rlDisableDepthMask();
        BeginBlendMode(BLEND_ADDITIVE);
        BeginShaderMode(shader_);
        DrawBillboardPro(camera, texture_, {0, 0, 1, 1}, face, up, {diameter, diameter}, {glowRadius, glowRadius}, 0.0f,
                         WHITE);
        EndShaderMode();
        EndBlendMode();
        rlDrawRenderBatchActive();
        rlEnableDepthMask();

        // Short rays keep the glow visible even close to the Sun.
        for (int i = 0; i < 8; ++i) {
            const float angle = i * std::numbers::pi_v<float> / 4.0f + elapsedSeconds * 0.008f;
            const Vector3 direction =
                Vector3Add(Vector3Scale(right, std::cos(angle)), Vector3Scale(up, std::sin(angle)));
            const float length = sunRadius * (1.23f + 0.07f * std::sin(elapsedSeconds * 1.4f + i * 2.7f));
            DrawLine3D(Vector3Add(face, Vector3Scale(direction, sunRadius * 0.98f)),
                       Vector3Add(face, Vector3Scale(direction, length)), Color{255, 240, 187, 210});
        }
    }
} // namespace solar
