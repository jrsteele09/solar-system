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

        void update(const Ephemeris& ephemeris, double utc, double elapsedSeconds, glm::dvec3 sunPositionKm) override {
            CelestialBody::update(ephemeris, utc, elapsedSeconds, sunPositionKm);
            clouds_.checkCloudDownload();
            if (clouds_.cloudsVisible())
                setCloudTexture(clouds_.cloudTexture());
        }

      protected:
        void prepareLighting() const override {
            if (sunlight_) {
                sunlight_->setNightLights(lightsEnabled_);
                sunlight_->setClouds(clouds_.cloudsVisible());
                sunlight_->setCloudOpacity(clouds_.cloudOpacity());
            }
        }

      private:
        EarthClouds clouds_;
        bool lightsEnabled_ = true;
    };
} // namespace solar
