#pragma once

#include "astronomy/ephemeris.hpp"
#include "clouds.hpp"
#include "mesh/mesh.hpp"
#include "shader/post_effect.hpp"
#include "shader/surface_shader.hpp"
#include "surface_texture.hpp"

#include <functional>
#include <glm/mat4x4.hpp>
#include <memory>
#include <optional>
#include <raylib.h>
#include <string>
#include <utility>
#include <vector>

namespace solar {
    class Camera;
    enum class SurfaceStyle { Wireframe, Solid };

    // Create and destroy celestial bodies while a raylib window and graphics context exist.
    class CelestialBody final {
      public:
        class Builder;

        ~CelestialBody();

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
        float radiusSceneUnits() const { return static_cast<float>(radiusSceneUnits_); }

        Body id() const { return id_; }

        bool lightsEnabled() const { return nightTexture_ && lightsEnabled_; }
        void setLightsEnabled(bool enabled) { lightsEnabled_ = enabled; }
        bool hasClouds() const { return clouds_ != nullptr; }
        Clouds& clouds();
        const Clouds& clouds() const;

        void update(const Ephemeris& ephemeris, double utc, double elapsedSeconds);

        // Set where the body is and which way it faces.
        void update(glm::dvec3 positionKm, const glm::mat4& orientation);

        // Draw the body's surface at its position relative to the camera.
        void draw(Vector3 position) const;

        // Return the radius of the body + its effects (for instance the suns glow + saturns rings)
        float visibleRadius(const Camera& camera, int screenHeight) const;

        // Draw effects after drawing the bodies
        void drawEffects(const Camera& camera, float elapsedSeconds, int screenHeight) const;

      private:
        explicit CelestialBody(const Builder& builder);

        // Material setup and per-body shader configuration.
        void setMaterialTexture(int map, Texture2D texture);
        void prepareLighting() const;

        // Astronomical identity and current position and orientation.
        Body id_;
        std::function<glm::mat4(double, double)> orientationFunction_;
        glm::dvec3 positionKm_;
        glm::mat4 orientation_{1.0f};

        // Body geometry, draw scale and bounding radius for selection.
        Model model_{};
        double size_;
        double radiusSceneUnits_ = 0.0;

        // Owned surface texture, shared shader and base appearance.
        std::unique_ptr<SurfaceTexture> surface_;
        const SurfaceShader* surfaceShader_;
        bool drawSolid_;
        Color color_;

        // Optional night-side emission and its visibility toggle.
        std::unique_ptr<SurfaceTexture> nightTexture_;
        bool lightsEnabled_ = true;

        // Optional cloud texture, download state and display settings.
        std::unique_ptr<Clouds> clouds_;

        // Optional rings, drawn separately from the solid surface.
        std::unique_ptr<SurfaceTexture> ringTexture_;
        Model rings_{};
        double ringOuterRadius_ = 1.0;

        // Optional post-rendering effects (glow, halos, etc), shared with the scene.
        std::vector<const PostEffect*> effects_;
    };

    // The mesh must outlive construction; shared shaders and glow must outlive the body.
    class CelestialBody::Builder {
      public:
        // Required geometry and draw scale.
        Builder(const Mesh& mesh, double radius) : mesh_(mesh), radius_(radius) {}

        // Astronomical identity, initial position and orientation updates.
        Builder& id(Body value) {
            id_ = value;
            return *this;
        }

        Builder& position(glm::dvec3 value) {
            position_ = value;
            return *this;
        }

        Builder& orientation(std::function<glm::mat4(double, double)> value) {
            orientation_ = std::move(value);
            return *this;
        }

        // Surface texture, shared shader and base appearance.
        Builder& surface(std::string path) {
            surface_ = std::move(path);
            return *this;
        }

        Builder& surfaceShader(const SurfaceShader& value) {
            surfaceShader_ = &value;
            return *this;
        }

        Builder& style(SurfaceStyle value) {
            style_ = value;
            return *this;
        }

        Builder& color(Color value) {
            color_ = value;
            return *this;
        }

        // Optional night-side emission.
        Builder& nightLights(std::string path) {
            nightTexture_ = std::move(path);
            return *this;
        }

        // Optional cloud imagery source.
        Builder& clouds(CloudSource source) {
            clouds_ = std::move(source);
            return *this;
        }

        // Optional rings, with radii expressed as multiples of the body's radius.
        Builder& rings(std::string path, float innerRadius, float outerRadius) {
            rings_ = RingSource{std::move(path), innerRadius, outerRadius};
            return *this;
        }

        // Add a post-rendering effect (glow, halos, etc).
        Builder& addEffect(const PostEffect& effect) {
            effects_.push_back(&effect);
            return *this;
        }

        // Validate configuration and create the body and its owned resources.
        std::unique_ptr<CelestialBody> build() const;

      private:
        friend class CelestialBody;

        // Astronomical identity and pose configuration.
        Body id_ = Body::Sun;
        glm::dvec3 position_{0.0};
        std::function<glm::mat4(double, double)> orientation_;

        // Borrowed mesh used during construction and its draw scale.
        const Mesh& mesh_;
        double radius_;

        // Surface asset, borrowed shader and base appearance.
        std::string surface_;
        const SurfaceShader* surfaceShader_ = nullptr;
        SurfaceStyle style_ = SurfaceStyle::Wireframe;
        Color color_ = WHITE;

        // Optional night-side emission asset.
        std::string nightTexture_;

        // Optional cloud download and image configuration.
        std::optional<CloudSource> clouds_;

        // Optional ring asset and geometry configuration.
        struct RingSource {
            std::string path;
            double innerRadius;
            double outerRadius;
        };
        std::optional<RingSource> rings_;

        // Optional borrowed post-rendering effects.
        std::vector<const PostEffect*> effects_;
    };
} // namespace solar
