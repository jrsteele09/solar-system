#pragma once

#include "mesh/mesh.hpp"
#include "shader/sunlight.hpp"
#include "surface_texture.hpp"

#include <glm/mat4x4.hpp>
#include <raylib.h>

namespace solar {
    class Camera;
    enum class SurfaceStyle { Wireframe, Solid };

    // Create and destroy celestial bodies while a raylib window and graphics context exist.
    class CelestialBody {
      public:
        // size scales the supplied mesh; use a unit-radius sphere for a radius of size.
        // Supplied texture and sunlight must outlive the body.
        CelestialBody(const Mesh& mesh, double size, glm::dvec3 positionKm, const SurfaceTexture* texture = nullptr,
                      const Sunlight* sunlight = nullptr, SurfaceStyle style = SurfaceStyle::Wireframe,
                      Color color = WHITE);

        // Release the model from the graphics card.
        virtual ~CelestialBody();

        // Each body owns its model, so it cannot be copied or moved.
        CelestialBody(const CelestialBody&) = delete;
        CelestialBody& operator=(const CelestialBody&) = delete;
        CelestialBody(CelestialBody&&) = delete;
        CelestialBody& operator=(CelestialBody&&) = delete;

        // Return the body's position in kilometres from the Sun.
        glm::dvec3 positionKm() const { return positionKm_; }
        const glm::mat4& orientation() const { return orientation_; }

        // Return the radius of the body used for drawing and selection.
        float radiusSceneUnits() const { return radiusSceneUnits_; }

        // Set where the body is and which way it faces.
        void update(glm::dvec3 positionKm, const glm::mat4& orientation);

        // The night texture must outlive the body.
        void setNightTexture(const SurfaceTexture& texture);
        void setCloudTexture(Texture2D texture);

        // Draw the body's surface at its position relative to the camera.
        void draw(Vector3 position) const;

        // Include any effects, such as the Sun's glow, when checking visibility.
        virtual float visibleRadius(const Camera& camera, int screenHeight) const;

        // Draw effects after solid bodies; most bodies have none.
        virtual void drawEffects(const Camera& camera, float elapsedSeconds, int screenHeight) const;

      private:
        Model model_{};
        bool drawSolid_;
        Color color_;
        float size_;
        float radiusSceneUnits_ = 0.0f;
        glm::dvec3 positionKm_;
        glm::mat4 orientation_{1.0f};
    };
} // namespace solar
