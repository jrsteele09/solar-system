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

# Build CSPICE from its source mirror without importing its standalone CMake project.
FetchContent_Declare(cspice_source
    GIT_REPOSITORY https://github.com/tudat-team/cspice-cmake.git
    GIT_TAG e44da83b8a7a420fdddf02d2e3accebd0ab86e5e
    SOURCE_SUBDIR unused
)

FetchContent_MakeAvailable(cspice_source)
file(GLOB CSPICE_SOURCES CONFIGURE_DEPENDS "${cspice_source_SOURCE_DIR}/src/cspice/*.c")
add_library(cspice STATIC ${CSPICE_SOURCES})
if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(cspice PRIVATE -Wno-implicit-int -Wno-implicit-function-declaration)
endif()

target_include_directories(cspice PUBLIC "${cspice_source_SOURCE_DIR}/include/cspice")
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
    src/celestial/clouds.cpp
    src/http/download.cpp
    src/celestial/star_field.cpp
    src/mesh/mesh.cpp
    src/mesh/sphere.cpp
    src/celestial/celestial_body.cpp
    src/celestial/surface_texture.cpp
    src/shader/surface_shader.cpp
    src/shader/sun_glow.cpp
)

target_include_directories(solar_system PRIVATE src)
target_link_libraries(solar_system PRIVATE glm::glm raylib CURL::libcurl cspice)

target_compile_definitions(solar_system PRIVATE GLM_FORCE_INTRINSICS)
target_compile_definitions(solar_system PRIVATE GLM_FORCE_CTOR_INIT)

# Cache runtime downloads in the checkout so all build configurations share them.
set(ASSET_DIR "${CMAKE_CURRENT_SOURCE_DIR}/assets")
find_program(CURL_EXE curl REQUIRED)
add_custom_target(solar_assets
    COMMAND "${CMAKE_COMMAND}" "-DASSET_DIR=${ASSET_DIR}" "-DCURL_EXE=${CURL_EXE}"
            -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/DownloadAssets.cmake"
    VERBATIM
)
add_dependencies(solar_system solar_assets)
configure_file(src/shader/surface_shader.vs surface_shader.vs COPYONLY)
configure_file(src/shader/surface_shader.fs surface_shader.fs COPYONLY)
configure_file(src/shader/sun_glow.fs sun_glow.fs COPYONLY)
target_compile_definitions(solar_system PRIVATE
    SOLAR_PCK_PATH="${ASSET_DIR}/kernels/pck00011.tpc"
    SOLAR_SATELLITE_PATH="${ASSET_DIR}/kernels/cg_1950_2050_v03.bsp"
    SOLAR_EPHEMERIS_PATH="${ASSET_DIR}/kernels/de442s.bsp"
    SOLAR_STAR_CATALOGUE_PATH="${ASSET_DIR}/stars/hip_main.dat"
    SOLAR_EARTH_TEXTURE_PATH="${ASSET_DIR}/textures/8k_earth_daymap.jpg"
    SOLAR_EARTH_NIGHT_TEXTURE_PATH="${ASSET_DIR}/textures/earth_night.jpg"
    SOLAR_MERCURY_TEXTURE_PATH="${ASSET_DIR}/textures/8k_mercury.jpg"
    SOLAR_VENUS_TEXTURE_PATH="${ASSET_DIR}/textures/4k_venus_atmosphere.jpg"
    SOLAR_MARS_TEXTURE_PATH="${ASSET_DIR}/textures/8k_mars.jpg"
    SOLAR_JUPITER_TEXTURE_PATH="${ASSET_DIR}/textures/8k_jupiter.jpg"
    SOLAR_SATURN_TEXTURE_PATH="${ASSET_DIR}/textures/8k_saturn.jpg"
    SOLAR_URANUS_TEXTURE_PATH="${ASSET_DIR}/textures/uranus.jpg"
    SOLAR_NEPTUNE_TEXTURE_PATH="${ASSET_DIR}/textures/neptune.jpg"
    SOLAR_PLUTO_TEXTURE_PATH="${ASSET_DIR}/textures/pluto_nasa.jpg"
    SOLAR_SATURN_RING_TEXTURE_PATH="${ASSET_DIR}/textures/8k_saturn_ring_alpha.png"
    SOLAR_ASSET_DIR="${ASSET_DIR}"
    SOLAR_UI_FONT_PATH="${ASSET_DIR}/fonts/Lato-Regular.ttf"
    SOLAR_SURFACE_SHADER_VERTEX_PATH="${CMAKE_CURRENT_BINARY_DIR}/surface_shader.vs"
    SOLAR_SURFACE_SHADER_FRAGMENT_PATH="${CMAKE_CURRENT_BINARY_DIR}/surface_shader.fs"
    SOLAR_SUN_GLOW_PATH="${CMAKE_CURRENT_BINARY_DIR}/sun_glow.fs"
)

include(CTest)
if(BUILD_TESTING)
    add_executable(orientation_test tests/orientation_test.cpp
        src/astronomy/planet_orientation.cpp src/astronomy/ephemeris.cpp src/mesh/mesh.cpp)
    target_include_directories(orientation_test PRIVATE src)
    target_link_libraries(orientation_test PRIVATE glm::glm cspice)
    target_compile_definitions(orientation_test PRIVATE
        SOLAR_PCK_PATH="${ASSET_DIR}/kernels/pck00011.tpc"
        SOLAR_SATELLITE_PATH="${ASSET_DIR}/kernels/cg_1950_2050_v03.bsp"
        SOLAR_EPHEMERIS_PATH="${ASSET_DIR}/kernels/de442s.bsp")
    add_dependencies(orientation_test solar_assets)
    add_test(NAME orientation COMMAND orientation_test)
endif()
