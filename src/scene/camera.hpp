#pragma once

#include <glm/vec3.hpp>
#include <raylib.h>

namespace solar {
    class CelestialBody;
    class Scene;

    // Limit a camera movement so it stops before entering a body's bounding sphere.
    double limitCameraMovement(glm::dvec3 positionKm, glm::dvec3 direction, double distanceKm,
                               glm::dvec3 centreKm, double radiusKm);
    // Holds the camera's position in kilometres and prepares positions for drawing.
    class Camera {
      public:
        explicit Camera(double kmPerSceneUnit);
        Camera(const Scene& scene, double kmPerSceneUnit, double initialDistanceKm);
        Camera(glm::dvec3 positionKm, glm::dvec3 targetKm, double kmPerSceneUnit);

        // Look at a target from the direction of another position.
        void reset(glm::dvec3 targetKm, glm::dvec3 viewFromKm, double distanceKm);

        // Click a body to fly to it, drag it to orbit, or scroll to zoom.
        void updateOrbitControls(const Scene& scene, bool acceptMouse = true);

        // Fly to a body or face it from a chosen direction at the current distance.
        void flyToBody(const CelestialBody& body);
        void resetToBody(const CelestialBody& body, glm::dvec3 viewFromKm);
        const CelestialBody* followedBody() const { return followedBody_; }

        // Start and advance a flight toward a moving target.
        void flyTo(glm::dvec3 targetKm, float radiusSceneUnits);
        bool updateFlight(glm::dvec3 targetKm, float elapsedSeconds);

        // Rotate around a world position using the mouse movement in pixels.
        void orbit(glm::dvec3 targetKm, Vector2 mouseMovement);

        // Move along the view direction, stopping at celestial bodies.
        void zoom(float wheelMovement, const Scene& scene);

        // Find where an object is relative to the camera, in scene units.
        Vector3 relativeScenePosition(glm::dvec3 positionKm) const;

        // Check whether a body might be visible, including its edges.
        bool isVisible(glm::dvec3 positionKm, float radiusSceneUnits, float aspectRatio) const;

        // Raylib draws with the camera at the origin to keep coordinates precise.
        const Camera3D& raylibCamera() const { return camera_; }

      private:
        glm::dvec3 positionKm_;
        double kmPerSceneUnit_;
        Camera3D camera_{};
        const CelestialBody* orbitTarget_ = nullptr;
        const CelestialBody* followedBody_ = nullptr;
        Vector2 pressPosition_{};
        bool dragging_ = false;
        bool flying_ = false;
        bool flightDeparted_ = false;
        double flightElapsed_ = 0.0;
        double flightArrivalDistanceKm_ = 0.0;
        glm::dvec3 flightStartOffset_{};
        glm::dvec3 flightEndOffset_{};
        glm::dvec3 flightStartDirection_{};
        glm::dvec3 lastFollowedPosition_{};

        const CelestialBody* bodyUnderMouse(const Scene& scene) const;
    };
} // namespace solar
