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

# Dev-build asset download; curl runs only when the output file is missing.
find_program(CURL_EXE curl REQUIRED)
set(DOWNLOADS "")
function(add_download name url)
    add_custom_command(
        OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/${name}"
        COMMAND "${CURL_EXE}" -fL --retry 3 -o "${CMAKE_CURRENT_BINARY_DIR}/${name}" "${url}"
        COMMENT "Downloading ${name}"
        VERBATIM
    )
    set(DOWNLOADS ${DOWNLOADS} "${CMAKE_CURRENT_BINARY_DIR}/${name}" PARENT_SCOPE)
endfunction()

add_download(pck00011.tpc "https://naif.jpl.nasa.gov/pub/naif/generic_kernels/pck/pck00011.tpc")
add_download(de442s.bsp "https://naif.jpl.nasa.gov/pub/naif/generic_kernels/spk/planets/de442s.bsp")
add_download(hip_main.dat "https://raw.githubusercontent.com/Lokilife/hipparcos-data/master/hip_main.dat")
add_download(earth_daymap.jpg "https://www.solarsystemscope.com/textures/download/2k_earth_daymap.jpg")
add_download(moon.jpg "https://www.solarsystemscope.com/textures/download/2k_moon.jpg")
add_download(earth_night.jpg "https://assets.science.nasa.gov/content/dam/science/esd/eo/images/imagerecords/144000/144898/BlackMarble_2016_01deg.jpg")
add_download(mercury.jpg "https://www.solarsystemscope.com/textures/download/2k_mercury.jpg")
add_download(venus.jpg "https://www.solarsystemscope.com/textures/download/2k_venus_atmosphere.jpg")
add_download(mars.jpg "https://www.solarsystemscope.com/textures/download/2k_mars.jpg")
add_download(jupiter.jpg "https://www.solarsystemscope.com/textures/download/2k_jupiter.jpg")
add_download(saturn.jpg "https://www.solarsystemscope.com/textures/download/2k_saturn.jpg")
add_download(uranus.jpg "https://www.solarsystemscope.com/textures/download/2k_uranus.jpg")
add_download(neptune.jpg "https://www.solarsystemscope.com/textures/download/2k_neptune.jpg")
add_download(saturn_ring.png "https://www.solarsystemscope.com/textures/download/2k_saturn_ring_alpha.png")
add_custom_target(solar_assets DEPENDS ${DOWNLOADS})
add_dependencies(solar_system solar_assets)
configure_file(assets/fonts/Lato-Regular.ttf Lato-Regular.ttf COPYONLY)
configure_file(assets/pluto/pluto_complete.jpg pluto_complete.jpg COPYONLY)
configure_file(src/shader/surface_shader.vs surface_shader.vs COPYONLY)
configure_file(src/shader/surface_shader.fs surface_shader.fs COPYONLY)
configure_file(src/shader/sun_glow.fs sun_glow.fs COPYONLY)
target_compile_definitions(solar_system PRIVATE
    SOLAR_PCK_PATH="${CMAKE_CURRENT_BINARY_DIR}/pck00011.tpc"
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
        SOLAR_PCK_PATH="${CMAKE_CURRENT_BINARY_DIR}/pck00011.tpc"
        SOLAR_EPHEMERIS_PATH="${CMAKE_CURRENT_BINARY_DIR}/de442s.bsp")
    add_dependencies(orientation_test solar_assets)
    add_test(NAME orientation COMMAND orientation_test)
endif()
