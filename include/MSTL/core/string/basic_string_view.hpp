#ifndef MSTL_CORE_STRING_BASIC_STRING_VIEW_HPP__
#define MSTL_CORE_STRING_BASIC_STRING_VIEW_HPP__

/**
 * @file basic_string_view.hpp
 * @brief MSTL字符串视图容器
 *
 * 此文件提供了字符串视图容器的实现。
 * string_view是一个非拥有（non-owning）的字符串视图，提供对字符串的只读访问，
 * 不进行内存分配，性能高效。适用于函数参数传递，避免不必要的拷贝。
 */

#include "MSTL/core/interface/iiterator.hpp"
#include "MSTL/core/iterator/reverse_iterator.hpp"
#include "MSTL/core/string/char_traits.hpp"
#include "MSTL/core/string/char_types.hpp"
MSTL_BEGIN_NAMESPACE__

/**
 * @defgroup StringView 字符串视图
 * @brief 非拥有只读字符串视图
 * @{
 */

/// @cond
template <typename CharT, typename Traits = char_traits<CharT>>
class basic_string_view;
/// @endcond


/**
 * @struct basic_string_view_iterator
 * @brief 字符串视图迭代器
 * @tparam Traits 字符特征类型
 *
 * 提供对字符串视图元素的随机访问迭代器支持。
 * 所有操作都是const的，因为string_view是只读的。
 */
template <typename Traits>
struct basic_string_view_iterator : iiterator<basic_string_view_iterator<Traits>> {
public:
    using container_type	= basic_string_view<typename Traits::char_type, Traits>;  ///< 容器类型
    using value_type		= typename container_type::value_type;  ///< 值类型
    using size_type			= typename container_type::size_type;  ///< 大小类型
    using difference_type	= typename container_type::difference_type;  ///< 差值类型
    using iterator_category = contiguous_iterator_tag;  ///< 迭代器类别
    using reference			= typename container_type::const_reference;  ///< 引用类型
    using pointer			= typename container_type::const_pointer;  ///< 指针类型

private:
    pointer data_ = nullptr;  ///< 数据指针
    size_t size_ = 0;  ///< 总大小
    size_t idx_ = 0;  ///< 当前索引

public:
    constexpr basic_string_view_iterator() noexcept = default;
    MSTL_CONSTEXPR20 ~basic_string_view_iterator() = default;

    constexpr basic_string_view_iterator(const basic_string_view_iterator&) noexcept = default;
    constexpr basic_string_view_iterator& operator =(const basic_string_view_iterator&) noexcept = default;
    constexpr basic_string_view_iterator(basic_string_view_iterator&&) noexcept = default;
    constexpr basic_string_view_iterator& operator =(basic_string_view_iterator&&) noexcept = default;

    /**
     * @brief 构造函数
     * @param data 数据指针
     * @param size 总大小
     * @param off 初始偏移
     */
    constexpr basic_string_view_iterator(const pointer data, const size_t size, const size_t off) noexcept
    : data_(data), size_(size), idx_(off) {}

    /**
     * @brief 解引用操作
     * @return 当前元素的引用
     */
    MSTL_NODISCARD constexpr reference dereference() const noexcept {
        MSTL_DEBUG_VERIFY(data_, "Attempting to dereference on a null pointer");
        MSTL_DEBUG_VERIFY(idx_ < size_, "Attempting to dereference out of boundary");
        return data_[idx_];
    }

    /**
     * @brief 递增操作
     */
    constexpr void increment() noexcept {
        MSTL_DEBUG_VERIFY(data_, "Attempting to increment a null pointer");
        MSTL_DEBUG_VERIFY(idx_ < size_, "Attempting to increment out of boundary");
        ++idx_;
    }

    /**
     * @brief 递减操作
     */
    constexpr void decrement() noexcept {
        MSTL_DEBUG_VERIFY(data_, "Attempting to decrement a null pointer");
        MSTL_DEBUG_VERIFY(idx_ != 0, "Attempting to decrement out of boundary");
        --idx_;
    }

    /**
     * @brief 前进操作
     * @param off 前进距离
     */
    constexpr void advance(difference_type off) noexcept {
        MSTL_DEBUG_VERIFY(data_ || off == 0, "Attempting to advance a null pointer");
        MSTL_DEBUG_VERIFY(
            (off < 0 ? idx_ >= -off : size_ - idx_ >= off),
            "Attempting to advance out of boundary");
        idx_ += off;
    }

    /**
     * @brief 计算距离操作
     * @param other 另一个迭代器
     * @return 两个迭代器之间的距离
     */
    MSTL_NODISCARD constexpr difference_type distance_to(const basic_string_view_iterator& other) const noexcept {
        MSTL_DEBUG_VERIFY(
            data_ == other.data_ && size_ == other.size_,
            "Attempting to distance to a different container");
        return static_cast<difference_type>(idx_ - other.idx_);
    }

    /**
     * @brief 下标访问操作符
     * @param n 偏移量
     * @return 偏移位置元素的引用
     */
    MSTL_NODISCARD constexpr reference operator [](const difference_type n) const noexcept {
        return *(*this + n);
    }

    /**
     * @brief 相等比较
     * @param rhs 右侧迭代器
     * @return 是否相等
     */
    MSTL_NODISCARD constexpr bool equal(const basic_string_view_iterator& rhs) const noexcept {
        MSTL_DEBUG_VERIFY(data_ == rhs.data_ && size_ == rhs.size_, "Attempting to equal to a different container");
        return idx_ == rhs.idx_;
    }

    /**
     * @brief 小于比较
     * @param rhs 右侧迭代器
     * @return 当前迭代器是否在rhs之前
     */
    MSTL_NODISCARD constexpr bool less_than(const basic_string_view_iterator& rhs) const noexcept {
        MSTL_DEBUG_VERIFY(
            data_ == rhs.data_ && size_ == rhs.size_,
            "Attempting to less than a different container");
        return idx_ < rhs.idx_;
    }

    /**
     * @brief 获取底层指针
     * @return 当前元素指针
     */
    MSTL_NODISCARD constexpr pointer base() const noexcept {
        return data_ + idx_;
    }
};


/**
 * @class basic_string_view
 * @brief 基本字符串视图模板
 * @tparam CharT 字符类型
 * @tparam Traits 字符特征类型，默认为char_traits<CharT>
 *
 * string_view是一个非拥有的字符串视图，提供对字符序列的只读访问。
 * 它不进行内存分配，不拥有所指向的字符串，因此生命周期必须由调用者保证。
 * 提供类似string的接口，但所有操作都是const的。
 */
template <typename CharT, typename Traits>
class basic_string_view : public icommon<basic_string_view<CharT, Traits>> {
    static_assert(
        is_same_v<CharT, typename Traits::char_type>,
        "char type of basic string view should be same with char traits.");
    static_assert(
        !is_array_v<CharT> && is_trivial_v<CharT> && is_standard_layout_v<CharT>,
        "basic string view only contains non-array trivial standard-layout types.");

public:
    using value_type        = CharT;  ///< 值类型
    using pointer           = const CharT*;  ///< 指针类型
    using reference         = const CharT&;  ///< 引用类型
    using const_pointer     = const CharT*;  ///< 常量指针类型
    using const_reference   = const CharT&;  ///< 常量引用类型
    using size_type         = size_t;  ///< 大小类型
    using difference_type   = ptrdiff_t;  ///< 差值类型
    using const_iterator            = basic_string_view_iterator<Traits>;  ///< 常量迭代器类型
    using iterator                  = const_iterator;  ///< 迭代器类型
    using const_reverse_iterator    = _MSTL reverse_iterator<const_iterator>;  ///< 常量反向迭代器类型
    using reverse_iterator          = const_reverse_iterator;  ///< 反向迭代器类型
    using traits_type               = Traits;  ///< 字符特征类型

    /// 特殊值，表示未找到或直到末尾
    static constexpr auto npos = static_cast<size_type>(-1);

private:
    const_pointer data_ = "";  ///< 数据指针
    size_type size_ = 0;  ///< 字符串长度

    /**
     * @brief 限制大小不超过可用范围
     * @param position 起始位置
     * @param size 请求大小
     * @return 实际可用大小
     */
    MSTL_NODISCARD MSTL_ALWAYS_INLINE
    constexpr size_type clamp_size(const size_type position, const size_type size) const noexcept {
        return _MSTL min(size, size_ - position);
    }

public:
    constexpr basic_string_view() noexcept = default;
    constexpr basic_string_view(const basic_string_view&) noexcept = default;
    constexpr basic_string_view& operator =(const basic_string_view&) noexcept = default;

    /**
     * @brief 从C风格字符串构造
     * @param str 以空字符结尾的C风格字符串
     *
     * 注意：字符串长度通过Traits::length计算，要求字符串以空字符结尾。
     */
    constexpr basic_string_view(const_pointer str) noexcept
    : data_(str), size_(Traits::length(str)) {}

    /**
     * @brief 从字符数组构造（指定长度）
     * @param str 字符数组指针
     * @param n 字符数量
     *
     * 不要求字符串以空字符结尾，可以包含空字符。
     */
    constexpr basic_string_view(const_pointer str, const size_type n) noexcept
    : data_(str), size_(n) {}

    /**
     * @brief 从迭代器范围构造
     * @tparam Iterator 迭代器类型
     * @param start 起始迭代器
     * @param finish 结束迭代器
     *
     * 要求迭代器指向的值为value_type类型。
     */
    template <typename Iterator, enable_if_t<is_same_v<iter_value_t<Iterator>, value_type>, int> = 0>
    constexpr basic_string_view(Iterator start, Iterator finish)
    : data_(&*start), size_(_MSTL distance(start, finish)) {}

    /**
     * @brief 析构函数
     */
    MSTL_CONSTEXPR20 ~basic_string_view() noexcept = default;

    /**
     * @brief 获取起始迭代器
     * @return 指向第一个字符的迭代器
     */
    MSTL_NODISCARD constexpr const_iterator begin() const noexcept {
        return const_iterator(data_, size_, 0);
    }

    /**
     * @brief 获取结束迭代器
     * @return 指向最后一个字符之后位置的迭代器
     */
    MSTL_NODISCARD constexpr const_iterator end() const noexcept {
        return const_iterator(data_, size_, size_);
    }

    /**
     * @brief 获取常量起始迭代器
     * @return 指向第一个字符的常量迭代器
     */
    MSTL_NODISCARD constexpr const_iterator cbegin() const noexcept {
        return begin();
    }

    /**
     * @brief 获取常量结束迭代器
     * @return 指向最后一个字符之后位置的常量迭代器
     */
    MSTL_NODISCARD constexpr const_iterator cend() const noexcept {
        return end();
    }

    /**
     * @brief 获取反向起始迭代器
     * @return 指向最后一个字符的反向迭代器
     */
    MSTL_NODISCARD constexpr const_reverse_iterator rbegin() const noexcept {
        return const_reverse_iterator(end());
    }

    /**
     * @brief 获取反向结束迭代器
     * @return 指向第一个字符之前位置的反向迭代器
     */
    MSTL_NODISCARD constexpr const_reverse_iterator rend() const noexcept {
        return const_reverse_iterator(begin());
    }

    /**
     * @brief 获取常量反向起始迭代器
     * @return 指向最后一个字符的常量反向迭代器
     */
    MSTL_NODISCARD constexpr const_reverse_iterator crbegin() const noexcept {
        return rbegin();
    }

    /**
     * @brief 获取常量反向结束迭代器
     * @return 指向第一个字符之前位置的常量反向迭代器
     */
    MSTL_NODISCARD constexpr const_reverse_iterator crend() const noexcept {
        return rend();
    }

    /**
     * @brief 获取字符串长度
     * @return 字符数量
     */
    MSTL_NODISCARD constexpr size_type size() const noexcept {
        return size_;
    }

    /**
     * @brief 获取最大可能长度
     * @return 最大长度
     */
    MSTL_NODISCARD constexpr size_type max_size() const noexcept {
        return (npos - sizeof(size_type) - sizeof(void*)) / sizeof(value_type) / 4;
    }

    /**
     * @brief 获取字符串长度
     * @return 字符数量
     */
    MSTL_NODISCARD constexpr size_type length() const noexcept {
        return size_;
    }

    /**
     * @brief 检查是否为空
     * @return 是否为空
     */
    MSTL_NODISCARD constexpr bool empty() const noexcept {
        return size_ == 0;
    }

    /**
     * @brief 获取底层数据指针
     * @return 指向字符串数据的指针
     *
     * 注意：返回的指针不保证以空字符结尾，不应作为C风格字符串使用。
     */
    MSTL_NODISCARD constexpr const_pointer data() const noexcept {
        return data_;
    }

    /**
     * @brief 访问第一个字符
     * @return 第一个字符的引用
     */
    MSTL_NODISCARD constexpr const_reference front() const noexcept {
        MSTL_DEBUG_VERIFY(!empty(), "cannot call front on empty string_view");
        return data_[0];
    }

    /**
     * @brief 访问最后一个字符
     * @return 最后一个字符的引用
     */
    MSTL_NODISCARD constexpr const_reference back() const noexcept {
        MSTL_DEBUG_VERIFY(!empty(), "cannot call back on empty string_view");
        return data_[size_ - 1];
    }

    /**
     * @brief 下标访问操作符
     * @param n 索引
     * @return 指定位置的字符引用
     */
    MSTL_NODISCARD constexpr const_reference operator [](const size_type n) const noexcept {
        MSTL_DEBUG_VERIFY(n < size_, "basic string view index out of ranges.");
        return data_[n];
    }

    /**
     * @brief 带边界检查的访问
     * @param n 索引
     * @return 指定位置的字符引用
     */
    MSTL_NODISCARD constexpr const_reference at(const size_type n) const {
        MSTL_DEBUG_VERIFY(n < size_, "basic string view index out of ranges.");
        return data_[n];
    }

    /**
     * @brief 移除前缀
     * @param n 要移除的字符数
     *
     * 将视图的起始位置向前移动n个字符。
     */
    constexpr void remove_prefix(const size_type n) noexcept {
        MSTL_DEBUG_VERIFY(size_ >= n, "cannot remove prefix longer than total size");
        data_ += n;
        size_ -= n;
    }

    /**
     * @brief 移除后缀
     * @param n 要移除的字符数
     *
     * 将视图的结束位置向前移动n个字符。
     */
    constexpr void remove_suffix(const size_type n) noexcept {
        MSTL_DEBUG_VERIFY(size_ >= n, "cannot remove suffix longer than total size");
        size_ -= n;
    }

    /**
     * @brief 复制字符到目标缓冲区
     * @param str 目标缓冲区
     * @param count 要复制的字符数
     * @param off 起始偏移
     * @return 实际复制的字符数
     */
    constexpr size_type copy(CharT* const str, size_type count, const size_type off = 0) const {
        MSTL_DEBUG_VERIFY(off < size_, "basic string view index out of ranges.");
        count = clamp_size(off, count);
        Traits::copy(str, data_ + off, count);
        return count;
    }

    /**
     * @brief 获取子视图
     * @param off 起始偏移，默认为0
     * @param count 子视图长度，默认为npos
     * @return 子字符串视图
     */
    MSTL_NODISCARD constexpr basic_string_view substr(const size_type off = 0, size_type count = npos) const {
        MSTL_DEBUG_VERIFY(off < size_, "basic string view index out of ranges.");
        count = clamp_size(off, count);
        return basic_string_view(data_ + off, count);
    }

    /**
     * @brief 获取子视图
     * @param off 起始偏移
     * @param count 子视图长度
     * @return 子字符串视图
     */
    MSTL_NODISCARD constexpr basic_string_view view(const size_type off, size_type count = npos) const {
        return substr(off, count);
    }

    /**
     * @brief 比较字符串视图
     * @param view 要比较的字符串视图
     * @return 负值（*this<view）、0（相等）、正值（*this>view）
     */
    MSTL_NODISCARD constexpr int compare(const basic_string_view view) const noexcept {
        return (char_traits_compare<Traits>)(data_, size_, view.data_, view.size_);
    }

    /**
     * @brief 比较子串与字符串视图
     * @param off 起始偏移
     * @param n 子串长度
     * @param view 要比较的字符串视图
     * @return 比较结果
     */
    MSTL_NODISCARD constexpr int compare(const size_type off, const size_type n, const basic_string_view view) const {
        return substr(off, n).compare(view);
    }

    /**
     * @brief 比较子串与另一个子串
     * @param off 起始偏移
     * @param n 子串长度
     * @param view 要比较的字符串视图
     * @param roff 目标起始偏移
     * @param count 目标子串长度
     * @return 比较结果
     */
    MSTL_NODISCARD constexpr int compare(const size_type off, const size_type n, const basic_string_view view,
        const size_type roff, const size_type count) const {
        return substr(off, n).compare(view.substr(roff, count));
    }

    /**
     * @brief 比较与C风格字符串
     * @param str C风格字符串
     * @return 比较结果
     */
    MSTL_NODISCARD constexpr int compare(const CharT* const str) const noexcept {
        return compare(basic_string_view(str));
    }

    /**
     * @brief 比较子串与C风格字符串
     * @param off 起始偏移
     * @param n 子串长度
     * @param str C风格字符串
     * @return 比较结果
     */
    MSTL_NODISCARD constexpr int compare(const size_type off, const size_type n, const CharT* const str) const {
        return substr(off, n).compare(basic_string_view(str));
    }

    /**
     * @brief 比较子串与指定长度的字符数组
     * @param off 起始偏移
     * @param n 子串长度
     * @param str 字符数组
     * @param count 字符数组长度
     * @return 比较结果
     */
    MSTL_NODISCARD constexpr int compare(const size_type off, const size_type n,
        const CharT* const str, const size_type count) const {
        return substr(off, n).compare(basic_string_view(str, count));
    }

    /**
     * @brief 查找子串
     * @param view 要查找的子串
     * @param n 起始位置
     * @return 子串首次出现的位置，未找到则返回npos
     */
    MSTL_NODISCARD constexpr size_type find(const basic_string_view view, const size_type n = 0) const noexcept {
        return (char_traits_find<Traits>)(data_, size_, n, view.data_, view.size_);
    }

    /**
     * @brief 查找字符
     * @param chr 要查找的字符
     * @param n 起始位置
     * @return 字符首次出现的位置，未找到则返回npos
     */
    MSTL_NODISCARD constexpr size_type find(const CharT chr, const size_type n = 0) const noexcept {
        return (char_traits_find_char<Traits>)(data_, size_, n, chr);
    }

    /**
     * @brief 查找指定长度的子串
     * @param str 子串指针
     * @param off 起始位置
     * @param count 子串长度
     * @return 子串首次出现的位置，未找到则返回npos
     */
    MSTL_NODISCARD constexpr size_type find(const CharT* const str,
        const size_type off, const size_type count) const noexcept {
        return (char_traits_find<Traits>)(data_, size_, off, str, count);
    }

    /**
     * @brief 查找C风格字符串
     * @param str C风格字符串
     * @param off 起始位置
     * @return 子串首次出现的位置，未找到则返回npos
     */
    MSTL_NODISCARD constexpr size_type find(const CharT* const str, const size_type off = 0) const noexcept {
        return (char_traits_find<Traits>)(data_, size_, off, str, Traits::length(str));
    }

    /**
     * @brief 从后向前查找子串
     * @param view 要查找的子串
     * @param off 起始位置，默认为npos
     * @return 子串最后一次出现的位置，未找到则返回npos
     */
    MSTL_NODISCARD constexpr size_type rfind(const basic_string_view view, const size_type off = npos) const noexcept {
        return (char_traits_rfind<Traits>)(data_, size_, off, view.data_, view.size_);
    }

    /**
     * @brief 从后向前查找字符
     * @param chr 要查找的字符
     * @param n 起始位置，默认为npos
     * @return 字符最后一次出现的位置，未找到则返回npos
     */
    MSTL_NODISCARD constexpr size_type rfind(const CharT chr, const size_type n = npos) const noexcept {
        return (char_traits_rfind_char<Traits>)(data_, size_, n, chr);
    }

    /**
     * @brief 从后向前查找指定长度的子串
     * @param str 子串指针
     * @param off 起始位置
     * @param n 子串长度
     * @return 子串最后一次出现的位置，未找到则返回npos
     */
    MSTL_NODISCARD constexpr size_type rfind(const CharT* const str, const size_type off,
        const size_type n) const noexcept {
        return (char_traits_rfind<Traits>)(data_, size_, off, str, n);
    }

    /**
     * @brief 从后向前查找C风格字符串
     * @param str C风格字符串
     * @param off 起始位置
     * @return 子串最后一次出现的位置，未找到则返回npos
     */
    MSTL_NODISCARD constexpr size_type rfind(const CharT* const str, const size_type off = npos) const noexcept {
        return (char_traits_rfind<Traits>)(data_, size_, off, str, Traits::length(str));
    }

    /**
     * @brief 查找第一个出现在字符集合中的字符
     * @param view 字符集合
     * @param off 起始位置
     * @return 第一个匹配字符的位置，未找到则返回npos
     */
    MSTL_NODISCARD constexpr size_type find_first_of(const basic_string_view view, const size_type off = 0) const noexcept {
        return (char_traits_find_first_of<Traits>)(data_, size_, off, view.data_, view.size_);
    }

    /**
     * @brief 查找第一个等于指定字符的位置
     * @param chr 要查找的字符
     * @param off 起始位置
     * @return 第一个匹配字符的位置，未找到则返回npos
     */
    MSTL_NODISCARD constexpr size_type find_first_of(const CharT chr, const size_type off = 0) const noexcept {
        return (char_traits_find_char<Traits>)(data_, size_, off, chr);
    }

    /**
     * @brief 查找第一个出现在指定字符集合中的字符
     * @param str 字符集合指针
     * @param off 起始位置
     * @param n 集合长度
     * @return 第一个匹配字符的位置，未找到则返回npos
     */
    MSTL_NODISCARD constexpr size_type find_first_of(const CharT* const str, const size_type off,
        const size_type n) const noexcept {
        return (char_traits_find_first_of<Traits>)(data_, size_, off, str, n);
    }

    /**
     * @brief 查找第一个出现在C风格字符串集合中的字符
     * @param str C风格字符串集合
     * @param off 起始位置
     * @return 第一个匹配字符的位置，未找到则返回npos
     */
    MSTL_NODISCARD constexpr size_type find_first_of(const CharT* const str, const size_type off = 0) const noexcept {
        return (char_traits_find_first_of<Traits>)(data_, size_, off, str, Traits::length(str));
    }

    /**
     * @brief 查找最后一个出现在字符集合中的字符
     * @param view 字符集合
     * @param off 起始位置
     * @return 最后一个匹配字符的位置，未找到则返回npos
     */
    MSTL_NODISCARD constexpr size_type find_last_of(const basic_string_view view, const size_type off = npos) const noexcept {
        return (char_traits_find_last_of<Traits>)(data_, size_, off, view.data_, view.size_);
    }

    /**
     * @brief 查找最后一个等于指定字符的位置
     * @param chr 要查找的字符
     * @param off 起始位置
     * @return 最后一个匹配字符的位置，未找到则返回npos
     */
    MSTL_NODISCARD constexpr size_type find_last_of(const CharT chr, const size_type off = npos) const noexcept {
        return (char_traits_rfind_char<Traits>)(data_, size_, off, chr);
    }

    /**
     * @brief 查找最后一个出现在指定字符集合中的字符
     * @param str 字符集合指针
     * @param off 起始位置
     * @param n 集合长度
     * @return 最后一个匹配字符的位置，未找到则返回npos
     */
    MSTL_NODISCARD constexpr size_type find_last_of(const CharT* const str, const size_type off,
        const size_type n) const noexcept {
        return (char_traits_find_last_of<Traits>)(data_, size_, off, str, n);
    }

    /**
     * @brief 查找最后一个出现在C风格字符串集合中的字符
     * @param str C风格字符串集合
     * @param off 起始位置
     * @return 最后一个匹配字符的位置，未找到则返回npos
     */
    MSTL_NODISCARD constexpr size_type find_last_of(const CharT* const str, const size_type off = npos) const noexcept {
        return (char_traits_find_last_of<Traits>)(data_, size_, off, str, Traits::length(str));
    }

    /**
     * @brief 查找第一个不在字符集合中的字符
     * @param view 字符集合
     * @param off 起始位置
     * @return 第一个不匹配字符的位置，未找到则返回npos
     */
    MSTL_NODISCARD constexpr size_type find_first_not_of(const basic_string_view view,
        const size_type off = 0) const noexcept {
        return (char_traits_find_first_not_of<Traits>)(data_, size_, off, view.data_, view.size_);
    }

    /**
     * @brief 查找第一个不等于指定字符的位置
     * @param chr 指定字符
     * @param off 起始位置
     * @return 第一个不等于chr的位置，未找到则返回npos
     */
    MSTL_NODISCARD constexpr size_type find_first_not_of(const CharT chr, const size_type off = 0) const noexcept {
        return (char_traits_find_not_char<Traits>)(data_, size_, off, chr);
    }

    /**
     * @brief 查找第一个不在指定字符集合中的字符
     * @param str 字符集合指针
     * @param off 起始位置
     * @param n 集合长度
     * @return 第一个不匹配字符的位置，未找到则返回npos
     */
    MSTL_NODISCARD constexpr size_type find_first_not_of(const CharT* const str, const size_type off,
        const size_type n) const noexcept {
        return (char_traits_find_first_not_of<Traits>)(data_, size_, off, str, n);
    }

    /**
     * @brief 查找第一个不在C风格字符串集合中的字符
     * @param str C风格字符串集合
     * @param off 起始位置
     * @return 第一个不匹配字符的位置，未找到则返回npos
     */
    MSTL_NODISCARD constexpr size_type find_first_not_of(const CharT* const str, const size_type off = 0) const noexcept {
        return (char_traits_find_first_not_of<Traits>)(data_, size_, off, str, Traits::length(str));
    }

    /**
     * @brief 查找最后一个不在字符集合中的字符
     * @param view 字符集合
     * @param off 起始位置
     * @return 最后一个不匹配字符的位置，未找到则返回npos
     */
    MSTL_NODISCARD constexpr size_type find_last_not_of(const basic_string_view view,
        const size_type off = npos) const noexcept {
        return (char_traits_find_last_not_of<Traits>)(data_, size_, off, view.data_, view.size_);
    }

    /**
     * @brief 查找最后一个不等于指定字符的位置
     * @param chr 指定字符
     * @param off 起始位置
     * @return 最后一个不等于chr的位置，未找到则返回npos
     */
    MSTL_NODISCARD constexpr size_type find_last_not_of(const CharT chr, const size_type off = npos) const noexcept {
        return (char_traits_rfind_not_char<Traits>)(data_, size_, off, chr);
    }

    /**
     * @brief 查找最后一个不在指定字符集合中的字符
     * @param str 字符集合指针
     * @param off 起始位置
     * @param n 集合长度
     * @return 最后一个不匹配字符的位置，未找到则返回npos
     */
    MSTL_NODISCARD constexpr size_type find_last_not_of(const CharT* const str, const size_type off,
        const size_type n) const noexcept {
        return (char_traits_find_last_not_of<Traits>)(data_, size_, off, str, n);
    }

    /**
     * @brief 查找最后一个不在C风格字符串集合中的字符
     * @param str C风格字符串集合
     * @param off 起始位置
     * @return 最后一个不匹配字符的位置，未找到则返回npos
     */
    MSTL_NODISCARD constexpr size_type find_last_not_of(const CharT* const str,
        const size_type off = npos) const noexcept {
        return (char_traits_find_last_not_of<Traits>)(data_, size_, off, str, Traits::length(str));
    }

    /**
     * @brief 统计指定字符出现的次数
     * @param chr 要统计的字符
     * @param position 起始位置
     * @return 字符出现的次数
     */
    MSTL_CONSTEXPR20 size_type count(value_type chr, const size_type position = 0) const noexcept {
        size_type n = 0;
        for (size_type idx = position; idx < size_; ++idx) {
            if (*(data() + idx) == chr) ++n;
        }
        return n;
    }

    /**
     * @brief 检查是否以指定视图开头
     * @param view 要检查的视图
     * @return 是否以view开头
     */
    MSTL_NODISCARD MSTL_CONSTEXPR20 bool starts_with(const basic_string_view view) const noexcept {
        return view.size() <= size_ && traits_type::compare(data(), view.data(), view.size()) == 0;
    }

    /**
     * @brief 检查是否以指定字符开头
     * @param chr 要检查的字符
     * @return 是否以chr开头
     */
    MSTL_NODISCARD MSTL_CONSTEXPR20 bool starts_with(value_type chr) const noexcept {
        return !empty() && traits_type::eq(front(), chr);
    }

    /**
     * @brief 检查是否以C风格字符串开头
     * @param str 要检查的字符串
     * @return 是否以str开头
     */
    MSTL_NODISCARD MSTL_CONSTEXPR20 bool starts_with(const_pointer str) const noexcept {
        return this->starts_with(basic_string_view(str));
    }

    /**
     * @brief 检查是否以指定视图结尾
     * @param view 要检查的视图
     * @return 是否以view结尾
     */
    MSTL_NODISCARD MSTL_CONSTEXPR20 bool ends_with(const basic_string_view view) const noexcept {
        const size_type view_size = view.size();
        return view_size <= size_ && traits_type::compare(data_ + size_ - view_size, view.data(), view_size) == 0;
    }

    /**
     * @brief 检查是否以指定字符结尾
     * @param chr 要检查的字符
     * @return 是否以chr结尾
     */
    MSTL_NODISCARD MSTL_CONSTEXPR20 bool ends_with(value_type chr) const noexcept {
        return !empty() && traits_type::eq(back(), chr);
    }

    /**
     * @brief 检查是否以C风格字符串结尾
     * @param str 要检查的字符串
     * @return 是否以str结尾
     */
    MSTL_NODISCARD MSTL_CONSTEXPR20 bool ends_with(const_pointer str) const noexcept {
        return this->ends_with(basic_string_view(str));
    }

    /**
     * @brief 检查是否包含指定视图
     * @param view 要检查的视图
     * @return 是否包含view
     */
    MSTL_NODISCARD MSTL_CONSTEXPR20 bool contains(const basic_string_view view) const noexcept {
        return this->find(view) != npos;
    }

    /**
     * @brief 检查是否包含指定字符
     * @param chr 要检查的字符
     * @return 是否包含chr
     */
    MSTL_NODISCARD MSTL_CONSTEXPR20 bool contains(value_type chr) const noexcept {
        return this->find(chr) != npos;
    }

    /**
     * @brief 检查是否包含C风格字符串
     * @param str 要检查的字符串
     * @return 是否包含str
     */
    MSTL_NODISCARD MSTL_CONSTEXPR20 bool contains(const_pointer str) const noexcept {
        return this->find(str) != npos;
    }

    /**
     * @brief 去除左侧空白字符
     * @return 去除左侧空白后的新视图
     */
    MSTL_NODISCARD MSTL_CONSTEXPR20 basic_string_view trim_left() const noexcept {
        return this->trim_left_if([](value_type ch) { return _MSTL is_space(ch); });
    }

    /**
     * @brief 去除右侧空白字符
     * @return 去除右侧空白后的新视图
     */
    MSTL_NODISCARD MSTL_CONSTEXPR20 basic_string_view trim_right() const noexcept {
        return this->trim_right_if([](value_type ch) { return _MSTL is_space(ch); });
    }

    /**
     * @brief 去除两侧空白字符
     * @return 去除两侧空白后的新视图
     */
    MSTL_NODISCARD MSTL_CONSTEXPR20 basic_string_view trim() const noexcept {
        return this->trim_left().trim_right();
    }

    /**
     * @brief 根据谓词去除左侧字符
     * @tparam Predicate 谓词类型
     * @param pred 谓词函数，返回true表示要移除的字符
     * @return 去除后的新视图
     */
    template <typename Predicate>
    MSTL_NODISCARD MSTL_CONSTEXPR20 basic_string_view trim_left_if(Predicate pred) const
    noexcept(noexcept(pred(*cbegin()))) {
        if (empty()) return *this;

        const_iterator it = cbegin();
        while (it != cend() && pred(*it)) {
            ++it;
        }

        if (it != cbegin()) {
            return basic_string_view(data_ + (it - cbegin()), size_ - (it - cbegin()));
        }

        return *this;
    }

    /**
     * @brief 根据谓词去除右侧字符
     * @tparam Predicate 谓词类型
     * @param pred 谓词函数，返回true表示要移除的字符
     * @return 去除后的新视图
     */
    template <typename Predicate>
    MSTL_NODISCARD MSTL_CONSTEXPR20 basic_string_view trim_right_if(Predicate pred) const
    noexcept(noexcept(pred(*crbegin()))) {
        if (empty()) return *this;

        const_reverse_iterator rit = crbegin();
        while (rit != crend() && pred(*rit)) {
            ++rit;
        }

        if (rit != crbegin()) {
            return basic_string_view(data_, size_ - (rit - crbegin()));
        }

        return *this;
    }

    /**
     * @brief 根据谓词去除两侧字符
     * @tparam Predicate 谓词类型
     * @param pred 谓词函数，返回true表示要移除的字符
     * @return 去除后的新视图
     */
    template <typename Predicate>
    MSTL_NODISCARD MSTL_CONSTEXPR20 basic_string_view trim_if(Predicate pred) const
    noexcept(noexcept(this->trim_right_if(pred)) && noexcept(this->trim_left_if(pred))) {
        return this->trim_left_if(pred).trim_right_if(pred);
    }

    /**
     * @brief 相等比较
     * @param str 要比较的字符串视图
     * @return 是否相等
     */
    MSTL_NODISCARD MSTL_CONSTEXPR20 bool equal_to(const basic_string_view str) const noexcept {
        return (char_traits_equal<Traits>)(data_, size_, str.data_, str.size_);
    }

    /**
     * @brief 与C风格字符串相等比较
     * @param str C风格字符串
     * @return 是否相等
     */
    MSTL_NODISCARD MSTL_CONSTEXPR20 bool equal_to(const CharT* str) const noexcept {
        return equal_to(view_type(str));
    }

    /**
     * @brief 交换两个字符串视图
     * @param view 要交换的另一个视图
     */
    constexpr void swap(basic_string_view& view) noexcept {
        const basic_string_view tmp(view);
        view = *this;
        *this = tmp;
    }

    /**
     * @brief 相等比较操作符
     * @param rhs 右侧字符串视图
     * @return 是否相等
     */
    MSTL_NODISCARD constexpr bool operator ==(const basic_string_view& rhs) const noexcept {
        return this->equal_to(rhs);
    }

    /**
     * @brief 小于比较操作符
     * @param rhs 右侧字符串视图
     * @return 是否小于
     */
    MSTL_NODISCARD constexpr bool operator <(const basic_string_view& rhs) const noexcept {
        return this->compare(rhs) < 0;
    }

    /**
     * @brief 计算哈希值
     * @return 哈希值
     */
    MSTL_NODISCARD constexpr size_t to_hash() const noexcept {
        return _INNER FNV_hash_string(this->data(), this->length());
    }
};

/** @} */ // StringView

MSTL_END_NAMESPACE__
#endif // MSTL_CORE_STRING_BASIC_STRING_VIEW_HPP__
