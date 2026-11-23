#include "celestial_body.hpp"

#include <glm/geometric.hpp>

#include <algorithm>
#include <cmath>
#include <limits>
#include <new>
#include <stdexcept>

namespace solar {
    namespace {
        float checkedSize(double size) {
            if (!std::isfinite(size) || size <= 0.0 || size > std::numeric_limits<float>::max()) {
                throw std::invalid_argument{"CelestialBody size must be positive and finite"};
            }
            const float result = static_cast<float>(size);
            if (result <= 0.0f) {
                throw std::invalid_argument{"CelestialBody size must be positive and finite"};
            }
            return result;
        }
    } // namespace

    CelestialBody::CelestialBody(const Mesh& mesh, double size, glm::dvec3 positionKm, const SurfaceTexture* texture,
                                 const Sunlight* sunlight, SurfaceStyle style, Color color)
        : sunlight_(sunlight), drawSolid_(texture != nullptr || style == SurfaceStyle::Solid), color_(color),
          size_(checkedSize(size)), positionKm_(positionKm) {
        constexpr auto maxAllocationBytes = std::numeric_limits<unsigned int>::max();
        constexpr auto maxVerticesForAllocation = maxAllocationBytes / (3 * sizeof(float));

        const auto count = mesh.vertices().size();
        // MemAlloc takes an unsigned int byte count.
        if (count == 0) {
            throw std::invalid_argument{"CelestialBody mesh is empty"};
        }
        if (count > maxVerticesForAllocation) {
            throw std::invalid_argument{"CelestialBody mesh is too large"};
        }

        // Use a bounding sphere to allow the camera to select this body.
        for (const auto& vertex : mesh.vertices()) {
            radiusSceneUnits_ = std::max(radiusSceneUnits_, glm::length(vertex) * size_);
        }

        // Convert the mesh to a raylib Mesh and upload it to the GPU.
        ::Mesh rayMesh{};
        rayMesh.vertexCount = static_cast<int>(count);
        rayMesh.triangleCount = rayMesh.vertexCount / 3;

        rayMesh.vertices = static_cast<float*>(MemAlloc(static_cast<unsigned int>(count * 3 * sizeof(float))));

        rayMesh.normals = static_cast<float*>(MemAlloc(static_cast<unsigned int>(count * 3 * sizeof(float))));

        if (!mesh.texcoords().empty()) {
            rayMesh.texcoords = static_cast<float*>(MemAlloc(static_cast<unsigned int>(count * 2 * sizeof(float))));
        }

        if (!rayMesh.vertices || !rayMesh.normals || (!mesh.texcoords().empty() && !rayMesh.texcoords)) {
            MemFree(rayMesh.vertices);
            MemFree(rayMesh.normals);
            MemFree(rayMesh.texcoords);
            throw std::bad_alloc{};
        }

        for (std::size_t i = 0; i < count; ++i) {
            const auto& vertex = mesh.vertices()[i];
            const auto& normal = mesh.normals()[i];
            rayMesh.vertices[i * 3] = vertex.x;
            rayMesh.vertices[i * 3 + 1] = vertex.y;
            rayMesh.vertices[i * 3 + 2] = vertex.z;
            rayMesh.normals[i * 3] = normal.x;
            rayMesh.normals[i * 3 + 1] = normal.y;
            rayMesh.normals[i * 3 + 2] = normal.z;
            if (rayMesh.texcoords) {
                rayMesh.texcoords[i * 2] = mesh.texcoords()[i].x;
                rayMesh.texcoords[i * 2 + 1] = mesh.texcoords()[i].y;
            }
        }

        UploadMesh(&rayMesh, false); // Copy mesh data to GPU
        model_ = LoadModelFromMesh(rayMesh);

        // Set the modesl texture and the sunlight shader if switched on
        if (texture) {
            SetMaterialTexture(&model_.materials[0], MATERIAL_MAP_ALBEDO, texture->texture());
            if (sunlight)
                model_.materials[0].shader = sunlight->shader();
        }
    }

    CelestialBody::~CelestialBody() { UnloadModel(model_); }

    float CelestialBody::visibleRadius(const Camera&, int) const { return radiusSceneUnits_; }
    void CelestialBody::drawEffects(const Camera&, float, int) const {}

    void CelestialBody::update(glm::dvec3 positionKm, const glm::mat4& orientation) {
        positionKm_ = positionKm;
        orientation_ = orientation;
    }

    void CelestialBody::configure(Body id, std::function<glm::mat4(double, double)> orientation,
                                  std::unique_ptr<SurfaceTexture> surface, std::unique_ptr<SurfaceTexture> night,
                                  std::unique_ptr<SurfaceTexture> rings) {
        id_ = id;
        orientationFunction_ = std::move(orientation);
        surface_ = std::move(surface);
        night_ = std::move(night);
        rings_ = std::move(rings);
        if (night_)
            setNightTexture(*night_);
    }

    void CelestialBody::update(const Ephemeris& ephemeris, double utc, double elapsedSeconds,
                               glm::dvec3 sunPositionKm) {
        const auto rotation = orientationFunction_ ? orientationFunction_(utc, elapsedSeconds) : glm::mat4{1.0f};
        update(sunPositionKm + ephemeris.position(id_, utc), rotation);
    }

    void CelestialBody::setNightTexture(const SurfaceTexture& texture) {
        SetMaterialTexture(&model_.materials[0], MATERIAL_MAP_EMISSION, texture.texture());
    }

    void CelestialBody::setCloudTexture(Texture2D texture) {
        SetMaterialTexture(&model_.materials[0], MATERIAL_MAP_OCCLUSION, texture);
    }

    void CelestialBody::prepareLighting() const {
        if (sunlight_) {
            sunlight_->setNightLights(false);
            sunlight_->setClouds(false);
        }
    }

    void CelestialBody::draw(Vector3 position) const {
        prepareLighting();
        Model model = model_;
        model.transform = {orientation_[0][0], orientation_[1][0], orientation_[2][0], orientation_[3][0],
                           orientation_[0][1], orientation_[1][1], orientation_[2][1], orientation_[3][1],
                           orientation_[0][2], orientation_[1][2], orientation_[2][2], orientation_[3][2],
                           orientation_[0][3], orientation_[1][3], orientation_[2][3], orientation_[3][3]};
        if (drawSolid_) {
            DrawModel(model, position, size_, color_);
        } else {
            DrawModelWires(model, position, size_, color_);
        }
    }
} // namespace solar
