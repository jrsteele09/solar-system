#pragma once

#include "astronomy/ephemeris.hpp"
#include "camera/camera.hpp"
#include "celestial/celestial_body.hpp"
#include "shader/sunlight.hpp"
#include "earth_clouds.hpp"
#include "star_field.hpp"

#include <functional>
#include <algorithm>
#include <span>
#include <vector>

namespace solar {
    // A body and the astronomy used to update it. A null orientation leaves it unrotated.
    struct BodyUpdate {
        CelestialBody* body;
        Body ephemerisBody;
        std::function<glm::mat4(double utcSeconds, double elapsedSeconds)> orientation;
    };

    // Scene borrows its bodies and lighting; they must outlive it.
    class Scene {
      public:
        Scene(std::span<const BodyUpdate> bodies, const Sunlight& sunlight);
        void update(const Ephemeris& ephemeris, double utc, double elapsedSeconds, Camera& camera, bool acceptMouse = true);
        void draw(const Camera& camera, glm::dvec3 sunPositionKm, float elapsedSeconds) const;
        std::span<const BodyUpdate> bodies() const { return bodies_; }
        bool earthLights() const { return earthLights_; }
        void setEarthLights(bool enabled) { earthLights_ = enabled; }
        bool cloudsEnabled() const { return clouds_.enabled(); }
        bool cloudsLoading() const { return clouds_.loading(); }
        bool cloudsFailed() const { return clouds_.failed(); }
        void setCloudsEnabled(bool enabled) { clouds_.enable(enabled); }
        float cloudOpacity() const { return cloudOpacity_; }
        void setCloudOpacity(float opacity) { cloudOpacity_ = std::clamp(opacity, 0.0f, 1.0f); }
        bool shadowsEnabled() const { return shadowsEnabled_; }
        void setShadowsEnabled(bool enabled) { shadowsEnabled_ = enabled; }

      private:
        std::vector<BodyUpdate> bodies_;
        std::vector<const CelestialBody*> cameraBodies_;
        const Sunlight& sunlight_;
        EarthClouds clouds_;
        StarField stars_;
        bool earthLights_ = true;
        float cloudOpacity_ = 0.58f;
        bool shadowsEnabled_ = true;
    };
} // namespace solar
