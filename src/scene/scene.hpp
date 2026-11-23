#pragma once

#include "astronomy/ephemeris.hpp"
#include "camera.hpp"
#include "celestial/celestial_body.hpp"
#include "celestial/earth.hpp"
#include "celestial/star_field.hpp"
#include "shader/sun_glow.hpp"
#include "shader/sunlight.hpp"

#include <functional>
#include <memory>
#include <span>
#include <vector>

namespace solar {
    // Scene owns its bodies, textures and lighting.
    class Scene {
      public:
        Scene(double utcSeconds, double kmPerSceneUnit, glm::dvec3 sunPositionKm);

        const CelestialBody& getCelestialBody(Body id) const;
        CelestialBody& getCelestialBody(Body id);

        void update(double utc, double elapsedSeconds);
        void draw(const Camera& camera, float elapsedSeconds) const;
        std::span<const std::unique_ptr<CelestialBody>> bodies() const { return bodies_; }

        bool shadowsEnabled() const { return shadowsEnabled_; }
        void setShadowsEnabled(bool enabled) { shadowsEnabled_ = enabled; }

      private:
        Ephemeris ephemeris_;
        Sunlight sunlight_;
        SunGlow sunGlow_;
        glm::dvec3 sunPositionKm_;
        std::vector<std::unique_ptr<CelestialBody>> bodies_;
        StarField stars_;
        bool shadowsEnabled_ = true;
    };
} // namespace solar
