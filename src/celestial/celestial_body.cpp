#include "celestial_body.hpp"
#include "scene/camera.hpp"

#include <glm/geometric.hpp>

#include <algorithm>
#include <cmath>
#include <limits>
#include <new>
#include <rlgl.h>
#include <stdexcept>

namespace solar {
    static double checkedSize(double size) {
        if (!std::isfinite(size) || size <= 0.0 || size > std::numeric_limits<double>::max()) {
            throw std::invalid_argument{"CelestialBody size must be positive and finite"};
        }
        return size;
    }

    // Convert a solar::Mesh to a raylib Mesh and upload it to the GPU.
    static ::Mesh toRaylibMesh(const Mesh& mesh) {
        const auto count = mesh.vertices().size();
        constexpr auto maxAllocationBytes = std::numeric_limits<unsigned int>::max();
        constexpr auto maxVerticesForAllocation = maxAllocationBytes / (3 * sizeof(float));

        if (count == 0) {
            throw std::invalid_argument{"Mesh is empty"};
        }
        if (count > maxVerticesForAllocation) {
            throw std::invalid_argument{"Mesh is too large"};
        }

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
        return rayMesh;
    }

    CelestialBody::CelestialBody(const Builder& builder)
        : id_(builder.id_), orientationFunction_(builder.orientation_), positionKm_(builder.position_),
          size_(checkedSize(builder.radius_)), surfaceShader_(builder.surfaceShader_),
          drawSolid_(!builder.surface_.empty() || builder.style_ == SurfaceStyle::Solid), color_(builder.color_),
          effects_(builder.effects_) {

        const auto& mesh = builder.mesh_;
        const auto count = mesh.vertices().size();

        if (count == 0) {
            throw std::invalid_argument{"CelestialBody mesh is empty"};
        }

        // Use a bounding sphere to allow the camera to select this body.
        for (const auto& vertex : mesh.vertices()) {
            radiusSceneUnits_ = std::max(radiusSceneUnits_, glm::length(vertex) * size_);
        }

        // Convert the mesh to a raylib Mesh and upload it to the GPU.
        model_ = LoadModelFromMesh(toRaylibMesh(mesh));
    }

    CelestialBody::~CelestialBody() {
        if (rings_.meshCount != 0) {
            UnloadModel(rings_);
        }
        UnloadModel(model_);
    }

    float CelestialBody::visibleRadius(const Camera& camera, int screenHeight) const {
        double radius = radiusSceneUnits_ * ringOuterRadius_;
        for (const auto* effect : effects_) {
            radius = std::max(radius, static_cast<double>(effect->radius(camera.relativeScenePosition(positionKm_),
                                                      static_cast<float>(radiusSceneUnits_), camera.raylibCamera().fovy, screenHeight)));
        }
        return static_cast<float>(radius);
    }

    void CelestialBody::drawEffects(const Camera& camera, float elapsedSeconds, int screenHeight) const {
        for (const auto* effect : effects_) {
            effect->draw(camera.raylibCamera(), camera.relativeScenePosition(positionKm_), 
                         static_cast<float>(radiusSceneUnits_), elapsedSeconds, screenHeight);
        }
        if (rings_.meshCount == 0) {
            return;
        }
        const auto& o = orientation_;
        Model model = rings_;
        model.transform = {o[0][0], o[1][0], o[2][0], o[3][0], o[0][1], o[1][1], o[2][1], o[3][1],
                           o[0][2], o[1][2], o[2][2], o[3][2], o[0][3], o[1][3], o[2][3], o[3][3]};
        rlDrawRenderBatchActive();
        rlDisableBackfaceCulling();
        rlDisableDepthMask();
        DrawModel(model, camera.relativeScenePosition(positionKm_), static_cast<float>(radiusSceneUnits_), WHITE);
        rlDrawRenderBatchActive();
        rlEnableDepthMask();
        rlEnableBackfaceCulling();
    }

    Clouds& CelestialBody::clouds() {
        if (!clouds_) {
            throw std::logic_error{"Body has no cloud source"};
        }
        return *clouds_;
    }

    const Clouds& CelestialBody::clouds() const {
        if (!clouds_) {
            throw std::logic_error{"Body has no cloud source"};
        }
        return *clouds_;
    }

    void CelestialBody::update(glm::dvec3 positionKm, const glm::mat4& orientation) {
        positionKm_ = positionKm;
        orientation_ = orientation;
    }

    void CelestialBody::update(const Ephemeris& ephemeris, double utc, double elapsedSeconds) {
        const auto rotation = orientationFunction_ ? orientationFunction_(utc, elapsedSeconds) : glm::mat4{1.0f};
        update(ephemeris.position(id_, utc), rotation);
        if (clouds_) {
            clouds_->checkCloudDownload();
            if (clouds_->cloudsVisible()) {
                setMaterialTexture(MATERIAL_MAP_OCCLUSION, clouds_->cloudTexture());
            }
        }
    }

    void CelestialBody::setMaterialTexture(int map, Texture2D texture) {
        SetMaterialTexture(&model_.materials[0], map, texture);
    }

    void CelestialBody::prepareLighting() const {
        if (surfaceShader_) {
            surfaceShader_->setNightLights(lightsEnabled());
            surfaceShader_->setClouds(clouds_ && clouds_->cloudsVisible());
            if (clouds_) {
                surfaceShader_->setCloudOpacity(clouds_->cloudOpacity());
            }
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
            DrawModel(model, position, static_cast<float>(size_), color_);
        } else {
            DrawModelWires(model, position, static_cast<float>(size_), color_);
        }
    }

    std::unique_ptr<CelestialBody> CelestialBody::Builder::build() const {
        if (rings_ && (!std::isfinite(rings_->innerRadius) || !std::isfinite(rings_->outerRadius) ||
                       rings_->innerRadius <= 0 || rings_->outerRadius <= rings_->innerRadius)) {
            throw std::invalid_argument{"Ring radii must be positive, finite and increasing"};
        }

        if ((!nightTexture_.empty() || clouds_) && (surface_.empty() || !surfaceShader_)) {
            throw std::invalid_argument{"Night lights and clouds require a surface and surface shader"};
        }

        auto body = std::unique_ptr<CelestialBody>{new CelestialBody{*this}};

        if (!surface_.empty()) {
            body->surface_ = std::make_unique<SurfaceTexture>(surface_.c_str());
            body->setMaterialTexture(MATERIAL_MAP_ALBEDO, body->surface_->texture());
            if (surfaceShader_) {
                body->model_.materials[0].shader = surfaceShader_->shader();
            }
        }

        if (!nightTexture_.empty()) {
            body->nightTexture_ = std::make_unique<SurfaceTexture>(nightTexture_.c_str());
            body->setMaterialTexture(MATERIAL_MAP_EMISSION, body->nightTexture_->texture());
        }

        if (clouds_) {
            body->clouds_ = std::make_unique<Clouds>(*clouds_);
        }

        if (rings_) {
            body->ringTexture_ = std::make_unique<SurfaceTexture>(rings_->path.c_str());
            const Mesh ringMesh = Mesh::createRings(rings_->innerRadius, rings_->outerRadius);
            body->rings_ = LoadModelFromMesh(toRaylibMesh(ringMesh));
            SetMaterialTexture(&body->rings_.materials[0], MATERIAL_MAP_ALBEDO, body->ringTexture_->texture());
            // Rings retain the unlit default shader.
            body->ringOuterRadius_ = std::max(1.0, rings_->outerRadius);
        }
        return body;
    }
} // namespace solar
