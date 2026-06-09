#include <NeForce/network/http/http_compress.hpp>
#ifdef NEFORCE_SUPPORT_ZLIB
NEFORCE_BEGIN_NAMESPACE__
NEFORCE_BEGIN_HTTP__

namespace {
    constexpr string_view skip_prefixes[] = {
            "image/",
            "video/",
            "audio/",
            "application/zip",
            "application/gzip",
            "application/x-gzip",
            "application/x-compress",
            "application/x-compressed",
            "application/x-7z-compressed",
            "application/x-rar-compressed",
            "application/x-tar",
            "application/x-bzip2",
            "application/octet-stream",
    };

    bool is_compressible(const string_view content_type) {
        if (content_type.empty()) {
            return true;
        }

        for (const auto& prefix: skip_prefixes) {
            if (content_type.starts_with(prefix)) {
                return false;
            }
        }
        return true;
    }

    string_view select_encoding(const string_view accept_encoding) {
        if (accept_encoding.empty()) {
            return {};
        }

        bool has_gzip = false;
        bool has_deflate = false;

        size_t pos = 0;
        while (pos < accept_encoding.size()) {
            size_t comma = accept_encoding.find(',', pos);
            if (comma == string_view::npos) {
                comma = accept_encoding.size();
            }
            string_view encoding = accept_encoding.view(pos, comma - pos).trim();

            // Parse quality value q=N (RFC 7231 §5.3.1)
            double q_value = 1.0;
            size_t semi = encoding.find(';');
            if (semi != string_view::npos) {
                string_view params = encoding.view(semi + 1);
                encoding = encoding.head(semi).trim();

                // Find q= parameter
                size_t q_pos = params.find("q=");
                if (q_pos != string_view::npos) {
                    string_view q_str = params.view(q_pos + 2).trim();
                    size_t q_end = q_str.find(';');
                    if (q_end != string_view::npos) {
                        q_str = q_str.head(q_end);
                    }
                    if (!q_str.empty()) {
                        try {
                            q_value = float64::parse(q_str).value();
                            // NOLINTNEXTLINE(bugprone-empty-catch)
                        } catch (...) {
                            q_value = 0.0;
                        }
                    }
                }
            }

            if (q_value == 0.0) {
                pos = comma + 1;
                continue;
            }

            const string enc_lower = string(encoding).lowercase();
            if (enc_lower == "gzip") {
                has_gzip = true;
            } else if (enc_lower == "deflate") {
                has_deflate = true;
            } else if (enc_lower == "*" || enc_lower == "identity") {
                // * matches everything, but gzip has priority
            }

            pos = comma + 1;
        }

        if (has_gzip) {
            return "gzip";
        }
        if (has_deflate) {
            return "deflate";
        }
        return {};
    }
} // namespace


void compress_filter::post_filter(http_request& request, http_response& response) {
    if (!enabled) {
        return;
    }

    if (request.method.is_head()) {
        return;
    }

    {
        using UT = underlying_type_t<http_status>;
        const int code = static_cast<UT>(response.status);
        if (code < 200 || code >= 300) {
            return;
        }
    }

    if (response.body.size() < min_size.bytes()) {
        return;
    }

    const auto content_type = response.header(http_key::Content_Type());
    if (!is_compressible(content_type)) {
        return;
    }

    const auto accept_encoding = request.header("Accept-Encoding");
    const auto encoding = select_encoding(accept_encoding);
    if (encoding.empty()) {
        return;
    }

    try {
        const auto format = (encoding == "gzip") ? compress_format::gzip : compress_format::deflate;
        zlib_compressor::stream_compressor compressor(compress_level::default_level,
                                                      compress_strategy::default_strategy, format);
        const byte_vector compressed = compressor.compress(response.body.view(), true);
        response.body = string(reinterpret_cast<const char*>(compressed.data()), compressed.size());
        response.set_header("Content-Encoding", string(encoding));
        response.set_header(http_key::Content_Length(), to_string(response.body.size()));

        const auto vary = response.header("Vary");
        if (vary.empty()) {
            response.set_header("Vary", "Accept-Encoding");
        } else if (!string(vary).contains("Accept-Encoding")) {
            response.set_header("Vary", string(vary) + ", Accept-Encoding");
        }
        // NOLINTNEXTLINE(bugprone-empty-catch)
    } catch (...) {
        // ignore
    }
}

NEFORCE_END_HTTP__
NEFORCE_END_NAMESPACE__
#endif
