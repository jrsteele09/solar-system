#include "scene.hpp"
#include "astronomy/moons.hpp"
#include "astronomy/planet_orientation.hpp"

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
    constexpr double marsRadiusKm = 3389.5;
    constexpr double jupiterRadiusKm = 69911.0;
    constexpr double saturnRadiusKm = 58232.0;
    constexpr double uranusRadiusKm = 25362.0;
    constexpr double neptuneRadiusKm = 24622.0;
    constexpr double plutoRadiusKm = 1188.3;

    // Cloud texture URL and dimensions
    constexpr const char* earthCloudsUrl = "https://clouds.matteason.co.uk/images/2048x1024/clouds.jpg";
    constexpr int earthCloudsWidth = 2048;
    constexpr int earthCloudsHeight = 1024;

    Scene::Scene(double utcSeconds, double kmPerSceneUnit)
        : ephemeris_(SOLAR_EPHEMERIS_PATH, SOLAR_SATELLITE_PATH),
          surfaceShader_(SOLAR_SURFACE_SHADER_VERTEX_PATH, SOLAR_SURFACE_SHADER_FRAGMENT_PATH),
          sunGlow_(SOLAR_SUN_GLOW_PATH), stars_(SOLAR_STAR_CATALOGUE_PATH) {

        loadOrientationModels(SOLAR_PCK_PATH);

        // Make the Sun at its barycentric position
        const Mesh sunSphereMesh = Mesh::createSphere(
            1.0f, 32, 64); // The Sun is drawn with a glow, so it doesn't need a high-resolution mesh.

        bodies_.push_back(CelestialBody::Builder{sunSphereMesh, sunRadiusKm / kmPerSceneUnit}
                              .id(Body::Sun)
                              .position(ephemeris_.position(Body::Sun, utcSeconds))
                              .style(SurfaceStyle::Solid)
                              .color(Color{255, 255, 248, 255})
                              .addEffect(sunGlow_)
                              .build());

        const Mesh planetSphereMesh = Mesh::createSphere(1.0f, 96, 160);

        // Lambda to create a celestial body builder with common settings.
        auto body = [&](Body id, double radius, const char* texture) {
            CelestialBody::Builder builder{planetSphereMesh, radius / kmPerSceneUnit};
            builder.id(id)
                .position(ephemeris_.position(id, utcSeconds))
                .surface(texture)
                .surfaceShader(surfaceShader_)
                .orientation([id](double utc, double) {
                    auto rotation = orientation(id, utc);
                    if (id == Body::Pluto) {
                        // NASA's Pluto map is centred near 180 degrees longitude;
                        // the sphere mesh places the centre of its map at zero.
                        rotation[0] = -rotation[0];
                        rotation[2] = -rotation[2];
                    }
                    return rotation;
                });
            return builder;
        };

        const std::filesystem::path assetDir{SOLAR_ASSET_DIR};

        // Keep each moon next to its planet in the fly-to list.
        auto addSystem = [&](const CelestialBody::Builder& builder) {
            const Body planetId = bodies_.emplace_back(builder.build())->id();
            for (const auto& moon : majorMoons) {
                if (moon.planet != planetId)
                    continue;
                const auto mesh = moon.shapeModel
                                      ? Mesh::loadSbmtObj(assetDir / moon.shapeModel)
                                      : Mesh::createEllipsoid(glm::vec3(bodyAxesKm(moon.id)), moon.rings, moon.slices);
                bodies_.push_back(CelestialBody::Builder{mesh, 1.0 / kmPerSceneUnit}
                                      .id(moon.id)
                                      .position(ephemeris_.position(moon.id, utcSeconds))
                                      .orientation([id = moon.id](double utc, double) { return orientation(id, utc); })
                                      .surface((assetDir / moon.texture).string(), moon.maxTextureWidth)
                                      .surfaceShader(surfaceShader_)
                                      .build());
            }
        };
        addSystem(body(Body::Mercury, mercuryRadiusKm, SOLAR_MERCURY_TEXTURE_PATH));
        addSystem(body(Body::Venus, venusRadiusKm, SOLAR_VENUS_TEXTURE_PATH));
        addSystem(body(Body::Earth, earthRadiusKm, SOLAR_EARTH_TEXTURE_PATH)
                      .nightLights(SOLAR_EARTH_NIGHT_TEXTURE_PATH)
                      .clouds({earthCloudsUrl, earthCloudsWidth, earthCloudsHeight}));
        addSystem(body(Body::Mars, marsRadiusKm, SOLAR_MARS_TEXTURE_PATH));
        addSystem(body(Body::Jupiter, jupiterRadiusKm, SOLAR_JUPITER_TEXTURE_PATH));
        addSystem(body(Body::Saturn, saturnRadiusKm, SOLAR_SATURN_TEXTURE_PATH)
                      .rings(SOLAR_SATURN_RING_TEXTURE_PATH, 74658.0f / 58232.0f, 136780.0f / 58232.0f));
        addSystem(body(Body::Uranus, uranusRadiusKm, SOLAR_URANUS_TEXTURE_PATH));
        addSystem(body(Body::Neptune, neptuneRadiusKm, SOLAR_NEPTUNE_TEXTURE_PATH));
        addSystem(body(Body::Pluto, plutoRadiusKm, SOLAR_PLUTO_TEXTURE_PATH));
    }

    const CelestialBody& Scene::getCelestialBody(Body id) const {
        const auto entry = std::find_if(bodies_.begin(), bodies_.end(),
                                        [centre = planetCentre(id)](const auto& item) { return item->id() == centre; });

        if (entry == bodies_.end()) {
            throw std::out_of_range{"Body is not in the scene"};
        }

        return **entry;
    }

    CelestialBody& Scene::getCelestialBody(Body id) {
        const auto entry = std::find_if(bodies_.begin(), bodies_.end(),
                                        [centre = planetCentre(id)](const auto& item) { return item->id() == centre; });
        if (entry == bodies_.end()) {
            throw std::out_of_range{"Body is not in the scene"};
        }
        return **entry;
    }

    void Scene::update(double utc, double elapsedSeconds) {
        currentUtc_ = utc;
        for (const auto& body : bodies_)
            body->update(ephemeris_, utc, elapsedSeconds);
    }

    void Scene::draw(const Camera& camera, float elapsedSeconds) const {
        const int screenHeight = std::max(GetScreenHeight(), 1);
        const float aspectRatio = static_cast<float>(GetScreenWidth()) / screenHeight;
        double farDistance = 4000.0;
        double nearDistance = RL_CULL_DISTANCE_NEAR;

        for (const auto& entry : bodies_) {
            const auto& body = *entry;
            const Vector3 position = camera.relativeScenePosition(body.positionKm());
            const glm::vec3 relativePosition{position.x, position.y, position.z};
            // At the Martian moons the default near plane is larger than the whole body.
            const double clearance = glm::length(relativePosition) - body.radiusSceneUnits();
            nearDistance = std::min(nearDistance, std::max(0.000001, clearance * 0.25));
            farDistance = std::max(farDistance, static_cast<double>(glm::length(relativePosition) +
                                                                    body.visibleRadius(camera, screenHeight)));
        }

        rlSetClipPlanes(nearDistance, farDistance * 1.1);

        stars_.draw(camera.raylibCamera());
        BeginMode3D(camera.raylibCamera());
        // The shader uses camera-relative coordinates. Use the Sun's actual position for lighting.
        surfaceShader_.setSunPosition(camera.relativeScenePosition(ephemeris_.position(Body::Sun, currentUtc_)));
        surfaceShader_.setShadows(shadowsEnabled_);

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
