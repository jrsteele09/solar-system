#pragma once

#include "post_effect.hpp"

#include <raylib.h>

namespace solar {
    // Draws the Sun's animated glow around its solid surface.
    class SunGlow final : public PostEffect {
      public:
        explicit SunGlow(const char* fragmentPath);
        ~SunGlow();
        SunGlow(const SunGlow&) = delete;
        SunGlow& operator=(const SunGlow&) = delete;

        float radius(Vector3 position, float sunRadius, float fov, int screenHeight) const override;
        void draw(Camera3D camera, Vector3 position, float sunRadius, float elapsedSeconds, int screenHeight) const override;

      private:
        Shader shader_{};
        Texture2D texture_{};
        int timeLocation_;
        int coreRadiusLocation_;
    };
} // namespace solar
