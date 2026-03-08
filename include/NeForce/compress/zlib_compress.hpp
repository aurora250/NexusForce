#ifndef NEFORCE_COMPRESS_ZLIB_COMPRESS_HPP__
#define NEFORCE_COMPRESS_ZLIB_COMPRESS_HPP__

/**
 * @file zlib_compress.hpp
 * @brief ZLib压缩解压缩工具
 *
 * 此文件提供了基于zlib库的压缩解压缩功能实现。
 * 支持内存数据的压缩解压缩，提供流式处理能力，
 * 包括一次性压缩和流式压缩两种模式。
 */

#ifdef NEFORCE_SUPPORT_ZLIB
#include "NeForce/core/container/vector.hpp"
#include "NeForce/core/string/string.hpp"
#include <zlib.h>
NEFORCE_BEGIN_NAMESPACE__

/**
 * @defgroup Exceptions 异常类集
 * @brief 全部异常类的集合
 * @{
 */

/**
 * @struct zlib_exception
 * @brief ZLib操作异常类
 *
 * zlib操作异常。
 */
NEFORCE_ERROR_BUILD_FINAL_CLASS(zlib_exception, system_exception, "Zlib Operation Failed.")

/** @} */ // Exceptions

/**
 * @defgroup Compression 压缩解压缩
 * @brief 数据压缩和解压缩功能
 * @{
 */

/**
 * @enum compress_level
 * @brief 压缩级别枚举
 *
 * 定义不同的压缩级别，在压缩速度和压缩率之间进行权衡。
 */
enum class compress_level {
    none = Z_NO_COMPRESSION,      ///< 无压缩
    best_speed = Z_BEST_SPEED,    ///< 最快速度，压缩率最低
    default_level = Z_DEFAULT_COMPRESSION, ///< 默认压缩级别
    best_compression = Z_BEST_COMPRESSION  ///< 最佳压缩率，速度最慢
};

/**
 * @enum compress_strategy
 * @brief 压缩策略枚举
 *
 * 定义不同的压缩策略，针对不同类型的数据进行优化。
 */
enum class compress_strategy {
    default_strategy = Z_DEFAULT_STRATEGY, ///< 默认策略，适用于通用数据
    filtered = Z_FILTERED,                 ///< 过滤策略，适用于由过滤器产生的数据
    huffman_only = Z_HUFFMAN_ONLY,         ///< 仅使用霍夫曼编码
    rle = Z_RLE,                           ///< 游程编码
    fixed = Z_FIXED                        ///< 固定霍夫曼编码
};

/**
 * @enum compress_format
 * @brief 压缩格式枚举
 *
 * 定义不同的压缩格式。
 */
enum class compress_format {
    zlib,      ///< ZLIB格式（RFC 1950）
    gzip,      ///< GZIP格式（RFC 1952）
    deflate    ///< 原始Deflate流（无头尾）
};


/**
 * @class zlib_compressor
 * @brief ZLib压缩解压缩工具类
 *
 * 提供静态方法和流式处理类，用于数据的压缩和解压缩。
 * 支持多种数据类型输入，包括迭代器范围、字符串视图、字节向量等。
 */
class NEFORCE_API zlib_compressor {
private:
    /**
     * @brief 压缩数据的内部实现
     * @param data 输入数据指针
     * @param size 输入数据大小
     * @param level 压缩级别
     * @param strategy 压缩策略
     * @param format 压缩格式
     * @return 压缩后的字节向量
     * @throws zlib_exception 当压缩失败时抛出
     */
    NEFORCE_NODISCARD static byte_vector compress_data(
        const byte_t* data, size_t size,
        compress_level level,
        compress_strategy strategy,
        compress_format format);

    /**
     * @brief 解压缩数据的内部实现
     * @param data 压缩数据指针
     * @param size 压缩数据大小
     * @param estimated_original_size 预估原始大小
     * @param format 压缩格式
     * @return 解压缩后的字节向量
     * @throws zlib_exception 当解压缩失败或超出最大缓冲区限制时抛出
     *
     * 如果预估大小不足，会自动调整缓冲区大小重试。
     */
    NEFORCE_NODISCARD static byte_vector decompress_data(
        byte_t* data, size_t size,
        size_t estimated_original_size,
        compress_format format);

public:
    /**
     * @brief 压缩迭代器范围中的数据
     * @tparam Iterator 迭代器类型
     * @param begin 起始迭代器
     * @param end 结束迭代器
     * @param level 压缩级别，默认为default_level
     * @param strategy 压缩策略，默认为default_strategy
     * @param format 压缩格式，默认为zlib
     * @return 压缩后的字节向量
     * @throws zlib_exception 当压缩失败时抛出
     *
     * 要求迭代器指向的元素大小为1字节。
     */
    template <typename Iterator>
    NEFORCE_NODISCARD static byte_vector compress(
        Iterator begin, Iterator end, const compress_level level = compress_level::default_level,
        const compress_strategy strategy = compress_strategy::default_strategy,
        const compress_format format = compress_format::zlib) {
        static_assert(is_ranges_cot_iter_v<Iterator>, "Iterator must be contiguous_iterator");
        static_assert(sizeof(iter_value_t<Iterator>) == 1, "Iterator must point to byte-sized elements");
        
        const auto* data = reinterpret_cast<const byte_t*>(&*begin);
        const size_t data_size = _NEFORCE distance(begin, end);
        return compress_data(data, data_size, level, strategy, format);
    }

    /**
     * @brief 压缩字符串视图
     * @param data 字符串视图
     * @param level 压缩级别，默认为default_level
     * @param strategy 压缩策略，默认为default_strategy
     * @param format 压缩格式，默认为zlib
     * @return 压缩后的字节向量
     * @throws zlib_exception 当压缩失败时抛出
     */
    NEFORCE_NODISCARD static byte_vector compress(
        const string_view data,
        const compress_level level = compress_level::default_level,
        const compress_strategy strategy = compress_strategy::default_strategy,
        const compress_format format = compress_format::zlib) {
        
        return compress_data(
            reinterpret_cast<const byte_t*>(data.data()),
            data.size(),
            level,
            strategy,
            format
        );
    }

    /**
     * @brief 压缩字节向量
     * @tparam T 元素类型
     * @param data 字节向量
     * @param level 压缩级别，默认为default_level
     * @param strategy 压缩策略，默认为default_strategy
     * @param format 压缩格式，默认为zlib
     * @return 压缩后的字节向量
     * @throws zlib_exception 当压缩失败时抛出
     *
     * 要求向量元素大小为1字节。
     */
    template <typename T>
    NEFORCE_NODISCARD static byte_vector compress(
        const vector<T>& data,
        const compress_level level = compress_level::default_level,
        const compress_strategy strategy = compress_strategy::default_strategy,
        const compress_format format = compress_format::zlib) {
        static_assert(sizeof(T) == 1, "Vector must contain byte-sized elements");
        
        return zlib_compressor::compress_data(
            reinterpret_cast<const byte_t*>(data.data()),
            data.size() * sizeof(T),
            level,
            strategy,
            format
        );
    }

    /**
     * @brief 解压缩迭代器范围中的数据
     * @tparam Iterator 迭代器类型
     * @param begin 起始迭代器
     * @param end 结束迭代器
     * @param estimated_original_size 预估原始大小，为0时自动估计
     * @param format 压缩格式，默认为zlib
     * @return 解压缩后的字节向量
     * @throws zlib_exception 当解压缩失败或超出最大缓冲区限制时抛出
     */
    template <typename Iterator>
    NEFORCE_NODISCARD static byte_vector decompress(
        Iterator begin, Iterator end,
        const size_t estimated_original_size = 0,
        const compress_format format = compress_format::zlib) {

        static_assert(is_ranges_cot_iter_v<Iterator>, "Iterator must be contiguous_iterator");
        static_assert(sizeof(iter_value_t<Iterator>) == 1, "Iterator must point to byte-sized elements");

        auto* data = reinterpret_cast<byte_t*>(&*begin);
        const size_t data_size = _NEFORCE distance(begin, end);
        
        return decompress_data(data, data_size, estimated_original_size, format);
    }

    /**
     * @brief 解压缩常量字节视图
     * @param data 常量字节视图
     * @param estimated_original_size 预估原始大小，为0时自动估计
     * @param format 压缩格式，默认为zlib
     * @return 解压缩后的字节向量
     * @throws zlib_exception 当解压缩失败或超出最大缓冲区限制时抛出
     */
    NEFORCE_NODISCARD static byte_vector decompress(
        const byte_view& data,
        const size_t estimated_original_size = 0,
        const compress_format format = compress_format::zlib) {
        return decompress_data(data.data(), data.size(), estimated_original_size, format);
    }

    /**
     * @class stream_compressor
     * @brief 流式压缩器
     *
     * 支持分块压缩数据，适用于大文件或网络传输场景。
     * 维护压缩状态，可以多次添加数据并最终完成压缩。
     */
    class NEFORCE_API stream_compressor {
    private:
        ::z_stream stream_{};      ///< zlib流对象
        bool initialized_ = false; ///< 是否已初始化
        size_t bytes_input_ = 0;   ///< 输入字节计数
        size_t bytes_output_ = 0;  ///< 输出字节计数

    public:
        /**
         * @brief 构造函数
         * @param level 压缩级别，默认为default_level
         * @param strategy 压缩策略，默认为default_strategy
         * @param format 压缩格式，默认为zlib
         * @throws zlib_exception 当重置失败时抛出
         */
        explicit stream_compressor(
            compress_level level = compress_level::default_level,
            compress_strategy strategy = compress_strategy::default_strategy,
            compress_format format = compress_format::zlib);

        /**
         * @brief 析构函数
         */
        ~stream_compressor();

        stream_compressor(const stream_compressor&) = delete;
        stream_compressor& operator =(const stream_compressor&) = delete;

        /**
         * @brief 移动构造函数
         * @param other 源对象
         */
        stream_compressor(stream_compressor&& other) noexcept;

        /**
         * @brief 移动赋值运算符
         * @param other 源对象
         * @return 自身引用
         */
        stream_compressor& operator =(stream_compressor&& other) noexcept;

        /**
         * @brief 压缩数据
         * @param data 输入数据视图
         * @param finish 是否完成（最后一块数据）
         * @return 压缩后的数据块
         * @throws zlib_exception 当未初始化或压缩失败时抛出
         */
        byte_vector compress(const cbyte_view& data, bool finish = false);

        /**
         * @brief 压缩字符串视图
         * @param data 字符串视图
         * @param finish 是否完成
         * @return 压缩后的数据块
         * @throws zlib_exception 当未初始化或压缩失败时抛出
         */
        byte_vector compress(string_view data, bool finish = false);

        /**
         * @brief 完成压缩并返回剩余数据
         * @return 最后的压缩数据块
         * @throws zlib_exception 当未初始化或压缩失败时抛出
         */
        byte_vector finish();

        /**
         * @brief 重置压缩器状态
         * @param level 新压缩级别，默认为default_level
         * @param strategy 新压缩策略，默认为default_strategy
         * @param format 新压缩格式，默认为zlib
         * @throws zlib_exception 当重置失败时抛出
         */
        void reset(
            compress_level level = compress_level::default_level,
            compress_strategy strategy = compress_strategy::default_strategy,
            compress_format format = compress_format::zlib);

        /**
         * @brief 获取输入字节数
         * @return 已处理的输入字节数
         */
        NEFORCE_NODISCARD size_t bytes_input() const noexcept {
            return bytes_input_;
        }

        /**
         * @brief 获取输出字节数
         * @return 已产生的输出字节数
         */
        NEFORCE_NODISCARD size_t bytes_output() const noexcept {
            return bytes_output_;
        }

        /**
         * @brief 获取压缩率
         * @return 输出/输入字节比率
         */
        NEFORCE_NODISCARD double compression_ratio() const noexcept {
            return bytes_input_ > 0 ? static_cast<double>(bytes_output_) / bytes_input_ : 0.0;
        }
    };

    /**
     * @class stream_decompressor
     * @brief 流式解压缩器
     *
     * 支持分块解压缩数据，适用于处理流式压缩的数据。
     * 维护解压缩状态，可以多次添加数据并最终完成。
     */
    class NEFORCE_API stream_decompressor {
    private:
        ::z_stream stream_{};      ///< zlib流对象
        bool initialized_ = false; ///< 是否已初始化
        size_t bytes_input_ = 0;   ///< 输入字节计数
        size_t bytes_output_ = 0;  ///< 输出字节计数

    public:
        /**
         * @brief 构造函数
         * @param format 压缩格式，默认为zlib
         * @throws zlib_exception 当重置失败时抛出
         */
        stream_decompressor(compress_format format = compress_format::zlib);

        /**
         * @brief 析构函数
         */
        ~stream_decompressor();

        stream_decompressor(const stream_decompressor&) = delete;
        stream_decompressor& operator =(const stream_decompressor&) = delete;

        /**
         * @brief 移动构造函数
         * @param other 源对象
         */
        stream_decompressor(stream_decompressor&& other) noexcept;

        /**
         * @brief 移动赋值运算符
         * @param other 源对象
         * @return 自身引用
         */
        stream_decompressor& operator =(stream_decompressor&& other) noexcept;

        /**
         * @brief 解压缩数据
         * @param data 压缩数据视图
         * @param finish 是否完成
         * @return 解压缩后的数据块
         * @throws zlib_exception 当未初始化或解压缩失败时抛出
         */
        byte_vector decompress(const byte_view& data, bool finish = false);

        /**
         * @brief 完成解压缩并返回剩余数据
         * @return 最后的解压缩数据块
         * @throws zlib_exception 当未初始化或解压缩失败时抛出
         */
        byte_vector finish();

        /**
         * @brief 重置解压缩器状态
         * @param format 压缩格式，默认为zlib
         * @throws zlib_exception 当重置失败时抛出
         */
        void reset(compress_format format = compress_format::zlib);

        /**
         * @brief 获取输入字节数
         * @return 已处理的输入字节数
         */
        NEFORCE_NODISCARD size_t bytes_input() const noexcept {
            return bytes_input_;
        }

        /**
         * @brief 获取输出字节数
         * @return 已产生的输出字节数
         */
        NEFORCE_NODISCARD size_t bytes_output() const noexcept {
            return bytes_output_;
        }

        /**
         * @brief 获取扩展率
         * @return 输出/输入字节比率
         */
        NEFORCE_NODISCARD double expansion_ratio() const noexcept {
            return bytes_input_ > 0 ? static_cast<double>(bytes_output_) / bytes_input_ : 0.0;
        }
    };
};

/** @} */ // Compression

NEFORCE_END_NAMESPACE__
#endif
#endif // NEFORCE_COMPRESS_ZLIB_COMPRESS_HPP__
