#ifndef NEFORCE_CORE_UTILITY_COMPRESSED_PAIR_HPP__
#define NEFORCE_CORE_UTILITY_COMPRESSED_PAIR_HPP__

/**
 * @file compressed_pair.hpp
 * @brief 压缩对实现
 *
 * 此文件提供了压缩对的实现，使用空基类优化技术，
 * 当第一个类型为空时进行存储优化，减少内存占用。
 */

#include "NeForce/core/interface/icommon.hpp"
NEFORCE_BEGIN_NAMESPACE__

/**
 * @defgroup CompressedPair 压缩对
 * @brief 使用空基类优化的键值对实现
 * @{
 */

/**
 * @struct compressed_pair
 * @brief 压缩对主模板，使用EBCO优化
 * @tparam IfEmpty 第一个类型，可能为空
 * @tparam T 第二个类型
 * @tparam Compressed 是否启用压缩
 *
 * 使用空基类优化技术，当第一个类型为空时，通过继承来优化存储空间。
 */
template <typename IfEmpty, typename T, bool Compressed = is_empty_v<IfEmpty> && !is_final_v<IfEmpty>>
struct compressed_pair final : IfEmpty, icommon<compressed_pair<IfEmpty, T, Compressed>> {
    using base_type = IfEmpty; ///< 基类类型

    T value; ///< 存储的值

    /**
     * @brief 默认构造函数
     */
    constexpr compressed_pair() noexcept(is_nothrow_default_constructible_v<T>) :
    value() {}

    /**
     * @brief 拷贝构造函数
     * @param p 要拷贝的压缩对
     */
    constexpr compressed_pair(const compressed_pair& p) noexcept(is_nothrow_copy_constructible_v<T>) :
    value(p.value) {}

    /**
     * @brief 拷贝赋值运算符
     * @param pir 要拷贝的压缩对
     * @return 当前压缩对的引用
     */
    constexpr compressed_pair& operator=(const compressed_pair& pir) noexcept(is_nothrow_copy_assignable_v<T>) {
        value = _NEFORCE move(pir.value);
        return *this;
    }

    /**
     * @brief 移动构造函数
     * @param p 要移动的压缩对
     */
    constexpr compressed_pair(compressed_pair&& p) noexcept(is_nothrow_move_constructible_v<T>) :
    value(_NEFORCE move(p.value)) {}

    /**
     * @brief 移动赋值运算符
     * @param pir 要移动的压缩对
     * @return 当前压缩对的引用
     */
    constexpr compressed_pair& operator=(compressed_pair&& pir) noexcept(is_nothrow_move_assignable_v<T>) {
        value = _NEFORCE move(pir.value);
        return *this;
    }

    /**
     * @brief 默认构造标签构造函数
     * @tparam Args 参数类型
     * @param args 构造参数
     *
     * 使用默认构造的基类和给定的参数构造值。
     */
    template <typename... Args, enable_if_t<is_constructible_v<T, Args...>, int> = 0>
    constexpr explicit compressed_pair(default_construct_tag, Args&&... args) noexcept(
            conjunction_v<is_nothrow_default_constructible<IfEmpty>, is_nothrow_constructible<T, Args...>>) :
    IfEmpty(),
    value(_NEFORCE forward<Args>(args)...) {}

    /**
     * @brief 精确参数构造标签构造函数
     * @tparam ToEmpty 基类参数类型
     * @tparam Args 值参数类型
     * @param first 基类构造参数
     * @param args 值构造参数
     *
     * 使用给定的参数构造基类和值。
     */
    template <typename ToEmpty, typename... Args,
              enable_if_t<conjunction_v<is_constructible<IfEmpty, ToEmpty>, is_constructible<T, Args...>>, int> = 0>
    constexpr explicit compressed_pair(exact_arg_construct_tag, ToEmpty&& first, Args&&... args) noexcept(
            conjunction_v<is_nothrow_constructible<IfEmpty, ToEmpty>, is_nothrow_constructible<T, Args...>>) :
    IfEmpty(_NEFORCE forward<ToEmpty>(first)),
    value(_NEFORCE forward<Args>(args)...) {}

    /**
     * @brief 获取基类引用
     * @return 基类的引用
     */
    constexpr compressed_pair& get_base() & noexcept { return *this; }

    /**
     * @brief 获取基类常量引用
     * @return 基类的常量引用
     */
    constexpr const compressed_pair& get_base() const& noexcept { return *this; }

    /**
     * @brief 获取基类引用
     * @return 基类的引用
     */
    constexpr compressed_pair&& get_base() && noexcept { return _NEFORCE move(*this); }

    /**
     * @brief 获取基类常量引用
     * @return 基类的常量引用
     */
    constexpr const compressed_pair&& get_base() const&& noexcept { return _NEFORCE move(*this); }

    /**
     * @brief 交换两个压缩对
     * @param rhs 要交换的压缩对
     */
    constexpr void swap(compressed_pair& rhs) noexcept(is_nothrow_swappable_v<T>) { _NEFORCE swap(value, rhs.value); }

    /**
     * @brief 计算哈希值
     * @return 值的哈希值
     */
    constexpr size_t to_hash() const noexcept(noexcept(hash<T>{}(value))) { return hash<T>{}(value); }

    /**
     * @brief 相等比较运算符
     * @param y 要比较的压缩对
     * @return 如果值相等返回true，否则返回false
     */
    NEFORCE_NODISCARD constexpr bool equal_to(const compressed_pair& y) const noexcept(noexcept(this->value == y.value)) {
        return this->value == y.value;
    }

    /**
     * @brief 小于比较运算符
     * @param y 要比较的压缩对
     * @return 如果当前值小于另一个值返回true，否则返回false
     */
    NEFORCE_NODISCARD constexpr bool less_than(const compressed_pair& y) const noexcept(noexcept(this->value < y.value)) {
        return this->value < y.value;
    }
};


/**
 * @brief 压缩对特化，未启用EBCO优化
 * @tparam IfEmpty 第一个类型
 * @tparam T 第二个类型
 *
 * 当第一个类型不为空或是final类时，不使用EBCO优化，正常存储两个成员。
 */
template <typename IfEmpty, typename T>
struct compressed_pair<IfEmpty, T, false> final : icommon<compressed_pair<IfEmpty, T, false>> {
    IfEmpty no_compressed; ///< 未压缩的基类成员
    T value;               ///< 存储的值

    /**
     * @brief 默认构造函数
     */
    constexpr compressed_pair() noexcept(
            conjunction_v<is_nothrow_default_constructible<IfEmpty>, is_nothrow_default_constructible<T>>) :
    no_compressed(),
    value() {}

    /**
     * @brief 拷贝构造函数
     * @param pir 要拷贝的压缩对
     */
    constexpr compressed_pair(const compressed_pair& pir) noexcept(
            conjunction_v<is_nothrow_copy_constructible<IfEmpty>, is_nothrow_copy_constructible<T>>) :
    no_compressed(pir.no_compressed),
    value(pir.value) {}

    /**
     * @brief 拷贝赋值运算符
     * @param pir 要拷贝的压缩对
     * @return 当前压缩对的引用
     */
    constexpr compressed_pair& operator=(const compressed_pair& pir) noexcept(
            conjunction_v<is_nothrow_copy_assignable<IfEmpty>, is_nothrow_copy_assignable<T>>) {
        no_compressed = pir.no_compressed;
        value = pir.value;
        return *this;
    }

    /**
     * @brief 移动构造函数
     * @param pir 要移动的压缩对
     */
    constexpr compressed_pair(compressed_pair&& pir) noexcept(
            conjunction_v<is_nothrow_move_constructible<IfEmpty>, is_nothrow_move_constructible<T>>) :
    no_compressed(_NEFORCE move(pir.no_compressed)),
    value(_NEFORCE move(pir.value)) {}

    /**
     * @brief 移动赋值运算符
     * @param pir 要移动的压缩对
     * @return 当前压缩对的引用
     */
    constexpr compressed_pair& operator=(compressed_pair&& pir) noexcept(
            conjunction_v<is_nothrow_move_assignable<IfEmpty>, is_nothrow_move_assignable<T>>) {
        no_compressed = _NEFORCE move(pir.no_compressed);
        value = _NEFORCE move(pir.value);
        return *this;
    }

    /**
     * @brief 默认构造标签构造函数
     * @tparam Args 参数类型
     * @param args 构造参数
     */
    template <typename... Args>
    constexpr explicit compressed_pair(default_construct_tag, Args&&... args) noexcept(
            conjunction_v<is_nothrow_default_constructible<IfEmpty>, is_nothrow_constructible<T, Args...>>) :
    no_compressed(),
    value(_NEFORCE forward<Args>(args)...) {}

    /**
     * @brief 精确参数构造标签构造函数
     * @tparam ToEmpty 基类参数类型
     * @tparam Args 值参数类型
     * @param first 基类构造参数
     * @param args 值构造参数
     */
    template <typename ToEmpty, typename... Args>
    constexpr compressed_pair(exact_arg_construct_tag, ToEmpty&& first, Args&&... args) noexcept(
            conjunction_v<is_nothrow_constructible<IfEmpty, ToEmpty>, is_nothrow_constructible<T, Args...>>) :
    no_compressed(_NEFORCE forward<ToEmpty>(first)),
    value(_NEFORCE forward<Args>(args)...) {}

    /**
     * @brief 获取基类引用
     * @return 基类的引用
     */
    constexpr IfEmpty& get_base() & noexcept { return no_compressed; }

    /**
     * @brief 获取基类常量引用
     * @return 基类的常量引用
     */
    constexpr const IfEmpty& get_base() const& noexcept { return no_compressed; }

    /**
     * @brief 获取基类引用
     * @return 基类的引用
     */
    constexpr IfEmpty&& get_base() && noexcept { return _NEFORCE move(no_compressed); }

    /**
     * @brief 获取基类常量引用
     * @return 基类的常量引用
     */
    constexpr const IfEmpty&& get_base() const&& noexcept { return _NEFORCE move(no_compressed); }

    /**
     * @brief 交换两个压缩对
     * @param rhs 要交换的压缩对
     */
    constexpr void
    swap(compressed_pair& rhs) noexcept(conjunction_v<is_nothrow_swappable<IfEmpty>, is_nothrow_swappable<T>>) {
        _NEFORCE swap(value, rhs.value);
        _NEFORCE swap(no_compressed, rhs.no_compressed);
    }

    /**
     * @brief 计算哈希值
     * @return 基类和值的组合哈希值
     */
    constexpr size_t to_hash() const noexcept(noexcept(hash<IfEmpty>{}(no_compressed) ^ hash<T>{}(value))) {
        return hash<IfEmpty>{}(no_compressed) ^ hash<T>{}(value);
    }

    /**
     * @brief 相等比较运算符
     * @param y 要比较的压缩对
     * @return 如果基类和值都相等返回true，否则返回false
     */
    NEFORCE_NODISCARD constexpr bool equal_to(const compressed_pair& y) const
            noexcept(noexcept(this->no_compressed == y.no_compressed && this->value == y.value)) {
        return this->no_compressed == y.no_compressed && this->value == y.value;
    }

    /**
     * @brief 小于比较运算符
     * @param y 要比较的压缩对
     * @return 如果当前压缩对小于另一个压缩对返回true，否则返回false
     */
    NEFORCE_NODISCARD constexpr bool less_than(const compressed_pair& y) const
            noexcept(noexcept(this->no_compressed < y.no_compressed ||
                              (!(y.no_compressed < this->no_compressed) && this->value < y.value))) {
        return this->no_compressed < y.no_compressed ||
               (!(y.no_compressed < this->no_compressed) && this->value < y.value);
    }
};

#ifdef NEFORCE_STANDARD_17
template <typename IfEmpty, typename T>
compressed_pair(IfEmpty, T) -> compressed_pair<IfEmpty, T>;
#endif

/** @} */ // CompressedPair

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_CORE_UTILITY_COMPRESSED_PAIR_HPP__
