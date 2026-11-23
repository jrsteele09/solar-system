#include "earth.hpp"

namespace solar {
    Earth::Earth(const Mesh& mesh, double radius, glm::dvec3 position, const SurfaceTexture& surface,
                 const Sunlight& sunlight, const char* nightTexturePath)
        : CelestialBody(mesh, radius, position, &surface, &sunlight), nightTexture_(nightTexturePath) {
        setMaterialTexture(MATERIAL_MAP_EMISSION, nightTexture_.texture());
    }

    void Earth::update(const Ephemeris& ephemeris, double utc, double elapsedSeconds, glm::dvec3 sunPositionKm) {
        CelestialBody::update(ephemeris, utc, elapsedSeconds, sunPositionKm);
        clouds_.checkCloudDownload();
        if (clouds_.cloudsVisible())
            setMaterialTexture(MATERIAL_MAP_OCCLUSION, clouds_.cloudTexture());
    }

    void Earth::prepareLighting() const {
        if (sunlight_) {
            sunlight_->setNightLights(lightsEnabled_);
            sunlight_->setClouds(clouds_.cloudsVisible());
            sunlight_->setCloudOpacity(clouds_.cloudOpacity());
        }
    }
} // namespace solar
