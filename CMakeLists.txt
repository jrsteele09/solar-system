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

# Earth and other planet surface maps use JPEG images.
target_compile_definitions(raylib PRIVATE SUPPORT_FILEFORMAT_JPG=1)

add_executable(solar_system
    src/main.cpp
    src/astronomy/earth.cpp
    src/astronomy/moon.cpp
    src/astronomy/ephemeris.cpp
    src/camera/camera.cpp
    src/scene/scene.cpp
    src/scene/controls_panel.cpp
    src/mesh/mesh.cpp
    src/mesh/sphere.cpp
    src/celestial/celestial_body.cpp
    src/celestial/sun.cpp
    src/celestial/surface_texture.cpp
    src/shader/sunlight.cpp
    src/shader/sun_glow.cpp
)

target_include_directories(solar_system PRIVATE src)
target_link_libraries(solar_system PRIVATE glm::glm raylib)

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
add_custom_target(solar_textures DEPENDS
    "${CMAKE_CURRENT_BINARY_DIR}/earth_daymap.jpg"
    "${CMAKE_CURRENT_BINARY_DIR}/earth_night.jpg"
    "${CMAKE_CURRENT_BINARY_DIR}/moon.jpg")
add_dependencies(solar_system solar_textures)
configure_file(assets/fonts/Lato-Regular.ttf Lato-Regular.ttf COPYONLY)
configure_file(src/shader/sunlight.vs sunlight.vs COPYONLY)
configure_file(src/shader/sunlight.fs sunlight.fs COPYONLY)
configure_file(src/shader/sun_glow.fs sun_glow.fs COPYONLY)
target_compile_definitions(solar_system PRIVATE
    SOLAR_EPHEMERIS_PATH="${CMAKE_CURRENT_BINARY_DIR}/de442s.bsp"
    SOLAR_EARTH_TEXTURE_PATH="${CMAKE_CURRENT_BINARY_DIR}/earth_daymap.jpg"
    SOLAR_EARTH_NIGHT_TEXTURE_PATH="${CMAKE_CURRENT_BINARY_DIR}/earth_night.jpg"
    SOLAR_MOON_TEXTURE_PATH="${CMAKE_CURRENT_BINARY_DIR}/moon.jpg"
    SOLAR_UI_FONT_PATH="${CMAKE_CURRENT_BINARY_DIR}/Lato-Regular.ttf"
    SOLAR_SUNLIGHT_VERTEX_PATH="${CMAKE_CURRENT_BINARY_DIR}/sunlight.vs"
    SOLAR_SUNLIGHT_FRAGMENT_PATH="${CMAKE_CURRENT_BINARY_DIR}/sunlight.fs"
    SOLAR_SUN_GLOW_PATH="${CMAKE_CURRENT_BINARY_DIR}/sun_glow.fs"
)

