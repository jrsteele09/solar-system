#pragma once

#include "scene.hpp"

namespace solar {
    // A small overlay for navigating between the bodies in the scene.
    class ControlsPanel {
      public:
        ControlsPanel();
        ~ControlsPanel();
        ControlsPanel(const ControlsPanel&) = delete;
        ControlsPanel& operator=(const ControlsPanel&) = delete;
        bool containsMouse(const Scene& scene, const Camera& camera) const;
        void update(Scene& scene, Camera& camera);
        void draw(const Scene& scene, const Camera& camera) const;

      private:
        Font font_{};
        bool open_ = false;
        bool hidden_ = false;
        bool draggingCloudOpacity_ = false;
        std::size_t listFirst_ = 0;
    };
} // namespace solar
