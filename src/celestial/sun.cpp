#include "sun.hpp"
#include "camera/camera.hpp"

namespace solar {
    Sun::Sun(const Mesh& mesh, double size, const SunGlow& glow, Color color)
        : CelestialBody(mesh, size, glm::dvec3{0.0}, nullptr, nullptr, SurfaceStyle::Solid, color), glow_(glow) {}

    float Sun::visibleRadius(const Camera& camera, int screenHeight) const {
        return glow_.radius(camera.relativeScenePosition(positionKm()), radiusSceneUnits(), camera.raylibCamera().fovy,
                            screenHeight);
    }

    void Sun::drawEffects(const Camera& camera, float elapsedSeconds, int screenHeight) const {
        glow_.draw(camera.raylibCamera(), camera.relativeScenePosition(positionKm()), radiusSceneUnits(),
                   elapsedSeconds, screenHeight);
    }
} // namespace solar
