#include <MSTL/compress/zlib_compress.hpp>
#ifdef MSTL_SUPPORT_ZLIB__
MSTL_BEGIN_NAMESPACE__

static void check_zlib_error(const int ret_code) {
    if (ret_code != Z_OK) {
        const char* msg;
        switch (ret_code) {
            case Z_MEM_ERROR: msg = "Memory error"; break;
            case Z_BUF_ERROR: msg = "Buffer error"; break;
            case Z_STREAM_ERROR: msg = "Stream error"; break;
            case Z_DATA_ERROR: msg = "Data error"; break;
            case Z_VERSION_ERROR: msg = "Version mismatch"; break;
            default: msg = "Unknown error"; break;
        }
        throw_exception(zlib_exception(msg, zlib_exception::static_type, ret_code));
    }
}

byte_vector zlib_compressor::compress_data(const byte_t* data, const size_t size,
                                           compress_level level, compress_strategy strategy) {
    ::uLongf compressed_size = ::compressBound(static_cast<::uLong>(size));
    byte_vector compressed(compressed_size);

    ::z_stream stream{};
    stream.next_in = const_cast<byte_t*>(data);
    stream.avail_in = static_cast<::uInt>(size);
    stream.next_out = compressed.data();
    stream.avail_out = static_cast<::uInt>(compressed_size);

    const int init_result = ::deflateInit2(&stream,
        static_cast<int>(level),
        Z_DEFLATED,
        MAX_WBITS,
        MAX_MEM_LEVEL,
        static_cast<int>(strategy)
    );

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

byte_vector zlib_compressor::decompress_data(const byte_t* data, const size_t size, size_t estimated_original_size) {
    if (estimated_original_size == 0) {
        estimated_original_size = size * 4;
    }
    
    byte_vector decompressed(estimated_original_size);
    ::uLongf decompressed_size = 0;
    int result = Z_BUF_ERROR;
    int attempt = 0;
    constexpr int MAX_ATTEMPTS = 5;
    constexpr size_t MAX_BUFFER_SIZE = 1024 * 1024 * 1024;
    
    do {
        if (attempt > 0) {
            estimated_original_size *= 2;
            if (estimated_original_size > MAX_BUFFER_SIZE) {
                throw_exception(zlib_exception("Decompression buffer size exceeded maximum limit"));
            }
        }

        decompressed.resize(estimated_original_size);
        decompressed_size = static_cast<::uLongf>(decompressed.size());

        result = ::uncompress(
            decompressed.data(), &decompressed_size,
            data, static_cast<::uLong>(size)
        );

        if (result == Z_OK) {
            decompressed.resize(decompressed_size);
            break;
        } else if (result == Z_BUF_ERROR) {
            attempt++;
            if (attempt >= MAX_ATTEMPTS) {
                throw_exception(zlib_exception("Exceeded maximum decompression buffer attempts"));
            }
        } else {
            check_zlib_error(result);
        }
    } while (result == Z_BUF_ERROR);

    return decompressed;
}

zlib_compressor::stream_compressor::stream_compressor(const compress_level level, const compress_strategy strategy) {
    reset(level, strategy);
}

zlib_compressor::stream_compressor::~stream_compressor() {
    if (initialized_) {
        ::deflateEnd(&stream_);
    }
}

zlib_compressor::stream_compressor::stream_compressor(stream_compressor&& other) noexcept
: stream_(other.stream_), initialized_(other.initialized_),
  bytes_input_(other.bytes_input_), bytes_output_(other.bytes_output_) {
    other.initialized_ = false;
    other.stream_ = {};
    other.bytes_input_ = 0;
    other.bytes_output_ = 0;
}

zlib_compressor::stream_compressor&
zlib_compressor::stream_compressor::operator =(stream_compressor&& other) noexcept {
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

void zlib_compressor::stream_compressor::reset(compress_level level, compress_strategy strategy) {
    if (initialized_) {
        ::deflateEnd(&stream_);
    }
    
    stream_ = {};
    const int result = ::deflateInit2(
        &stream_,
        static_cast<int>(level),
        Z_DEFLATED,
        MAX_WBITS,
        MAX_MEM_LEVEL,
        static_cast<int>(strategy));
    
    check_zlib_error(result);
    initialized_ = true;
    bytes_input_ = 0;
    bytes_output_ = 0;
}

byte_vector zlib_compressor::stream_compressor::compress(const cbyte_view& data, const bool finish) {
    if (!initialized_) {
        throw_exception(zlib_exception("Compressor not initialized"));
    }

    if (data.size() > 0) {
        stream_.avail_in = static_cast<::uInt>(data.size());
        stream_.next_in = const_cast<byte_t*>(data.data());
        bytes_input_ += data.size();
    }

    constexpr size_t CHUNK_SIZE = 16384;
    byte_vector output;
    output.reserve(CHUNK_SIZE);

    const int flush = finish ? Z_FINISH : Z_NO_FLUSH;
    int result = Z_OK;

    do {
        output.resize(output.size() + CHUNK_SIZE);
        stream_.avail_out = CHUNK_SIZE;
        stream_.next_out = output.data() + output.size() - CHUNK_SIZE;

        result = ::deflate(&stream_, flush);

        if (result == Z_STREAM_ERROR) {
            check_zlib_error(result);
        }

        const size_t have = CHUNK_SIZE - stream_.avail_out;
        output.resize(output.size() - CHUNK_SIZE + have);
        bytes_output_ += have;
    } while (stream_.avail_out == 0 || (finish && result != Z_STREAM_END && data.size() > 0));
    
    return output;
}

byte_vector zlib_compressor::stream_compressor::compress(const string_view data, const bool finish) {
    return compress(cbyte_view(reinterpret_cast<const byte_t*>(data.data()), data.size()), finish);
}

byte_vector zlib_compressor::stream_compressor::finish() {
    return compress(cbyte_view{}, true);
}

zlib_compressor::stream_decompressor::stream_decompressor() {
    reset();
}

zlib_compressor::stream_decompressor::~stream_decompressor() {
    if (initialized_) {
        ::inflateEnd(&stream_);
    }
}

zlib_compressor::stream_decompressor::stream_decompressor(stream_decompressor&& other) noexcept
: stream_(other.stream_), initialized_(other.initialized_),
  bytes_input_(other.bytes_input_), bytes_output_(other.bytes_output_) {
    other.initialized_ = false;
    other.stream_ = {};
    other.bytes_input_ = 0;
    other.bytes_output_ = 0;
}

zlib_compressor::stream_decompressor&
zlib_compressor::stream_decompressor::operator =(stream_decompressor&& other) noexcept {
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

void zlib_compressor::stream_decompressor::reset() {
    if (initialized_) {
        ::inflateEnd(&stream_);
    }
    
    stream_ = {};
    const int result = ::inflateInit2(&stream_, MAX_WBITS);
    check_zlib_error(result);
    initialized_ = true;
    bytes_input_ = 0;
    bytes_output_ = 0;
}

byte_vector zlib_compressor::stream_decompressor::decompress(const cbyte_view& data, const bool finish) {
    if (!initialized_) {
        throw_exception(zlib_exception("Decompressor not initialized"));
    }

    if (data.size() > 0) {
        stream_.avail_in = static_cast<::uInt>(data.size());
        stream_.next_in = const_cast<byte_t*>(data.data());
        bytes_input_ += data.size();
    }

    constexpr size_t CHUNK_SIZE = 16384;
    byte_vector output;
    output.reserve(CHUNK_SIZE);

    int result = Z_OK;
    do {
        output.resize(output.size() + CHUNK_SIZE);
        stream_.avail_out = CHUNK_SIZE;
        stream_.next_out = output.data() + output.size() - CHUNK_SIZE;

        result = ::inflate(&stream_, finish ? Z_FINISH : Z_NO_FLUSH);

        if (result == Z_STREAM_ERROR || result == Z_NEED_DICT ||
            result == Z_DATA_ERROR || result == Z_MEM_ERROR) {
            check_zlib_error(result);
        }

        const size_t have = CHUNK_SIZE - stream_.avail_out;
        output.resize(output.size() - CHUNK_SIZE + have);
        bytes_output_ += have;
    } while (stream_.avail_out == 0 || (finish && result != Z_STREAM_END && data.size() > 0));

    return output;
}

byte_vector zlib_compressor::stream_decompressor::finish() {
    return decompress(cbyte_view{}, true);
}

MSTL_END_NAMESPACE__
#endif
