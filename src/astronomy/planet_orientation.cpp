#include "planet_orientation.hpp"
#include "coordinates.hpp"
#include "time.hpp"

#include <SpiceUsr.h>
#include <cmath>
#include <glm/geometric.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <numbers>
#include <stdexcept>
#include <string>

namespace solar {

    // Keep Earth separate from the generic PCK models: this ERFA-derived
    // calculation includes precession and preserves the existing Earth orientation.
    // Derived from ERFA (see erfa.txt in this folder), itself derived from IAU SOFA:
    //  - eraEra00: Earth rotation angle (`rotation`)
    //  - eraGmst06: Greenwich mean sidereal time (`sidereal`, polynomial series)
    //  - eraPfw06: IAU 2006 precession angles gamma, phi, psi, epsilon
    // Changes from the originals: the C functions are merged into one function; the
    // precession matrix is composed with glm rotations instead of ERFA's rotation-matrix
    // helpers (eraRz/eraRx); and the result is a glm matrix in the scene's coordinate frame
    // for rendering. UTC stands in for UT1, and nutation and polar motion are omitted.
    static glm::mat4 earthOrientation(double utc) {
        const double days = (utc - j2000UnixSeconds) / secondsPerDay;
        const double t = terrestrialTime(utc) / (secondsPerDay * 36525.0);
        const double rotation =
            2 * pi * std::fmod(.7790572732640 + std::fmod(days, 1.0) + .00273781191135448 * days, 1.0);
        const double sidereal =
            rotation +
            (.014506 + (4612.156534 + (1.3915817 + (-.00000044 + (-.000029956 - .0000000368 * t) * t) * t) * t) * t) *
                arcseconds;
        // These angles account for the slow change in Earth’s axis over time.
        const double gamma =
            (-.052928 + (10.556378 + (.4932044 + (-.00031238 + (-.000002788 + .0000000260 * t) * t) * t) * t) * t) *
            arcseconds;
        const double phi =
            (84381.412819 + (-46.811016 + (.0511268 + (.00053289 + (-.000000440 - .0000000176 * t) * t) * t) * t) * t) *
            arcseconds;
        const double psi =
            (-.041775 + (5038.481484 + (1.5584175 + (-.00018522 + (-.000026452 - .0000000148 * t) * t) * t) * t) * t) *
            arcseconds;
        const double epsilon =
            (84381.406 + (-46.836769 + (-.0001831 + (.00200340 + (-.000000576 - .0000000434 * t) * t) * t) * t) * t) *
            arcseconds;
        const glm::dmat4 identity{1.0};
        const auto frame =
            glm::rotate(identity, gamma, glm::dvec3{0, 0, 1}) * glm::rotate(identity, phi, glm::dvec3{1, 0, 0}) *
            glm::rotate(identity, -psi, glm::dvec3{0, 0, 1}) * glm::rotate(identity, -epsilon, glm::dvec3{1, 0, 0});
        // Build the Earth’s local axes: Greenwich, north, and the third perpendicular axis.
        const glm::dvec3 x = toScene(glm::dvec3(frame * glm::dvec4{std::cos(sidereal), std::sin(sidereal), 0, 0}));
        const glm::dvec3 y = toScene(glm::dvec3(frame * glm::dvec4{0, 0, 1, 0}));
        const glm::dvec3 z = glm::cross(x, y);
        glm::mat4 result{1.0f};
        result[0] = glm::vec4(glm::vec3(x), 0);
        result[1] = glm::vec4(glm::vec3(y), 0);
        result[2] = glm::vec4(glm::vec3(z), 0);
        return result;
    }

    static void checkSpice() {
        if (failed_c()) {
            SpiceChar message[1841]{};
            getmsg_c("LONG", sizeof(message), message);
            reset_c();
            throw std::runtime_error{std::string{"SPICE orientation error: "} + message};
        }
    }

    // SPICE is NASA/JPL NAIF's observation-geometry toolkit. The text PCK
    // pck00011.tpc comes from https://naif.jpl.nasa.gov/pub/naif/generic_kernels/pck/.
    // It supplies IAU pole and prime-meridian models, including the Moon's
    // periodic terms. Load it once before evaluating body-fixed frames.
    void loadOrientationModels(const char* pckPath) {
        SpiceChar action[] = "RETURN";
        SpiceChar output[] = "NONE";
        erract_c("SET", 0, action);
        errprt_c("SET", 0, output);
        furnsh_c(pckPath);
        checkSpice();
    }

    glm::mat4 orientation(Body body, double utcSeconds) {
        if (body == Body::Earth) {
            return earthOrientation(utcSeconds);
        }

        const char* frame = nullptr;
        switch (body) {
        case Body::Mercury:
            frame = "IAU_MERCURY";
            break;
        case Body::Venus:
            frame = "IAU_VENUS";
            break;
        case Body::Moon:
            frame = "IAU_MOON";
            break;
        case Body::MarsSystem:
            frame = "IAU_MARS";
            break;
        case Body::JupiterSystem:
            frame = "IAU_JUPITER";
            break;
        case Body::SaturnSystem:
            frame = "IAU_SATURN";
            break;
        case Body::UranusSystem:
            frame = "IAU_URANUS";
            break;
        case Body::NeptuneSystem:
            frame = "IAU_NEPTUNE";
            break;
        case Body::PlutoSystem:
            frame = "IAU_PLUTO";
            break;
        case Body::Sun:
            frame = "IAU_SUN";
            break;
        case Body::Earth:
            return earthOrientation(utcSeconds);
        }
        if (!frame || !std::isfinite(utcSeconds)) {
            throw std::invalid_argument{"Invalid body or orientation time"};
        }

        // Evaluate the PCK body-fixed frame relative to J2000 at TDB seconds
        // past J2000. The position ephemeris uses the same time conversion.
        SpiceDouble rotation[3][3];
        pxform_c(frame, "J2000", ephemerisTime(utcSeconds), rotation);
        checkSpice();
        // Mesh +X is longitude zero, +Y north, and -Z is east.
        const auto axis = [&](int column) {
            return toScene({rotation[0][column], rotation[1][column], rotation[2][column]});
        };
        const auto x = axis(0), y = axis(2), z = -axis(1);
        glm::mat4 result{1.0f};
        result[0] = glm::vec4(glm::vec3(x), 0);
        result[1] = glm::vec4(glm::vec3(y), 0);
        result[2] = glm::vec4(glm::vec3(z), 0);
        return result;
    }
} // namespace solar
