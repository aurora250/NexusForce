#include "NeForce/compress/zstd_compress.hpp"
#include "NeForce/core/container/array.hpp"
#ifdef NEFORCE_HAVE_ZSTD
#    include <zstd.h>
#endif
NEFORCE_BEGIN_NAMESPACE__

#ifdef NEFORCE_HAVE_ZSTD
namespace {
    int clamp_level(const int level) noexcept { return level < -5 ? -5 : (level > 22 ? 22 : level); }
} // namespace
#endif


bool zstd_compressor::available() noexcept {
#ifdef NEFORCE_HAVE_ZSTD
    return true;
#else
    return false;
#endif
}

byte_vector zstd_compressor::compress_data(const byte_t* data, const size_t size, const int level) {
#ifdef NEFORCE_HAVE_ZSTD
    ::ZSTD_CCtx* context = ::ZSTD_createCCtx();
    if (context == nullptr) {
        NEFORCE_THROW_EXCEPTION(zstd_exception("zstd compression context allocation failed"));
    }
    byte_vector output(::ZSTD_compressBound(size));
    ::ZSTD_CCtx_setParameter(context, ::ZSTD_c_compressionLevel, clamp_level(level));
    ::ZSTD_CCtx_setParameter(context, ::ZSTD_c_checksumFlag, 1);
    const size_t written = ::ZSTD_compress2(context, output.data(), output.size(), data, size);
    ::ZSTD_freeCCtx(context);
    if (::ZSTD_isError(written) != 0U) {
        NEFORCE_THROW_EXCEPTION(zstd_exception("zstd compression failed"));
    }
    output.resize(written);
    return output;
#else
    (void) data;
    (void) size;
    (void) level;
    NEFORCE_THROW_EXCEPTION(zstd_exception("zstd is not linked in this build"));
#endif
}

bool zstd_compressor::decompress_prefix_data(const byte_t* data, const size_t size, string& output, bool* torn) {
    if (torn != nullptr) {
        *torn = false;
    }
#ifdef NEFORCE_HAVE_ZSTD
    output.clear();
    ::ZSTD_DStream* stream = ::ZSTD_createDStream();
    if (stream == nullptr) {
        return false;
    }
    if (::ZSTD_isError(::ZSTD_initDStream(stream)) != 0U) {
        ::ZSTD_freeDStream(stream);
        return false;
    }
    ::ZSTD_inBuffer input{data, size, 0};
    size_t result = 1;
    while (input.pos < input.size) {
        array<char, 16384> buffer{};
        ::ZSTD_outBuffer chunk{buffer.data(), buffer.size(), 0};
        result = ::ZSTD_decompressStream(stream, &chunk, &input);
        if (::ZSTD_isError(result) != 0U) {
            if (torn != nullptr) {
                *torn = true;
            }
            output.append(buffer.data(), chunk.pos);
            ::ZSTD_freeDStream(stream);
            return !output.empty();
        }
        output.append(buffer.data(), chunk.pos);
        if (result == 0 && input.pos < input.size) {
            if (::ZSTD_isError(::ZSTD_initDStream(stream)) != 0U) {
                break;
            }
        }
    }
    if (result != 0 && torn != nullptr) {
        *torn = true;
    }
    ::ZSTD_freeDStream(stream);
    return true;
#else
    (void) data;
    (void) size;
    output.clear();
    return false;
#endif
}

byte_vector zstd_compressor::decompress(const cbyte_view& data, const size_t estimated_original_size) {
    (void) estimated_original_size;
    string decoded;
    bool torn = false;
    if (!decompress_prefix_data(data.data(), data.size(), decoded, &torn)) {
        NEFORCE_THROW_EXCEPTION(zstd_exception("zstd decompression failed"));
    }
    if (torn) {
        NEFORCE_THROW_EXCEPTION(zstd_exception("zstd frame is truncated"));
    }
    byte_vector output(decoded.size());
    for (size_t i = 0; i < decoded.size(); ++i) {
        output[i] = static_cast<byte_t>(decoded[i]);
    }
    return output;
}

bool zstd_compressor::decompress_prefix(const string_view data, string& output, bool* torn) {
    return decompress_prefix_data(reinterpret_cast<const byte_t*>(data.data()), data.size(), output, torn);
}

size_t zstd_compressor::frame_count(const string_view data) noexcept {
#ifdef NEFORCE_HAVE_ZSTD
    size_t count = 0;
    size_t offset = 0;
    while (offset < data.size()) {
        const size_t frame_size = ::ZSTD_findFrameCompressedSize(data.data() + offset, data.size() - offset);
        if (::ZSTD_isError(frame_size) != 0U || frame_size == 0) {
            break;
        }
        ++count;
        offset += frame_size;
    }
    return count;
#else
    (void) data;
    return 0;
#endif
}

NEFORCE_END_NAMESPACE__
