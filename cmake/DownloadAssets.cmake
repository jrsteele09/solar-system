cmake_minimum_required(VERSION 3.24)

# Separate build trees share this cache. Publish a file only after curl succeeds.
file(MAKE_DIRECTORY "${ASSET_DIR}")
file(LOCK "${ASSET_DIR}/.download.lock" GUARD PROCESS)
function(download_asset name url)
    set(path "${ASSET_DIR}/${name}")
    if(EXISTS "${path}")
        return()
    endif()
    get_filename_component(directory "${path}" DIRECTORY)
    file(MAKE_DIRECTORY "${directory}")
    message(STATUS "Downloading ${name}")
    execute_process(
        COMMAND "${CURL_EXE}" -fL --retry 3 -o "${path}.part" "${url}"
        COMMAND_ERROR_IS_FATAL ANY
    )
    file(RENAME "${path}.part" "${path}")
endfunction()

download_asset("kernels/pck00011.tpc" "https://naif.jpl.nasa.gov/pub/naif/generic_kernels/pck/pck00011.tpc")
download_asset("textures/pluto_nasa.jpg" "https://assets.science.nasa.gov/content/dam/science/psd/solar/2023/09/p/l/pluto_color_mapmosaic.jpg")
download_asset("kernels/de442s.bsp" "https://naif.jpl.nasa.gov/pub/naif/generic_kernels/spk/planets/de442s.bsp")
download_asset("stars/hip_main.dat" "https://raw.githubusercontent.com/Lokilife/hipparcos-data/master/hip_main.dat")
download_asset("textures/8k_earth_daymap.jpg" "https://www.solarsystemscope.com/textures/download/8k_earth_daymap.jpg")
download_asset("textures/8k_moon.jpg" "https://www.solarsystemscope.com/textures/download/8k_moon.jpg")
download_asset("textures/earth_night.jpg" "https://assets.science.nasa.gov/content/dam/science/esd/eo/images/imagerecords/144000/144898/BlackMarble_2016_01deg.jpg")
download_asset("textures/8k_mercury.jpg" "https://www.solarsystemscope.com/textures/download/8k_mercury.jpg")
download_asset("textures/4k_venus_atmosphere.jpg" "https://www.solarsystemscope.com/textures/download/4k_venus_atmosphere.jpg")
download_asset("textures/8k_mars.jpg" "https://www.solarsystemscope.com/textures/download/8k_mars.jpg")
download_asset("textures/8k_jupiter.jpg" "https://www.solarsystemscope.com/textures/download/8k_jupiter.jpg")
download_asset("textures/8k_saturn.jpg" "https://www.solarsystemscope.com/textures/download/8k_saturn.jpg")
download_asset("textures/uranus.jpg" "https://www.solarsystemscope.com/textures/download/2k_uranus.jpg")
download_asset("textures/neptune.jpg" "https://www.solarsystemscope.com/textures/download/2k_neptune.jpg")
download_asset("textures/8k_saturn_ring_alpha.png" "https://www.solarsystemscope.com/textures/download/8k_saturn_ring_alpha.png")

# NAIF's combined satellite kernel contains the physical centres as well as their moons.
download_asset("kernels/cg_1950_2050_v03.bsp" "https://naif.jpl.nasa.gov/pub/naif/cosmographia/kernels/spice/spk/cg_1950_2050_v03.bsp")

set(MOON_MAPS "https://raw.githubusercontent.com/CelestiaProject/CelestiaContent/1bbbf41ed4639a17fd9a884fd843e425caedd9a3/textures")

# JHU/APL Small Body Mapping Tool global shape models by Ernst et al.
set(SBMT_MODELS "https://sbmt.jhuapl.edu/shared-files/files")
foreach(model IN ITEMS phobos_g_296m_spc_obj_0000n00000_v004 deimos_g_167m_spc_obj_0000n00000_v002)
    string(REGEX MATCH "^[^_]+" moon "${model}")
    download_asset("moons/${model}.obj.zip" "${SBMT_MODELS}/${model}.obj.zip")
    set(moon_dir "${ASSET_DIR}/moons")
    set(archive "${moon_dir}/${model}.obj.zip")
    set(obj "${moon_dir}/${moon}.obj")
    if(EXISTS "${obj}" AND "${obj}" IS_NEWER_THAN "${archive}")
        continue()
    endif()
    message(STATUS "Extracting ${moon}.obj")
    file(ARCHIVE_EXTRACT INPUT "${archive}" DESTINATION "${moon_dir}" PATTERNS "${model}.obj" TOUCH)
    file(RENAME "${moon_dir}/${model}.obj" "${obj}")
endforeach()

foreach(map IN ITEMS phobos.jpg deimos.jpg io.png europa.jpg ganymede.jpg callisto.jpg
        mimas.jpg enceladus.jpg tethys.jpg dione.jpg rhea.jpg titan-clouds.jpg iapetus.jpg
        miranda.jpg ariel.jpg triton.jpg charon.jpg)

    download_asset("moons/${map}" "${MOON_MAPS}/hires/${map}")
endforeach()

foreach(map IN ITEMS umbriel.jpg titania.jpg oberon.jpg)
    download_asset("moons/${map}" "${MOON_MAPS}/medres/${map}")
endforeach()
