#include "controls_panel.hpp"

#include <raylib.h>
#include <cmath>
#include <limits>

namespace solar {
    namespace {
        constexpr float panelWidth = 196.0f;
        constexpr float margin = 18.0f;
        constexpr float rowHeight = 32.0f;

        Rectangle panelBounds(bool earthNearest) {
            return {GetScreenWidth() - panelWidth - margin, margin, panelWidth, earthNearest ? 234.0f : 190.0f};
        }
        Rectangle comboBounds(Rectangle panel) { return {panel.x + 12, panel.y + 42, panel.width - 24, 34}; }
        Rectangle resetBounds(Rectangle panel) { return {panel.x + 12, panel.y + 98, panel.width - 24, 32}; }
        Rectangle lightsBounds(Rectangle panel) { return {panel.x + 12, panel.y + 142, panel.width - 24, 32}; }
        Rectangle shadowBounds(Rectangle panel, bool earthNearest) {
            return {panel.x + 12, panel.y + (earthNearest ? 186 : 142), panel.width - 24, 32};
        }
        Rectangle listBounds(Rectangle combo, std::size_t count) {
            return {combo.x, combo.y + combo.height + 4, combo.width, static_cast<float>(count) * rowHeight + 8};
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
        return open_ || CheckCollisionPointRec(GetMousePosition(), panelBounds(earthIsNearest(scene.bodies(), camera)));
    }

    void ControlsPanel::update(Scene& scene, Camera& camera, glm::dvec3 sunPositionKm) {
        const auto bodies = scene.bodies();
        if (!IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
            return;
        const Vector2 mouse = GetMousePosition();
        const bool earthNearest = earthIsNearest(bodies, camera);
        const Rectangle panel = panelBounds(earthNearest);
        const Rectangle combo = comboBounds(panel);
        if (CheckCollisionPointRec(mouse, combo)) {
            open_ = !open_;
            return;
        }
        if (open_) {
            const Rectangle list = listBounds(combo, bodies.size());
            for (std::size_t i = 0; i < bodies.size(); ++i) {
                if (bodies[i].body && CheckCollisionPointRec(mouse, bodyBounds(list, i))) {
                    camera.flyToBody(*bodies[i].body);
                    break;
                }
            }
            open_ = false;
            return;
        }
        const BodyUpdate* selected = selectedBody(bodies, camera);
        if (selected && selected->ephemerisBody != Body::Sun && CheckCollisionPointRec(mouse, resetBounds(panel)))
            camera.resetToBody(*selected->body, sunPositionKm);
        if (earthNearest && CheckCollisionPointRec(mouse, lightsBounds(panel)))
            scene.setEarthLights(!scene.earthLights());
        if (CheckCollisionPointRec(mouse, shadowBounds(panel, earthNearest)))
            scene.setShadowsEnabled(!scene.shadowsEnabled());
    }

    void ControlsPanel::draw(const Scene& scene, const Camera& camera) const {
        const auto bodies = scene.bodies();
        const bool earthNearest = earthIsNearest(bodies, camera);
        const Rectangle panel = panelBounds(earthNearest);
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
        }

        const Rectangle shadows = shadowBounds(panel, earthNearest);
        const bool shadowsHovered = !open_ && CheckCollisionPointRec(mouse, shadows);
        DrawRectangleRounded(shadows, 0.2f, 6, shadowsHovered ? Color{58, 79, 99, 255} : Color{35, 48, 64, 255});
        label(font_, "Shadow", shadows.x + 10, shadows.y + 7, 16, Color{225, 233, 240, 255});
        label(font_, scene.shadowsEnabled() ? "ON" : "OFF", shadows.x + shadows.width - 38, shadows.y + 7, 15,
              scene.shadowsEnabled() ? Color{153, 211, 185, 255} : Color{157, 171, 184, 255});

        if (open_) {
            const Rectangle list = listBounds(combo, bodies.size());
            DrawRectangleRounded(list, 0.1f, 6, Color{24, 36, 50, 255});
            for (std::size_t i = 0; i < bodies.size(); ++i) {
                const Rectangle row = bodyBounds(list, i);
                const bool isSelected = selected == &bodies[i];
                if (isSelected || CheckCollisionPointRec(mouse, row))
                    DrawRectangleRounded(row, 0.2f, 6, isSelected ? Color{54, 83, 109, 255} : Color{42, 59, 77, 255});
                label(font_, bodyName(bodies[i].ephemerisBody), row.x + 8, row.y + 6, 17, Color{232, 238, 244, 255});
            }
        }
    }
} // namespace solar
