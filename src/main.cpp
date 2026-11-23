#include "astronomy/ephemeris.hpp"
#include "scene/camera.hpp"
#include "scene/controls_panel.hpp"
#include "scene/scene.hpp"

#include <chrono>
#include <raylib.h>

namespace {
    constexpr int screenWidth = 1280;
    constexpr int screenHeight = 720;
    constexpr int minimumWindowWidth = 480;
    constexpr int minimumWindowHeight = 360;
    constexpr int targetFps = 60;
    constexpr double kmPerSceneUnit = 3185.5042;
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
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(screenWidth, screenHeight, "Earth");
    SetWindowMinSize(minimumWindowWidth, minimumWindowHeight);
    SetTargetFPS(targetFps);
    {
        // Record the current UTC time and a steady clock time to measure elapsed time.
        const double startUtc = currentUtcSeconds();
        const auto start = std::chrono::steady_clock::now();

        // The ephemeris contains NASA JPL data for finding solar system bodies
        // at different times.
        const solar::Ephemeris ephemeris{SOLAR_EPHEMERIS_PATH};

        // Solar System Scene and controls
        const glm::dvec3 sunPositionKm{0.0, 0.0, 0.0};
        solar::Scene scene{ephemeris, startUtc, kmPerSceneUnit, sunPositionKm};
        solar::ControlsPanel controls;

        // Place the camera between Earth and the Sun
        solar::Camera camera{scene, kmPerSceneUnit, initialCameraDistanceKm, sunPositionKm};

        // Main loop
        while (!WindowShouldClose()) {
            const double elapsed = elapsedSeconds(start);
            const double utc = startUtc + elapsed;
            scene.update(ephemeris, utc, elapsed);
            camera.updateOrbitControls(scene, !controls.containsMouse(scene, camera));
            controls.update(scene, camera);

            BeginDrawing();
            ClearBackground(Color{0, 0, 0, 255});
            scene.draw(camera, static_cast<float>(elapsed));
            controls.draw(scene, camera);
            EndDrawing();
        }
    }
    CloseWindow();
}
