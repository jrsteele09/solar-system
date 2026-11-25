#pragma once

#include "ephemeris.hpp"

namespace solar {
    struct MoonInfo {
        Body id;
        Body planet;
        const char* name;
        const char* texture;
        const char* shapeModel = nullptr;
        int rings = 40;
        int slices = 80;
        int maxTextureWidth = 2048;
    };

    inline constexpr MoonInfo majorMoons[] = {
        {Body::Moon, Body::Earth, "Moon", "textures/8k_moon.jpg", nullptr, 96, 160, 0},
        {Body::Phobos, Body::Mars, "Phobos", "moons/phobos.jpg", "moons/phobos.obj"},
        {Body::Deimos, Body::Mars, "Deimos", "moons/deimos.jpg", "moons/deimos.obj"},
        {Body::Io, Body::Jupiter, "Io", "moons/io.png"},
        {Body::Europa, Body::Jupiter, "Europa", "moons/europa.jpg"},
        {Body::Ganymede, Body::Jupiter, "Ganymede", "moons/ganymede.jpg"},
        {Body::Callisto, Body::Jupiter, "Callisto", "moons/callisto.jpg"},
        {Body::Mimas, Body::Saturn, "Mimas", "moons/mimas.jpg"},
        {Body::Enceladus, Body::Saturn, "Enceladus", "moons/enceladus.jpg"},
        {Body::Tethys, Body::Saturn, "Tethys", "moons/tethys.jpg"},
        {Body::Dione, Body::Saturn, "Dione", "moons/dione.jpg"},
        {Body::Rhea, Body::Saturn, "Rhea", "moons/rhea.jpg"},
        {Body::Titan, Body::Saturn, "Titan", "moons/titan-clouds.jpg"},
        {Body::Iapetus, Body::Saturn, "Iapetus", "moons/iapetus.jpg"},
        {Body::Miranda, Body::Uranus, "Miranda", "moons/miranda.jpg"},
        {Body::Ariel, Body::Uranus, "Ariel", "moons/ariel.jpg"},
        {Body::Umbriel, Body::Uranus, "Umbriel", "moons/umbriel.jpg"},
        {Body::Titania, Body::Uranus, "Titania", "moons/titania.jpg"},
        {Body::Oberon, Body::Uranus, "Oberon", "moons/oberon.jpg"},
        {Body::Triton, Body::Neptune, "Triton", "moons/triton.jpg"},
        {Body::Charon, Body::Pluto, "Charon", "moons/charon.jpg"},
    };

    inline const MoonInfo* moonInfo(Body id) {
        for (const auto& moon : majorMoons)
            if (moon.id == id)
                return &moon;
        return nullptr;
    }
} // namespace solar
