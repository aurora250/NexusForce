#ifndef NEFORCE_COMPRESS_LZ4_COMPRESS_HPP__
#define NEFORCE_COMPRESS_LZ4_COMPRESS_HPP__

/**
 * @file lz4_compress.hpp
 * @brief LZ4压缩解压缩工具
 *
 * 此文件提供了基于LZ4库的压缩解压缩功能实现。
 * 支持内存数据的压缩解压缩，提供流式处理能力，
 * 包括一次性压缩和流式压缩两种模式。
 */

#if defined(NEFORCE_SUPPORT_LZ4) || defined(NEXUSFORCE_ENABLE_DOXYGEN)
#    include <lz4.h>
#    include "NeForce/core/container/vector.hpp"
#    include "NeForce/core/string/string.hpp"
NEFORCE_BEGIN_NAMESPACE__

/**
 * @defgroup Exceptions 异常类集
 * @brief 全部异常类的集合
 * @{
 */

/**
 * @struct lz4_exception
 * @brief lz4操作异常类
 *
 * lz4操作异常。
 */
struct lz4_exception final : thirdparty_exception {
    explicit lz4_exception(const char* info = "LZ4 Operation Failed.", const char* type = static_type,
                           const int code = 0) noexcept :
    thirdparty_exception(info, type, code) {}

    explicit lz4_exception(const exception& e) :
    thirdparty_exception(e) {}

    ~lz4_exception() override = default;
    static constexpr auto static_type = "lz4_exception";
};

/** @} */ // Exceptions

/**
 * @defgroup Compression 压缩解压缩
 * @brief 数据压缩和解压缩功能
 * @{
 */

/**
 * @class lz4_compressor
 * @brief LZ4压缩解压缩工具类
 *
 * 提供静态方法和流式处理类，用于数据的压缩和解压缩。
 * 支持多种数据类型输入，包括迭代器范围、字符串视图、字节向量等。
 */
class NEFORCE_API lz4_compressor {
public:
    /**
     * @brief 最大块大小常量
     *
     * LZ4流式压缩处理的最大数据块大小（64KB）。
     */
    static constexpr size_t block_size = 65536;

private:
    /**
     * @brief 压缩数据的内部实现
     * @param data 输入数据指针
     * @param size 输入数据大小
     * @param level 压缩级别，范围0-12，0为快速压缩，1-12为HC压缩
     * @return 压缩后的字节向量
     * @throws lz4_exception 当压缩失败时抛出
     */
    NEFORCE_NODISCARD static byte_vector compress_data(const byte_t* data, size_t size, int level);

    /**
     * @brief 解压缩数据的内部实现
     * @param data 压缩数据指针
     * @param size 压缩数据大小
     * @param estimated_original_size 预估原始大小
     * @return 解压缩后的字节向量
     * @throws lz4_exception 当解压缩失败或超出最大缓冲区限制时抛出
     *
     * 如果预估大小不足，会自动调整缓冲区大小重试，最多尝试5次。
     */
    NEFORCE_NODISCARD static byte_vector decompress_data(const byte_t* data, size_t size,
                                                         size_t estimated_original_size);

public:
    /**
     * @brief 压缩迭代器范围中的数据
     * @tparam Iterator 迭代器类型
     * @param begin 起始迭代器
     * @param end 结束迭代器
     * @param level 压缩级别
     * @return 压缩后的字节向量
     * @throws lz4_exception 当压缩失败时抛出
     *
     * 要求迭代器指向的元素大小为1字节。
     * 压缩级别：
     * - 0：快速压缩（默认）
     * - 1-12：HC（高压缩率）模式，级别越高压缩率越高但速度越慢
     */
    template <typename Iterator>
    NEFORCE_NODISCARD static byte_vector compress(Iterator begin, Iterator end, const int level = 0) {
        static_assert(is_ranges_cot_iter_v<Iterator>, "Iterator must be contiguous_iterator");
        static_assert(sizeof(iter_value_t<Iterator>) == 1, "Iterator must point to byte-sized elements");

        const auto* data = reinterpret_cast<const byte_t*>(&*begin);
        const size_t data_size = _NEFORCE distance(begin, end);
        return compress_data(data, data_size, level);
    }

    /**
     * @brief 压缩字符串视图
     * @param data 字符串视图
     * @param level 压缩级别
     * @return 压缩后的字节向量
     * @throws lz4_exception 当压缩失败时抛出
     *
     * 压缩级别：
     * - 0：快速压缩（默认）
     * - 1-12：HC（高压缩率）模式，级别越高压缩率越高但速度越慢
     */
    NEFORCE_NODISCARD static byte_vector compress(const string_view data, const int level = 0) {
        return compress_data(reinterpret_cast<const byte_t*>(data.data()), data.size(), level);
    }

    /**
     * @brief 压缩字节向量
     * @tparam T 元素类型
     * @param data 字节向量
     * @param level 压缩级别
     * @return 压缩后的字节向量
     * @throws lz4_exception 当压缩失败时抛出
     *
     * 要求向量元素大小为1字节。
     * 压缩级别：
     * - 0：快速压缩（默认）
     * - 1-12：HC（高压缩率）模式，级别越高压缩率越高但速度越慢
     */
    template <typename T>
    NEFORCE_NODISCARD static byte_vector compress(const vector<T>& data, const int level = 0) {
        static_assert(sizeof(T) == 1, "Iterator must point to byte-sized elements");

        return lz4_compressor::compress_data(reinterpret_cast<const byte_t*>(data.data()), data.size() * sizeof(T),
                                             level);
    }

    /**
     * @brief 解压缩迭代器范围中的数据
     * @tparam Iterator 迭代器类型
     * @param begin 起始迭代器
     * @param end 结束迭代器
     * @param estimated_original_size 预估原始大小，为0时自动估计（默认使用压缩大小的4倍）
     * @return 解压缩后的字节向量
     * @throws lz4_exception 当解压缩失败或超出最大缓冲区限制时抛出
     *
     * 要求迭代器指向的元素大小为1字节。
     */
    template <typename Iterator, enable_if_t<is_ranges_cot_iter_v<Iterator>, int> = 0>
    NEFORCE_NODISCARD static byte_vector decompress(Iterator begin, Iterator end,
                                                    const size_t estimated_original_size = 0) {
        static_assert(sizeof(iter_value_t<Iterator>) == 1, "Iterator must point to byte-sized elements");

        const auto* data = reinterpret_cast<const byte_t*>(&*begin);
        const size_t data_size = _NEFORCE distance(begin, end);

        return decompress_data(data, data_size, estimated_original_size);
    }

    /**
     * @brief 解压缩常量字节视图
     * @param data 常量字节视图
     * @param estimated_original_size 预估原始大小，为0时自动估计
     * @return 解压缩后的字节向量
     * @throws lz4_exception 当解压缩失败或超出最大缓冲区限制时抛出
     */
    NEFORCE_NODISCARD static byte_vector decompress(const cbyte_view& data, const size_t estimated_original_size = 0) {
        return decompress_data(data.data(), data.size(), estimated_original_size);
    }

    /**
     * @class stream_compressor
     * @brief 流式压缩器
     *
     * 支持分块压缩数据，适用于大文件或网络传输场景。
     * 维护压缩状态，可以多次添加数据并最终完成压缩。
     * 每个数据块大小不能超过block_size（64KB）。
     */
    class NEFORCE_API stream_compressor {
    private:
        ::LZ4_stream_t* stream_ = nullptr; ///< LZ4流对象
        bool use_hc_ = false;              ///< 是否使用HC压缩模式
        int level_ = 0;                    ///< 压缩级别
        size_t bytes_input_ = 0;           ///< 输入字节计数
        size_t bytes_output_ = 0;          ///< 输出字节计数
        bool finished_ = false;            ///< 是否已完成

    public:
        /**
         * @brief 构造函数
         * @param level 压缩级别
         * @throws lz4_exception 当创建流对象失败时抛出
         *
         * 压缩级别：
         * - 0：快速压缩（默认）
         * - 1-12：HC（高压缩率）模式，级别越高压缩率越高但速度越慢
         */
        explicit stream_compressor(int level = 0);

        /**
         * @brief 析构函数
         */
        ~stream_compressor();

        stream_compressor(const stream_compressor&) = delete;
        stream_compressor& operator=(const stream_compressor&) = delete;

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
        stream_compressor& operator=(stream_compressor&& other) noexcept;

        /**
         * @brief 压缩数据
         * @param data 输入数据视图
         * @param finish 是否完成（最后一块数据）
         * @return 压缩后的数据块
         * @throws lz4_exception 当未初始化、已结束、输入块过大或压缩失败时抛出
         *
         * 输入数据块大小不能超过block_size（64KB）。
         */
        byte_vector compress(const cbyte_view& data, bool finish = false);

        /**
         * @brief 压缩字符串视图
         * @param data 字符串视图
         * @param finish 是否完成
         * @return 压缩后的数据块
         * @throws lz4_exception 当未初始化、已结束或压缩失败时抛出
         */
        byte_vector compress(string_view data, bool finish = false);

        /**
         * @brief 完成压缩并返回剩余数据
         * @return 最后的压缩数据块（LZ4流式压缩完成后无额外数据）
         * @throws lz4_exception 当未初始化时抛出
         */
        byte_vector finish();

        /**
         * @brief 重置压缩器状态
         * @param level 新压缩级别，默认为0
         * @throws lz4_exception 当重置失败时抛出
         *
         * 压缩级别：
         * - 0：快速压缩（默认）
         * - 1-12：HC（高压缩率）模式，级别越高压缩率越高但速度越慢
         */
        void reset(int level = 0);

        /**
         * @brief 获取输入字节数
         * @return 已处理的输入字节数
         */
        NEFORCE_NODISCARD size_t bytes_input() const noexcept { return bytes_input_; }

        /**
         * @brief 获取输出字节数
         * @return 已产生的输出字节数
         */
        NEFORCE_NODISCARD size_t bytes_output() const noexcept { return bytes_output_; }

        /**
         * @brief 获取压缩率
         * @return 输出/输入字节比率
         */
        NEFORCE_NODISCARD double compression_ratio() const noexcept {
            return bytes_input_ > 0 ? static_cast<double>(bytes_output_) / static_cast<double>(bytes_input_) : 0.0;
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
        ::LZ4_streamDecode_t* stream_ = nullptr; ///< LZ4解码流对象
        size_t bytes_input_ = 0;                 ///< 输入字节计数
        size_t bytes_output_ = 0;                ///< 输出字节计数
        bool finished_ = false;                  ///< 是否已完成

    public:
        /**
         * @brief 构造函数
         * @throws lz4_exception 当创建解码流对象失败时抛出
         */
        stream_decompressor();

        /**
         * @brief 析构函数
         */
        ~stream_decompressor();

        stream_decompressor(const stream_decompressor&) = delete;
        stream_decompressor& operator=(const stream_decompressor&) = delete;

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
        stream_decompressor& operator=(stream_decompressor&& other) noexcept;

        /**
         * @brief 解压缩数据
         * @param data 压缩数据视图
         * @param finish 是否完成
         * @return 解压缩后的数据块
         * @throws lz4_exception 当未初始化、已结束或解压缩失败时抛出
         */
        byte_vector decompress(const byte_view& data, bool finish = false);

        /**
         * @brief 完成解压缩并返回剩余数据
         * @return 最后的解压缩数据块（LZ4流式解压缩完成后无额外数据）
         * @throws lz4_exception 当未初始化时抛出
         */
        byte_vector finish();

        /**
         * @brief 重置解压缩器状态
         * @throws lz4_exception 当重置失败时抛出
         */
        void reset();

        /**
         * @brief 获取输入字节数
         * @return 已处理的输入字节数
         */
        NEFORCE_NODISCARD size_t bytes_input() const noexcept { return bytes_input_; }

        /**
         * @brief 获取输出字节数
         * @return 已产生的输出字节数
         */
        NEFORCE_NODISCARD size_t bytes_output() const noexcept { return bytes_output_; }

        /**
         * @brief 获取扩展率
         * @return 输出/输入字节比率
         */
        NEFORCE_NODISCARD double expansion_ratio() const noexcept {
            return bytes_input_ > 0 ? static_cast<double>(bytes_output_) / static_cast<double>(bytes_input_) : 0.0;
        }
    };
};

/** @} */ // Compression

NEFORCE_END_NAMESPACE__
#endif
#endif // NEFORCE_COMPRESS_LZ4_COMPRESS_HPP__
