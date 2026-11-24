#pragma once

#include <cstddef>
#include <string>
#include <vector>

namespace solar::http {
    // HTTPS GET with redirects, timeouts and a response size limit. Throws on failure.
    std::vector<unsigned char> download(const std::string& url, std::size_t maxBytes);
}
