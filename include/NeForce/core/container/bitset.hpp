#ifndef NEFORCE_CORE_CONTAINER_BITSET_HPP__
#define NEFORCE_CORE_CONTAINER_BITSET_HPP__

/**
 * @file bitset.hpp
 * @brief 固定大小位集容器
 *
 * 此文件提供了固定大小位集容器的实现。
 * bitset是一个固定大小的位序列，支持各种位操作，
 * 如设置、重置、翻转、移位以及逻辑运算。其大小在编译时确定。
 */

#include "NeForce/core/container/array.hpp"
#include "NeForce/core/interface/istringify.hpp"
#include "NeForce/core/memory/bit.hpp"
NEFORCE_BEGIN_NAMESPACE__

/**
 * @defgroup BitManipulation 位操作
 * @brief 位操作类与函数的实现
 * @{
 */

/**
 * @class bitset
 * @brief 固定大小的位集
 * @tparam N 位数，在编译时确定
 *
 * bitset表示一个固定大小的二进制位序列，提供对每个位的独立访问
 * 以及常见的位操作（与、或、异或、移位等）。存储效率高，操作速度快。
 * 位数N在编译时确定，因此不能动态改变大小。
 */
template <size_t N>
class bitset : public icommon<bitset<N>>, public ibinary<bitset<N>>, public istringify<bitset<N>> {
public:
    /**
     * @class reference
     * @brief 位引用类，用于返回可修改的引用
     *
     * 模拟对单个位的引用，允许赋值、转换和翻转操作。
     */
    class reference {
    private:
        bitset& set_;     ///< 关联的bitset对象
        size_t position_; ///< 位位置

    public:
        /**
         * @brief 构造函数
         * @param set bitset引用
         * @param position 位位置
         */
        constexpr reference(bitset& set, const size_t position) noexcept :
        set_(set),
        position_(position) {}

        /**
         * @brief 赋值操作符（bool版本）
         * @param value 要赋的值
         * @return 自身引用
         */
        constexpr reference& operator=(const bool value) noexcept {
            set_.set(position_, value);
            return *this;
        }

        /**
         * @brief 赋值操作符（引用版本）
         * @param value 另一个引用
         * @return 自身引用
         */
        constexpr reference& operator=(const reference& value) noexcept { return *this = static_cast<bool>(value); }

        /**
         * @brief 转换为bool
         * @return 位的值
         */
        explicit constexpr operator bool() const noexcept { return set_.test(position_); }

        /**
         * @brief 翻转该位
         * @return 自身引用
         */
        constexpr reference& flip() noexcept {
            set_.flip(position_);
            return *this;
        }
    };

private:
    using block_type = size_t;                                                       ///< 底层存储单元类型
    static constexpr size_t bits_per_block = sizeof(block_type) * 8;                 ///< 每个存储单元的位数
    static constexpr size_t block_count = (N + bits_per_block - 1) / bits_per_block; ///< 所需存储单元数量

    array<block_type, block_count> blocks{}; ///< 存储单元的数组

private:
    static constexpr block_type last_block_mask() noexcept {
        const size_t excess = block_count * bits_per_block - N;
        if (excess == 0) {
            return static_cast<block_type>(~0ULL);
        }
        return (static_cast<block_type>(1ULL) << (bits_per_block - excess)) - 1;
    }

    NEFORCE_ALWAYS_INLINE constexpr void check_range(const size_t pos) const noexcept {
        NEFORCE_DEBUG_VERIFY(pos < N, "bitset position out of range");
    }

    NEFORCE_ALWAYS_INLINE constexpr size_t block_index(const size_t pos) const noexcept { return pos / bits_per_block; }

    NEFORCE_ALWAYS_INLINE constexpr size_t bit_index(const size_t pos) const noexcept { return pos % bits_per_block; }

public:
    /**
     * @brief 默认构造函数，所有位初始化为0
     */
    constexpr bitset() noexcept { blocks.fill(0); }

    /**
     * @brief 整数初始化
     * @param value 无符号整数值，用于初始化低位
     *
     * 将二进制位复制到bitset的低位，超出N的高位被忽略。
     */
    constexpr explicit bitset(const block_type value) noexcept {
        blocks.fill(0);
        NEFORCE_IF_CONSTEXPR(block_count > 0) { blocks[0] = value & last_block_mask(); }
    }

    /**
     * @brief 用字符串初始化bitset
     * @param str 字符串视图
     * @param zero 表示0的字符，默认为'0'
     * @param one 表示1的字符，默认为'1'
     * @throw value_exception 如果字符串包含无效字符
     *
     * 字符串从左到右对应从高位到低位。如果字符串长度小于N，高位补0；
     * 如果长度大于N，只使用前N个字符。
     */
    constexpr explicit bitset(const string_view str, const char zero = '0', const char one = '1') {
        blocks.fill(0);
        const size_t str_len = str.length();
        const size_t M = (str_len < N) ? str_len : N;

        for (size_t i = 0; i < M; ++i) {
            const char c = str[i];
            size_t pos = N - 1 - i;
            if (c == one) {
                set(pos);
            } else if (c == zero) {
                reset(pos);
            } else {
                NEFORCE_THROW_EXCEPTION(value_exception("bitset string ctor: invalid character"));
            }
        }
    }

    /**
     * @brief string初始化
     * @param str string对象
     * @param zero 表示0的字符
     * @param one 表示1的字符
     */
    constexpr explicit bitset(const string& str, const char zero = '0', const char one = '1') :
    bitset(str.view(), zero, one) {}

    /**
     * @brief C风格字符串初始化
     * @param str C风格字符串
     * @param zero 表示0的字符
     * @param one 表示1的字符
     */
    constexpr explicit bitset(const char* str, const char zero = '0', const char one = '1') :
    bitset(string_view{str}, zero, one) {}

    /**
     * @brief 将所有位设置为1
     * @return 自身引用
     */
    constexpr bitset& set() noexcept {
        for (auto& b: blocks) {
            b = ~static_cast<block_type>(0ULL);
        }
        blocks[block_count - 1] &= last_block_mask();
        return *this;
    }

    /**
     * @brief 将指定位置的位设置为指定值
     * @param pos 位位置
     * @param value 要设置的值，默认为true
     * @return 自身引用
     */
    constexpr bitset& set(const size_t pos, const bool value = true) noexcept {
        check_range(pos);
        const size_t idx = block_index(pos);
        const size_t bit = bit_index(pos);
        if (value) {
            blocks[idx] |= (static_cast<block_type>(1) << bit);
        } else {
            blocks[idx] &= ~(static_cast<block_type>(1) << bit);
        }
        return *this;
    }

    /**
     * @brief 将所有位重置为0
     * @return 自身引用
     */
    constexpr bitset& reset() noexcept {
        blocks.fill(0);
        return *this;
    }

    /**
     * @brief 将指定位置的位重置为0
     * @param pos 位位置
     * @return 自身引用
     */
    constexpr bitset& reset(const size_t pos) noexcept {
        check_range(pos);
        const size_t idx = block_index(pos);
        const size_t bit = bit_index(pos);
        blocks[idx] &= ~(static_cast<block_type>(1) << bit);
        return *this;
    }

    /**
     * @brief 翻转所有位
     * @return 自身引用
     */
    constexpr bitset& flip() noexcept {
        for (auto& b: blocks) {
            b = ~b;
        }
        blocks[block_count - 1] &= last_block_mask();
        return *this;
    }

    /**
     * @brief 翻转指定位置的位
     * @param pos 位位置
     * @return 自身引用
     */
    constexpr bitset& flip(const size_t pos) noexcept {
        check_range(pos);
        const size_t idx = block_index(pos);
        const size_t bit = bit_index(pos);
        blocks[idx] ^= (static_cast<block_type>(1) << bit);
        return *this;
    }

    /**
     * @brief 常量下标访问
     * @param pos 位位置
     * @return 该位的bool值
     */
    NEFORCE_NODISCARD constexpr bool operator[](const size_t pos) const noexcept { return test(pos); }

    /**
     * @brief 非常量下标访问
     * @param pos 位位置
     * @return 位引用对象，可用于修改
     */
    NEFORCE_NODISCARD constexpr reference operator[](size_t pos) noexcept {
        check_range(pos);
        return reference(*this, pos);
    }

    /**
     * @brief 获取位数
     * @return 模板参数N
     */
    NEFORCE_NODISCARD NEFORCE_ALWAYS_INLINE constexpr size_t size() const noexcept { return N; }

    /**
     * @brief 检查是否为空
     * @return 如果N为0返回true
     */
    NEFORCE_NODISCARD NEFORCE_ALWAYS_INLINE constexpr bool empty() const noexcept { return N == 0; }

    /**
     * @brief 测试指定位置的位
     * @param position 位位置
     * @return 位的值
     */
    NEFORCE_NODISCARD constexpr bool test(const size_t position) const noexcept {
        check_range(position);
        const size_t idx = block_index(position);
        const size_t bit = bit_index(position);
        return (blocks[idx] & (static_cast<block_type>(1) << bit)) != 0;
    }

    /**
     * @brief 按位与赋值
     * @param other 另一个bitset
     * @return 自身引用
     */
    NEFORCE_NODISCARD constexpr bitset& operator&=(const bitset& other) noexcept {
        for (size_t i = 0; i < block_count; ++i) {
            blocks[i] &= other.blocks[i];
        }
        return *this;
    }

    /**
     * @brief 按位或赋值
     * @param other 另一个bitset
     * @return 自身引用
     */
    NEFORCE_NODISCARD constexpr bitset& operator|=(const bitset& other) noexcept {
        for (size_t i = 0; i < block_count; ++i) {
            blocks[i] |= other.blocks[i];
        }
        return *this;
    }

    /**
     * @brief 按位异或赋值
     * @param other 另一个bitset
     * @return 自身引用
     */
    NEFORCE_NODISCARD constexpr bitset& operator^=(const bitset& other) noexcept {
        for (size_t i = 0; i < block_count; ++i) {
            blocks[i] ^= other.blocks[i];
        }
        return *this;
    }

    /**
     * @brief 按位取反
     * @return 取反后的新bitset
     */
    NEFORCE_NODISCARD constexpr bitset operator~() const noexcept {
        bitset res = *this;
        res.flip();
        return res;
    }

    /**
     * @brief 左移赋值
     * @param pos 左移位数
     * @return 自身引用
     */
    NEFORCE_NODISCARD constexpr bitset& operator<<=(const uint32_t pos) noexcept {
        if (pos >= N) {
            reset();
            return *this;
        }
        const size_t block_shift = pos / bits_per_block;
        const size_t bit_shift = pos % bits_per_block;

        if (block_shift > 0) {
            for (size_t i = block_count - 1; i >= block_shift; --i) {
                blocks[i] = blocks[i - block_shift];
            }
            for (size_t i = 0; i < block_shift; ++i) {
                blocks[i] = 0;
            }
        }
        if (bit_shift > 0) {
            for (size_t i = block_count - 1; i > 0; --i) {
                blocks[i] = (blocks[i] << bit_shift) | (blocks[i - 1] >> (bits_per_block - bit_shift));
            }
            blocks[0] <<= bit_shift;
        }
        blocks[block_count - 1] &= last_block_mask();

        return *this;
    }

    /**
     * @brief 右移赋值
     * @param pos 右移位数
     * @return 自身引用
     */
    NEFORCE_NODISCARD constexpr bitset& operator>>=(const uint32_t pos) noexcept {
        if (pos >= N) {
            reset();
            return *this;
        }
        const size_t block_shift = pos / bits_per_block;
        const size_t bit_shift = pos % bits_per_block;

        if (block_shift > 0) {
            for (size_t i = 0; i < block_count - block_shift; ++i) {
                blocks[i] = blocks[i + block_shift];
            }
            for (size_t i = block_count - block_shift; i < block_count; ++i) {
                blocks[i] = 0;
            }
        }
        if (bit_shift > 0) {
            for (size_t i = 0; i < block_count - 1; ++i) {
                blocks[i] = (blocks[i] >> bit_shift) | (blocks[i + 1] << (bits_per_block - bit_shift));
            }
            blocks[block_count - 1] >>= bit_shift;
            blocks[block_count - 1] &= last_block_mask();
        }
        return *this;
    }

    /**
     * @brief 统计值为1的位的数量
     * @return 1的位数
     */
    NEFORCE_NODISCARD constexpr size_t count() const noexcept {
        size_t cnt = 0;
        for (size_t i = 0; i < block_count; ++i) {
            cnt += _NEFORCE popcount(blocks[i]);
        }
        return cnt;
    }

    /**
     * @brief 检查所有位是否都为1
     * @return 如果所有位都是1返回true
     */
    NEFORCE_NODISCARD constexpr bool all() const noexcept {
        for (size_t i = 0; i < block_count - 1; ++i) {
            if (blocks[i] != ~static_cast<block_type>(0ULL)) {
                return false;
            }
        }
        return blocks[block_count - 1] == last_block_mask();
    }

    /**
     * @brief 检查是否存在值为1的位
     * @return 如果至少有一个1返回true
     */
    NEFORCE_NODISCARD constexpr bool any() const noexcept {
        for (auto b: blocks) {
            if (b != 0) {
                return true;
            }
        }
        return false;
    }

    /**
     * @brief 检查是否所有位都是0
     * @return 如果没有1返回true
     */
    NEFORCE_NODISCARD constexpr bool none() const noexcept { return !any(); }

    /**
     * @brief 转换为unsigned long
     * @return unsigned long表示的位值
     * @throw value_exception 如果值超出unsigned long范围
     */
    NEFORCE_NODISCARD constexpr unsigned long to_ulong() const noexcept {
        NEFORCE_IF_CONSTEXPR(N > sizeof(unsigned long) * 8) {
            constexpr size_t ulong_blocks = (sizeof(unsigned long) * 8 + bits_per_block - 1) / bits_per_block;
            for (size_t i = ulong_blocks; i < block_count; ++i) {
                if (blocks[i] != 0) {
                    NEFORCE_THROW_EXCEPTION(value_exception("bitset to_ulong overflow"));
                }
            }

            constexpr size_t ulong_bits = sizeof(unsigned long) * 8;
            constexpr size_t remainder_bits = ulong_bits % bits_per_block;
            NEFORCE_IF_CONSTEXPR(remainder_bits != 0) {
                constexpr size_t last_ulong_block = ulong_bits / bits_per_block;
                block_type mask = (~static_cast<block_type>(0ULL)) << remainder_bits;
                if ((blocks[last_ulong_block] & mask) != 0) {
                    NEFORCE_THROW_EXCEPTION(value_exception("bitset to_ulong overflow"));
                }
            }
        }

        NEFORCE_IF_CONSTEXPR(sizeof(unsigned long) >= sizeof(block_type)) {
            return static_cast<unsigned long>(blocks[0]);
        }
        else {
            unsigned long result = 0;
            constexpr size_t ulong_blocks = (sizeof(unsigned long) * 8 + bits_per_block - 1) / bits_per_block;
            constexpr size_t blocks_to_use = ulong_blocks < block_count ? ulong_blocks : block_count;
            for (size_t i = 0; i < blocks_to_use; ++i) {
                result |= static_cast<unsigned long>(blocks[i]) << (i * bits_per_block);
            }
            return result;
        }
    }

    /**
     * @brief 转换为unsigned long long
     * @return unsigned long long表示的位值
     * @throw value_exception 如果值超出unsigned long long范围
     */
    NEFORCE_NODISCARD constexpr unsigned long long to_ullong() const noexcept {
        NEFORCE_IF_CONSTEXPR(N > sizeof(unsigned long long) * 8) {
            constexpr size_t ullong_blocks = (sizeof(unsigned long long) * 8 + bits_per_block - 1) / bits_per_block;
            for (size_t i = ullong_blocks; i < block_count; ++i) {
                if (blocks[i] != 0) {
                    NEFORCE_THROW_EXCEPTION(value_exception("bitset to_ullong overflow"));
                }
            }

            constexpr size_t ullong_bits = sizeof(unsigned long long) * 8;
            constexpr size_t remainder_bits = ullong_bits % bits_per_block;
            NEFORCE_IF_CONSTEXPR(remainder_bits != 0) {
                constexpr size_t last_ullong_block = ullong_bits / bits_per_block;
                block_type mask = (~static_cast<block_type>(0ULL)) << remainder_bits;
                if ((blocks[last_ullong_block] & mask) != 0) {
                    NEFORCE_THROW_EXCEPTION(value_exception("bitset to_ullong overflow"));
                }
            }
        }

        NEFORCE_IF_CONSTEXPR(sizeof(unsigned long long) >= sizeof(block_type)) {
            return static_cast<unsigned long long>(blocks[0]);
        }
        else {
            unsigned long long result = 0;
            constexpr size_t ullong_blocks = (sizeof(unsigned long long) * 8 + bits_per_block - 1) / bits_per_block;
            constexpr size_t blocks_to_use = ullong_blocks < block_count ? ullong_blocks : block_count;
            for (size_t i = 0; i < blocks_to_use; ++i) {
                result |= static_cast<unsigned long long>(blocks[i]) << (i * bits_per_block);
            }
            return result;
        }
    }

    /**
     * @brief 相等比较操作符
     * @param other 另一个bitset
     * @return 如果所有位相等返回true
     */
    NEFORCE_NODISCARD constexpr bool operator==(const bitset& other) const noexcept { return blocks == other.blocks; }

    /**
     * @brief 小于比较操作符（按字典序）
     * @param other 另一个bitset
     * @return 比较结果
     */
    NEFORCE_NODISCARD constexpr bool operator<(const bitset& other) const noexcept { return blocks < other.blocks; }

    /**
     * @brief 计算哈希值
     * @return 哈希值
     */
    NEFORCE_NODISCARD constexpr size_t to_hash() const noexcept { return blocks.to_hash(); }

    /**
     * @brief 转换为字符串
     * @param zero 表示0的字符
     * @param one 表示1的字符
     * @return 字符串表示，高位在左
     */
    NEFORCE_NODISCARD NEFORCE_CONSTEXPR20 string to_string(const char zero, const char one) const {
        string result;
        result.reserve(N);
        for (size_t i = N; i > 0; --i) {
            result.push_back(test(i - 1) ? one : zero);
        }
        return result;
    }

    /**
     * @brief 转换为字符串（默认使用'0'和'1'）
     * @return 字符串表示
     */
    NEFORCE_NODISCARD NEFORCE_CONSTEXPR20 string to_string() const { return bitset::to_string('0', '1'); }

    /**
     * @brief 交换两个bitset的内容
     * @param other 另一个bitset
     */
    constexpr void swap(bitset& other) noexcept { blocks.swap(other.blocks); }
};

/** @} */ // BitManipulation

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_CORE_CONTAINER_BITSET_HPP__
