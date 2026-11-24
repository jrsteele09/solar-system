#pragma once

#include <raylib.h>

namespace solar {
    // Abstract base class for post-rendering effects on celestial bodies.
    // Implementations can render glows, halos, rings, or other visual effects.
    class PostEffect {
      public:
        virtual ~PostEffect() = default;

        // Calculate the visible radius of this effect given the body's position, size, and camera properties.
        virtual float radius(Vector3 position, float bodyRadius, float fov, int screenHeight) const = 0;

        // Draw this effect at the given position with the specified elapsed time.
        virtual void draw(Camera3D camera, Vector3 position, float bodyRadius, float elapsedSeconds,
                          int screenHeight) const = 0;
    };
} // namespace solar
