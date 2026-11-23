cmake_minimum_required(VERSION 3.24)
project(solar_system LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(FetchContent)
FetchContent_Declare(
    glm
    GIT_REPOSITORY https://github.com/g-truc/glm.git
    GIT_TAG 1.0.2
    GIT_SHALLOW TRUE
)
FetchContent_Declare(
    raylib
    GIT_REPOSITORY https://github.com/raysan5/raylib.git
    GIT_TAG 6.2
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(glm raylib)
find_package(CURL 7.85 REQUIRED)

# Earth and other planet surface maps use JPEG images.
target_compile_definitions(raylib PRIVATE SUPPORT_FILEFORMAT_JPG=1)

add_executable(solar_system
    src/main.cpp
    src/astronomy/planet_orientation.cpp
    src/astronomy/ephemeris.cpp
    src/scene/camera.cpp
    src/scene/scene.cpp
    src/scene/controls_panel.cpp
    src/celestial/earth_clouds.cpp
    src/celestial/star_field.cpp
    src/mesh/mesh.cpp
    src/mesh/sphere.cpp
    src/celestial/celestial_body.cpp
    src/celestial/sun.cpp
    src/celestial/saturn.cpp
    src/celestial/surface_texture.cpp
    src/shader/sunlight.cpp
    src/shader/sun_glow.cpp
)

target_include_directories(solar_system PRIVATE src)
target_link_libraries(solar_system PRIVATE glm::glm raylib CURL::libcurl)

target_compile_definitions(solar_system PRIVATE GLM_FORCE_INTRINSICS)
target_compile_definitions(solar_system PRIVATE GLM_FORCE_CTOR_INIT)

add_custom_command(
    OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/de442s.bsp"
    COMMAND "${CMAKE_COMMAND}"
        "-DTEXTURE_SOURCE=${CMAKE_CURRENT_SOURCE_DIR}/assets/de442s.bsp"
        "-DTEXTURE_OUTPUT=${CMAKE_CURRENT_BINARY_DIR}/de442s.bsp"
        "-DTEXTURE_URL=https://naif.jpl.nasa.gov/pub/naif/generic_kernels/spk/planets/de442s.bsp"
        "-DTEXTURE_SHA256=54d97562a5b094d298b1b8eafa5a2e17e3e010ce85e1a366d07f003ad159323c"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/DownloadTexture.cmake"
    DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/cmake/DownloadTexture.cmake"
    COMMENT "Preparing de442s.bsp"
    VERBATIM
)
add_custom_target(solar_ephemeris DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/de442s.bsp")
add_dependencies(solar_system solar_ephemeris)

add_custom_command(
    OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/hip_main.dat"
    COMMAND "${CMAKE_COMMAND}"
        "-DTEXTURE_SOURCE=${CMAKE_CURRENT_SOURCE_DIR}/assets/stars/hip_main.dat"
        "-DTEXTURE_OUTPUT=${CMAKE_CURRENT_BINARY_DIR}/hip_main.dat"
        "-DTEXTURE_URL=https://raw.githubusercontent.com/Lokilife/hipparcos-data/master/hip_main.dat"
        "-DTEXTURE_SHA256=58ceabb104d647160d9437ce6e513a02a036bb4ad9f8879a5a22fd52943616e0"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/DownloadTexture.cmake"
    DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/cmake/DownloadTexture.cmake"
    COMMENT "Preparing Hipparcos star catalogue"
    VERBATIM
)
add_custom_target(solar_stars DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/hip_main.dat")
add_dependencies(solar_system solar_stars)

function(add_texture name url sha256)
    add_custom_command(
        OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/${name}"
        COMMAND "${CMAKE_COMMAND}"
            "-DTEXTURE_SOURCE=${CMAKE_CURRENT_SOURCE_DIR}/assets/textures/${name}"
            "-DTEXTURE_OUTPUT=${CMAKE_CURRENT_BINARY_DIR}/${name}"
            "-DTEXTURE_URL=${url}"
            "-DTEXTURE_SHA256=${sha256}"
            -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/DownloadTexture.cmake"
        DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/cmake/DownloadTexture.cmake"
        COMMENT "Preparing ${name}"
        VERBATIM
    )
endfunction()

add_texture(earth_daymap.jpg "https://www.solarsystemscope.com/textures/download/2k_earth_daymap.jpg"
            767ee1dc6eb3802699bfccf6f264880f8acd0b80de3191cd24984fe279b07b7c)
add_texture(moon.jpg "https://www.solarsystemscope.com/textures/download/2k_moon.jpg"
            2764ba6535ea0481a062846ee033cc7a909dae05b31a8fd13f3e98f3a7fd92bd)
add_texture(earth_night.jpg "https://assets.science.nasa.gov/content/dam/science/esd/eo/images/imagerecords/144000/144898/BlackMarble_2016_01deg.jpg"
            d87de751a264e4f8ff69c68de5dab9606daee87a6f15ae743c93200743bd7ec1)
add_texture(mercury.jpg "https://www.solarsystemscope.com/textures/download/2k_mercury.jpg" 5a5c80607f643496bac9a631e71957def35ed788895f18b678ac849c2b38e48a)
add_texture(venus.jpg "https://www.solarsystemscope.com/textures/download/2k_venus_atmosphere.jpg" 225012ad4911730605c4e189ca2a3bf674fce50cc48aab4102b936b47d6991ac)
add_texture(mars.jpg "https://www.solarsystemscope.com/textures/download/2k_mars.jpg" 2d187f3e77a98eaa8cea5f4cc722f633c122ef170b9e94ace6b5fb6cbc3f8e01)
add_texture(jupiter.jpg "https://www.solarsystemscope.com/textures/download/2k_jupiter.jpg" b0f04d005350252636b0e3396fc592548cbd9e9126b269d32d5c6abd4b0e4f2b)
add_texture(saturn.jpg "https://www.solarsystemscope.com/textures/download/2k_saturn.jpg" 54a900ca9bf7ab62e70f862852759abdf342e6d6436a95a2fe9ebdb6bcd3bbac)
add_texture(uranus.jpg "https://www.solarsystemscope.com/textures/download/2k_uranus.jpg" d15239d46f82d3ea13d2b260b5b29b2a382f42f2916dae0694d0387b1204a09d)
add_texture(neptune.jpg "https://www.solarsystemscope.com/textures/download/2k_neptune.jpg" cb42ea82709741d28b0af44d8b283cbc6dbd0c521a7f0e1e1e010ade00977df6)
add_texture(saturn_ring.png "https://www.solarsystemscope.com/textures/download/2k_saturn_ring_alpha.png" 4b0644b2f3ef259fd0bfaedd9dc52f9e7d9738f65f50363e8c4dd19f650d7334)
add_custom_target(solar_textures DEPENDS
    "${CMAKE_CURRENT_BINARY_DIR}/earth_daymap.jpg"
    "${CMAKE_CURRENT_BINARY_DIR}/earth_night.jpg"
    "${CMAKE_CURRENT_BINARY_DIR}/mercury.jpg"
    "${CMAKE_CURRENT_BINARY_DIR}/venus.jpg"
    "${CMAKE_CURRENT_BINARY_DIR}/mars.jpg"
    "${CMAKE_CURRENT_BINARY_DIR}/jupiter.jpg"
    "${CMAKE_CURRENT_BINARY_DIR}/saturn.jpg"
    "${CMAKE_CURRENT_BINARY_DIR}/uranus.jpg"
    "${CMAKE_CURRENT_BINARY_DIR}/neptune.jpg"
    "${CMAKE_CURRENT_BINARY_DIR}/saturn_ring.png"
    "${CMAKE_CURRENT_BINARY_DIR}/moon.jpg")
add_dependencies(solar_system solar_textures)
configure_file(assets/fonts/Lato-Regular.ttf Lato-Regular.ttf COPYONLY)
configure_file(assets/pluto/pluto_complete.jpg pluto_complete.jpg COPYONLY)
configure_file(src/shader/sunlight.vs sunlight.vs COPYONLY)
configure_file(src/shader/sunlight.fs sunlight.fs COPYONLY)
configure_file(src/shader/sun_glow.fs sun_glow.fs COPYONLY)
target_compile_definitions(solar_system PRIVATE
    SOLAR_EPHEMERIS_PATH="${CMAKE_CURRENT_BINARY_DIR}/de442s.bsp"
    SOLAR_STAR_CATALOGUE_PATH="${CMAKE_CURRENT_BINARY_DIR}/hip_main.dat"
    SOLAR_EARTH_TEXTURE_PATH="${CMAKE_CURRENT_BINARY_DIR}/earth_daymap.jpg"
    SOLAR_EARTH_NIGHT_TEXTURE_PATH="${CMAKE_CURRENT_BINARY_DIR}/earth_night.jpg"
    SOLAR_MERCURY_TEXTURE_PATH="${CMAKE_CURRENT_BINARY_DIR}/mercury.jpg"
    SOLAR_VENUS_TEXTURE_PATH="${CMAKE_CURRENT_BINARY_DIR}/venus.jpg"
    SOLAR_MARS_TEXTURE_PATH="${CMAKE_CURRENT_BINARY_DIR}/mars.jpg"
    SOLAR_JUPITER_TEXTURE_PATH="${CMAKE_CURRENT_BINARY_DIR}/jupiter.jpg"
    SOLAR_SATURN_TEXTURE_PATH="${CMAKE_CURRENT_BINARY_DIR}/saturn.jpg"
    SOLAR_URANUS_TEXTURE_PATH="${CMAKE_CURRENT_BINARY_DIR}/uranus.jpg"
    SOLAR_NEPTUNE_TEXTURE_PATH="${CMAKE_CURRENT_BINARY_DIR}/neptune.jpg"
    SOLAR_PLUTO_TEXTURE_PATH="${CMAKE_CURRENT_BINARY_DIR}/pluto_complete.jpg"
    SOLAR_SATURN_RING_TEXTURE_PATH="${CMAKE_CURRENT_BINARY_DIR}/saturn_ring.png"
    SOLAR_MOON_TEXTURE_PATH="${CMAKE_CURRENT_BINARY_DIR}/moon.jpg"
    SOLAR_UI_FONT_PATH="${CMAKE_CURRENT_BINARY_DIR}/Lato-Regular.ttf"
    SOLAR_SUNLIGHT_VERTEX_PATH="${CMAKE_CURRENT_BINARY_DIR}/sunlight.vs"
    SOLAR_SUNLIGHT_FRAGMENT_PATH="${CMAKE_CURRENT_BINARY_DIR}/sunlight.fs"
    SOLAR_SUN_GLOW_PATH="${CMAKE_CURRENT_BINARY_DIR}/sun_glow.fs"
)

