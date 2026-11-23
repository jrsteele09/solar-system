#include "saturn.hpp"
#include "scene/camera.hpp"

#include <cmath>
#include <numbers>
#include <rlgl.h>
#include <stdexcept>

namespace solar {
    namespace {
        constexpr float innerRadius = 74658.0f / 58232.0f;
        constexpr float outerRadius = 136780.0f / 58232.0f;
        constexpr int segments = 256;

        ::Mesh createRingMesh() {
            ::Mesh mesh{};
            mesh.vertexCount = segments * 6;
            mesh.triangleCount = mesh.vertexCount / 3;
            mesh.vertices = static_cast<float*>(MemAlloc(mesh.vertexCount * 3 * sizeof(float)));
            mesh.normals = static_cast<float*>(MemAlloc(mesh.vertexCount * 3 * sizeof(float)));
            mesh.texcoords = static_cast<float*>(MemAlloc(mesh.vertexCount * 2 * sizeof(float)));
            if (!mesh.vertices || !mesh.normals || !mesh.texcoords) {
                MemFree(mesh.vertices); MemFree(mesh.normals); MemFree(mesh.texcoords);
                throw std::bad_alloc{};
            }
            int vertex = 0;
            auto add = [&](float angle, float radius, float u) {
                mesh.vertices[vertex * 3] = std::cos(angle) * radius;
                mesh.vertices[vertex * 3 + 1] = 0;
                mesh.vertices[vertex * 3 + 2] = std::sin(angle) * radius;
                mesh.normals[vertex * 3] = 0;
                mesh.normals[vertex * 3 + 1] = 1;
                mesh.normals[vertex * 3 + 2] = 0;
                mesh.texcoords[vertex * 2] = u;
                mesh.texcoords[vertex * 2 + 1] = 0.5f;
                ++vertex;
            };
            for (int i = 0; i < segments; ++i) {
                const float a = 2.0f * std::numbers::pi_v<float> * i / segments;
                const float b = 2.0f * std::numbers::pi_v<float> * (i + 1) / segments;
                add(a, innerRadius, 0); add(b, outerRadius, 1); add(a, outerRadius, 1);
                add(a, innerRadius, 0); add(b, innerRadius, 0); add(b, outerRadius, 1);
            }
            UploadMesh(&mesh, false);
            return mesh;
        }
    } // namespace

    Saturn::Saturn(const Mesh& sphere, double radius, glm::dvec3 position, const SurfaceTexture& texture,
                   const SurfaceTexture& ringTexture, const Sunlight& sunlight)
        : CelestialBody(sphere, radius, position, &texture, &sunlight) {
        rings_ = LoadModelFromMesh(createRingMesh());
        SetMaterialTexture(&rings_.materials[0], MATERIAL_MAP_ALBEDO, ringTexture.texture());
        // Match the Odin viewer: the ring profile is drawn with the unlit default shader.
    }

    Saturn::~Saturn() { UnloadModel(rings_); }

    float Saturn::visibleRadius(const Camera&, int) const { return radiusSceneUnits() * outerRadius; }

    void Saturn::drawEffects(const Camera& camera, float, int) const {
        const auto& o = orientation();
        Model model = rings_;
        model.transform = {o[0][0], o[1][0], o[2][0], o[3][0], o[0][1], o[1][1], o[2][1], o[3][1],
                           o[0][2], o[1][2], o[2][2], o[3][2], o[0][3], o[1][3], o[2][3], o[3][3]};
        rlDrawRenderBatchActive();
        rlDisableBackfaceCulling();
        rlDisableDepthMask();
        DrawModel(model, camera.relativeScenePosition(positionKm()), radiusSceneUnits(), WHITE);
        rlDrawRenderBatchActive();
        rlEnableDepthMask();
        rlEnableBackfaceCulling();
    }
} // namespace solar
