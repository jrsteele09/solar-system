#pragma once

#include <raylib.h>

namespace solar {
    // Renders surfaces with sunlight, shadows, night lights and clouds.
    class SurfaceShader {
      public:
        SurfaceShader(const char* vertexPath, const char* fragmentPath);
        ~SurfaceShader();

        SurfaceShader(const SurfaceShader&) = delete;
        SurfaceShader& operator=(const SurfaceShader&) = delete;

        // The Sun's position uses the same camera-relative coordinates as the body.
        void setSunPosition(Vector3 position) const;
        void setNightLights(bool enabled) const;
        void setClouds(bool enabled) const;
        void setCloudOpacity(float opacity) const;
        void setShadows(bool enabled) const;
        Shader shader() const { return shader_; }

      private:
        Shader shader_{};
        int sunPositionLocation_;
        int nightLightsLocation_;
        int shadowsLocation_;
        int cloudsLocation_;
        int cloudOpacityLocation_;
    };
} // namespace solar
