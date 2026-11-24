#include "surface_shader.hpp"

#include <rlgl.h>
#include <stdexcept>

namespace solar {
    const char* const sunPositionUniform = "sunPosition";
    const char* const nightLightsUniform = "nightLightsEnabled";
    const char* const nightMapUniform = "nightMap";
    const char* const shadowsUniform = "shadowsEnabled";

    SurfaceShader::SurfaceShader(const char* vertexPath, const char* fragmentPath) {
        shader_ = LoadShader(vertexPath, fragmentPath);
        // Raylib falls back to its default shader when compilation fails.
        if (!IsShaderValid(shader_) || shader_.id == rlGetShaderIdDefault()) {
            throw std::runtime_error{"Could not load the surface shader"};
        }

        sunPositionLocation_ = GetShaderLocation(shader_, sunPositionUniform);
        nightLightsLocation_ = GetShaderLocation(shader_, nightLightsUniform);
        shadowsLocation_ = GetShaderLocation(shader_, shadowsUniform);
        cloudsLocation_ = GetShaderLocation(shader_, "cloudsEnabled");
        cloudOpacityLocation_ = GetShaderLocation(shader_, "cloudOpacityScale");

        shader_.locs[SHADER_LOC_MAP_EMISSION] = GetShaderLocation(shader_, nightMapUniform);
        shader_.locs[SHADER_LOC_MAP_OCCLUSION] = GetShaderLocation(shader_, "cloudMap");
        if (sunPositionLocation_ < 0 || nightLightsLocation_ < 0 || shadowsLocation_ < 0 || cloudsLocation_ < 0 ||
            cloudOpacityLocation_ < 0 || shader_.locs[SHADER_LOC_MAP_OCCLUSION] < 0 ||
            shader_.locs[SHADER_LOC_MAP_EMISSION] < 0) {
            UnloadShader(shader_);
            throw std::runtime_error{"Surface shader is missing a required uniform"};
        }
    }

    SurfaceShader::~SurfaceShader() { UnloadShader(shader_); }

    void SurfaceShader::setSunPosition(Vector3 position) const {
        SetShaderValue(shader_, sunPositionLocation_, &position, SHADER_UNIFORM_VEC3);
    }

    void SurfaceShader::setNightLights(bool enabled) const {
        const int value = enabled ? 1 : 0;
        SetShaderValue(shader_, nightLightsLocation_, &value, SHADER_UNIFORM_INT);
    }

    void SurfaceShader::setClouds(bool enabled) const {
        const int value = enabled ? 1 : 0;
        SetShaderValue(shader_, cloudsLocation_, &value, SHADER_UNIFORM_INT);
    }

    void SurfaceShader::setCloudOpacity(float opacity) const {
        SetShaderValue(shader_, cloudOpacityLocation_, &opacity, SHADER_UNIFORM_FLOAT);
    }

    void SurfaceShader::setShadows(bool enabled) const {
        const int value = enabled ? 1 : 0;
        SetShaderValue(shader_, shadowsLocation_, &value, SHADER_UNIFORM_INT);
    }
} // namespace solar
