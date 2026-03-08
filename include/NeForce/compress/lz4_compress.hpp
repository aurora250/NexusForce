#ifndef NEFORCE_COMPRESS_LZ4_COMPRESS_HPP__
#define NEFORCE_COMPRESS_LZ4_COMPRESS_HPP__
#ifdef NEFORCE_SUPPORT_LZ4
#include "NeForce/core/container/vector.hpp"
#include "NeForce/core/string/string.hpp"
#include <lz4.h>
NEFORCE_BEGIN_NAMESPACE__

NEFORCE_ERROR_BUILD_FINAL_CLASS(lz4_exception, system_exception, "LZ4 Operation Failed.")

class NEFORCE_API lz4_compressor {
public:
    static constexpr size_t block_size = 65536;

private:
    NEFORCE_NODISCARD static byte_vector compress_data(
        const byte_t* data, size_t size, int level);

    NEFORCE_NODISCARD static byte_vector decompress_data(
        const byte_t* data, size_t size, size_t estimated_original_size);

public:
    template <typename Iterator>
    NEFORCE_NODISCARD static byte_vector compress(
    Iterator begin, Iterator end, const int level = 0) {
        static_assert(is_ranges_cot_iter_v<Iterator>, "Iterator must be contiguous_iterator");
        static_assert(sizeof(iter_value_t<Iterator>) == 1, "Iterator must point to byte-sized elements");

        const auto* data = reinterpret_cast<const byte_t*>(&*begin);
        const size_t data_size = _NEFORCE distance(begin, end);
        return compress_data(data, data_size, level);
    }

    NEFORCE_NODISCARD static byte_vector compress(const string_view data, const int level = 0) {
        return compress_data(
            reinterpret_cast<const byte_t*>(data.data()),
            data.size(),
            level
        );
    }

    template <typename T>
    NEFORCE_NODISCARD static byte_vector compress(const vector<T>& data, const int level = 0) {
        static_assert(sizeof(T) == 1, "Iterator must point to byte-sized elements");

        return lz4_compressor::compress_data(
            reinterpret_cast<const byte_t*>(data.data()),
            data.size() * sizeof(T),
            level
        );
    }

    template <typename Iterator, enable_if_t<is_ranges_cot_iter_v<Iterator>, int> = 0>
    NEFORCE_NODISCARD static byte_vector decompress(
        Iterator begin, Iterator end,
        const size_t estimated_original_size = 0) {
        static_assert(sizeof(iter_value_t<Iterator>) == 1, "Iterator must point to byte-sized elements");

        const auto* data = reinterpret_cast<const byte_t*>(&*begin);
        const size_t data_size = _NEFORCE distance(begin, end);

        return decompress_data(data, data_size, estimated_original_size);
    }

    NEFORCE_NODISCARD static byte_vector decompress(const cbyte_view& data, const size_t estimated_original_size = 0) {
        return decompress_data(data.data(), data.size(), estimated_original_size);
    }

    class NEFORCE_API stream_compressor {
    private:
        LZ4_stream_t* stream_ = nullptr;
        bool use_hc_ = false;
        int level_ = 0;
        size_t bytes_input_ = 0;
        size_t bytes_output_ = 0;
        bool finished_ = false;

    public:
        explicit stream_compressor(int level = 0);
        ~stream_compressor();

        stream_compressor(const stream_compressor&) = delete;
        stream_compressor& operator =(const stream_compressor&) = delete;

        stream_compressor(stream_compressor&& other) noexcept;
        stream_compressor& operator =(stream_compressor&& other) noexcept;

        byte_vector compress(const cbyte_view& data, bool finish = false);
        byte_vector compress(string_view data, bool finish = false);
        byte_vector finish();
        void reset(int level = 0);

        NEFORCE_NODISCARD size_t bytes_input() const noexcept {
            return bytes_input_;
        }

        NEFORCE_NODISCARD size_t bytes_output() const noexcept {
            return bytes_output_;
        }

        NEFORCE_NODISCARD double compression_ratio() const noexcept {
            return bytes_input_ > 0 ? static_cast<double>(bytes_output_) / bytes_input_ : 0.0;
        }
    };

    class NEFORCE_API stream_decompressor {
    private:
        LZ4_streamDecode_t* stream_ = nullptr;
        size_t bytes_input_ = 0;
        size_t bytes_output_ = 0;
        bool finished_ = false;

    public:
        stream_decompressor();
        ~stream_decompressor();

        stream_decompressor(const stream_decompressor&) = delete;
        stream_decompressor& operator =(const stream_decompressor&) = delete;

        stream_decompressor(stream_decompressor&& other) noexcept;
        stream_decompressor& operator =(stream_decompressor&& other) noexcept;

        byte_vector decompress(const byte_view& data, bool finish = false);
        byte_vector finish();
        void reset();

        NEFORCE_NODISCARD size_t bytes_input() const noexcept {
            return bytes_input_;
        }

        NEFORCE_NODISCARD size_t bytes_output() const noexcept {
            return bytes_output_;
        }

        NEFORCE_NODISCARD double expansion_ratio() const noexcept {
            return bytes_input_ > 0 ? static_cast<double>(bytes_output_) / bytes_input_ : 0.0;
        }
    };
};

NEFORCE_END_NAMESPACE__
#endif
#endif // NEFORCE_COMPRESS_LZ4_COMPRESS_HPP__
