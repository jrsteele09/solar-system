#include "controls_panel.hpp"
#include "astronomy/moons.hpp"

#include <algorithm>
#include <cmath>
#include <iterator>
#include <limits>
#include <vector>
#include <raylib.h>

namespace solar {
    constexpr float panelWidth = 196.0f;
    constexpr float margin = 18.0f;
    constexpr float rowHeight = 32.0f;

    static Rectangle panelBounds(bool earthNearest, bool showOpacity) {
        return {GetScreenWidth() - panelWidth - margin, margin, panelWidth,
                earthNearest ? (showOpacity ? 322.0f : 278.0f) : 190.0f};
    }
    static Rectangle comboBounds(Rectangle panel) { return {panel.x + 12, panel.y + 42, panel.width - 24, 34}; }

    struct FlyToButtons {
        Rectangle systems;
        Rectangle moons;

        FlyToButtons(Rectangle combo, bool hasMoons) : systems(combo), moons{} {
            if (hasMoons) {
                constexpr float gap = 8;
                systems.width = (combo.width - gap) / 2;
                moons = {systems.x + systems.width + gap, combo.y, systems.width, combo.height};
            }
        }
    };
    static Rectangle hideBounds(Rectangle panel) { return {panel.x + panel.width - 58, panel.y + 9, 46, 25}; }
    static Rectangle revealBounds() { return {GetScreenWidth() - margin - 90, margin, 90, 28}; }
    static Rectangle resetBounds(Rectangle panel) { return {panel.x + 12, panel.y + 98, panel.width - 24, 32}; }
    static Rectangle lightsBounds(Rectangle panel) { return {panel.x + 12, panel.y + 142, panel.width - 24, 32}; }
    static Rectangle cloudsBounds(Rectangle panel, bool enabled) {
        return {panel.x + 12, panel.y + 186, enabled ? 100.0f : panel.width - 24, 32};
    }
    static Rectangle refreshCloudsBounds(Rectangle panel) {
        return {panel.x + 118, panel.y + 186, panel.width - 130, 32};
    }
    static Rectangle opacityBounds(Rectangle panel) { return {panel.x + 12, panel.y + 222, panel.width - 24, 44}; }
    static Rectangle opacityTrack(Rectangle panel) { return {panel.x + 20, panel.y + 251, panel.width - 40, 4}; }
    static Rectangle shadowBounds(Rectangle panel, bool earthNearest, bool showOpacity) {
        return {panel.x + 12, panel.y + (earthNearest ? (showOpacity ? 274 : 230) : 142), panel.width - 24, 32};
    }

    static std::size_t visibleRows(Rectangle combo, std::size_t count) {
        const float available = GetScreenHeight() - margin - (combo.y + combo.height + 4) - 8;
        return std::min(count, static_cast<std::size_t>(std::max(1.0f, std::floor(available / rowHeight))));
    }

    static Rectangle listBounds(Rectangle combo, std::size_t count) {
        return {combo.x, combo.y + combo.height + 4, combo.width,
                static_cast<float>(visibleRows(combo, count)) * rowHeight + 8};
    }

    static Rectangle bodyBounds(Rectangle list, std::size_t index) {
        return {list.x + 4, list.y + 4 + static_cast<float>(index) * rowHeight, list.width - 8, rowHeight};
    }

    static const CelestialBody* selectedBody(std::span<const std::unique_ptr<CelestialBody>> bodies,
                                             const Camera& camera) {
        for (const auto& entry : bodies)
            if (entry && entry.get() == camera.followedBody())
                return entry.get();
        return nullptr;
    }

    static Body currentSystem(const CelestialBody* selected) {
        if (!selected)
            return Body::Sun;
        const auto* moon = moonInfo(selected->id());
        return moon ? moon->planet : selected->id();
    }

    static std::vector<const CelestialBody*> flyToBodies(
        std::span<const std::unique_ptr<CelestialBody>> bodies, bool moons, Body system) {
        std::vector<const CelestialBody*> options;
        for (const auto& body : bodies) {
            if (!body)
                continue;
            const auto* moon = moonInfo(body->id());
            if (moons ? moon && moon->planet == system : !moon)
                options.push_back(body.get());
        }
        return options;
    }

    static bool systemHasMoons(Body system) {
        return std::any_of(std::begin(majorMoons), std::end(majorMoons),
                           [system](const MoonInfo& moon) { return moon.planet == system; });
    }

    static const CelestialBody* nearestBody(std::span<const std::unique_ptr<CelestialBody>> bodies,
                                            const Camera& camera) {
        const CelestialBody* nearest = nullptr;
        double nearestDistance = std::numeric_limits<double>::infinity();
        for (const auto& entry : bodies) {
            if (!entry) {
                continue;
            }
            const Vector3 position = camera.relativeScenePosition(entry->positionKm());
            const double distance = std::hypot(position.x, position.y, position.z) - entry->radiusSceneUnits();
            if (distance < nearestDistance) {
                nearestDistance = distance;
                nearest = entry.get();
            }
        }
        return nearest;
    }

    static const CelestialBody* localBody(std::span<const std::unique_ptr<CelestialBody>> bodies,
                                          const Camera& camera) {
        if (camera.isFlying()) {
            if (const auto* departure = selectedBody(bodies, camera))
                return departure;
        }
        return nearestBody(bodies, camera);
    }

    static void label(Font font, const char* text, float x, float y, float size, Color color) {
        DrawTextEx(font, text, {x, y}, size, 0.2f, color);
    }

    static void drawLocation(Font font, const CelestialBody* nearest, const CelestialBody* selected) {
        const auto* moon = selected ? moonInfo(selected->id()) : nullptr;
        if (!moon && !nearest)
            return;

        const char* text = moon ? TextFormat("%s - %s", bodyName(moon->planet), moon->name)
                                : bodyName(currentSystem(nearest));
        constexpr float size = 18.0f;
        constexpr float padding = 10.0f;
        const Vector2 textSize = MeasureTextEx(font, text, size, 0.2f);
        const Rectangle badge{margin, margin, textSize.x + padding * 2, 34};
        DrawRectangleRounded(badge, 0.2f, 6, Color{12, 20, 32, 225});
        label(font, text, badge.x + padding, badge.y + (badge.height - textSize.y) / 2, size,
              Color{232, 238, 244, 255});
    }

    static void drawFlyToButton(Font font, const char* text, Rectangle button, Vector2 mouse, bool open) {
        const bool hovered = CheckCollisionPointRec(mouse, button);
        DrawRectangleRounded(button, 0.2f, 6, hovered || open ? Color{54, 72, 90, 255} : Color{35, 48, 64, 255});
        label(font, text, button.x + 10, button.y + 7, 17, Color{232, 238, 244, 255});
        const float arrowX = button.x + button.width - 14;
        const float arrowY = button.y + button.height / 2;
        DrawLineEx({arrowX - 4, arrowY - 2}, {arrowX, arrowY + 2}, 1.5f, Color{184, 199, 212, 255});
        DrawLineEx({arrowX, arrowY + 2}, {arrowX + 4, arrowY - 2}, 1.5f, Color{184, 199, 212, 255});
    }

    ControlsPanel::ControlsPanel() : font_(LoadFontEx(SOLAR_UI_FONT_PATH, 36, nullptr, 0)) {
        SetTextureFilter(font_.texture, TEXTURE_FILTER_BILINEAR);
    }

    ControlsPanel::~ControlsPanel() { UnloadFont(font_); }

    bool ControlsPanel::containsMouse(const Scene& scene, const Camera& camera) const {
        if (hidden_) {
            return CheckCollisionPointRec(GetMousePosition(), revealBounds());
        }
        const auto& earth = scene.getCelestialBody(Body::Earth);
        // While the list is open, an outside click dismisses it without affecting the scene.
        return openMenu_ != FlyToMenu::None || draggingCloudOpacity_ ||
               CheckCollisionPointRec(GetMousePosition(), panelBounds(localBody(scene.bodies(), camera) == &earth,
                                                                      earth.clouds().cloudsEnabled()));
    }

    void ControlsPanel::update(Scene& scene, Camera& camera) {
        const Vector2 mouse = GetMousePosition();
        if (hidden_) {
            if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && CheckCollisionPointRec(mouse, revealBounds()))
                hidden_ = false;
            return;
        }
        auto& earth = scene.getCelestialBody(Body::Earth);
        const auto bodies = scene.bodies();
        const bool earthNearest = localBody(bodies, camera) == &earth;
        const Rectangle panel = panelBounds(earthNearest, earth.clouds().cloudsEnabled());
        const Rectangle combo = comboBounds(panel);
        const CelestialBody* selected = selectedBody(bodies, camera);
        const Body system = currentSystem(selected);
        const bool hasMoons = systemHasMoons(system);
        const FlyToButtons buttons(combo, hasMoons);
        // Dismiss an open moon list before any click can select a row in the new system.
        if (openMenu_ == FlyToMenu::Moons && moonsMenuSystem_ != system) {
            openMenu_ = FlyToMenu::None;
            return;
        }

        const auto options = openMenu_ == FlyToMenu::None
                                 ? std::vector<const CelestialBody*>{}
                                 : flyToBodies(bodies, openMenu_ == FlyToMenu::Moons, system);
        const std::size_t rows = visibleRows(combo, options.size());
        if (openMenu_ != FlyToMenu::None) {
            const std::size_t maxFirst = options.size() - rows;
            listFirst_ = std::min(listFirst_, maxFirst);
            const float wheel = GetMouseWheelMove();
            if (wheel != 0 || IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_UP)) {
                const int direction = wheel < 0 || IsKeyPressed(KEY_DOWN) ? 1 : -1;
                listFirst_ = static_cast<std::size_t>(
                    std::clamp(static_cast<int>(listFirst_) + direction, 0, static_cast<int>(maxFirst)));
            }
        }

        if (draggingCloudOpacity_) {
            if (!earthNearest || !earth.clouds().cloudsEnabled() || !IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
                draggingCloudOpacity_ = false;
            } else {
                const Rectangle track = opacityTrack(panel);
                earth.clouds().setCloudOpacity((mouse.x - track.x) / track.width);
            }
            return;
        }

        if (!IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
            return;

        if (CheckCollisionPointRec(mouse, hideBounds(panel))) {
            hidden_ = true;
            openMenu_ = FlyToMenu::None;
            draggingCloudOpacity_ = false;
            return;
        }

        FlyToMenu clickedMenu = FlyToMenu::None;
        if (CheckCollisionPointRec(mouse, buttons.systems))
            clickedMenu = FlyToMenu::System;
        else if (hasMoons && CheckCollisionPointRec(mouse, buttons.moons))
            clickedMenu = FlyToMenu::Moons;

        if (clickedMenu != FlyToMenu::None) {
            openMenu_ = openMenu_ == clickedMenu ? FlyToMenu::None : clickedMenu;
            if (openMenu_ == FlyToMenu::Moons)
                moonsMenuSystem_ = system;
            listFirst_ = 0;
            return;
        }

        if (openMenu_ != FlyToMenu::None) {
            const Rectangle list = listBounds(combo, options.size());
            for (std::size_t row = 0; row < rows; ++row) {
                if (CheckCollisionPointRec(mouse, bodyBounds(list, row))) {
                    camera.flyToBody(*options[listFirst_ + row]);
                    break;
                }
            }
            openMenu_ = FlyToMenu::None;
            return;
        }
        if (earthNearest && earth.clouds().cloudsEnabled() && CheckCollisionPointRec(mouse, opacityBounds(panel))) {
            draggingCloudOpacity_ = true;
            const Rectangle track = opacityTrack(panel);
            earth.clouds().setCloudOpacity((mouse.x - track.x) / track.width);
            return;
        }
        if (!camera.isFlying() && selected && selected->id() != Body::Sun &&
            CheckCollisionPointRec(mouse, resetBounds(panel))) {
            // Reset a moon to its planet-facing side.
            const auto* moon = moonInfo(selected->id());
            const Body observer = moon ? moon->planet : Body::Sun;
            const auto& viewpoint = scene.getCelestialBody(observer);
            camera.resetToBody(*selected, viewpoint.positionKm());
        }
        if (earthNearest && CheckCollisionPointRec(mouse, lightsBounds(panel)))
            earth.setLightsEnabled(!earth.lightsEnabled());
        if (earthNearest && earth.clouds().cloudsEnabled() &&
            CheckCollisionPointRec(mouse, refreshCloudsBounds(panel))) {
            earth.clouds().refreshClouds();
            return;
        }
        if (earthNearest && CheckCollisionPointRec(mouse, cloudsBounds(panel, earth.clouds().cloudsEnabled())))
            earth.clouds().enableClouds(!earth.clouds().cloudsEnabled());
        if (CheckCollisionPointRec(mouse, shadowBounds(panel, earthNearest, earth.clouds().cloudsEnabled())))
            scene.setShadowsEnabled(!scene.shadowsEnabled());
    }

    void ControlsPanel::draw(const Scene& scene, const Camera& camera) const {
        const auto bodies = scene.bodies();
        const auto* local = localBody(bodies, camera);
        const auto* selected = selectedBody(bodies, camera);
        drawLocation(font_, local, selected);

        if (hidden_) {
            const Rectangle button = revealBounds();
            const bool hovered = CheckCollisionPointRec(GetMousePosition(), button);
            DrawRectangleRounded(button, 0.25f, 6, hovered ? Color{58, 79, 99, 255} : Color{24, 36, 50, 225});
            label(font_, "Controls", button.x + 12, button.y + 6, 15, Color{225, 233, 240, 255});
            return;
        }
        const auto& earth = scene.getCelestialBody(Body::Earth);
        const bool earthNearest = local == &earth;
        const Rectangle panel = panelBounds(earthNearest, earth.clouds().cloudsEnabled());
        const Rectangle combo = comboBounds(panel);
        const Vector2 mouse = GetMousePosition();
        DrawRectangleRounded(panel, 0.08f, 8, Color{12, 20, 32, 225});
        label(font_, "Fly to", panel.x + 14, panel.y + 14, 16, Color{167, 184, 200, 255});
        const Rectangle hide = hideBounds(panel);
        const bool hideHovered = CheckCollisionPointRec(mouse, hide);
        DrawRectangleRounded(hide, 0.25f, 6, hideHovered ? Color{58, 79, 99, 255} : Color{35, 48, 64, 255});
        label(font_, "Hide", hide.x + 8, hide.y + 5, 14, Color{225, 233, 240, 255});
        const Body system = currentSystem(selected);
        const bool hasMoons = systemHasMoons(system);
        const FlyToButtons buttons(combo, hasMoons);
        const bool moonsOpen = openMenu_ == FlyToMenu::Moons && moonsMenuSystem_ == system;
        const bool menuOpen = openMenu_ == FlyToMenu::System || moonsOpen;
        drawFlyToButton(font_, "System", buttons.systems, mouse, openMenu_ == FlyToMenu::System);
        if (hasMoons)
            drawFlyToButton(font_, "Moons", buttons.moons, mouse, moonsOpen);

        const Rectangle reset = resetBounds(panel);
        const bool canReset = !camera.isFlying() && selected && selected->id() != Body::Sun;
        const bool hovered = canReset && !menuOpen && CheckCollisionPointRec(mouse, reset);
        DrawRectangleRounded(reset, 0.2f, 6, hovered ? Color{58, 79, 99, 255} : Color{35, 48, 64, 255});
        label(font_, "Reset view", reset.x + 10, reset.y + 7, 16,
              canReset ? Color{225, 233, 240, 255} : Color{100, 113, 126, 255});

        if (earthNearest) {
            const Rectangle lights = lightsBounds(panel);
            const bool lightsHovered = !menuOpen && CheckCollisionPointRec(mouse, lights);
            DrawRectangleRounded(lights, 0.2f, 6, lightsHovered ? Color{58, 79, 99, 255} : Color{35, 48, 64, 255});
            label(font_, "Earth lights", lights.x + 10, lights.y + 7, 16, Color{225, 233, 240, 255});
            label(font_, earth.lightsEnabled() ? "ON" : "OFF", lights.x + lights.width - 38, lights.y + 7, 15,
                  earth.lightsEnabled() ? Color{153, 211, 185, 255} : Color{157, 171, 184, 255});

            const Rectangle clouds = cloudsBounds(panel, earth.clouds().cloudsEnabled());
            const bool cloudsHovered = !menuOpen && CheckCollisionPointRec(mouse, clouds);
            DrawRectangleRounded(clouds, 0.2f, 6, cloudsHovered ? Color{58, 79, 99, 255} : Color{35, 48, 64, 255});
            label(font_, "Clouds", clouds.x + 10, clouds.y + 7, 16, Color{225, 233, 240, 255});
            label(font_, earth.clouds().cloudsEnabled() ? "ON" : "OFF", clouds.x + clouds.width - 38, clouds.y + 7, 15,
                  earth.clouds().cloudsEnabled() ? Color{153, 211, 185, 255} : Color{157, 171, 184, 255});

            if (earth.clouds().cloudsEnabled()) {
                const Rectangle refresh = refreshCloudsBounds(panel);
                const bool refreshHovered =
                    !menuOpen && !earth.clouds().cloudsLoading() && CheckCollisionPointRec(mouse, refresh);
                DrawRectangleRounded(refresh, 0.2f, 6,
                                     refreshHovered ? Color{58, 79, 99, 255} : Color{35, 48, 64, 255});
                const char* status = earth.clouds().cloudsLoading()        ? "Loading"
                                     : earth.clouds().cloudRefreshFailed() ? "Retry"
                                                                           : "Refresh";
                label(font_, status, refresh.x + 6, refresh.y + 9, 13,
                      earth.clouds().cloudRefreshFailed() ? Color{235, 168, 145, 255} : Color{225, 233, 240, 255});
                const Rectangle area = opacityBounds(panel);
                const Rectangle track = opacityTrack(panel);
                const float knobX = track.x + track.width * earth.clouds().cloudOpacity();
                label(font_, "Opacity", area.x + 8, area.y + 2, 14, Color{167, 184, 200, 255});
                label(font_, TextFormat("%d%%", static_cast<int>(std::round(earth.clouds().cloudOpacity() * 100))),
                      area.x + area.width - 38, area.y + 2, 14, Color{225, 233, 240, 255});
                DrawRectangleRounded(track, 1.0f, 4, Color{69, 84, 99, 255});
                DrawRectangleRounded({track.x, track.y, knobX - track.x, track.height}, 1.0f, 4,
                                     Color{153, 211, 185, 255});
                DrawCircleV({knobX, track.y + track.height / 2}, 6, Color{225, 233, 240, 255});
            }
        }

        const Rectangle shadows = shadowBounds(panel, earthNearest, earth.clouds().cloudsEnabled());
        const bool shadowsHovered = !menuOpen && CheckCollisionPointRec(mouse, shadows);
        DrawRectangleRounded(shadows, 0.2f, 6, shadowsHovered ? Color{58, 79, 99, 255} : Color{35, 48, 64, 255});
        label(font_, "Shadow", shadows.x + 10, shadows.y + 7, 16, Color{225, 233, 240, 255});
        label(font_, scene.shadowsEnabled() ? "ON" : "OFF", shadows.x + shadows.width - 38, shadows.y + 7, 15,
              scene.shadowsEnabled() ? Color{153, 211, 185, 255} : Color{157, 171, 184, 255});

        if (menuOpen) {
            const bool moons = openMenu_ == FlyToMenu::Moons;
            const auto options = flyToBodies(bodies, moons, system);
            const Rectangle list = listBounds(combo, options.size());
            DrawRectangleRounded(list, 0.1f, 6, Color{24, 36, 50, 255});
            const std::size_t rows = visibleRows(combo, options.size());
            const std::size_t first = std::min(listFirst_, options.size() - rows);
            for (std::size_t index = 0; index < rows; ++index) {
                const std::size_t i = first + index;
                const Rectangle row = bodyBounds(list, index);
                const bool isSelected = moons ? selected == options[i] : selected && system == options[i]->id();
                if (isSelected || CheckCollisionPointRec(mouse, row))
                    DrawRectangleRounded(row, 0.2f, 6, isSelected ? Color{54, 83, 109, 255} : Color{42, 59, 77, 255});
                label(font_, bodyName(options[i]->id()), row.x + 8, row.y + 6, 17, Color{232, 238, 244, 255});
            }
            if (options.size() > rows) {
                const float scrollHeight = (list.height - 8) * static_cast<float>(rows) / options.size();
                const float scrollY = list.y + 4 + (list.height - 8 - scrollHeight) * first / (options.size() - rows);
                DrawRectangleRounded({list.x + list.width - 5, scrollY, 3, scrollHeight}, 1.0f, 4,
                                     Color{153, 173, 190, 255});
            }
        }
    }
} // namespace solar
