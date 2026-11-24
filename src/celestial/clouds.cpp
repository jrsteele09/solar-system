#include "clouds.hpp"
#include "http/download.hpp"

#include <chrono>
#include <cstdint>
#include <stdexcept>
#include <utility>

namespace solar {
    constexpr std::size_t maxBytes = 8 * 1024 * 1024;

    // Check JPEG dimensions before handing the data to the image decoder.
    static bool expectedJpeg(const std::vector<unsigned char>& bytes, const CloudSource& source) {
        if (bytes.size() < 4 || bytes[0] != 0xff || bytes[1] != 0xd8 || bytes[bytes.size() - 2] != 0xff ||
            bytes.back() != 0xd9)
            return false;
        for (std::size_t pos = 2; pos + 4 <= bytes.size();) {
            if (bytes[pos++] != 0xff) {
                return false;
            }
            while (pos < bytes.size() && bytes[pos] == 0xff)
                ++pos;
            if (pos >= bytes.size())
                return false;
            const auto marker = bytes[pos++];
            if (marker == 0xda || marker == 0xd9) {
                return false;
            }
            if (marker == 0x01 || (marker >= 0xd0 && marker <= 0xd7))
                continue;
            if (pos + 2 > bytes.size()) {
                return false;
            }
            const std::size_t length = (std::size_t(bytes[pos]) << 8) | bytes[pos + 1];
            if (length < 2 || length > bytes.size() - pos)
                return false;
            const bool frame = (marker >= 0xc0 && marker <= 0xc3) || (marker >= 0xc5 && marker <= 0xc7) ||
                               (marker >= 0xc9 && marker <= 0xcb) || (marker >= 0xcd && marker <= 0xcf);
            if (frame) {
                if (length < 8) {
                    return false;
                }
                const int height = (bytes[pos + 3] << 8) | bytes[pos + 4];
                const int width = (bytes[pos + 5] << 8) | bytes[pos + 6];
                return bytes[pos + 2] == 8 && width == source.width && height == source.height;
            }
            pos += length;
        }
        return false;
    }

    Clouds::Clouds(CloudSource source) : source_(std::move(source)) {
        if (source_.url.empty() || source_.width <= 0 || source_.height <= 0 ||
            std::int64_t{source_.width} * source_.height > 16 * 1024 * 1024) {
            throw std::invalid_argument{"Cloud source requires a URL and dimensions of at most 16 megapixels"};
        }
    }

    Clouds::~Clouds() {
        if (cloudDownload_.valid()) {
            cloudDownload_.wait();
        }
        if (IsTextureValid(cloudTexture_)) {
            UnloadTexture(cloudTexture_);
        }
    }

    void Clouds::enableClouds(bool enabled) {
        cloudsEnabled_ = enabled;
        if (enabled) {
            refreshClouds();
        }
    }

    void Clouds::refreshClouds() {
        if (!cloudsEnabled_ || cloudDownload_.valid()) {
            return;
        }
        cloudRefreshFailed_ = false;
        try {
            cloudDownload_ =
                std::async(std::launch::async, [url = source_.url] { return http::download(url, maxBytes); });
        } catch (const std::exception& error) {
            TraceLog(LOG_WARNING, "Cloud download could not start: %s", error.what());
            cloudRefreshFailed_ = true;
        }
    }

    void Clouds::checkCloudDownload() {
        if (!cloudDownload_.valid() || cloudDownload_.wait_for(std::chrono::seconds(0)) != std::future_status::ready)
            return;

        std::vector<unsigned char> bytes;
        try {
            bytes = cloudDownload_.get();
        } catch (const std::exception& error) {
            TraceLog(LOG_WARNING, "Cloud download failed: %s", error.what());
            cloudRefreshFailed_ = true;
            return;
        }

        if (!expectedJpeg(bytes, source_)) {
            TraceLog(LOG_WARNING, "Cloud image is not an 8-bit JPEG of size %dx%d", source_.width, source_.height);
            cloudRefreshFailed_ = true;
            return;
        }

        Image image = LoadImageFromMemory(".jpg", bytes.data(), static_cast<int>(bytes.size()));
        if (!IsImageValid(image) || image.width != source_.width || image.height != source_.height) {
            if (IsImageValid(image))
                UnloadImage(image);
            TraceLog(LOG_WARNING, "Cloud image could not be decoded at the expected dimensions");
            cloudRefreshFailed_ = true;
            return;
        }
        Texture2D replacement = LoadTextureFromImage(image);
        UnloadImage(image);
        if (!IsTextureValid(replacement)) {
            TraceLog(LOG_WARNING, "Cloud texture could not be uploaded");
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
