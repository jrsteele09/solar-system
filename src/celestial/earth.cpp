#include "earth.hpp"

namespace solar {
    void Earth::update(const Ephemeris& ephemeris, double utc, double elapsedSeconds, glm::dvec3 sunPositionKm) {
        CelestialBody::update(ephemeris, utc, elapsedSeconds, sunPositionKm);
        clouds_.checkCloudDownload();
        if (clouds_.cloudsVisible())
            setCloudTexture(clouds_.cloudTexture());
    }

    void Earth::prepareLighting() const {
        if (sunlight_) {
            sunlight_->setNightLights(lightsEnabled_);
            sunlight_->setClouds(clouds_.cloudsVisible());
            sunlight_->setCloudOpacity(clouds_.cloudOpacity());
        }
    }
} // namespace solar
