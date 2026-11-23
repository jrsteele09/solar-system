#pragma once

#include <algorithm>
#include <future>
#include <raylib.h>
#include <vector>

namespace solar {
    // Downloads cloud data on a worker; image decoding and texture upload happen on the render thread.
    class EarthClouds {
      public:
        EarthClouds();
        ~EarthClouds();
        EarthClouds(const EarthClouds&) = delete;
        EarthClouds& operator=(const EarthClouds&) = delete;

        void enableClouds(bool enabled);
        void refreshClouds();
        void checkCloudDownload();
        bool cloudsEnabled() const { return cloudsEnabled_; }
        bool cloudsLoading() const { return cloudDownload_.valid(); }
        bool cloudRefreshFailed() const { return cloudRefreshFailed_; }
        bool cloudsVisible() const { return cloudsEnabled_ && IsTextureValid(cloudTexture_); }
        Texture2D cloudTexture() const { return cloudTexture_; }
        float cloudOpacity() const { return cloudOpacity_; }
        void setCloudOpacity(float opacity) { cloudOpacity_ = std::clamp(opacity, 0.0f, 1.0f); }

      private:
        std::future<std::vector<unsigned char>> cloudDownload_;
        Texture2D cloudTexture_{};
        bool cloudsEnabled_ = false;
        bool cloudRefreshFailed_ = false;
        bool cloudHttpReady_ = false;
        float cloudOpacity_ = 0.58f;
    };
} // namespace solar
