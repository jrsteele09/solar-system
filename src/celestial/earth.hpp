#pragma once

#include "celestial_body.hpp"
#include "earth_clouds.hpp"

namespace solar {
    class Earth : public CelestialBody {
      public:
        using CelestialBody::CelestialBody;

        EarthClouds& clouds() { return clouds_; }
        const EarthClouds& clouds() const { return clouds_; }

        bool lightsEnabled() const { return lightsEnabled_; }
        void setLightsEnabled(bool enabled) { lightsEnabled_ = enabled; }

        void update(const Ephemeris& ephemeris, double utc, double elapsedSeconds, glm::dvec3 sunPositionKm) override;

      protected:
        void prepareLighting() const override;

      private:
        EarthClouds clouds_;
        bool lightsEnabled_ = true;
    };
} // namespace solar
