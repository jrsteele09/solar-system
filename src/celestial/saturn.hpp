#pragma once

#include "celestial_body.hpp"

namespace solar {
    // Saturn's main rings share the planet's pole and are drawn after solid bodies.
    class Saturn : public CelestialBody {
      public:
        Saturn(const Mesh& sphere, double radius, glm::dvec3 position, const SurfaceTexture& texture,
               const char* ringTexturePath, const Sunlight& sunlight);
        ~Saturn() override;
        float visibleRadius(const Camera& camera, int screenHeight) const override;
        void drawEffects(const Camera& camera, float elapsedSeconds, int screenHeight) const override;

      private:
        SurfaceTexture ringTexture_;
        Model rings_{};
    };
} // namespace solar
