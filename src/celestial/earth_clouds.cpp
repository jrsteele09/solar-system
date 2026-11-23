#include "earth_clouds.hpp"

#include <chrono>
#include <cstdint>
#include <curl/curl.h>

namespace solar {
    constexpr char cloudUrl[] = "https://clouds.matteason.co.uk/images/2048x1024/clouds.jpg";
    constexpr std::size_t maxBytes = 8 * 1024 * 1024;

    static size_t receive(char* data, size_t size, size_t count, void* context) {
        auto& bytes = *static_cast<std::vector<unsigned char>*>(context);
        if (size != 0 && count > (maxBytes - bytes.size()) / size)
            return 0;
        const auto length = size * count;
        try {
            bytes.insert(bytes.end(), data, data + length);
        } catch (...) {
            return 0;
        }
        return length;
    }

    static std::vector<unsigned char> download() {
        std::vector<unsigned char> bytes;
        CURL* curl = curl_easy_init();
        if (!curl)
            return {};
        curl_easy_setopt(curl, CURLOPT_URL, cloudUrl);
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
        curl_easy_setopt(curl, CURLOPT_MAXREDIRS, 5L);
        curl_easy_setopt(curl, CURLOPT_PROTOCOLS_STR, "https");
        curl_easy_setopt(curl, CURLOPT_REDIR_PROTOCOLS_STR, "https");
        curl_easy_setopt(curl, CURLOPT_FAILONERROR, 1L);
        curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 5L);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 15L);
        curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, receive);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &bytes);
        const CURLcode result = curl_easy_perform(curl);
        curl_easy_cleanup(curl);
        return result == CURLE_OK ? bytes : std::vector<unsigned char>{};
    }

    // Check JPEG dimensions before handing the data to the image decoder.
    static bool expectedJpeg(const std::vector<unsigned char>& bytes) {
        if (bytes.size() < 4 || bytes[0] != 0xff || bytes[1] != 0xd8 || bytes[bytes.size() - 2] != 0xff ||
            bytes.back() != 0xd9)
            return false;
        for (std::size_t pos = 2; pos + 4 <= bytes.size();) {
            if (bytes[pos++] != 0xff)
                return false;
            while (pos < bytes.size() && bytes[pos] == 0xff)
                ++pos;
            if (pos >= bytes.size())
                return false;
            const auto marker = bytes[pos++];
            if (marker == 0xda || marker == 0xd9)
                return false;
            if (marker == 0x01 || (marker >= 0xd0 && marker <= 0xd7))
                continue;
            if (pos + 2 > bytes.size())
                return false;
            const std::size_t length = (std::size_t(bytes[pos]) << 8) | bytes[pos + 1];
            if (length < 2 || length > bytes.size() - pos)
                return false;
            const bool frame = (marker >= 0xc0 && marker <= 0xc3) || (marker >= 0xc5 && marker <= 0xc7) ||
                               (marker >= 0xc9 && marker <= 0xcb) || (marker >= 0xcd && marker <= 0xcf);
            if (frame) {
                if (length < 8)
                    return false;
                const int height = (bytes[pos + 3] << 8) | bytes[pos + 4];
                const int width = (bytes[pos + 5] << 8) | bytes[pos + 6];
                return bytes[pos + 2] == 8 && width == 2048 && height == 1024;
            }
            pos += length;
        }
        return false;
    }

    EarthClouds::EarthClouds() : cloudHttpReady_(curl_global_init(CURL_GLOBAL_DEFAULT) == CURLE_OK) {}
    EarthClouds::~EarthClouds() {
        if (cloudDownload_.valid())
            cloudDownload_.wait();
        if (IsTextureValid(cloudTexture_))
            UnloadTexture(cloudTexture_);
        if (cloudHttpReady_)
            curl_global_cleanup();
    }

    void EarthClouds::enableClouds(bool enabled) {
        cloudsEnabled_ = enabled;
        if (enabled)
            refreshClouds();
    }

    void EarthClouds::refreshClouds() {
        if (!cloudsEnabled_ || cloudDownload_.valid())
            return;
        cloudRefreshFailed_ = !cloudHttpReady_;
        if (cloudHttpReady_) {
            try {
                cloudDownload_ = std::async(std::launch::async, download);
            } catch (...) {
                cloudRefreshFailed_ = true;
            }
        }
    }

    void EarthClouds::checkCloudDownload() {
        if (!cloudDownload_.valid() || cloudDownload_.wait_for(std::chrono::seconds(0)) != std::future_status::ready)
            return;

        std::vector<unsigned char> bytes;
        try {
            bytes = cloudDownload_.get();
        } catch (...) {
            cloudRefreshFailed_ = true;
            return;
        }

        if (!expectedJpeg(bytes)) {
            cloudRefreshFailed_ = true;
            return;
        }

        Image image = LoadImageFromMemory(".jpg", bytes.data(), static_cast<int>(bytes.size()));
        if (!IsImageValid(image) || image.width != 2048 || image.height != 1024) {
            if (IsImageValid(image))
                UnloadImage(image);
            cloudRefreshFailed_ = true;
            return;
        }
        Texture2D replacement = LoadTextureFromImage(image);
        UnloadImage(image);
        if (!IsTextureValid(replacement)) {
            cloudRefreshFailed_ = true;
            return;
        }
        GenTextureMipmaps(&replacement);
        SetTextureFilter(replacement, TEXTURE_FILTER_TRILINEAR);
        if (IsTextureValid(cloudTexture_))
            UnloadTexture(cloudTexture_);
        cloudTexture_ = replacement;
        cloudRefreshFailed_ = false;
    }
} // namespace solar
