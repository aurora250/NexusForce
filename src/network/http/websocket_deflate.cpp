#include <NeForce/network/http/websocket_deflate.hpp>
#include <NeForce/core/utility/packages.hpp>
NEFORCE_BEGIN_NAMESPACE__
NEFORCE_BEGIN_HTTP__

websocket_deflate_config websocket_deflate_config::negotiate(const string_view request_extensions) {
    websocket_deflate_config cfg;
    if (request_extensions.empty()) {
        return cfg;
    }
    const string lower = string(request_extensions).lowercase();
    const size_t pos = lower.find("permessage-deflate");
    if (pos == string::npos) {
        return cfg;
    }
    cfg.active = true;
    cfg.client_no_context_takeover = true;
    cfg.server_no_context_takeover = true;
    size_t semi = lower.find(',', pos);
    if (semi == string::npos) {
        semi = request_extensions.size();
    }
    const string_view params_section = request_extensions.view(pos, semi - pos);
    {
        const string params_lower = string(params_section).lowercase();
        const size_t p = params_lower.find("client_max_window_bits");
        if (p != string::npos) {
            size_t eq_pos = params_section.find('=', p);
            const size_t semi_pos = params_section.find(';', p);
            if (eq_pos != string::npos && (semi_pos == string::npos || eq_pos < semi_pos)) {
                ++eq_pos;
                while (eq_pos < params_section.size() &&
                       (params_section[eq_pos] == ' ' || params_section[eq_pos] == '\t')) {
                    ++eq_pos;
                }
                const size_t val_end = params_section.find_first_of(";, \t", eq_pos);
                const string_view val = (val_end == string::npos) ? params_section.view(eq_pos)
                                                                  : params_section.view(eq_pos, val_end - eq_pos);
                if (!val.empty()) {
                    try {
                        const int bits = static_cast<int>(uinteger64::parse(val).value());
                        if (bits >= 8 && bits <= 15) {
                            cfg.client_max_window_bits = bits;
                        }
                        // NOLINTNEXTLINE(bugprone-empty-catch)
                    } catch (...) {
                        // ignore
                    }
                }
            }
        }
    }
    {
        const string params_lower = string(params_section).lowercase();
        const size_t p = params_lower.find("server_max_window_bits");
        if (p != string::npos) {
            size_t eq_pos = params_section.find('=', p);
            const size_t semi_pos = params_section.find(';', p);
            if (eq_pos != string::npos && (semi_pos == string::npos || eq_pos < semi_pos)) {
                ++eq_pos;
                while (eq_pos < params_section.size() &&
                       (params_section[eq_pos] == ' ' || params_section[eq_pos] == '\t')) {
                    ++eq_pos;
                }
                const size_t val_end = params_section.find_first_of(";, \t", eq_pos);
                const string_view val = (val_end == string::npos) ? params_section.view(eq_pos)
                                                                  : params_section.view(eq_pos, val_end - eq_pos);
                if (!val.empty()) {
                    try {
                        const int bits = static_cast<int>(uinteger64::parse(val).value());
                        if (bits >= 8 && bits <= 15) {
                            cfg.server_max_window_bits = bits;
                        }
                        // NOLINTNEXTLINE(bugprone-empty-catch)
                    } catch (...) {
                        // ignore
                    }
                }
            }
        }
    }
    {
        const string params_lower = string(params_section).lowercase();
        if (params_lower.contains("client_no_context_takeover")) {
            cfg.client_no_context_takeover = true;
        }
    }
    {
        const string params_lower = string(params_section).lowercase();
        if (params_lower.contains("server_no_context_takeover")) {
            cfg.server_no_context_takeover = true;
        }
    }
    return cfg;
}

string websocket_deflate_config::to_response_header() const {
    if (!active) {
        return {};
    }
    string result = "permessage-deflate";
    if (client_max_window_bits != 15) {
        result += "; client_max_window_bits=" + to_string(client_max_window_bits);
    }
    if (server_max_window_bits != 15) {
        result += "; server_max_window_bits=" + to_string(server_max_window_bits);
    }
    if (client_no_context_takeover) {
        result += "; client_no_context_takeover";
    }
    if (server_no_context_takeover) {
        result += "; server_no_context_takeover";
    }
    return result;
}

#ifdef NEFORCE_SUPPORT_ZLIB

websocket_deflate::websocket_deflate(bool compress, int window_bits, bool no_context_takeover) :
compress_mode_(compress),
window_bits_(window_bits),
no_context_takeover_(no_context_takeover) {}

websocket_deflate::websocket_deflate(websocket_deflate&& other) noexcept :
compress_mode_(other.compress_mode_),
window_bits_(other.window_bits_),
no_context_takeover_(other.no_context_takeover_),
initialized_(other.initialized_),
compressor_(move(other.compressor_)),
decompressor_(move(other.decompressor_)) {
    other.initialized_ = false;
}

websocket_deflate& websocket_deflate::operator=(websocket_deflate&& other) noexcept {
    if (this != &other) {
        compress_mode_ = other.compress_mode_;
        window_bits_ = other.window_bits_;
        no_context_takeover_ = other.no_context_takeover_;
        initialized_ = other.initialized_;
        compressor_ = move(other.compressor_);
        decompressor_ = move(other.decompressor_);
        other.initialized_ = false;
    }
    return *this;
}

void websocket_deflate::ensure_initialized() {
    if (initialized_) {
        return;
    }
    if (compress_mode_) {
        compressor_ = make_unique<zlib_compressor::stream_compressor>(
                compress_level::default_level, compress_strategy::default_strategy, compress_format::deflate);
    } else {
        decompressor_ = make_unique<zlib_compressor::stream_decompressor>(compress_format::deflate);
    }
    initialized_ = true;
}

string websocket_deflate::process(const string_view data, bool is_final) {
    if (data.empty() && !is_final) {
        return {};
    }
    ensure_initialized();

    if (stream_finished_) {
        reset_context();
        stream_finished_ = false;
    }

    try {
        if (compress_mode_) {
            const auto cdata = cbyte_view(reinterpret_cast<const byte_t*>(data.data()), data.size());
            byte_vector result = compressor_->compress(cdata, is_final);
            if (is_final) {
                size_t rlen = result.size();
                // RFC 7692 §7.2.3.1: strip trailing empty DEFLATE block's LEN+NLEN.
                // zlib may or may not append an empty stored block with Z_FINISH.
                // If it didn't, we manually append one so the output always ends
                // with the BFINAL+BTYPE byte (0x01), which the decompressor
                // completes by appending 0x00 0x00 0xFF 0xFF.
                if (rlen < 4 || result[rlen - 4] != 0x00 || result[rlen - 3] != 0x00 || result[rlen - 2] != 0xFF ||
                    result[rlen - 1] != 0xFF) {
                    // zlib didn't append empty stored block → add one now
                    result.push_back(0x01); // BFINAL=1, BTYPE=00, padding=00000
                    result.push_back(0x00); // LEN lo
                    result.push_back(0x00); // LEN hi
                    result.push_back(0xFF); // NLEN lo
                    result.push_back(0xFF); // NLEN hi
                    rlen = result.size();
                }
                // Strip 4-byte LEN+NLEN trailer
#    ifdef NEFORCE_COMPILER_GCC
#        pragma GCC diagnostic push
#        pragma GCC diagnostic ignored "-Wstringop-overflow"
#    endif
                result.resize(rlen - 4);
#    ifdef NEFORCE_COMPILER_GCC
#        pragma GCC diagnostic pop
#    endif
                reset_context();
                stream_finished_ = true;
            }
            return {reinterpret_cast<const char*>(result.data()), result.size()};
        } else {
            const byte_vector decompressed = decompressor_->decompress(
                    byte_view(reinterpret_cast<byte_t*>(const_cast<char*>(data.data())), data.size()), is_final);
            if (is_final) {
                reset_context();
                stream_finished_ = true;
            }
            return {reinterpret_cast<const char*>(decompressed.data()), decompressed.size()};
        }
    } catch (...) {
        reset_context();
        stream_finished_ = false;
        return {data};
    }
}

void websocket_deflate::reset_context() {
    if (compress_mode_ && compressor_) {
        compressor_->reset(compress_level::default_level, compress_strategy::default_strategy,
                           compress_format::deflate);
    } else if (!compress_mode_ && decompressor_) {
        decompressor_->reset(compress_format::deflate);
    }
}

#endif // NEFORCE_SUPPORT_ZLIB

NEFORCE_END_HTTP__
NEFORCE_END_NAMESPACE__
