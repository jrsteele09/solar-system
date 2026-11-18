#pragma once

#include "celestial_body.hpp"
#include "shader/sun_glow.hpp"

namespace solar {
    // The Sun is a solid, unlit body with an animated glow.
    class Sun : public CelestialBody {
      public:
        Sun(const Mesh& mesh, double size, const SunGlow& glow, Color color);
        float visibleRadius(const Camera& camera, int screenHeight) const override;
        void drawEffects(const Camera& camera, float elapsedSeconds, int screenHeight) const override;

      private:
        const SunGlow& glow_;
    };
} // namespace solar
