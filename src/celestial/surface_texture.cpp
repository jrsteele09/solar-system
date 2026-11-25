#include "surface_texture.hpp"

#include <algorithm>
#include <stdexcept>
#include <string>

namespace solar {
    SurfaceTexture::SurfaceTexture(const char* path, int maxWidth, bool preserveAlpha) {
        Image image = LoadImage(path);
        if (!IsImageValid(image)) {
            throw std::runtime_error{std::string{"Could not load surface texture: "} + path};
        }

        // Some colour maps (Io in particular) store a specular mask in alpha.
        if (!preserveAlpha && image.format == PIXELFORMAT_UNCOMPRESSED_R8G8B8A8)
            ImageFormat(&image, PIXELFORMAT_UNCOMPRESSED_R8G8B8);
        if (maxWidth > 0 && image.width > maxWidth) {
            ImageResize(&image, maxWidth, std::max(1, image.height * maxWidth / image.width));
        }
        texture_ = LoadTextureFromImage(image);
        UnloadImage(image);
        if (!IsTextureValid(texture_)) {
            throw std::runtime_error{std::string{"Could not upload surface texture: "} + path};
        }
        GenTextureMipmaps(&texture_);
        SetTextureFilter(texture_, TEXTURE_FILTER_TRILINEAR);
    }

    SurfaceTexture::~SurfaceTexture() { UnloadTexture(texture_); }
} // namespace solar
