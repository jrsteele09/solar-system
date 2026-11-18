#include "sunlight.hpp"

#include <rlgl.h>
#include <stdexcept>

namespace solar {
    const char* const sunPositionUniform = "sunPosition";
    const char* const nightLightsUniform = "nightLightsEnabled";
    const char* const nightMapUniform = "nightMap";
    const char* const shadowsUniform = "shadowsEnabled";

    Sunlight::Sunlight(const char* vertexPath, const char* fragmentPath) {
        shader_ = LoadShader(vertexPath, fragmentPath);
        // Raylib falls back to its default shader when compilation fails.
        if (!IsShaderValid(shader_) || shader_.id == rlGetShaderIdDefault()) {
            throw std::runtime_error{"Could not load the sunlight shader"};
        }

        sunPositionLocation_ = GetShaderLocation(shader_, sunPositionUniform);
        nightLightsLocation_ = GetShaderLocation(shader_, nightLightsUniform);
        shadowsLocation_ = GetShaderLocation(shader_, shadowsUniform);

        shader_.locs[SHADER_LOC_MAP_EMISSION] = GetShaderLocation(shader_, nightMapUniform);
        if (sunPositionLocation_ < 0 || nightLightsLocation_ < 0 || shadowsLocation_ < 0 ||
            shader_.locs[SHADER_LOC_MAP_EMISSION] < 0) {
            UnloadShader(shader_);
            throw std::runtime_error{"Sunlight shader is missing a required uniform"};
        }
    }

    Sunlight::~Sunlight() { UnloadShader(shader_); }

    void Sunlight::setSunPosition(Vector3 position) const {
        SetShaderValue(shader_, sunPositionLocation_, &position, SHADER_UNIFORM_VEC3);
    }

    void Sunlight::setNightLights(bool enabled) const {
        const int value = enabled ? 1 : 0;
        SetShaderValue(shader_, nightLightsLocation_, &value, SHADER_UNIFORM_INT);
    }

    void Sunlight::setShadows(bool enabled) const {
        const int value = enabled ? 1 : 0;
        SetShaderValue(shader_, shadowsLocation_, &value, SHADER_UNIFORM_INT);
    }
} // namespace solar
