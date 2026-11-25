#pragma once

#include <raylib.h>

namespace solar {
    // Loads a surface map and keeps it on the graphics card until it is destroyed.
    class SurfaceTexture {
      public:
        explicit SurfaceTexture(const char* path, int maxWidth = 0, bool preserveAlpha = true);
        ~SurfaceTexture();

        SurfaceTexture(const SurfaceTexture&) = delete;
        SurfaceTexture& operator=(const SurfaceTexture&) = delete;

        Texture2D texture() const { return texture_; }

      private:
        Texture2D texture_{};
    };
} // namespace solar
