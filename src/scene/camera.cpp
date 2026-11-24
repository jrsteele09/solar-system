#include "camera.hpp"
#include "celestial/celestial_body.hpp"
#include "scene/scene.hpp"

#include <algorithm>
#include <cmath>
#include <glm/geometric.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <limits>
#include <numbers>
#include <stdexcept>

namespace solar {
    // Construct a perspective ray without unprojecting the distant far plane.
    // The solar-system-sized depth range makes the usual float matrix inversion unstable.
    static glm::dvec3 mouseRayDirection(const Camera3D& camera, Vector2 mouse, int width, int height) {
        const glm::dvec3 forward = glm::normalize(glm::dvec3{camera.target.x, camera.target.y, camera.target.z});
        const glm::dvec3 right =
            glm::normalize(glm::cross(forward, glm::dvec3{camera.up.x, camera.up.y, camera.up.z}));
        const glm::dvec3 up = glm::cross(right, forward);
        const double halfHeight = std::tan(camera.fovy * std::numbers::pi_v<double> / 360.0);
        const double x = (2.0 * mouse.x / width - 1.0) * halfHeight * width / height;
        const double y = (1.0 - 2.0 * mouse.y / height) * halfHeight;
        return glm::normalize(forward + right * x + up * y);
    }

    static double raySphereDistance(glm::dvec3 origin, glm::dvec3 direction, glm::dvec3 centre, double radius) {
        const glm::dvec3 toCentre = centre - origin;
        const double projection = glm::dot(toCentre, direction);
        // The cross product avoids subtracting two enormous squared distances.
        const glm::dvec3 perpendicular = glm::cross(toCentre, direction);
        const double inside = radius * radius - glm::dot(perpendicular, perpendicular);
        if (inside < 0.0) {
            return std::numeric_limits<double>::infinity();
        }
        const double halfChord = std::sqrt(inside);
        const double nearHit = projection - halfChord;
        const double farHit = projection + halfChord;
        return nearHit >= 0.0 ? nearHit : farHit >= 0.0 ? farHit : std::numeric_limits<double>::infinity();
    }

    // Keep the camera's up direction perpendicular to its view, including near the poles.
    static void alignCameraUp(Camera3D& camera) {
        const glm::dvec3 view{camera.target.x, camera.target.y, camera.target.z};
        if (glm::length(view) < 1e-12) {
            return;
        }
        const glm::dvec3 forward = glm::normalize(view);
        glm::dvec3 up{camera.up.x, camera.up.y, camera.up.z};
        up -= forward * glm::dot(up, forward);
        if (glm::length(up) < 1e-6) {
            const glm::dvec3 fallback = std::abs(forward.y) < 0.9 ? glm::dvec3{0, 1, 0} : glm::dvec3{1, 0, 0};
            up = fallback - forward * glm::dot(fallback, forward);
        }
        up = glm::normalize(up);
        camera.up = {static_cast<float>(up.x), static_cast<float>(up.y), static_cast<float>(up.z)};
    }

    double limitCameraMovement(glm::dvec3 positionKm, glm::dvec3 direction, double distanceKm, glm::dvec3 centreKm,
                               double radiusKm) {
        const glm::dvec3 offset = positionKm - centreKm;
        const double projection = glm::dot(offset, direction);
        const double outside = glm::dot(offset, offset) - radiusKm * radiusKm;
        // If already inside the clearance area, allow movement away from the centre.
        if (outside <= 0.0) {
            return projection < 0.0 ? 0.0 : distanceKm;
        }
        if (projection >= 0.0) {
            return distanceKm;
        }
        const double discriminant = projection * projection - outside;
        if (discriminant < 0.0) {
            return distanceKm;
        }
        return std::min(distanceKm, std::max(0.0, -projection - std::sqrt(discriminant)));
    }

    Camera::Camera(double kmPerSceneUnit) : Camera(glm::dvec3{0.0, 0.0, 1.0}, glm::dvec3{0.0}, kmPerSceneUnit) {}

    Camera::Camera(const Scene& scene, double kmPerSceneUnit, double initialDistanceKm)
        : Camera(kmPerSceneUnit) {
        const auto& earth = scene.getCelestialBody(Body::Earth);
        const auto& sun = scene.getCelestialBody(Body::Sun);
        sunPositionKm_ = sun.positionKm();
        reset(earth.positionKm(), sunPositionKm_, initialDistanceKm);
        resetToBody(earth);
    }

    Camera::Camera(glm::dvec3 positionKm, glm::dvec3 targetKm, double kmPerSceneUnit)
        : positionKm_(positionKm), kmPerSceneUnit_(kmPerSceneUnit) {
        camera_.position = {0.0f, 0.0f, 0.0f};
        camera_.target = relativeScenePosition(targetKm);
        camera_.up = {0.0f, 1.0f, 0.0f};
        alignCameraUp(camera_);
        camera_.fovy = 45.0f;
        camera_.projection = CAMERA_PERSPECTIVE;
    }

    void Camera::reset(glm::dvec3 targetKm, glm::dvec3 viewFromKm, double distanceKm) {
        const glm::dvec3 direction = viewFromKm - targetKm;
        const double directionLength = glm::length(direction);
        if (!std::isfinite(directionLength) || directionLength == 0.0 || !std::isfinite(distanceKm) ||
            distanceKm <= 0.0) {
            throw std::invalid_argument{"Camera reset requires a direction and a positive distance"};
        }
        positionKm_ = targetKm + direction / directionLength * distanceKm;
        camera_.target = relativeScenePosition(targetKm);
        camera_.up = {0.0f, 1.0f, 0.0f};
        alignCameraUp(camera_);
        orbitTarget_ = nullptr;
        followedBody_ = nullptr;
        flying_ = false;
    }

    const CelestialBody* Camera::bodyUnderMouse(const Scene& scene) const {
        const int width = GetScreenWidth();
        const int height = GetScreenHeight();
        if (width <= 0 || height <= 0) {
            return nullptr;
        }
        const glm::dvec3 direction = mouseRayDirection(camera_, GetMousePosition(), width, height);
        const CelestialBody* closest = nullptr;
        double closestDistance = std::numeric_limits<double>::infinity();
        for (const auto& entry : scene.bodies()) {
            const auto* body = entry.get();
            if (!body) {
                continue;
            }
            const double distance = raySphereDistance(positionKm_, direction, body->positionKm(),
                                                      body->radiusSceneUnits() * kmPerSceneUnit_);
            if (distance < closestDistance) {
                closest = body;
                closestDistance = distance;
            }
        }
        return closest;
    }

    void Camera::updateOrbitControls(const Scene& scene, bool acceptMouse) {
        const auto bodies = scene.bodies();
        const auto containsBody = [bodies](const CelestialBody* body) {
            return std::any_of(bodies.begin(), bodies.end(), [body](const auto& entry) { return entry.get() == body; });
        };

        if (followedBody_ && !containsBody(followedBody_)) {
            followedBody_ = nullptr;
            flying_ = false;
        }

        if (followedBody_) {
            const glm::dvec3 target = followedBody_->positionKm();
            if (flying_) {
                updateFlight(target, GetFrameTime());
            } else {
                positionKm_ += target - lastFollowedPosition_;
                camera_.target = relativeScenePosition(target);
            }
            lastFollowedPosition_ = target;
        }

        if (orbitTarget_ && !containsBody(orbitTarget_)) {
            orbitTarget_ = nullptr;
        }
        if (!acceptMouse && !orbitTarget_) {
            if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
                orbitTarget_ = nullptr;
            }
            return;
        }

        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            orbitTarget_ = bodyUnderMouse(scene);
            pressPosition_ = GetMousePosition();
            dragging_ = false;
        }

        if (orbitTarget_ && IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
            const Vector2 mouse = GetMousePosition();
            if (std::hypot(mouse.x - pressPosition_.x, mouse.y - pressPosition_.y) > 4.0f) {
                dragging_ = true;
            }
            if (dragging_) {
                flying_ = false;
                followedBody_ = orbitTarget_;
                lastFollowedPosition_ = orbitTarget_->positionKm();
                orbit(lastFollowedPosition_, GetMouseDelta());
            }
        }

        if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
            if (acceptMouse && orbitTarget_ && !dragging_ && bodyUnderMouse(scene) == orbitTarget_) {
                flyToBody(*orbitTarget_);
            }
            orbitTarget_ = nullptr;
        }

        if (acceptMouse) {
            zoom(GetMouseWheelMove(), scene);
        }
    }

    void Camera::flyToBody(const CelestialBody& body) {
        followedBody_ = &body;
        lastFollowedPosition_ = body.positionKm();
        flyTo(lastFollowedPosition_, body.radiusSceneUnits());
    }

    void Camera::resetToBody(const CelestialBody& body) {
        const glm::dvec3 target = body.positionKm();
        const double distance = glm::length(positionKm_ - target);
        if (glm::length(sunPositionKm_ - target) == 0.0 || distance == 0.0) {
            return;
        }
        reset(target, sunPositionKm_, distance);
        followedBody_ = &body;
        lastFollowedPosition_ = target;
    }

    void Camera::zoom(float wheelMovement, const Scene& scene) {
        if (wheelMovement == 0.0f) {
            return;
        }
        const glm::dvec3 view{camera_.target.x, camera_.target.y, camera_.target.z};
        const double viewDistance = glm::length(view);
        if (viewDistance == 0.0) {
            return;
        }

        // Scale each wheel step with distance, allowing both close inspection and long journeys.
        const double steps = std::clamp(static_cast<double>(wheelMovement), -10.0, 10.0);
        double distance = viewDistance * kmPerSceneUnit_ * std::abs(1.0 - std::pow(0.8, steps));
        const glm::dvec3 direction = view / viewDistance * (steps > 0.0 ? 1.0 : -1.0);

        for (const auto& entry : scene.bodies()) {
            const auto* body = entry.get();
            if (!body) {
                continue;
            }
            const double radius = body->radiusSceneUnits();
            const double clearance = std::max(0.1, radius * 0.01);
            distance = limitCameraMovement(positionKm_, direction, distance, body->positionKm(),
                                           (radius + clearance) * kmPerSceneUnit_);
        }

        positionKm_ += direction * distance;
        if (followedBody_) {
            camera_.target = relativeScenePosition(followedBody_->positionKm());
        }
        flying_ = false;
    }

    void Camera::flyTo(glm::dvec3 targetKm, float radiusSceneUnits) {
        const double distance = glm::length(positionKm_ - targetKm);
        if (distance <= 0.0 || radiusSceneUnits <= 0.0f) {
            return;
        }
        const glm::dvec3 view{camera_.target.x, camera_.target.y, camera_.target.z};
        const double viewLength = glm::length(view);
        flightStartDirection_ = viewLength > 0.0 ? view / viewLength : (targetKm - positionKm_) / distance;
        flightArrivalDistanceKm_ = std::min(distance, 3.5 * radiusSceneUnits * kmPerSceneUnit_);
        flightElapsed_ = 0.0;
        flightDeparted_ = false;
        flying_ = true;
    }

    bool Camera::updateFlight(glm::dvec3 targetKm, float elapsedSeconds) {
        if (!flying_) {
            return false;
        }
        constexpr double turnDuration = 0.7;
        constexpr double travelDuration = 2.0;
        constexpr double duration = turnDuration + travelDuration;
        flightElapsed_ = std::min(duration, flightElapsed_ + std::max(0.0f, elapsedSeconds));
        if (flightElapsed_ < turnDuration) {
            const glm::dvec3 toTarget = targetKm - positionKm_;
            const double distance = glm::length(toTarget);
            if (distance == 0.0) {
                flying_ = false;
                return false;
            }
            const glm::dvec3 destination = toTarget / distance;
            const double angle = std::acos(std::clamp(glm::dot(flightStartDirection_, destination), -1.0, 1.0));
            glm::dvec3 axis = glm::cross(flightStartDirection_, destination);
            if (glm::length(axis) < 1e-8) {
                axis = glm::dvec3{0.0, 1.0, 0.0} - flightStartDirection_ * flightStartDirection_.y;
                if (glm::length(axis) < 1e-8) {
                    axis = glm::dvec3{1.0, 0.0, 0.0} - flightStartDirection_ * flightStartDirection_.x;
                }
            }
            axis = glm::normalize(axis);
            const double t = flightElapsed_ / turnDuration;
            const double rotation = angle * t * t * (3.0 - 2.0 * t);
            const glm::dvec3 direction = flightStartDirection_ * std::cos(rotation) +
                                         glm::cross(axis, flightStartDirection_) * std::sin(rotation) +
                                         axis * glm::dot(axis, flightStartDirection_) * (1.0 - std::cos(rotation));
            camera_.target = {static_cast<float>(direction.x), static_cast<float>(direction.y),
                              static_cast<float>(direction.z)};
            alignCameraUp(camera_);
            return true;
        }
        if (!flightDeparted_) {
            flightStartOffset_ = positionKm_ - targetKm;
            const double distance = glm::length(flightStartOffset_);
            if (distance == 0.0) {
                flying_ = false;
                return false;
            }
            flightEndOffset_ = flightStartOffset_ / distance * std::min(distance, flightArrivalDistanceKm_);
            flightDeparted_ = true;
        }
        const double t = (flightElapsed_ - turnDuration) / travelDuration;
        const double smooth = t * t * (3.0 - 2.0 * t);
        positionKm_ = targetKm + flightStartOffset_ * (1.0 - smooth) + flightEndOffset_ * smooth;
        camera_.target = relativeScenePosition(targetKm);
        alignCameraUp(camera_);
        flying_ = flightElapsed_ < duration;
        return flying_;
    }

    void Camera::orbit(glm::dvec3 targetKm, Vector2 mouseMovement) {
        if (mouseMovement.x == 0.0f && mouseMovement.y == 0.0f) {
            return;
        }

        constexpr double radiansPerPixel = 0.007;
        const glm::dvec3 offset = positionKm_ - targetKm;
        const double distance = glm::length(offset);
        if (distance == 0.0) {
            return;
        }

        alignCameraUp(camera_);
        glm::dvec3 up{camera_.up.x, camera_.up.y, camera_.up.z};
        const glm::dmat4 identity{1.0};
        const auto yaw = glm::rotate(identity, -mouseMovement.x * radiansPerPixel, up);
        glm::dvec3 rotatedOffset = glm::dvec3(yaw * glm::dvec4(offset, 0.0));
        const glm::dvec3 right = glm::normalize(glm::cross(-rotatedOffset, up));
        const auto pitch = glm::rotate(identity, -mouseMovement.y * radiansPerPixel, right);
        rotatedOffset = glm::dvec3(pitch * glm::dvec4(rotatedOffset, 0.0));
        up = glm::normalize(glm::dvec3(pitch * glm::dvec4(up, 0.0)));
        positionKm_ = targetKm + rotatedOffset;
        camera_.target = relativeScenePosition(targetKm);
        camera_.up = {static_cast<float>(up.x), static_cast<float>(up.y), static_cast<float>(up.z)};
    }

    bool Camera::isVisible(glm::dvec3 positionKm, float radiusSceneUnits, float aspectRatio) const {
        const Vector3 position = relativeScenePosition(positionKm);
        const glm::dvec3 centre{position.x, position.y, position.z};
        const glm::dvec3 forward = glm::normalize(glm::dvec3{camera_.target.x, camera_.target.y, camera_.target.z});
        const glm::dvec3 right =
            glm::normalize(glm::cross(forward, glm::dvec3{camera_.up.x, camera_.up.y, camera_.up.z}));
        const glm::dvec3 up = glm::cross(right, forward);
        const double depth = glm::dot(centre, forward);
        const double vertical = std::tan(camera_.fovy * std::numbers::pi_v<double> / 360.0);
        const double horizontal = vertical * aspectRatio;
        const double radius = radiusSceneUnits;

        // Expand each view boundary by the body's radius so edges remain visible.
        return depth >= -radius &&
               std::abs(glm::dot(centre, right)) <= depth * horizontal + radius * std::hypot(1.0, horizontal) &&
               std::abs(glm::dot(centre, up)) <= depth * vertical + radius * std::hypot(1.0, vertical);
    }

    Vector3 Camera::relativeScenePosition(glm::dvec3 positionKm) const {
        const glm::dvec3 relative = (positionKm - positionKm_) / kmPerSceneUnit_;
        return {static_cast<float>(relative.x), static_cast<float>(relative.y), static_cast<float>(relative.z)};
    }
} // namespace solar
