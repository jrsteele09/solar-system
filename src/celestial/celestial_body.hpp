#pragma once

#include "astronomy/ephemeris.hpp"
#include "mesh/mesh.hpp"
#include "shader/sunlight.hpp"
#include "surface_texture.hpp"

#include <functional>
#include <glm/mat4x4.hpp>
#include <memory>
#include <raylib.h>

namespace solar {
    class Camera;
    enum class SurfaceStyle { Wireframe, Solid };

    // Create and destroy celestial bodies while a raylib window and graphics context exist.
    class CelestialBody {
      public:
        // Construct
        // Mesh: The shape of the body, usually a sphere.
        // Size: The radius of the body in scene units.
        // InitialPositionKm: The body's position in kilometres from the Sun.
        // Texture: The surface texture
        // Sunlight: The shader that draws the body with lighting.
        // Style: Whether to draw the body as a solid sphere or a wireframe.
        // Color: The wireframe colour, or the solid colour if no texture is provided.
        CelestialBody(const Mesh& mesh, double size, glm::dvec3 initialPositionKm,
                      const SurfaceTexture* texture = nullptr, const Sunlight* sunlight = nullptr,
                      SurfaceStyle style = SurfaceStyle::Wireframe, Color color = WHITE);

        virtual ~CelestialBody();

        // Remove copy and move operations
        CelestialBody(const CelestialBody&) = delete;
        CelestialBody& operator=(const CelestialBody&) = delete;
        CelestialBody(CelestialBody&&) = delete;
        CelestialBody& operator=(CelestialBody&&) = delete;

        // Return the body's position in kilometres from the Sun.
        glm::dvec3 positionKm() const { return positionKm_; }

        // Return the body's orientation matrix, which rotates the body from its default orientation.
        const glm::mat4& orientation() const { return orientation_; }

        // Return the radius of the body used for drawing and selection.
        float radiusSceneUnits() const { return radiusSceneUnits_; }

        Body id() const { return id_; }

        // The body owns the textures used by its model and its astronomical update information.
        void configure(Body id, std::function<glm::mat4(double, double)> orientation,
                       std::unique_ptr<SurfaceTexture> surface = {}, std::unique_ptr<SurfaceTexture> night = {},
                       std::unique_ptr<SurfaceTexture> rings = {});

        virtual void update(const Ephemeris& ephemeris, double utc, double elapsedSeconds, glm::dvec3 sunPositionKm);

        // Set where the body is and which way it faces.
        void update(glm::dvec3 positionKm, const glm::mat4& orientation);

        // Night texture - used for Earth Lights
        void setNightTexture(const SurfaceTexture& texture);

        // Cloud texture - used for Earth Clouds
        void setCloudTexture(Texture2D texture);

        // Draw the body's surface at its position relative to the camera.
        void draw(Vector3 position) const;

        // Return the radius of the body + its effects (for instance the suns glow + saturns rings)
        virtual float visibleRadius(const Camera& camera, int screenHeight) const;

        // Draw effects after drawing the bodies
        virtual void drawEffects(const Camera& camera, float elapsedSeconds, int screenHeight) const;

      protected:
        // Each body configures its own use of the shared lighting shader before drawing.
        virtual void prepareLighting() const;
        const Sunlight* sunlight_;

      private:
        std::unique_ptr<SurfaceTexture> surface_;
        std::unique_ptr<SurfaceTexture> night_;
        std::unique_ptr<SurfaceTexture> rings_;
        Body id_ = Body::Sun;
        std::function<glm::mat4(double, double)> orientationFunction_;
        Model model_{};
        bool drawSolid_;
        Color color_;
        float size_;
        float radiusSceneUnits_ = 0.0f;
        glm::dvec3 positionKm_;
        glm::mat4 orientation_{1.0f};
    };
} // namespace solar
