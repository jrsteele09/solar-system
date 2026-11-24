#include "astronomy/time.hpp"
#include "scene/camera.hpp"
#include "scene/controls_panel.hpp"
#include "scene/scene.hpp"

#include <chrono>
#include <raylib.h>

constexpr int screenWidth = 1280;
constexpr int screenHeight = 720;
constexpr int minimumWindowWidth = 480;
constexpr int minimumWindowHeight = 360;
constexpr int targetFps = 60;
constexpr double kmPerSceneUnit = 3185.5042;
constexpr double initialCameraDistanceSceneUnits = 7.0;
constexpr double initialCameraDistanceKm = initialCameraDistanceSceneUnits * kmPerSceneUnit;


int main() {
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(screenWidth, screenHeight, "Earth");
    SetWindowMinSize(minimumWindowWidth, minimumWindowHeight);
    SetTargetFPS(targetFps);
    {
        // Record the current UTC time and a steady clock time to measure elapsed time.
        const double startUtc = solar::currentUtcSeconds();
        const auto steadyStartTime = std::chrono::steady_clock::now();

        // Solar System Scene and controls
        solar::Scene scene{startUtc, kmPerSceneUnit};
        solar::ControlsPanel controls;

        // Place the camera between Earth and the Sun
        solar::Camera camera{scene, kmPerSceneUnit, initialCameraDistanceKm};

        // Main loop
        while (!WindowShouldClose()) {
            const double elapsed = solar::elapsedSeconds(steadyStartTime);
            const double utc = startUtc + elapsed;
            scene.update(utc, elapsed);
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
