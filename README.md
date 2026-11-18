# Solar System

A solar system viewer written in C++.

You can currently view the Sun, Earth and Moon.


## Setup

You need CMake 3.24 or newer, a C++23 compiler, Git and an internet connection
for the first build.

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
./build/solar_system
```

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

## Architecture

The application sets up the scene and runs the update and draw loop. The scene
uses ephemeris data and body orientations to update the Sun, Earth and Moon;
the controls panel and camera handle navigation.

```mermaid
flowchart TD
    App[Application and main loop] --> Scene
    App --> Controls[Controls panel]
    App --> Camera
    Controls --> Scene
    Controls --> Camera
    Scene --> Astronomy[Astronomy: positions and orientations]
    Astronomy --> Ephemeris[DE442s ephemeris]
    Scene --> Camera
    Scene --> Bodies[Celestial bodies: Sun, Earth and Moon]
    Bodies --> Meshes[Sphere meshes]
    Bodies --> Textures[Surface textures]
    Bodies --> Shaders[Sunlight and glow shaders]
    Scene --> Shaders
    Camera --> Raylib[raylib: window, input and rendering]
    Controls --> Raylib
    Meshes --> Raylib
    Textures --> Raylib
    Shaders --> Raylib
    Astronomy --> GLM[GLM: mathematics]
    Camera --> GLM
```
