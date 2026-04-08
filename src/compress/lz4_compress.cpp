#include <NeForce/compress/lz4_compress.hpp>
#ifdef NEFORCE_SUPPORT_LZ4
#    include <lz4hc.h>
NEFORCE_BEGIN_NAMESPACE__

byte_vector lz4_compressor::compress_data(const byte_t* data, size_t size, int level) {
    int max_compressed_size = ::LZ4_compressBound(static_cast<int>(size));
    if (max_compressed_size <= 0) {
        NEFORCE_THROW_EXCEPTION(lz4_exception("Input size too large for LZ4 compression"));
    }

    byte_vector compressed(max_compressed_size);
    int compressed_size = 0;

    if (level >= 1 && level <= 12) {
        compressed_size =
                ::LZ4_compress_HC(reinterpret_cast<const char*>(data), reinterpret_cast<char*>(compressed.data()),
                                  static_cast<int>(size), max_compressed_size, level);
    } else {
        const int acceleration = (level > 12) ? (21 - level) : 1;
        compressed_size =
                ::LZ4_compress_fast(reinterpret_cast<const char*>(data), reinterpret_cast<char*>(compressed.data()),
                                    static_cast<int>(size), max_compressed_size, acceleration > 0 ? acceleration : 1);
    }

    if (compressed_size <= 0) {
        NEFORCE_THROW_EXCEPTION(lz4_exception("LZ4 compression failed"));
    }

    compressed.resize(compressed_size);
    return compressed;
}

byte_vector lz4_compressor::decompress_data(const byte_t* data, const size_t size, size_t estimated_original_size) {
    if (estimated_original_size == 0) {
        estimated_original_size = size * 4;
    }

    byte_vector decompressed(estimated_original_size);
    int result = 0;
    int attempt = 0;
    constexpr int max_attempts = 5;
    constexpr size_t max_buffer_size{1024ULL * 1024 * 1024};

    do {
        if (attempt > 0) {
            estimated_original_size *= 2;
            if (estimated_original_size > max_buffer_size) {
                NEFORCE_THROW_EXCEPTION(lz4_exception("Decompression buffer size exceeded maximum limit"));
            }
            decompressed.resize(estimated_original_size);
        }

        result =
                ::LZ4_decompress_safe(reinterpret_cast<const char*>(data), reinterpret_cast<char*>(decompressed.data()),
                                      static_cast<int>(size), static_cast<int>(decompressed.size()));

        if (result > 0) {
            decompressed.resize(result);
            break;
        }
        if (result == 0) {
            attempt++;
            if (attempt >= max_attempts) {
                NEFORCE_THROW_EXCEPTION(lz4_exception("Exceeded maximum decompression buffer attempts"));
            }
        } else {
            NEFORCE_THROW_EXCEPTION(lz4_exception("LZ4 decompression failed"));
        }
    } while (result <= 0);

    return decompressed;
}

lz4_compressor::stream_compressor::stream_compressor(const int level) { reset(level); }

lz4_compressor::stream_compressor::~stream_compressor() {
    if (stream_ != nullptr) {
        ::LZ4_freeStream(stream_);
    }
}

lz4_compressor::stream_compressor::stream_compressor(stream_compressor&& other) noexcept :
stream_(other.stream_),
use_hc_(other.use_hc_),
level_(other.level_),
bytes_input_(other.bytes_input_),
bytes_output_(other.bytes_output_) {
    other.stream_ = nullptr;
    other.bytes_input_ = 0;
    other.bytes_output_ = 0;
}

lz4_compressor::stream_compressor& lz4_compressor::stream_compressor::operator=(stream_compressor&& other) noexcept {
    if (this != &other) {
        if (stream_ != nullptr) {
            ::LZ4_freeStream(stream_);
        }

        stream_ = other.stream_;
        use_hc_ = other.use_hc_;
        level_ = other.level_;
        bytes_input_ = other.bytes_input_;
        bytes_output_ = other.bytes_output_;

        other.stream_ = nullptr;
        other.bytes_input_ = 0;
        other.bytes_output_ = 0;
    }
    return *this;
}

void lz4_compressor::stream_compressor::reset(const int level) {
    if (stream_ != nullptr) {
        ::LZ4_freeStream(stream_);
    }

    stream_ = ::LZ4_createStream();
    if (stream_ == nullptr) {
        NEFORCE_THROW_EXCEPTION(lz4_exception("Failed to create LZ4 stream"));
    }

    use_hc_ = (level >= 1 && level <= 12);
    level_ = level;
    bytes_input_ = 0;
    bytes_output_ = 0;
}

byte_vector lz4_compressor::stream_compressor::compress(const cbyte_view& data, const bool finish) {
    if (stream_ == nullptr) {
        NEFORCE_THROW_EXCEPTION(lz4_exception("Compressor not initialized"));
    }
    if (finished_) {
        NEFORCE_THROW_EXCEPTION(lz4_exception("Compressor already finished"));
    }

    if (data.size() > block_size) {
        NEFORCE_THROW_EXCEPTION(lz4_exception("Input block size exceeds maximum (64KB)"));
    }

    byte_vector output;

    if (!data.empty()) {
        const int max_compressed_size = ::LZ4_compressBound(static_cast<int>(data.size()));
        output.resize(max_compressed_size);

        int compressed_size = 0;
        if (use_hc_) {
            compressed_size = ::LZ4_compress_HC(reinterpret_cast<const char*>(data.data()),
                                                reinterpret_cast<char*>(output.data()), static_cast<int>(data.size()),
                                                max_compressed_size, level_);
        } else {
            const int acceleration = (level_ > 12) ? (21 - level_) : 1;
            compressed_size = ::LZ4_compress_fast_continue(
                    stream_, reinterpret_cast<const char*>(data.data()), reinterpret_cast<char*>(output.data()),
                    static_cast<int>(data.size()), max_compressed_size, acceleration > 0 ? acceleration : 1);
        }

        if (compressed_size <= 0) {
            NEFORCE_THROW_EXCEPTION(lz4_exception("LZ4 stream compression failed"));
        }

        output.resize(compressed_size);
        bytes_input_ += data.size();
        bytes_output_ += compressed_size;
    }

    if (finish) {
        finished_ = true;
    }

    return output;
}

byte_vector lz4_compressor::stream_compressor::compress(const string_view data, const bool finish) {
    return compress(cbyte_view(reinterpret_cast<const byte_t*>(data.data()), data.size()), finish);
}

byte_vector lz4_compressor::stream_compressor::finish() {
    if (stream_ == nullptr) {
        NEFORCE_THROW_EXCEPTION(lz4_exception("Compressor not initialized"));
    }

    finished_ = true;
    return byte_vector{};
}

lz4_compressor::stream_decompressor::stream_decompressor() { reset(); }

lz4_compressor::stream_decompressor::~stream_decompressor() {
    if (stream_ != nullptr) {
        ::LZ4_freeStreamDecode(stream_);
    }
}

lz4_compressor::stream_decompressor::stream_decompressor(stream_decompressor&& other) noexcept :
stream_(other.stream_),
bytes_input_(other.bytes_input_),
bytes_output_(other.bytes_output_) {
    other.stream_ = nullptr;
    other.bytes_input_ = 0;
    other.bytes_output_ = 0;
}

lz4_compressor::stream_decompressor&
lz4_compressor::stream_decompressor::operator=(stream_decompressor&& other) noexcept {
    if (this != &other) {
        if (stream_ != nullptr) {
            ::LZ4_freeStreamDecode(stream_);
        }

        stream_ = other.stream_;
        bytes_input_ = other.bytes_input_;
        bytes_output_ = other.bytes_output_;

        other.stream_ = nullptr;
        other.bytes_input_ = 0;
        other.bytes_output_ = 0;
    }
    return *this;
}

void lz4_compressor::stream_decompressor::reset() {
    if (stream_ != nullptr) {
        ::LZ4_freeStreamDecode(stream_);
    }

    stream_ = ::LZ4_createStreamDecode();
    if (stream_ == nullptr) {
        NEFORCE_THROW_EXCEPTION(lz4_exception("Failed to create LZ4 decode stream"));
    }

    bytes_input_ = 0;
    bytes_output_ = 0;
}

byte_vector lz4_compressor::stream_decompressor::decompress(const byte_view& data, const bool finish) {
    if (stream_ == nullptr) {
        NEFORCE_THROW_EXCEPTION(lz4_exception("Decompressor not initialized"));
    }
    if (finished_) {
        NEFORCE_THROW_EXCEPTION(lz4_exception("Decompressor already finished"));
    }

    byte_vector output;

    if (!data.empty()) {
        output.resize(block_size);

        const int decompressed_size = ::LZ4_decompress_safe_continue(
                stream_, reinterpret_cast<const char*>(data.data()), reinterpret_cast<char*>(output.data()),
                static_cast<int>(data.size()), static_cast<int>(output.size()));

        if (decompressed_size < 0) {
            NEFORCE_THROW_EXCEPTION(lz4_exception("LZ4 stream decompression failed"));
        }

        output.resize(decompressed_size);
        bytes_input_ += data.size();
        bytes_output_ += decompressed_size;
    }

    if (finish) {
        finished_ = true;
    }

    return output;
}

byte_vector lz4_compressor::stream_decompressor::finish() {
    if (stream_ == nullptr) {
        NEFORCE_THROW_EXCEPTION(lz4_exception("Compressor not initialized"));
    }

    finished_ = true;
    return byte_vector{};
}

NEFORCE_END_NAMESPACE__
#endif
