#ifndef NEFORCE_COMPRESS_ZSTD_COMPRESS_HPP__
#define NEFORCE_COMPRESS_ZSTD_COMPRESS_HPP__

/**
 * @file zstd_compress.hpp
 * @brief Zstandard 压缩解压缩工具
 *
 * 提供静态压缩/解压接口：迭代器范围、字符串视图、字节向量、常量字节视图；
 * 每次压缩生成一个带校验和的独立帧。解压支持多个连续帧，并提供保留完整前缀的流式恢复接口。
 */

#ifdef NEFORCE_SUPPORT_ZSTD
#    include "NeForce/core/string/string.hpp"
NEFORCE_BEGIN_NAMESPACE__

/**
 * @defgroup Exceptions 异常类集
 * @brief 全部异常类的集合
 * @{
 */

/**
 * @struct zstd_exception
 * @brief Zstandard 操作异常
 */
struct zstd_exception final : thirdparty_exception {
    explicit zstd_exception(const char* info = "Zstandard Operation Failed.") noexcept :
    thirdparty_exception(info) {}

    explicit zstd_exception(const exception& e) :
    thirdparty_exception(e) {}

    ~zstd_exception() override = default;

    NEFORCE_NODISCARD const char* type() const noexcept override { return "zstd_exception"; }
};

/** @} */ // Exceptions

/**
 * @defgroup Compression 压缩解压缩
 * @brief 数据压缩和解压缩功能
 * @{
 */

/**
 * @class zstd_compressor
 * @brief Zstandard 帧压缩解压缩工具类
 * @note 每个 compress() 调用只生成一个帧；需要多帧归档时按批次多次调用
 */
class NEFORCE_API zstd_compressor {
private:
    NEFORCE_NODISCARD static byte_vector compress_data(const byte_t* data, size_t size, int level);
    NEFORCE_NODISCARD static bool decompress_prefix_data(const byte_t* data, size_t size, string& output, bool* torn);

public:
    /**
     * @brief 压缩迭代器范围中的数据
     * @tparam Iterator 连续单字节迭代器
     * @param begin 起始迭代器
     * @param end 结束迭代器
     * @param level 压缩级别（-5..22）
     * @return 压缩后的字节向量
     * @throws zstd_exception 压缩失败时抛出
     */
    template <typename Iterator>
    NEFORCE_NODISCARD static byte_vector compress(Iterator begin, Iterator end, const int level = 3) {
        static_assert(is_ranges_cot_iter_v<Iterator>, "Iterator must be contiguous_iterator");
        static_assert(sizeof(iter_value_t<Iterator>) == 1, "Iterator must point to byte-sized elements");
        const auto* data = reinterpret_cast<const byte_t*>(&*begin);
        return compress_data(data, static_cast<size_t>(_NEFORCE distance(begin, end)), level);
    }

    /**
     * @brief 压缩字符串视图
     * @param data 原文
     * @param level 压缩级别（-5..22）
     * @return 压缩后的字节向量
     * @throws zstd_exception 压缩失败或时抛出
     */
    NEFORCE_NODISCARD static byte_vector compress(const string_view data, const int level = 3) {
        return compress_data(reinterpret_cast<const byte_t*>(data.data()), data.size(), level);
    }

    /**
     * @brief 压缩字节向量
     * @tparam T 单字节元素类型
     * @param data 数据
     * @param level 压缩级别（-5..22）
     * @return 压缩后的字节向量
     * @throws zstd_exception 压缩失败时抛出
     */
    template <typename T>
    NEFORCE_NODISCARD static byte_vector compress(const vector<T>& data, const int level = 3) {
        static_assert(sizeof(T) == 1, "Vector must contain byte-sized elements");
        return compress_data(reinterpret_cast<const byte_t*>(data.data()), data.size() * sizeof(T), level);
    }

    /**
     * @brief 解压一个或多个连续 Zstandard 帧
     * @param data 压缩数据
     * @param estimated_original_size 预估原文大小（0 表示自动流式解码）
     * @return 解压后的字节向量
     * @throws zstd_exception 数据损坏或尾部不完整时抛出
     */
    NEFORCE_NODISCARD static byte_vector decompress(const cbyte_view& data, size_t estimated_original_size = 0);

    /**
     * @brief 流式解压并保留不完整尾部之前的完整行前缀
     * @param data 压缩数据
     * @param output 解压后的原文
     * @param torn 非空时报告尾部帧是否不完整
     * @return 至少解出一个完整帧返回 true
     */
    static bool decompress_prefix(string_view data, string& output, bool* torn = nullptr);

    /**
     * @brief 统计连续 Zstandard 帧数量
     * @param data 压缩数据
     * @return 完整帧数量；尾部不完整帧不计数
     */
    NEFORCE_NODISCARD static size_t frame_count(string_view data) noexcept;
};

/** @} */ // Compression

NEFORCE_END_NAMESPACE__
#endif
#endif
