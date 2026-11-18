#pragma once

#include <raylib.h>

namespace solar {
    // Lights a surface from the Sun, leaving the opposite side dark.
    class Sunlight {
      public:
        Sunlight(const char* vertexPath, const char* fragmentPath);
        ~Sunlight();

        Sunlight(const Sunlight&) = delete;
        Sunlight& operator=(const Sunlight&) = delete;

        // The Sun's position uses the same camera-relative coordinates as the body.
        void setSunPosition(Vector3 position) const;
        void setNightLights(bool enabled) const;
        void setShadows(bool enabled) const;
        Shader shader() const { return shader_; }

      private:
        Shader shader_{};
        int sunPositionLocation_;
        int nightLightsLocation_;
        int shadowsLocation_;
    };
} // namespace solar
