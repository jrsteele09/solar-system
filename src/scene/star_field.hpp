#pragma once

#include <glm/vec3.hpp>
#include <raylib.h>
#include <vector>

namespace solar {
    // A distant background of real stars, fixed relative to the solar system.
    class StarField {
      public:
        explicit StarField(const char* cataloguePath);
        void draw(const Camera3D& camera) const;

      private:
        struct Star {
            glm::vec3 direction;
            float magnitude;
            Color color;
        };
        std::vector<Star> stars_;
    };
} // namespace solar
