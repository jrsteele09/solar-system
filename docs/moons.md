# Moons in the viewer

In **Fly to**, choose a system from **System**, then choose one of its moons from **Moons**. The camera follows its orbit. **Reset view** points the camera at the moon from the direction of its parent planet.

| Planet or dwarf planet | Moons |
|---|---|
| Earth | Moon |
| Mars | Phobos, Deimos |
| Jupiter | Io, Europa, Ganymede, Callisto |
| Saturn | Mimas, Enceladus, Tethys, Dione, Rhea, Titan, Iapetus |
| Uranus | Miranda, Ariel, Umbriel, Titania, Oberon |
| Neptune | Triton |
| Pluto | Charon |

## Positions and shapes

Earth and the Moon use DE442s. For Mars through Pluto, the viewer adds each body's displacement from its system barycentre to the DE442s position of that barycentre. The offsets for both the planet and its moons come from NASA/JPL's Cosmographia satellite kernel. It covers 1950 through 2049 and adds approximately 315 MiB to the first build. CSPICE reads the file from disk as needed. [NAIF Cosmographia kernel archive](https://naif.jpl.nasa.gov/pub/naif/cosmographia/kernels/spice/spk/)

Phobos and Deimos use spacecraft-derived shape models by Ernst et al. CMake downloads the versioned OBJ archives and extracts `phobos.obj` and `deimos.obj` into `assets/moons/`. Each mesh has 49,152 triangles. The ground spacing is 296 m for Phobos (v004) and 167 m for Deimos (v002). The viewer keeps the models’ kilometre scale and maps the existing textures by body-fixed longitude and latitude. The other added moons use the three principal radii from the IAU planetary constants kernel, which also supplies the moons’ orientations. [JHU/APL model downloads](https://sbmt.jhuapl.edu/shared-files/) · [Ernst et al.](https://doi.org/10.1186/s40623-023-01814-7) · [NAIF planetary constants](https://naif.jpl.nasa.gov/pub/naif/generic_kernels/pck/pck00011.tpc)

## Surface map credits

CMake downloads the moon maps from a pinned revision of CelestiaContent into `assets/moons/`. The viewer downsamples them to at most 2,048 pixels wide before uploading them. Titan shows its clouds. Some maps combine observations taken at different resolutions or fill gaps in spacecraft coverage. The source credits describe how each was made. [CelestiaContent credits](https://github.com/CelestiaProject/CelestiaContent/blob/1bbbf41ed4639a17fd9a884fd843e425caedd9a3/README) · [Upstream licensing and per-file notices](https://github.com/CelestiaProject/CelestiaContent/blob/1bbbf41ed4639a17fd9a884fd843e425caedd9a3/LICENSE.md)

| Maps | Contributors and upstream notices |
|---|---|
| Phobos and Deimos | Phil Stooke, NASA Planetary Data System. Phobos composite by Askaniy Anpilogov ([CC BY 3.0](https://creativecommons.org/licenses/by/3.0/)). Deimos is [CC0](https://creativecommons.org/publicdomain/zero/1.0/). |
| Io | ItzImcool, AstroChara, NASA/JPL-Caltech/ASI/USGS, NASA/JPL/SwRI/MSSS, Gerald Eichstädt, Jason Perry and John Rogers. [CC BY 4.0](https://creativecommons.org/licenses/by/4.0/) |
| Europa and Callisto | Europa by John van Vliet. Callisto by Askaniy Anpilogov from John van Vliet's map. |
| Ganymede | Askaniy Anpilogov, NASA/JPL-Caltech/ASI/USGS, Björn Jónsson, NASA/JPL-Caltech/SwRI/MSSS and Brian Swift. [CC BY 3.0](https://creativecommons.org/licenses/by/3.0/) |
| Mimas, Enceladus, Tethys, Dione, Rhea and Iapetus | FarGetaNik, using Paul Schenk's enhanced-colour maps. Rhea also uses Björn Jónsson's albedo map. |
| Titan's clouds | Gordan Ugarković, Kevin M. Gill, ShadowDragon777 and AstroChara. [CC BY 4.0](https://creativecommons.org/licenses/by/4.0/) |
| Miranda, Ariel, Umbriel, Titania and Oberon | ItzImcool, Paul Schenk, NASA/JPL and Ted Stryk. Umbriel also includes work by Phil Stooke. [CC BY-SA 4.0](https://creativecommons.org/licenses/by-sa/4.0/) |
| Triton | Askaniy Anpilogov and NASA/JPL-Caltech/ASI/USGS. [CC BY 3.0](https://creativecommons.org/licenses/by/3.0/) |
| Charon | Askaniy Anpilogov, with source maps credited to Steve Albers, USGS and Marc Buie. [CC BY-NC-SA 3.0](https://creativecommons.org/licenses/by-nc-sa/3.0/) |
