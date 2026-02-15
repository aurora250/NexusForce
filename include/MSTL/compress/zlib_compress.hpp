#ifndef MSTL_COMPRESS_ZLIB_COMPRESS_HPP__
#define MSTL_COMPRESS_ZLIB_COMPRESS_HPP__
#ifdef MSTL_SUPPORT_ZLIB__
#include "MSTL/core/container/vector.hpp"
#include "MSTL/core/memory/memory_view.hpp"
#include "MSTL/core/string/string.hpp"
#include <zlib.h>
MSTL_BEGIN_NAMESPACE__

MSTL_ERROR_BUILD_FINAL_CLASS(zlib_exception, system_exception, "Zlib Operation Failed.")


enum class COMPRESS_LEVEL {
    none = Z_NO_COMPRESSION,
    best_speed = Z_BEST_SPEED,
    default_level = Z_DEFAULT_COMPRESSION,
    best_compression = Z_BEST_COMPRESSION
};

enum class COMPRESS_STRATEGY {
    default_strategy = Z_DEFAULT_STRATEGY,
    filtered = Z_FILTERED,
    huffman_only = Z_HUFFMAN_ONLY,
    rle = Z_RLE,
    fixed = Z_FIXED
};


class MSTL_API zlib_compressor {
private:
    MSTL_NODISCARD static byte_vector compress_data(
        const byte_t* data, size_t size,
        COMPRESS_LEVEL level, COMPRESS_STRATEGY strategy);

    MSTL_NODISCARD static byte_vector decompress_data(
        const byte_t* data, size_t size,
        size_t estimated_original_size);

    static void check_zlib_error(int ret_code);

public:
    template <typename Iter, enable_if_t<is_ranges_cot_iter_v<Iter>, int> = 0>
    MSTL_NODISCARD static byte_vector compress(Iter begin, Iter end,
        const COMPRESS_LEVEL level = COMPRESS_LEVEL::default_level,
        const COMPRESS_STRATEGY strategy = COMPRESS_STRATEGY::default_strategy) {
        
        static_assert(sizeof(*begin) == 1, "Iterator must point to byte-sized elements");
        
        const auto* data = reinterpret_cast<const byte_t*>(&*begin);
        const size_t data_size = _MSTL distance(begin, end);
        return compress_data(data, data_size, level, strategy);
    }
    
    MSTL_NODISCARD static byte_vector compress(const string_view data,
        const COMPRESS_LEVEL level = COMPRESS_LEVEL::default_level,
        const COMPRESS_STRATEGY strategy = COMPRESS_STRATEGY::default_strategy) {
        
        return compress_data(
            reinterpret_cast<const byte_t*>(data.data()),
            data.size(),
            level,
            strategy
        );
    }
    
    template <typename T>
    MSTL_NODISCARD static byte_vector compress(
        const vector<T>& data,
        const COMPRESS_LEVEL level = COMPRESS_LEVEL::default_level,
        const COMPRESS_STRATEGY strategy = COMPRESS_STRATEGY::default_strategy) {
        
        static_assert(sizeof(T) == 1, "Vector must contain byte-sized elements");
        
        return zlib_compressor::compress_data(
            reinterpret_cast<const byte_t*>(data.data()),
            data.size() * sizeof(T),
            level,
            strategy
        );
    }

    template <typename Iter, enable_if_t<is_ranges_cot_iter_v<Iter>, int> = 0>
    MSTL_NODISCARD static byte_vector decompress(Iter begin, Iter end,
        const size_t estimated_original_size = 0) {
        
        static_assert(sizeof(*begin) == 1, "Iterator must point to byte-sized elements");
        
        const auto* data = reinterpret_cast<const byte_t*>(&*begin);
        const size_t data_size = _MSTL distance(begin, end);
        
        return decompress_data(data, data_size, estimated_original_size);
    }
    
    MSTL_NODISCARD static byte_vector decompress(
        const cbyte_view& data,
        const size_t estimated_original_size = 0) {
        
        return decompress_data(data.data(), data.size(), estimated_original_size);
    }

    class MSTL_API stream_compressor {
    public:
        explicit stream_compressor(
            COMPRESS_LEVEL level = COMPRESS_LEVEL::default_level,
            COMPRESS_STRATEGY strategy = COMPRESS_STRATEGY::default_strategy);
        
        ~stream_compressor();

        stream_compressor(const stream_compressor&) = delete;
        stream_compressor& operator =(const stream_compressor&) = delete;
        stream_compressor(stream_compressor&& other) noexcept;
        stream_compressor& operator =(stream_compressor&& other) noexcept;

        byte_vector compress(const cbyte_view& data, bool finish = false);
        byte_vector compress(string_view data, bool finish = false);

        byte_vector finish();

        void reset(COMPRESS_LEVEL level = COMPRESS_LEVEL::default_level,
                   COMPRESS_STRATEGY strategy = COMPRESS_STRATEGY::default_strategy);

        MSTL_NODISCARD size_t bytes_input() const noexcept { return bytes_input_; }
        MSTL_NODISCARD size_t bytes_output() const noexcept { return bytes_output_; }
        MSTL_NODISCARD double compression_ratio() const noexcept {
            return bytes_input_ > 0 ? static_cast<double>(bytes_output_) / bytes_input_ : 0.0;
        }

    private:
        ::z_stream stream_{};
        bool initialized_ = false;
        size_t bytes_input_ = 0;
        size_t bytes_output_ = 0;
    };

    class MSTL_API stream_decompressor {
    public:
        stream_decompressor();
        ~stream_decompressor();

        stream_decompressor(const stream_decompressor&) = delete;
        stream_decompressor& operator =(const stream_decompressor&) = delete;

        stream_decompressor(stream_decompressor&& other) noexcept;
        stream_decompressor& operator =(stream_decompressor&& other) noexcept;

        byte_vector decompress(const cbyte_view& data, bool finish = false);

        byte_vector finish();

        void reset();

        MSTL_NODISCARD size_t bytes_input() const noexcept { return bytes_input_; }
        MSTL_NODISCARD size_t bytes_output() const noexcept { return bytes_output_; }
        MSTL_NODISCARD double expansion_ratio() const noexcept {
            return bytes_input_ > 0 ? static_cast<double>(bytes_output_) / bytes_input_ : 0.0;
        }
        
    private:
        ::z_stream stream_{};
        bool initialized_ = false;
        size_t bytes_input_ = 0;
        size_t bytes_output_ = 0;
    };
};

using compressor = zlib_compressor::stream_compressor;
using decompressor = zlib_compressor::stream_decompressor;

MSTL_END_NAMESPACE__
#endif
#endif // MSTL_COMPRESS_ZLIB_COMPRESS_HPP__
