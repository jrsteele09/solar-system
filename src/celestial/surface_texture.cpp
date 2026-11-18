#include "surface_texture.hpp"

#include <stdexcept>
#include <string>

namespace solar {
    SurfaceTexture::SurfaceTexture(const char* path) {
        Image image = LoadImage(path);
        if (!IsImageValid(image)) {
            throw std::runtime_error{std::string{"Could not load surface texture: "} + path};
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
