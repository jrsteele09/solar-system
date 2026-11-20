#pragma once

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

        void enable(bool enabled);
        void refresh();
        void update();
        bool enabled() const { return enabled_; }
        bool loading() const { return pending_.valid(); }
        bool failed() const { return failed_; }
        bool visible() const { return enabled_ && IsTextureValid(texture_); }
        Texture2D texture() const { return texture_; }

      private:
        std::future<std::vector<unsigned char>> pending_;
        Texture2D texture_{};
        bool enabled_ = false;
        bool failed_ = false;
        bool curlReady_ = false;
    };
} // namespace solar
