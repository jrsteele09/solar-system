#include "controls_panel.hpp"

#include <raylib.h>
#include <algorithm>
#include <cmath>
#include <limits>

namespace solar {
    namespace {
        constexpr float panelWidth = 196.0f;
        constexpr float margin = 18.0f;
        constexpr float rowHeight = 32.0f;

        Rectangle panelBounds(bool earthNearest, bool showOpacity) {
            return {GetScreenWidth() - panelWidth - margin, margin, panelWidth,
                    earthNearest ? (showOpacity ? 322.0f : 278.0f) : 190.0f};
        }
        Rectangle comboBounds(Rectangle panel) { return {panel.x + 12, panel.y + 42, panel.width - 24, 34}; }
        Rectangle resetBounds(Rectangle panel) { return {panel.x + 12, panel.y + 98, panel.width - 24, 32}; }
        Rectangle lightsBounds(Rectangle panel) { return {panel.x + 12, panel.y + 142, panel.width - 24, 32}; }
        Rectangle cloudsBounds(Rectangle panel) { return {panel.x + 12, panel.y + 186, panel.width - 24, 32}; }
        Rectangle opacityBounds(Rectangle panel) { return {panel.x + 12, panel.y + 222, panel.width - 24, 44}; }
        Rectangle opacityTrack(Rectangle panel) { return {panel.x + 20, panel.y + 251, panel.width - 40, 4}; }
        Rectangle shadowBounds(Rectangle panel, bool earthNearest, bool showOpacity) {
            return {panel.x + 12, panel.y + (earthNearest ? (showOpacity ? 274 : 230) : 142), panel.width - 24, 32};
        }
        std::size_t visibleRows(Rectangle combo, std::size_t count) {
            const float available = GetScreenHeight() - margin - (combo.y + combo.height + 4) - 8;
            return std::min(count, static_cast<std::size_t>(std::max(1.0f, std::floor(available / rowHeight))));
        }
        Rectangle listBounds(Rectangle combo, std::size_t count) {
            return {combo.x, combo.y + combo.height + 4, combo.width, static_cast<float>(visibleRows(combo, count)) * rowHeight + 8};
        }
        Rectangle bodyBounds(Rectangle list, std::size_t index) {
            return {list.x + 4, list.y + 4 + static_cast<float>(index) * rowHeight, list.width - 8, rowHeight};
        }
        const BodyUpdate* selectedBody(std::span<const BodyUpdate> bodies, const Camera& camera) {
            for (const auto& entry : bodies)
                if (entry.body && entry.body == camera.followedBody())
                    return &entry;
            return nullptr;
        }
        bool earthIsNearest(std::span<const BodyUpdate> bodies, const Camera& camera) {
            const BodyUpdate* nearest = nullptr;
            double nearestDistance = std::numeric_limits<double>::infinity();
            for (const auto& entry : bodies) {
                if (!entry.body)
                    continue;
                const Vector3 position = camera.relativeScenePosition(entry.body->positionKm());
                const double distance = std::hypot(position.x, position.y, position.z) - entry.body->radiusSceneUnits();
                if (distance < nearestDistance) {
                    nearestDistance = distance;
                    nearest = &entry;
                }
            }
            return nearest && nearest->ephemerisBody == Body::Earth;
        }
        void label(Font font, const char* text, float x, float y, float size, Color color) {
            DrawTextEx(font, text, {x, y}, size, 0.2f, color);
        }
    } // namespace

    ControlsPanel::ControlsPanel() : font_(LoadFontEx(SOLAR_UI_FONT_PATH, 36, nullptr, 0)) {
        SetTextureFilter(font_.texture, TEXTURE_FILTER_BILINEAR);
    }

    ControlsPanel::~ControlsPanel() { UnloadFont(font_); }

    bool ControlsPanel::containsMouse(const Scene& scene, const Camera& camera) const {
        // While the list is open, an outside click dismisses it without affecting the scene.
        return open_ || draggingCloudOpacity_ ||
               CheckCollisionPointRec(GetMousePosition(), panelBounds(earthIsNearest(scene.bodies(), camera), scene.cloudsEnabled()));
    }

    void ControlsPanel::update(Scene& scene, Camera& camera, glm::dvec3 sunPositionKm) {
        const auto bodies = scene.bodies();
        const bool earthNearest = earthIsNearest(bodies, camera);
        const Rectangle panel = panelBounds(earthNearest, scene.cloudsEnabled());
        const Vector2 mouse = GetMousePosition();
        const Rectangle combo = comboBounds(panel);
        const std::size_t rows = visibleRows(combo, bodies.size());
        const std::size_t maxFirst = bodies.size() - rows;
        listFirst_ = std::min(listFirst_, maxFirst);
        if (open_) {
            const float wheel = GetMouseWheelMove();
            if (wheel != 0 || IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_UP)) {
                const int direction = wheel < 0 || IsKeyPressed(KEY_DOWN) ? 1 : -1;
                listFirst_ = static_cast<std::size_t>(std::clamp(static_cast<int>(listFirst_) + direction,
                                                                 0, static_cast<int>(maxFirst)));
            }
        }
        if (draggingCloudOpacity_) {
            if (!earthNearest || !scene.cloudsEnabled() || !IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
                draggingCloudOpacity_ = false;
            } else {
                const Rectangle track = opacityTrack(panel);
                scene.setCloudOpacity((mouse.x - track.x) / track.width);
            }
            return;
        }
        if (!IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
            return;
        if (CheckCollisionPointRec(mouse, combo)) {
            open_ = !open_;
            return;
        }
        if (open_) {
            const Rectangle list = listBounds(combo, bodies.size());
            for (std::size_t row = 0; row < rows; ++row) {
                const std::size_t i = listFirst_ + row;
                if (bodies[i].body && CheckCollisionPointRec(mouse, bodyBounds(list, row))) {
                    camera.flyToBody(*bodies[i].body);
                    break;
                }
            }
            open_ = false;
            return;
        }
        if (earthNearest && scene.cloudsEnabled() && CheckCollisionPointRec(mouse, opacityBounds(panel))) {
            draggingCloudOpacity_ = true;
            const Rectangle track = opacityTrack(panel);
            scene.setCloudOpacity((mouse.x - track.x) / track.width);
            return;
        }
        const BodyUpdate* selected = selectedBody(bodies, camera);
        if (selected && selected->ephemerisBody != Body::Sun && CheckCollisionPointRec(mouse, resetBounds(panel)))
            camera.resetToBody(*selected->body, sunPositionKm);
        if (earthNearest && CheckCollisionPointRec(mouse, lightsBounds(panel)))
            scene.setEarthLights(!scene.earthLights());
        if (earthNearest && CheckCollisionPointRec(mouse, cloudsBounds(panel)))
            scene.setCloudsEnabled(!scene.cloudsEnabled());
        if (CheckCollisionPointRec(mouse, shadowBounds(panel, earthNearest, scene.cloudsEnabled())))
            scene.setShadowsEnabled(!scene.shadowsEnabled());
    }

    void ControlsPanel::draw(const Scene& scene, const Camera& camera) const {
        const auto bodies = scene.bodies();
        const bool earthNearest = earthIsNearest(bodies, camera);
        const Rectangle panel = panelBounds(earthNearest, scene.cloudsEnabled());
        const Rectangle combo = comboBounds(panel);
        const Vector2 mouse = GetMousePosition();
        const BodyUpdate* selected = selectedBody(bodies, camera);
        DrawRectangleRounded(panel, 0.08f, 8, Color{12, 20, 32, 225});
        label(font_, "Fly to", panel.x + 14, panel.y + 14, 16, Color{167, 184, 200, 255});
        const bool comboHovered = CheckCollisionPointRec(mouse, combo);
        DrawRectangleRounded(combo, 0.2f, 6, comboHovered || open_ ? Color{54, 72, 90, 255} : Color{35, 48, 64, 255});
        label(font_, selected ? bodyName(selected->ephemerisBody) : "Choose a body", combo.x + 10, combo.y + 7, 17,
              Color{232, 238, 244, 255});
        const float arrowX = combo.x + combo.width - 18;
        const float arrowY = combo.y + combo.height / 2;
        DrawLineEx({arrowX - 4, arrowY - 2}, {arrowX, arrowY + 2}, 1.5f, Color{184, 199, 212, 255});
        DrawLineEx({arrowX, arrowY + 2}, {arrowX + 4, arrowY - 2}, 1.5f, Color{184, 199, 212, 255});

        const Rectangle reset = resetBounds(panel);
        const bool canReset = selected && selected->ephemerisBody != Body::Sun;
        const bool hovered = canReset && !open_ && CheckCollisionPointRec(mouse, reset);
        DrawRectangleRounded(reset, 0.2f, 6, hovered ? Color{58, 79, 99, 255} : Color{35, 48, 64, 255});
        label(font_, "Reset view", reset.x + 10, reset.y + 7, 16,
              canReset ? Color{225, 233, 240, 255} : Color{100, 113, 126, 255});

        if (earthNearest) {
            const Rectangle lights = lightsBounds(panel);
            const bool lightsHovered = !open_ && CheckCollisionPointRec(mouse, lights);
            DrawRectangleRounded(lights, 0.2f, 6, lightsHovered ? Color{58, 79, 99, 255} : Color{35, 48, 64, 255});
            label(font_, "Earth lights", lights.x + 10, lights.y + 7, 16, Color{225, 233, 240, 255});
            label(font_, scene.earthLights() ? "ON" : "OFF", lights.x + lights.width - 38, lights.y + 7, 15,
                  scene.earthLights() ? Color{153, 211, 185, 255} : Color{157, 171, 184, 255});

            const Rectangle clouds = cloudsBounds(panel);
            const bool cloudsHovered = !open_ && CheckCollisionPointRec(mouse, clouds);
            DrawRectangleRounded(clouds, 0.2f, 6, cloudsHovered ? Color{58, 79, 99, 255} : Color{35, 48, 64, 255});
            label(font_, "Clouds", clouds.x + 10, clouds.y + 7, 16, Color{225, 233, 240, 255});
            const char* cloudStatus = scene.cloudsLoading() ? "Loading" : scene.cloudsFailed() && scene.cloudsEnabled()
                                          ? "Failed" : scene.cloudsEnabled() ? "ON" : "OFF";
            label(font_, cloudStatus, clouds.x + clouds.width - (scene.cloudsLoading() ? 70 : scene.cloudsFailed() && scene.cloudsEnabled() ? 55 : 38), clouds.y + 7, 15,
                  scene.cloudsFailed() && scene.cloudsEnabled() ? Color{235, 168, 145, 255} : Color{153, 211, 185, 255});

            if (scene.cloudsEnabled()) {
                const Rectangle area = opacityBounds(panel);
                const Rectangle track = opacityTrack(panel);
                const float knobX = track.x + track.width * scene.cloudOpacity();
                label(font_, "Opacity", area.x + 8, area.y + 2, 14, Color{167, 184, 200, 255});
                label(font_, TextFormat("%d%%", static_cast<int>(std::round(scene.cloudOpacity() * 100))),
                      area.x + area.width - 38, area.y + 2, 14, Color{225, 233, 240, 255});
                DrawRectangleRounded(track, 1.0f, 4, Color{69, 84, 99, 255});
                DrawRectangleRounded({track.x, track.y, knobX - track.x, track.height}, 1.0f, 4, Color{153, 211, 185, 255});
                DrawCircleV({knobX, track.y + track.height / 2}, 6, Color{225, 233, 240, 255});
            }
        }

        const Rectangle shadows = shadowBounds(panel, earthNearest, scene.cloudsEnabled());
        const bool shadowsHovered = !open_ && CheckCollisionPointRec(mouse, shadows);
        DrawRectangleRounded(shadows, 0.2f, 6, shadowsHovered ? Color{58, 79, 99, 255} : Color{35, 48, 64, 255});
        label(font_, "Shadow", shadows.x + 10, shadows.y + 7, 16, Color{225, 233, 240, 255});
        label(font_, scene.shadowsEnabled() ? "ON" : "OFF", shadows.x + shadows.width - 38, shadows.y + 7, 15,
              scene.shadowsEnabled() ? Color{153, 211, 185, 255} : Color{157, 171, 184, 255});

        if (open_) {
            const Rectangle list = listBounds(combo, bodies.size());
            DrawRectangleRounded(list, 0.1f, 6, Color{24, 36, 50, 255});
            const std::size_t rows = visibleRows(combo, bodies.size());
            const std::size_t first = std::min(listFirst_, bodies.size() - rows);
            for (std::size_t index = 0; index < rows; ++index) {
                const std::size_t i = first + index;
                const Rectangle row = bodyBounds(list, index);
                const bool isSelected = selected == &bodies[i];
                if (isSelected || CheckCollisionPointRec(mouse, row))
                    DrawRectangleRounded(row, 0.2f, 6, isSelected ? Color{54, 83, 109, 255} : Color{42, 59, 77, 255});
                label(font_, bodyName(bodies[i].ephemerisBody), row.x + 8, row.y + 6, 17, Color{232, 238, 244, 255});
            }
            if (bodies.size() > rows) {
                const float scrollHeight = (list.height - 8) * static_cast<float>(rows) / bodies.size();
                const float scrollY = list.y + 4 + (list.height - 8 - scrollHeight) * first / (bodies.size() - rows);
                DrawRectangleRounded({list.x + list.width - 5, scrollY, 3, scrollHeight}, 1.0f, 4,
                                     Color{153, 173, 190, 255});
            }
        }
    }
} // namespace solar
