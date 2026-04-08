#include <NeForce/compress/zlib_compress.hpp>
#ifdef NEFORCE_SUPPORT_ZLIB
NEFORCE_BEGIN_NAMESPACE__

namespace {
    void check_zlib_error(const int ret_code) {
        if (ret_code != Z_OK) {
            const char* msg = nullptr;
            switch (ret_code) {
                case Z_MEM_ERROR:
                    msg = "Memory error";
                    break;
                case Z_BUF_ERROR:
                    msg = "Buffer error";
                    break;
                case Z_STREAM_ERROR:
                    msg = "Stream error";
                    break;
                case Z_DATA_ERROR:
                    msg = "Data error";
                    break;
                case Z_VERSION_ERROR:
                    msg = "Version mismatch";
                    break;
                default:
                    msg = "Unknown error";
                    break;
            }
            NEFORCE_THROW_EXCEPTION(zlib_exception(msg, zlib_exception::static_type, ret_code));
        }
    }
} // namespace


byte_vector zlib_compressor::compress_data(const byte_t* data, const size_t size, const compress_level level,
                                           const compress_strategy strategy, const compress_format format) {

    int window_bits = MAX_WBITS;
    switch (format) {
        case compress_format::gzip:
            window_bits = MAX_WBITS + 16;
            break;
        case compress_format::deflate:
            window_bits = -MAX_WBITS;
            break;
        case compress_format::zlib:
            window_bits = MAX_WBITS;
            break;
        default:
            unreachable();
    }

    ::uLongf compressed_size = ::compressBound(static_cast<::uLong>(size));
    byte_vector compressed(compressed_size);

    ::z_stream stream{};
    stream.next_in = const_cast<byte_t*>(data);
    stream.avail_in = static_cast<::uInt>(size);
    stream.next_out = compressed.data();
    stream.avail_out = static_cast<::uInt>(compressed_size);

    const int init_result = ::deflateInit2(&stream, static_cast<int>(level), Z_DEFLATED, window_bits, MAX_MEM_LEVEL,
                                           static_cast<int>(strategy));

    check_zlib_error(init_result);
    const int compress_result = ::deflate(&stream, Z_FINISH);

    if (compress_result != Z_STREAM_END) {
        ::deflateEnd(&stream);
        check_zlib_error(compress_result == Z_OK ? Z_BUF_ERROR : compress_result);
    }

    compressed_size = stream.total_out;

    const int end_result = ::deflateEnd(&stream);
    check_zlib_error(end_result);

    compressed.resize(compressed_size);
    return compressed;
}

byte_vector zlib_compressor::decompress_data(byte_t* data, const size_t size, size_t estimated_original_size,
                                             const compress_format format) {

    if (estimated_original_size == 0) {
        estimated_original_size = size * 4;
    }

    int window_bits = MAX_WBITS;
    switch (format) {
        case compress_format::gzip:
            window_bits = MAX_WBITS + 16;
            break;
        case compress_format::deflate:
            window_bits = -MAX_WBITS;
            break;
        case compress_format::zlib:
            window_bits = MAX_WBITS;
            break;
        default:
            unreachable();
    }

    ::z_stream stream{};
    stream.next_in = data;
    stream.avail_in = static_cast<::uInt>(size);

    const int init_result = ::inflateInit2(&stream, window_bits);
    check_zlib_error(init_result);

    byte_vector decompressed;
    decompressed.reserve(estimated_original_size);

    constexpr size_t chunk_size = 16384;
    int result = Z_OK;

    do {
        decompressed.resize(decompressed.size() + chunk_size);
        stream.next_out = decompressed.data() + decompressed.size() - chunk_size;
        stream.avail_out = chunk_size;

        result = ::inflate(&stream, Z_NO_FLUSH);

        if (result == Z_STREAM_ERROR || result == Z_NEED_DICT || result == Z_DATA_ERROR || result == Z_MEM_ERROR) {
            ::inflateEnd(&stream);
            check_zlib_error(result);
        }

        const size_t have = chunk_size - stream.avail_out;
        decompressed.resize(decompressed.size() - chunk_size + have);
    } while (result != Z_STREAM_END);

    const int end_result = ::inflateEnd(&stream);
    check_zlib_error(end_result);

    return decompressed;
}

zlib_compressor::stream_compressor::stream_compressor(const compress_level level, const compress_strategy strategy,
                                                      const compress_format format) {
    reset(level, strategy, format);
}

zlib_compressor::stream_compressor::~stream_compressor() {
    if (initialized_) {
        ::deflateEnd(&stream_);
    }
}

zlib_compressor::stream_compressor::stream_compressor(stream_compressor&& other) noexcept :
stream_(other.stream_),
initialized_(other.initialized_),
bytes_input_(other.bytes_input_),
bytes_output_(other.bytes_output_) {
    other.initialized_ = false;
    other.stream_ = {};
    other.bytes_input_ = 0;
    other.bytes_output_ = 0;
}

zlib_compressor::stream_compressor& zlib_compressor::stream_compressor::operator=(stream_compressor&& other) noexcept {
    if (this != &other) {
        if (initialized_) {
            ::deflateEnd(&stream_);
        }

        stream_ = other.stream_;
        initialized_ = other.initialized_;
        bytes_input_ = other.bytes_input_;
        bytes_output_ = other.bytes_output_;

        other.initialized_ = false;
        other.stream_ = {};
        other.bytes_input_ = 0;
        other.bytes_output_ = 0;
    }
    return *this;
}

void zlib_compressor::stream_compressor::reset(compress_level level, compress_strategy strategy,
                                               compress_format format) {

    if (initialized_) {
        ::deflateEnd(&stream_);
    }

    int window_bits = MAX_WBITS;
    switch (format) {
        case compress_format::gzip:
            window_bits = MAX_WBITS + 16;
            break;
        case compress_format::deflate:
            window_bits = -MAX_WBITS;
            break;
        case compress_format::zlib:
            window_bits = MAX_WBITS;
            break;
        default:
            unreachable();
    }

    stream_ = {};
    const int result = ::deflateInit2(&stream_, static_cast<int>(level), Z_DEFLATED, window_bits, MAX_MEM_LEVEL,
                                      static_cast<int>(strategy));

    check_zlib_error(result);
    initialized_ = true;
    bytes_input_ = 0;
    bytes_output_ = 0;
}

byte_vector zlib_compressor::stream_compressor::compress(const cbyte_view& data, const bool finish) {
    if (!initialized_) {
        NEFORCE_THROW_EXCEPTION(zlib_exception("Compressor not initialized"));
    }

    if (!data.empty()) {
        stream_.avail_in = static_cast<::uInt>(data.size());
        stream_.next_in = const_cast<byte_t*>(data.data());
        bytes_input_ += data.size();
    }

    constexpr size_t chunk_size = 16384;
    byte_vector output;
    output.reserve(chunk_size);

    const int flush = finish ? Z_FINISH : Z_NO_FLUSH;
    int result = Z_OK;

    do {
        output.resize(output.size() + chunk_size);
        stream_.avail_out = chunk_size;
        stream_.next_out = output.data() + output.size() - chunk_size;

        result = ::deflate(&stream_, flush);

        if (result == Z_STREAM_ERROR) {
            check_zlib_error(result);
        }

        const size_t have = chunk_size - stream_.avail_out;
        output.resize(output.size() - chunk_size + have);
        bytes_output_ += have;
    } while (stream_.avail_out == 0 || (finish && result != Z_STREAM_END && !data.empty()));

    return output;
}

byte_vector zlib_compressor::stream_compressor::compress(const string_view data, const bool finish) {
    return compress(cbyte_view(reinterpret_cast<const byte_t*>(data.data()), data.size()), finish);
}

byte_vector zlib_compressor::stream_compressor::finish() { return compress(cbyte_view{}, true); }

zlib_compressor::stream_decompressor::stream_decompressor(const compress_format format) { reset(format); }

zlib_compressor::stream_decompressor::~stream_decompressor() {
    if (initialized_) {
        ::inflateEnd(&stream_);
    }
}

zlib_compressor::stream_decompressor::stream_decompressor(stream_decompressor&& other) noexcept :
stream_(other.stream_),
initialized_(other.initialized_),
bytes_input_(other.bytes_input_),
bytes_output_(other.bytes_output_) {
    other.initialized_ = false;
    other.stream_ = {};
    other.bytes_input_ = 0;
    other.bytes_output_ = 0;
}

zlib_compressor::stream_decompressor&
zlib_compressor::stream_decompressor::operator=(stream_decompressor&& other) noexcept {
    if (this != &other) {
        if (initialized_) {
            ::inflateEnd(&stream_);
        }

        stream_ = other.stream_;
        initialized_ = other.initialized_;
        bytes_input_ = other.bytes_input_;
        bytes_output_ = other.bytes_output_;

        other.initialized_ = false;
        other.stream_ = {};
        other.bytes_input_ = 0;
        other.bytes_output_ = 0;
    }
    return *this;
}

void zlib_compressor::stream_decompressor::reset(const compress_format format) {
    if (initialized_) {
        ::inflateEnd(&stream_);
    }

    int window_bits = MAX_WBITS;
    switch (format) {
        case compress_format::gzip:
            window_bits = MAX_WBITS + 16;
            break;
        case compress_format::deflate:
            window_bits = -MAX_WBITS;
            break;
        case compress_format::zlib:
            window_bits = MAX_WBITS;
            break;
        default:
            unreachable();
    }

    stream_ = {};
    const int result = ::inflateInit2(&stream_, window_bits);
    check_zlib_error(result);
    initialized_ = true;
    bytes_input_ = 0;
    bytes_output_ = 0;
}

byte_vector zlib_compressor::stream_decompressor::decompress(const byte_view& data, const bool finish) {
    if (!initialized_) {
        NEFORCE_THROW_EXCEPTION(zlib_exception("Decompressor not initialized"));
    }

    if (!data.empty()) {
        stream_.avail_in = static_cast<::uInt>(data.size());
        stream_.next_in = data.data();
        bytes_input_ += data.size();
    }

    constexpr size_t chunk_size = 16384;
    byte_vector output;
    output.reserve(chunk_size);

    int result = Z_OK;
    do {
        output.resize(output.size() + chunk_size);
        stream_.avail_out = chunk_size;
        stream_.next_out = output.data() + output.size() - chunk_size;

        result = ::inflate(&stream_, finish ? Z_FINISH : Z_NO_FLUSH);

        if (result == Z_STREAM_ERROR || result == Z_NEED_DICT || result == Z_DATA_ERROR || result == Z_MEM_ERROR) {
            check_zlib_error(result);
        }

        const size_t have = chunk_size - stream_.avail_out;
        output.resize(output.size() - chunk_size + have);
        bytes_output_ += have;
    } while (stream_.avail_out == 0 || (finish && result != Z_STREAM_END && !data.empty()));

    return output;
}

byte_vector zlib_compressor::stream_decompressor::finish() { return decompress(byte_view{}, true); }

NEFORCE_END_NAMESPACE__
#endif
