#include "scene.hpp"

#include <algorithm>
#include <raymath.h>
#include <rlgl.h>

namespace solar {
    Scene::Scene(std::span<const BodyUpdate> bodies, const Sunlight& sunlight)
        : bodies_(bodies.begin(), bodies.end()), sunlight_(sunlight), stars_(SOLAR_STAR_CATALOGUE_PATH) {
        for (const auto& entry : bodies_)
            cameraBodies_.push_back(entry.body);
    }

    void Scene::update(const Ephemeris& ephemeris, double utc, double elapsedSeconds, Camera& camera, bool acceptMouse) {
        clouds_.update();
        for (const auto& entry : bodies_) {
            const glm::mat4 orientation = entry.orientation ? entry.orientation(utc, elapsedSeconds) : glm::mat4{1.0f};
            entry.body->update(ephemeris.position(entry.ephemerisBody, utc), orientation);
            if (entry.ephemerisBody == Body::Earth && clouds_.visible())
                entry.body->setCloudTexture(clouds_.texture());
        }
        camera.updateOrbitControls(cameraBodies_, acceptMouse);
    }

    void Scene::draw(const Camera& camera, glm::dvec3 sunPositionKm, float elapsedSeconds) const {
        const int screenHeight = std::max(GetScreenHeight(), 1);
        const float aspectRatio = static_cast<float>(GetScreenWidth()) / screenHeight;
        double farDistance = 4000.0;

        for (const auto& entry : bodies_) {
            const auto& body = *entry.body;
            const Vector3 position = camera.relativeScenePosition(body.positionKm());
            farDistance = std::max(
                farDistance, static_cast<double>(Vector3Length(position) + body.visibleRadius(camera, screenHeight)));
        }

        rlSetClipPlanes(rlGetCullDistanceNear(), farDistance * 1.1);

        stars_.draw(camera.raylibCamera());
        BeginMode3D(camera.raylibCamera());
        sunlight_.setSunPosition(camera.relativeScenePosition(
            sunPositionKm)); // Allows the Sunlight shader to light the bodies from the Sun's direction.
        sunlight_.setShadows(shadowsEnabled_);
        sunlight_.setCloudOpacity(cloudOpacity_);

        for (const auto& entry : bodies_) {
            const auto& body = *entry.body;
            if (camera.isVisible(body.positionKm(), body.radiusSceneUnits(), aspectRatio)) {
                sunlight_.setNightLights(entry.ephemerisBody == Body::Earth && earthLights_);
                sunlight_.setClouds(entry.ephemerisBody == Body::Earth && clouds_.visible());
                body.draw(camera.relativeScenePosition(body.positionKm()));
            }
        }

        // Draw transparent effects after solid bodies so the depth buffer can hide them.
        for (const auto& entry : bodies_) {
            const auto& body = *entry.body;
            if (camera.isVisible(body.positionKm(), body.visibleRadius(camera, screenHeight), aspectRatio)) {
                body.drawEffects(camera, elapsedSeconds, screenHeight);
            }
        }

        EndMode3D();
    }
} // namespace solar
