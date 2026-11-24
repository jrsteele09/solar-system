#include "download.hpp"

#include <curl/curl.h>
#include <exception>
#include <memory>
#include <stdexcept>
#include <utility>

namespace solar::http {
    namespace {
        class Runtime {
          public:
            Runtime() {
                const auto result = curl_global_init(CURL_GLOBAL_DEFAULT);
                if (result != CURLE_OK) {
                    throw std::runtime_error{curl_easy_strerror(result)};
                }
            }
            ~Runtime() { curl_global_cleanup(); }
        };

        struct Response {
            std::vector<unsigned char> bytes;
            std::size_t limit;
            std::exception_ptr error;
            bool limitExceeded = false;
        };

        size_t receive(char* data, size_t size, size_t count, void* context) {
            auto& response = *static_cast<Response*>(context);
            if (size != 0 && count > (response.limit - response.bytes.size()) / size) {
                response.limitExceeded = true;
                return 0;
            }
            const auto length = size * count;
            // Exceptions must not escape through libcurl's C callback.
            try {
                response.bytes.insert(response.bytes.end(), data, data + length);
            } catch (const std::exception&) {
                response.error = std::current_exception();
                return 0;
            }
            return length;
        }
    }

    std::vector<unsigned char> download(const std::string& url, std::size_t maxBytes) {
        if (url.empty() || maxBytes == 0) {
            throw std::invalid_argument{"HTTP download requires a URL and positive size limit"};
        }
        static const Runtime runtime;
        std::unique_ptr<CURL, decltype(&curl_easy_cleanup)> curl{curl_easy_init(), curl_easy_cleanup};
        if (!curl) {
            throw std::runtime_error{"Could not create HTTP client"};
        }
        auto option = [&](CURLoption key, auto value) {
            const auto result = curl_easy_setopt(curl.get(), key, value);
            if (result != CURLE_OK) {
                throw std::runtime_error{curl_easy_strerror(result)};
            }
        };
        Response response{{}, maxBytes, {}};
        option(CURLOPT_URL, url.c_str());
        option(CURLOPT_FOLLOWLOCATION, 1L);
        option(CURLOPT_MAXREDIRS, 5L);
        option(CURLOPT_PROTOCOLS_STR, "https");
        option(CURLOPT_REDIR_PROTOCOLS_STR, "https");
        option(CURLOPT_FAILONERROR, 1L);
        option(CURLOPT_CONNECTTIMEOUT, 5L);
        option(CURLOPT_TIMEOUT, 15L);
        option(CURLOPT_NOSIGNAL, 1L);
        option(CURLOPT_WRITEFUNCTION, &receive);
        option(CURLOPT_WRITEDATA, &response);
        const auto result = curl_easy_perform(curl.get());
        if (response.error) {
            std::rethrow_exception(response.error);
        }
        if (response.limitExceeded) {
            throw std::runtime_error{"HTTP response exceeds size limit: " + url};
        }
        if (result != CURLE_OK) {
            throw std::runtime_error{"HTTP download failed: " + url + ": " + curl_easy_strerror(result)};
        }
        return std::move(response.bytes);
    }
}
