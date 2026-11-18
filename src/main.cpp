#include "astronomy/earth.hpp"
#include "astronomy/ephemeris.hpp"
#include "astronomy/moon.hpp"
#include "camera/camera.hpp"
#include "celestial/celestial_body.hpp"
#include "celestial/sun.hpp"
#include "celestial/surface_texture.hpp"
#include "mesh/mesh.hpp"
#include "scene/controls_panel.hpp"
#include "scene/scene.hpp"
#include "shader/sun_glow.hpp"
#include "shader/sunlight.hpp"

#include <chrono>
#include <raylib.h>

namespace {
    constexpr int screenWidth = 1280;
    constexpr int screenHeight = 720;
    constexpr int targetFps = 60;
    constexpr double kmPerSceneUnit = 3185.5042;
    constexpr double earthRadiusKm = 6371.0084;
    constexpr double moonRadiusKm = 1737.4;
    constexpr double sunRadiusKm = 695700.0;
    constexpr Color sunColor{255, 255, 248, 255};
    constexpr double earthSceneUnits = earthRadiusKm / kmPerSceneUnit;
    constexpr double moonSceneUnits = moonRadiusKm / kmPerSceneUnit;
    constexpr double sunSceneUnits = sunRadiusKm / kmPerSceneUnit;
    constexpr double initialCameraDistanceSceneUnits = 7.0;
    constexpr double initialCameraDistanceKm = initialCameraDistanceSceneUnits * kmPerSceneUnit;

    // Read the current UTC time once; later clock corrections won't make the simulation jump.
    inline double currentUtcSeconds() {
        return std::chrono::duration<double>(std::chrono::system_clock::now().time_since_epoch()).count();
    }

    // A steady clock measures elapsed time without jumping when the system clock changes.
    inline double elapsedSeconds(std::chrono::steady_clock::time_point start) {
        return std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count();
    }

} // namespace

int main() {
    InitWindow(screenWidth, screenHeight, "Earth");
    SetTargetFPS(targetFps);
    {
        // Record the current UTC time and a steady clock time to measure elapsed time.
        const double startUtc = currentUtcSeconds();
        const auto start = std::chrono::steady_clock::now();

        // The ephemeris contains NASA JPL data for finding solar system bodies
        // at different times. Load it once for fast position lookups.
        const solar::Ephemeris ephemeris{SOLAR_EPHEMERIS_PATH};

        // The sunlight shader lights a celestial body's surface from the Sun, leaving the opposite side dark.
        const solar::Sunlight sunlight{SOLAR_SUNLIGHT_VERTEX_PATH, SOLAR_SUNLIGHT_FRAGMENT_PATH};

        // Setup the Earth as a celestial body with a mesh and initial position.
        const solar::SurfaceTexture earthTexture{SOLAR_EARTH_TEXTURE_PATH};
        const solar::SurfaceTexture earthNightTexture{SOLAR_EARTH_NIGHT_TEXTURE_PATH};

        const solar::Mesh planetMesh = solar::Mesh::createSphere(1.0f, 96, 160);
        solar::CelestialBody earth{planetMesh, earthSceneUnits, ephemeris.position(solar::Body::Earth, startUtc),
                                   &earthTexture, &sunlight};
        earth.setNightTexture(earthNightTexture);

        // The Moon uses the same sphere mesh and sunlight as Earth.
        const solar::SurfaceTexture moonTexture{SOLAR_MOON_TEXTURE_PATH};
        solar::CelestialBody moon{planetMesh, moonSceneUnits, ephemeris.position(solar::Body::Moon, startUtc),
                                  &moonTexture, &sunlight};

        // The Sun is a solid body at the origin, with its own glow.
        const solar::SunGlow sunGlow{SOLAR_SUN_GLOW_PATH};
        const solar::Mesh sunMesh = solar::Mesh::createSphere(1.0f, 32, 64);
        solar::Sun sun{sunMesh, sunSceneUnits, sunGlow, sunColor};

        // Setup the Scene
        const std::array sceneBodies{solar::BodyUpdate{&earth, solar::Body::Earth, solar::earthOrientation},
                                     solar::BodyUpdate{&moon, solar::Body::Moon, solar::moonOrientation},
                                     solar::BodyUpdate{&sun, solar::Body::Sun, nullptr}};

        solar::Scene scene{sceneBodies, sunlight};
        solar::ControlsPanel controls;

        // Place the camera between Earth and the Sun, which is at the origin.
        solar::Camera camera{kmPerSceneUnit};
        camera.reset(earth.positionKm(), sun.positionKm(), initialCameraDistanceKm);
        camera.resetToBody(earth, sun.positionKm());

        // Main loop
        while (!WindowShouldClose()) {
            const double elapsed = elapsedSeconds(start);
            const double utc = startUtc + elapsed;
            scene.update(ephemeris, utc, elapsed, camera, !controls.containsMouse(scene, camera));
            controls.update(scene, camera, sun.positionKm());

            BeginDrawing();
            ClearBackground(Color{0, 0, 0, 255});
            scene.draw(camera, sun.positionKm(), static_cast<float>(elapsed));
            controls.draw(scene, camera);
            EndDrawing();
        }
    }
    CloseWindow();
}
