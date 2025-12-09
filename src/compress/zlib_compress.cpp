#include <MSTL/compress/zlib_compress.hpp>
MSTL_BEGIN_NAMESPACE__

void zlib_compressor::check_zlib_error(const int ret_code) {
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
        throw_exception(zlib_exception(msg, ret_code));
    }
}


bvector zlib_compressor::compress_data(
    const byte_t* data, const size_t size,
    COMPRESS_LEVEL level, COMPRESS_STRATEGY strategy) {

    ::uLongf compressed_size = ::compressBound(static_cast<::uLong>(size));
    bvector compressed(compressed_size);

    const int result = ::compress2(
        compressed.data(), &compressed_size,
        data, static_cast<::uLong>(size),
        static_cast<int>(level)
    );
    
    check_zlib_error(result);
    compressed.resize(compressed_size);
    return compressed;
}

bvector zlib_compressor::decompress_data(const byte_t* data,
    const size_t size, size_t estimated_original_size) {

    if (estimated_original_size == 0) {
        estimated_original_size = size * 4;
    }
    
    bvector decompressed(estimated_original_size);
    auto decompressed_size = static_cast<::uLongf>(decompressed.size());
    
    int result = ::uncompress(
        decompressed.data(), &decompressed_size,
        data, static_cast<::uLong>(size)
    );

    if (result == Z_BUF_ERROR) {
        decompressed.resize(decompressed.size() * 2);
        decompressed_size = static_cast<::uLongf>(decompressed.size());
        result = ::uncompress(
            decompressed.data(), &decompressed_size,
            data, static_cast<::uLong>(size)
        );
    }
    
    check_zlib_error(result);
    decompressed.resize(decompressed_size);
    return decompressed;
}

zlib_compressor::stream_compressor::stream_compressor(
    const COMPRESS_LEVEL level, const COMPRESS_STRATEGY strategy) {
    reset(level, strategy);
}

zlib_compressor::stream_compressor::~stream_compressor() {
    if (initialized_) {
        ::deflateEnd(&stream_);
    }
}

zlib_compressor::stream_compressor::stream_compressor(
    stream_compressor&& other) noexcept
    : stream_(other.stream_)
    , initialized_(other.initialized_) {
    
    other.initialized_ = false;
    other.stream_ = {};
}

zlib_compressor::stream_compressor& zlib_compressor::stream_compressor::operator=(
    stream_compressor&& other) noexcept {
    
    if (this != &other) {
        if (initialized_) {
            ::deflateEnd(&stream_);
        }
        
        stream_ = other.stream_;
        initialized_ = other.initialized_;
        
        other.initialized_ = false;
        other.stream_ = {};
    }
    return *this;
}

void zlib_compressor::stream_compressor::reset(
    COMPRESS_LEVEL level, COMPRESS_STRATEGY strategy) {
    
    if (initialized_) {
        ::deflateEnd(&stream_);
    }
    
    stream_ = {};
    const int result = ::deflateInit2(&stream_,
        static_cast<int>(level),
        Z_DEFLATED,
        MAX_WBITS,
        MAX_MEM_LEVEL,
        static_cast<int>(strategy));
    
    check_zlib_error(result);
    initialized_ = true;
}

bvector zlib_compressor::stream_compressor::compress(
    const span<const byte_t> data, const bool finish) {
    
    if (!initialized_) {
        throw_exception(zlib_exception("Compressor not initialized"));
    }
    
    stream_.avail_in = static_cast<::uInt>(data.size());
    stream_.next_in = const_cast<byte_t*>(data.data());
    
    constexpr size_t CHUNK_SIZE = 16384;
    bvector output;
    output.reserve(CHUNK_SIZE);
    
    do {
        output.resize(output.size() + CHUNK_SIZE);
        stream_.avail_out = CHUNK_SIZE;
        stream_.next_out = output.data() + output.size() - CHUNK_SIZE;
        
        const int flush = finish ? Z_FINISH : Z_NO_FLUSH;
        const int result = ::deflate(&stream_, flush);
        
        if (result == Z_STREAM_ERROR) {
            check_zlib_error(result);
        }

        const size_t have = CHUNK_SIZE - stream_.avail_out;
        output.resize(output.size() - CHUNK_SIZE + have);
        
    } while (stream_.avail_out == 0);
    
    return output;
}

bvector zlib_compressor::stream_compressor::compress(
    const string_view data, const bool finish) {
    
    return compress(span<const byte_t>(
        reinterpret_cast<const byte_t*>(data.data()),
        data.size()
    ), finish);
}

bvector zlib_compressor::stream_compressor::finish() {
    return compress(span<const byte_t>{}, true);
}

zlib_compressor::stream_decompressor::stream_decompressor() {
    reset();
}

zlib_compressor::stream_decompressor::~stream_decompressor() {
    if (initialized_) {
        ::inflateEnd(&stream_);
    }
}

zlib_compressor::stream_decompressor::stream_decompressor(
    stream_decompressor&& other) noexcept
    : stream_(other.stream_)
    , initialized_(other.initialized_) {
    
    other.initialized_ = false;
    other.stream_ = {};
}

zlib_compressor::stream_decompressor& zlib_compressor::stream_decompressor::operator=(
    stream_decompressor&& other) noexcept {
    
    if (this != &other) {
        if (initialized_) {
            ::inflateEnd(&stream_);
        }
        
        stream_ = other.stream_;
        initialized_ = other.initialized_;
        
        other.initialized_ = false;
        other.stream_ = {};
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
}

bvector zlib_compressor::stream_decompressor::decompress(
    const span<const byte_t> data, const bool finish) {
    
    if (!initialized_) {
        throw_exception(zlib_exception("Decompressor not initialized"));
    }
    
    stream_.avail_in = static_cast<::uInt>(data.size());
    stream_.next_in = const_cast<byte_t*>(data.data());
    
    constexpr size_t CHUNK_SIZE = 16384;
    bvector output;
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
        
    } while (stream_.avail_out == 0);
    return output;
}

bvector zlib_compressor::stream_decompressor::finish() {
    return bvector{};
}

MSTL_END_NAMESPACE__
