#pragma once

#include <algorithm>
#include <future>
#include <raylib.h>
#include <string>
#include <vector>

namespace solar {
    struct CloudSource {
        std::string url;
        int width = 0;
        int height = 0;
    };

    // Downloads cloud data on a worker; image decoding and texture upload happen on the render thread.
    class Clouds {
      public:
        explicit Clouds(CloudSource source);
        ~Clouds();
        Clouds(const Clouds&) = delete;
        Clouds& operator=(const Clouds&) = delete;

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
        CloudSource source_;
        std::future<std::vector<unsigned char>> cloudDownload_;
        Texture2D cloudTexture_{};
        bool cloudsEnabled_ = false;
        bool cloudRefreshFailed_ = false;
        float cloudOpacity_ = 0.58f;
    };
} // namespace solar
