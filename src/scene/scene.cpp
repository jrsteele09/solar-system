#include "scene.hpp"
#include "astronomy/planet_orientation.hpp"
#include "celestial/saturn.hpp"
#include "celestial/sun.hpp"

#include <algorithm>
#include <glm/geometric.hpp>
#include <rlgl.h>
#include <stdexcept>

namespace solar {
    // Mean radii in kilometres. DE442s uses system barycentres for the outer planets.
    constexpr double sunRadiusKm = 695700.0;
    constexpr double mercuryRadiusKm = 2439.4;
    constexpr double venusRadiusKm = 6051.8;
    constexpr double earthRadiusKm = 6371.0084;
    constexpr double moonRadiusKm = 1737.4;
    constexpr double marsRadiusKm = 3389.5;
    constexpr double jupiterRadiusKm = 69911.0;
    constexpr double saturnRadiusKm = 58232.0;
    constexpr double uranusRadiusKm = 25362.0;
    constexpr double neptuneRadiusKm = 24622.0;
    constexpr double plutoRadiusKm = 1188.3;

    static std::unique_ptr<CelestialBody> makeBody(Body id, double radiusKm, const char* texturePath, const Mesh& mesh,
                                                   const Ephemeris& ephemeris, const Sunlight& sunlight, double utc,
                                                   double kmPerSceneUnit, glm::dvec3 sunPositionKm,
                                                   const char* nightPath = nullptr, const char* ringPath = nullptr,
                                                   std::function<glm::mat4(double, double)> orientation = {}) {

        auto surface = std::make_unique<SurfaceTexture>(texturePath);
        std::unique_ptr<SurfaceTexture> night;
        std::unique_ptr<SurfaceTexture> rings;
        std::unique_ptr<CelestialBody> result;
        const double radius = radiusKm / kmPerSceneUnit;
        const glm::dvec3 position = sunPositionKm + ephemeris.position(id, utc);
        if (id == Body::Earth) {
            result = std::make_unique<Earth>(mesh, radius, position, surface.get(), &sunlight);
        } else if (id == Body::SaturnSystem) {
            rings = std::make_unique<SurfaceTexture>(ringPath);
            result = std::make_unique<Saturn>(mesh, radius, position, *surface, *rings, sunlight);
        } else {
            result = std::make_unique<CelestialBody>(mesh, radius, position, surface.get(), &sunlight);
        }
        if (nightPath) {
            night = std::make_unique<SurfaceTexture>(nightPath);
        }
        if (!orientation) {
            orientation = [id](double, double elapsed) { return planetOrientation(id, elapsed); };
        }
        result->configure(id, std::move(orientation), std::move(surface), std::move(night), std::move(rings));
        return result;
    }

    Scene::Scene(double utcSeconds, double kmPerSceneUnit, glm::dvec3 sunPositionKm)
        : ephemeris_(SOLAR_EPHEMERIS_PATH), sunlight_(SOLAR_SUNLIGHT_VERTEX_PATH, SOLAR_SUNLIGHT_FRAGMENT_PATH),
          sunGlow_(SOLAR_SUN_GLOW_PATH), sunPositionKm_(sunPositionKm), stars_(SOLAR_STAR_CATALOGUE_PATH) {

        // Make the Sun
        const Mesh sunSphereMesh = Mesh::createSphere(
            1.0f, 32, 64); // The Sun is drawn with a glow, so it doesn't need a high-resolution mesh.

        auto sun =
            std::make_unique<Sun>(sunSphereMesh, sunRadiusKm / kmPerSceneUnit, sunGlow_, Color{255, 255, 248, 255});
        sun->update(sunPositionKm_, glm::mat4{1.0f});
        sun->configure(Body::Sun, {});
        bodies_.push_back(std::move(sun));

        // Make the planets and the Moon
        const Mesh planetSphereMesh = Mesh::createSphere(1.0f, 96, 160);

        // Lambda to add a body to the scene
        auto add = [&](Body id, double radius, const char* texture, const char* night = nullptr,
                       const char* rings = nullptr, std::function<glm::mat4(double, double)> orientation = {}) {
            bodies_.push_back(makeBody(id, radius, texture, planetSphereMesh, ephemeris_, sunlight_, utcSeconds,
                                       kmPerSceneUnit, sunPositionKm_, night, rings, std::move(orientation)));
        };

        // Add the planets and the Moon
        add(Body::Mercury, mercuryRadiusKm, SOLAR_MERCURY_TEXTURE_PATH);
        add(Body::Venus, venusRadiusKm, SOLAR_VENUS_TEXTURE_PATH);
        add(Body::Earth, earthRadiusKm, SOLAR_EARTH_TEXTURE_PATH, SOLAR_EARTH_NIGHT_TEXTURE_PATH, nullptr,
            earthOrientation);
        add(Body::Moon, moonRadiusKm, SOLAR_MOON_TEXTURE_PATH, nullptr, nullptr, moonOrientation);
        add(Body::MarsSystem, marsRadiusKm, SOLAR_MARS_TEXTURE_PATH);
        add(Body::JupiterSystem, jupiterRadiusKm, SOLAR_JUPITER_TEXTURE_PATH);
        add(Body::SaturnSystem, saturnRadiusKm, SOLAR_SATURN_TEXTURE_PATH, nullptr, SOLAR_SATURN_RING_TEXTURE_PATH);
        add(Body::UranusSystem, uranusRadiusKm, SOLAR_URANUS_TEXTURE_PATH);
        add(Body::NeptuneSystem, neptuneRadiusKm, SOLAR_NEPTUNE_TEXTURE_PATH);
        add(Body::PlutoSystem, plutoRadiusKm, SOLAR_PLUTO_TEXTURE_PATH);
    }

    const CelestialBody& Scene::getCelestialBody(Body id) const {
        const auto entry =
            std::find_if(bodies_.begin(), bodies_.end(), [id](const auto& item) { return item->id() == id; });
        if (entry == bodies_.end())
            throw std::out_of_range{"Body is not in the scene"};
        return **entry;
    }

    CelestialBody& Scene::getCelestialBody(Body id) {
        const auto entry =
            std::find_if(bodies_.begin(), bodies_.end(), [id](const auto& item) { return item->id() == id; });
        if (entry == bodies_.end())
            throw std::out_of_range{"Body is not in the scene"};
        return **entry;
    }

    void Scene::update(double utc, double elapsedSeconds) {
        for (const auto& body : bodies_)
            body->update(ephemeris_, utc, elapsedSeconds, sunPositionKm_);
    }

    void Scene::draw(const Camera& camera, float elapsedSeconds) const {
        const int screenHeight = std::max(GetScreenHeight(), 1);
        const float aspectRatio = static_cast<float>(GetScreenWidth()) / screenHeight;
        double farDistance = 4000.0;

        for (const auto& entry : bodies_) {
            const auto& body = *entry;
            const Vector3 position = camera.relativeScenePosition(body.positionKm());
            const glm::vec3 relativePosition{position.x, position.y, position.z};
            farDistance = std::max(farDistance, static_cast<double>(glm::length(relativePosition) +
                                                                    body.visibleRadius(camera, screenHeight)));
        }

        rlSetClipPlanes(rlGetCullDistanceNear(), farDistance * 1.1);

        stars_.draw(camera.raylibCamera());
        BeginMode3D(camera.raylibCamera());
        // The shader uses camera-relative coordinates.
        sunlight_.setSunPosition(camera.relativeScenePosition(sunPositionKm_));
        sunlight_.setShadows(shadowsEnabled_);

        for (const auto& entry : bodies_) {
            const auto& body = *entry;
            if (camera.isVisible(body.positionKm(), body.radiusSceneUnits(), aspectRatio)) {
                body.draw(camera.relativeScenePosition(body.positionKm()));
            }
        }

        // Draw transparent effects after solid bodies so the depth buffer can hide them.
        for (const auto& entry : bodies_) {
            const auto& body = *entry;
            if (camera.isVisible(body.positionKm(), body.visibleRadius(camera, screenHeight), aspectRatio)) {
                body.drawEffects(camera, elapsedSeconds, screenHeight);
            }
        }

        EndMode3D();
    }
} // namespace solar
