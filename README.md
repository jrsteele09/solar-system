# solar-cpp

A solar system viewer written in C++.

You can currently view; The Sun, The Earth and The Moon.


## Setup

You need CMake 3.24 or newer, a C++23 compiler, Git and an internet connection
for the first build.

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
./build/solar_cpp
```

### After moving the project

CMake build directories contain absolute paths and cannot be moved with the
source tree. If the project has been relocated, move the old build directory
aside and configure again:

```sh
mv build build-before-relocation
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

Choose a different backup name if `build-before-relocation` already exists.
The new build downloads its dependencies again; the old build directory can be
deleted once it is no longer needed. CMake's `--fresh` option alone is not
enough here because the downloaded dependencies also have CMake caches.

## Downloads

CMake downloads [GLM 1.0.2](https://github.com/g-truc/glm) and
[raylib 6.2](https://github.com/raysan5/raylib) when configuring the project.
During the build, it downloads any missing textures into `assets/textures/`:

- [Earth day map](https://www.solarsystemscope.com/textures/download/2k_earth_daymap.jpg)
  and [Moon map](https://www.solarsystemscope.com/textures/download/2k_moon.jpg)
  from Solar System Scope.
- [Earth at night](https://assets.science.nasa.gov/content/dam/science/esd/eo/images/imagerecords/144000/144898/BlackMarble_2016_01deg.jpg),
  NASA's Black Marble 2016 composite.

The build also downloads the [DE442s ephemeris](https://naif.jpl.nasa.gov/pub/naif/generic_kernels/spk/planets/de442s.bsp)
from NASA JPL into `assets/de442s.bsp`. It verifies the downloaded files and
reuses them on later builds. See [assets/README.md](assets/README.md) for details.
