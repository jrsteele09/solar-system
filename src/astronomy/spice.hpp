#pragma once

#include <SpiceUsr.h>
#include <stdexcept>
#include <string>

namespace solar {
    inline void configureSpiceErrors() {
        SpiceChar action[] = "RETURN";
        SpiceChar output[] = "NONE";
        erract_c("SET", 0, action);
        errprt_c("SET", 0, output);
    }

    inline void checkSpice() {
        if (failed_c()) {
            SpiceChar message[1841]{};
            getmsg_c("LONG", sizeof(message), message);
            reset_c();
            throw std::runtime_error{std::string{"SPICE error: "} + message};
        }
    }
} // namespace solar
