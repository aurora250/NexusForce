#ifndef NEFORCE_CORE_CONTAINER_ARRAY_HPP__
#define NEFORCE_CORE_CONTAINER_ARRAY_HPP__

/**
 * @file array.hpp
 * @brief 固定大小数组容器
 *
 * 此文件提供了固定大小数组容器的实现。
 */

#include "NeForce/core/algorithm/compare.hpp"
#include "NeForce/core/algorithm/shift.hpp"
#include "NeForce/core/interface/icollector.hpp"
#include "NeForce/core/interface/iiterator.hpp"
NEFORCE_BEGIN_NAMESPACE__

/**
 * @defgroup Array 数组
 * @brief 固定大小数组容器实现
 * @{
 */

/**
 * @struct array_iterator
 * @brief 数组迭代器
 * @tparam IsConst 是否常量迭代器
 * @tparam Size 数组大小
 * @tparam Array 数组类型
 *
 * 为array提供迭代器支持，包含边界检查和调试验证。
 */
template <bool IsConst, size_t Size, typename Array>
struct array_iterator : iiterator<array_iterator<IsConst, Size, Array>> {
public:
    using container_type	= Array;  ///< 容器类型
    using value_type		= typename container_type::value_type;  ///< 值类型
    using size_type			= typename container_type::size_type;  ///< 大小类型
    using difference_type	= typename container_type::difference_type;  ///< 差值类型
    using iterator_category = contiguous_iterator_tag;  ///< 迭代器类别
    using reference = conditional_t<IsConst, typename container_type::const_reference, typename container_type::reference>;  ///< 引用类型
    using pointer	= conditional_t<IsConst, typename container_type::const_pointer, typename container_type::pointer>;  ///< 指针类型

private:
    pointer current_ = nullptr;  ///< 当前指针位置
    const container_type* container_ = nullptr;  ///< 关联容器指针

public:
    constexpr array_iterator() noexcept = default;
    constexpr ~array_iterator() = default;

    constexpr array_iterator(const array_iterator&) noexcept = default;
    constexpr array_iterator& operator =(const array_iterator&) noexcept = default;
    constexpr array_iterator(array_iterator&&) noexcept = default;
    constexpr array_iterator& operator =(array_iterator&&) noexcept = default;

    /**
     * @brief 构造函数
     * @param ptr 初始指针位置
     * @param vec 关联容器指针
     */
    constexpr array_iterator(pointer ptr, const container_type* vec) noexcept
    : current_(ptr), container_(vec) {}

    /**
     * @brief 解引用操作
     * @return 当前元素的引用
     */
    NEFORCE_NODISCARD constexpr reference dereference() const noexcept {
        NEFORCE_DEBUG_VERIFY(current_ && container_, "Attempting to dereference on a null pointer");
        NEFORCE_DEBUG_VERIFY(
            current_ >= container_->data() && current_ < container_->data() + Size,
            "Attempting to dereference out of boundary");
        return *current_;
    }

    /**
     * @brief 递增操作
     */
    constexpr void increment() noexcept {
        NEFORCE_DEBUG_VERIFY(current_ && container_, "Attempting to increment a null pointer");
        NEFORCE_DEBUG_VERIFY(
            current_ < container_->data() + Size,
            "Attempting to increment out of boundary");
        ++current_;
    }

    /**
     * @brief 递减操作
     */
    constexpr void decrement() noexcept {
        NEFORCE_DEBUG_VERIFY(current_ && container_, "Attempting to decrement a null pointer");
        NEFORCE_DEBUG_VERIFY(
            current_ > container_->data(),
            "Attempting to decrement out of boundary");
        --current_;
    }

    /**
     * @brief 前进操作
     * @param off 前进距离
     */
    constexpr void advance(difference_type off) noexcept {
        NEFORCE_DEBUG_VERIFY((current_ && container_) || off == 0, "Attempting to advance a null pointer");
        NEFORCE_DEBUG_VERIFY(
            current_ + off >= container_->data() && current_ + off <= container_->data() + Size,
            "Attempting to advance out of boundary");
        current_ += off;
    }

    /**
     * @brief 计算距离操作
     * @param other 另一个迭代器
     * @return 两个迭代器之间的距离
     */
    NEFORCE_NODISCARD constexpr difference_type distance_to(const array_iterator& other) const noexcept {
        NEFORCE_DEBUG_VERIFY(container_ == other.container_, "Attempting to distance to a different container");
        return static_cast<difference_type>(other.current_ - current_);
    }

    /**
     * @brief 下标访问操作符
     * @param off 偏移量
     * @return 偏移位置元素的引用
     */
    NEFORCE_NODISCARD constexpr reference operator [](const difference_type off) const noexcept {
        return *(*this + off);
    }

    /**
     * @brief 相等比较
     * @param rhs 右侧迭代器
     * @return 是否相等
     */
    NEFORCE_NODISCARD constexpr bool equal(const array_iterator& rhs) const noexcept {
        NEFORCE_DEBUG_VERIFY(container_ == rhs.container_, "Attempting to equal to a different container");
        return current_ == rhs.current_;
    }

    /**
     * @brief 小于比较
     * @param rhs 右侧迭代器
     * @return 当前迭代器是否在rhs之前
     */
    NEFORCE_NODISCARD constexpr bool less_than(const array_iterator& rhs) const noexcept {
        NEFORCE_DEBUG_VERIFY(container_ == rhs.container_, "Attempting to less than a different container");
        return current_ < rhs.current_;
    }

    /**
     * @brief 获取底层指针
     * @return 当前指针
     */
    NEFORCE_NODISCARD constexpr pointer base() const noexcept {
        return current_;
    }

    /**
     * @brief 获取关联容器
     * @return 关联容器指针
     */
    NEFORCE_NODISCARD constexpr const container_type* container() const noexcept {
        return container_;
    }
};


/**
 * @class array
 * @brief 固定大小数组容器
 * @tparam T 元素类型
 * @tparam Size 数组大小
 *
 * 编译时固定大小的数组容器。
 *
 * @note 大小在编译时确定，分配在栈上
 * @note 支持结构化绑定和元组接口
 */
template <typename T, size_t Size>
class array : public icollector<array<T, Size>> {
    static_assert(is_object_v<T>, "array only containers of object types.");

public:
    using value_type        = T;
    using pointer           = T*;
    using reference         = T&;
    using const_pointer     = const T*;
    using const_reference   = const T&;
    using size_type         = size_t;
    using difference_type   = ptrdiff_t;

    using iterator                  = array_iterator<false, Size, array>;  ///< 迭代器类型
    using const_iterator            = array_iterator<true, Size, array>;  ///< 常量迭代器类型
    using reverse_iterator          = _NEFORCE reverse_iterator<iterator>;  ///< 反向迭代器类型
    using const_reverse_iterator    = _NEFORCE reverse_iterator<const_iterator>;  ///< 常量反向迭代器类型

private:
    T array_[Size];  ///< 底层数组存储

public:
    constexpr array() noexcept = default;
    NEFORCE_CONSTEXPR20 ~array() noexcept = default;

    constexpr array(const array&) noexcept = default;
    constexpr array& operator =(const array&) noexcept = default;
    constexpr array(array&&) noexcept = default;
    constexpr array& operator =(array&&) noexcept = default;

    /**
     * @brief 初始化列表构造函数
     * @param ilist 初始化列表
     */
    constexpr array(std::initializer_list<T> ilist) noexcept {
        size_t size = ilist.size() < Size ? ilist.size() : Size;
        _NEFORCE copy(ilist.begin(), ilist.begin() + size, array_);
    }

    /**
     * @brief 获取起始迭代器
     * @return 指向第一个元素的迭代器
     */
    NEFORCE_NODISCARD constexpr iterator begin() noexcept {
        return iterator(array_, this);
    }

    /**
     * @brief 获取结束迭代器
     * @return 指向无效元素的迭代器
     */
    NEFORCE_NODISCARD constexpr iterator end() noexcept {
        return iterator(array_ + Size, this);
    }

    /**
     * @brief 获取常量起始迭代器
     * @return 指向第一个元素的常量迭代器
     */
    NEFORCE_NODISCARD constexpr const_iterator begin() const noexcept {
        return const_iterator(array_, this);
    }

    /**
     * @brief 获取常量结束迭代器
     * @return 指向无效元素的常量迭代器
     */
    NEFORCE_NODISCARD constexpr const_iterator end() const noexcept {
        return const_iterator(array_ + Size, this);
    }

    /**
     * @brief 获取反向起始迭代器
     * @return 指向无效元素的反向迭代器
     */
    NEFORCE_NODISCARD constexpr reverse_iterator rbegin() noexcept {
        return reverse_iterator(end());
    }

    /**
     * @brief 获取反向结束迭代器
     * @return 指向第一个元素的反向迭代器
     */
    NEFORCE_NODISCARD constexpr reverse_iterator rend() noexcept {
        return reverse_iterator(begin());
    }

    /**
     * @brief 获取常量反向起始迭代器
     * @return 指向无效元素的常量反向迭代器
     */
    NEFORCE_NODISCARD constexpr const_reverse_iterator rbegin() const noexcept {
        return const_reverse_iterator(end());
    }

    /**
     * @brief 获取常量反向结束迭代器
     * @return 指向第一个元素的常量反向迭代器
     */
    NEFORCE_NODISCARD constexpr const_reverse_iterator rend() const noexcept {
        return const_reverse_iterator(begin());
    }

    /**
     * @brief 获取常量起始迭代器
     * @return 指向第一个元素的常量迭代器
     */
    NEFORCE_NODISCARD constexpr const_iterator cbegin() const noexcept {
        return const_iterator(array_, this);
    }

    /**
     * @brief 获取常量结束迭代器
     * @return 指向无效元素的常量迭代器
     */
    NEFORCE_NODISCARD constexpr const_iterator cend() const noexcept {
        return const_iterator(array_ + Size, this);
    }

    /**
     * @brief 获取常量反向起始迭代器
     * @return 指向无效元素的常量反向迭代器
     */
    NEFORCE_NODISCARD constexpr const_reverse_iterator crbegin() const noexcept {
        return reverse_iterator(cend());
    }

    /**
     * @brief 获取常量反向结束迭代器
     * @return 指向第一个元素的常量反向迭代器
     */
    NEFORCE_NODISCARD constexpr const_reverse_iterator crend() const noexcept {
        return reverse_iterator(cbegin());
    }

    /**
     * @brief 获取数组大小
     * @return 数组大小
     */
    NEFORCE_NODISCARD NEFORCE_CONST_FUNCTION NEFORCE_ALWAYS_INLINE
    constexpr size_type size() const noexcept {
        return Size;
    }

    /**
     * @brief 获取最大大小
     * @return 最大大小
     */
    NEFORCE_NODISCARD NEFORCE_CONST_FUNCTION NEFORCE_ALWAYS_INLINE
    constexpr size_type max_size() const noexcept {
        return Size;
    }

    /**
     * @brief 检查数组是否为空
     * @return 总是返回false
     */
    NEFORCE_NODISCARD NEFORCE_CONST_FUNCTION NEFORCE_ALWAYS_INLINE
    constexpr bool empty() const noexcept {
        return false;
    }

    /**
     * @brief 索引位置元素访问
     * @param position 索引位置
     * @return 索引位置元素的引用
     */
    NEFORCE_NODISCARD constexpr reference at(size_type position) noexcept {
        NEFORCE_DEBUG_VERIFY(position < Size, "array subscript out of range");
        return array_[position];
    }
    /**
     * @brief 索引位置元素常量访问
     * @param position 索引位置
     * @return 索引位置元素的常量引用
     */
    NEFORCE_NODISCARD constexpr const_reference at(size_type position) const noexcept {
        NEFORCE_DEBUG_VERIFY(position < Size, "array subscript out of range");
        return array_[position];
    }
    /**
     * @brief 下标访问操作符
     * @param position 索引位置
     * @return 索引位置元素的引用
     */
    NEFORCE_NODISCARD constexpr reference operator[](size_type position) noexcept {
        NEFORCE_DEBUG_VERIFY(position < Size, "array subscript out of range");
        return array_[position];
    }
    /**
     * @brief 常量下标访问操作符
     * @param position 索引位置
     * @return 索引位置元素的常量引用
     */
    NEFORCE_NODISCARD constexpr const_reference operator[](size_type position) const noexcept {
        NEFORCE_DEBUG_VERIFY(position < Size, "array subscript out of range");
        return array_[position];
    }

    /**
     * @brief 访问第一个元素
     * @return 第一个元素的引用
     */
    NEFORCE_NODISCARD NEFORCE_ALWAYS_INLINE constexpr
    reference front() noexcept {
        return array_[0];
    }
    /**
     * @brief 访问第一个常量元素
     * @return 第一个元素的常量引用
     */
    NEFORCE_NODISCARD NEFORCE_ALWAYS_INLINE constexpr
    const_reference front() const noexcept {
        return array_[0];
    }
    /**
     * @brief 访问最后一个元素
     * @return 最后一个元素的引用
     */
    NEFORCE_NODISCARD NEFORCE_ALWAYS_INLINE constexpr
    reference back() noexcept {
        return array_[Size - 1];
    }
    /**
     * @brief 访问最后一个常量元素
     * @return 最后一个元素的常量引用
     */
    NEFORCE_NODISCARD NEFORCE_ALWAYS_INLINE constexpr
    const_reference back() const noexcept {
        return array_[Size - 1];
    }
    /**
     * @brief 获取底层数据指针
     * @return 指向数组第一个元素的指针
     */
    NEFORCE_NODISCARD NEFORCE_CONST_FUNCTION NEFORCE_ALWAYS_INLINE constexpr
    T* data() noexcept {
        return array_;
    }
    /**
     * @brief 获取常量底层数据指针
     * @return 指向数组第一个元素的常量指针
     */
    NEFORCE_NODISCARD NEFORCE_CONST_FUNCTION NEFORCE_ALWAYS_INLINE constexpr
    const T* data() const noexcept {
        return array_;
    }

    /**
     * @brief 填充数组
     * @param value 填充的值
     */
    constexpr void fill(const T& value) {
        _NEFORCE fill_n(array_, Size, value);
    }

    /**
     * @brief 交换两个数组
     * @param other 要交换的另一个数组
     */
    constexpr void swap(array& other)
    noexcept(is_nothrow_swappable_v<T>) {
        _NEFORCE swap(array_, other.array_);
    }

    /**
     * @brief 相等比较操作符
     * @param rhs 右侧数组
     * @return 如果两个数组元素相等返回true
     */
    NEFORCE_NODISCARD constexpr bool operator ==(const array& rhs) const noexcept {
        return _NEFORCE equal(this->cbegin(), this->cend(), rhs.cbegin());
    }
    /**
     * @brief 小于比较操作符
     * @param rhs 右侧数组
     * @return 按字典序比较结果
     */
    NEFORCE_NODISCARD constexpr bool operator <(const array& rhs) const noexcept {
        return _NEFORCE lexicographical_compare(this->cbegin(), this->cend(), rhs.cbegin(), rhs.cend());
    }
};

struct empty_array_element_tag {
    constexpr explicit empty_array_element_tag() noexcept = default;
};

/**
 * @brief 大小为0的数组特化
 * @tparam T 元素类型
 *
 * 处理空数组的特殊情况，优化存储和操作。
 */
template <typename T>
class array<T, 0> : public icollector<array<T, 0>> {
    static_assert(is_object_v<T>, "array only containers of object types.");

public:
    using value_type        = T;
    using pointer           = T*;
    using reference         = T&;
    using const_pointer     = const T*;
    using const_reference   = const T&;
    using size_type         = size_t;
    using difference_type   = ptrdiff_t;

    using iterator                  = array_iterator<false, 0, array>;
    using const_iterator            = array_iterator<true, 0, array>;
    using reverse_iterator          = _NEFORCE reverse_iterator<iterator>;
    using const_reverse_iterator    = _NEFORCE reverse_iterator<const_iterator>;

private:
    conditional_t<
        disjunction_v<is_default_constructible<T>, is_implicitly_default_constructible<T>>,
        T, empty_array_element_tag
    > array_[1]{};

public:
    NEFORCE_NODISCARD NEFORCE_CONST_FUNCTION NEFORCE_ALWAYS_INLINE
    constexpr iterator begin() noexcept {
        return iterator{};
    }
    NEFORCE_NODISCARD NEFORCE_CONST_FUNCTION NEFORCE_ALWAYS_INLINE
    constexpr iterator end() noexcept {
        return iterator{};
    }
    NEFORCE_NODISCARD NEFORCE_CONST_FUNCTION NEFORCE_ALWAYS_INLINE
    constexpr const_iterator begin() const noexcept {
        return const_iterator{};
    }
    NEFORCE_NODISCARD NEFORCE_CONST_FUNCTION NEFORCE_ALWAYS_INLINE
    constexpr const_iterator end() const noexcept {
        return const_iterator{};
    }
    NEFORCE_NODISCARD NEFORCE_CONST_FUNCTION NEFORCE_ALWAYS_INLINE
    constexpr reverse_iterator rbegin() noexcept {
        return reverse_iterator(end());
    }
    NEFORCE_NODISCARD NEFORCE_CONST_FUNCTION NEFORCE_ALWAYS_INLINE
    constexpr reverse_iterator rend() noexcept {
        return reverse_iterator(begin());
    }
    NEFORCE_NODISCARD NEFORCE_CONST_FUNCTION NEFORCE_ALWAYS_INLINE
    constexpr const_reverse_iterator rbegin() const noexcept {
        return const_reverse_iterator(end());
    }
    NEFORCE_NODISCARD NEFORCE_CONST_FUNCTION NEFORCE_ALWAYS_INLINE
    constexpr const_reverse_iterator rend() const noexcept {
        return const_reverse_iterator(begin());
    }
    NEFORCE_NODISCARD NEFORCE_ALWAYS_INLINE
    constexpr const_iterator cbegin() const noexcept {
        return begin();
    }
    NEFORCE_NODISCARD NEFORCE_ALWAYS_INLINE
    constexpr const_iterator cend() const noexcept {
        return end();
    }
    NEFORCE_NODISCARD NEFORCE_ALWAYS_INLINE
    constexpr const_reverse_iterator crbegin() const noexcept {
        return rbegin();
    }
    NEFORCE_NODISCARD NEFORCE_ALWAYS_INLINE
    constexpr const_reverse_iterator crend() const noexcept {
        return rend();
    }

    NEFORCE_NODISCARD NEFORCE_CONST_FUNCTION NEFORCE_ALWAYS_INLINE
    constexpr size_type size() const noexcept {
        return 0;
    }
    NEFORCE_NODISCARD NEFORCE_CONST_FUNCTION NEFORCE_ALWAYS_INLINE
    constexpr size_type max_size() const noexcept {
        return 0;
    }
    NEFORCE_NODISCARD NEFORCE_CONST_FUNCTION NEFORCE_ALWAYS_INLINE
    constexpr bool empty() const noexcept {
        return true;
    }

    NEFORCE_NODISCARD reference at(size_type) {
        throw_exception(iterator_exception("array empty."));
        return array_[0];
    }

    NEFORCE_NODISCARD const_reference at(size_type) const {
        throw_exception(iterator_exception("array empty."));
        return array_[0];
    }

    NEFORCE_NODISCARD reference operator [](size_type) noexcept {
        throw_exception(iterator_exception("array index out of range"));
        return *data();
    }
    NEFORCE_NODISCARD const_reference operator [](size_type) const noexcept {
        throw_exception(iterator_exception("array index out of range"));
        return *data();
    }

    NEFORCE_NODISCARD reference front() noexcept {
        throw_exception(iterator_exception("array empty."));
        return *data();
    }
    NEFORCE_NODISCARD const_reference front() const noexcept {
        throw_exception(iterator_exception("array empty."));
        return *data();
    }

    NEFORCE_NODISCARD reference back() noexcept {
        throw_exception(iterator_exception("array empty."));
        return *data();
    }

    NEFORCE_NODISCARD const_reference back() const noexcept {
        throw_exception(iterator_exception("array empty."));
        return *data();
    }

    NEFORCE_NODISCARD NEFORCE_CONST_FUNCTION NEFORCE_ALWAYS_INLINE
    constexpr T* data() noexcept {
        return nullptr;
    }
    NEFORCE_NODISCARD NEFORCE_CONST_FUNCTION NEFORCE_ALWAYS_INLINE
    constexpr const T* data() const noexcept {
        return nullptr;
    }

    NEFORCE_ALWAYS_INLINE constexpr void fill(const T&) {}
    NEFORCE_ALWAYS_INLINE constexpr void swap(array&) noexcept {}

    NEFORCE_NODISCARD NEFORCE_ALWAYS_INLINE constexpr
    bool operator ==(const array&) const noexcept { return true; }

    NEFORCE_NODISCARD NEFORCE_ALWAYS_INLINE constexpr
    bool operator <(const array&) const noexcept { return false; }
};

#if NEFORCE_SUPPORT_DEDUCTION_GUIDES
NEFORCE_BEGIN_INNER__
template <typename First, typename... Rest>
struct __array_same {
    static_assert(conjunction_v<is_same<First, Rest>...>, "array types mismatch.");
    using type = First;
};
NEFORCE_END_INNER__

template <typename First, typename... Rest>
array(First, Rest...) -> array<typename _INNER __array_same<First, Rest...>::type, 1 + sizeof...(Rest)>;
#endif // NEFORCE_SUPPORT_DEDUCTION_GUIDES


/**
 * @brief 获取左值数组指定位置的元素
 * @tparam Idx 索引位置
 * @tparam T 元素类型
 * @tparam Size 数组大小
 * @param arr 数组引用
 * @return 指定位置元素的引用
 */
template <size_t Idx, typename T, size_t Size>
NEFORCE_NODISCARD constexpr T& get(array<T, Size>& arr) noexcept {
    static_assert(Idx < Size, "array index out of bounds");
    return arr[Idx];
}

/**
 * @brief 获取常量左值数组指定位置的常量元素
 * @tparam Idx 索引位置
 * @tparam T 元素类型
 * @tparam Size 数组大小
 * @param arr 数组引用
 * @return 指定位置元素的常量引用
 */
template <size_t Idx, typename T, size_t Size>
NEFORCE_NODISCARD constexpr const T& get(const array<T, Size>& arr) noexcept {
    static_assert(Idx < Size, "array index out of bounds");
    return arr[Idx];
}

/**
 * @brief 获取右值数组指定位置的常量元素
 * @tparam Idx 索引位置
 * @tparam T 元素类型
 * @tparam Size 数组大小
 * @param arr 数组引用
 * @return 指定位置元素的右值引用
 */
template <size_t Idx, typename T, size_t Size>
NEFORCE_NODISCARD constexpr T&& get(array<T, Size>&& arr) noexcept {
    static_assert(Idx < Size, "array index out of bounds");
    return _NEFORCE move(arr[Idx]);
}

/**
 * @brief 获取常量右值数组指定位置的常量元素
 * @tparam Idx 索引位置
 * @tparam T 元素类型
 * @tparam Size 数组大小
 * @param arr 数组引用
 * @return 指定位置元素的常量右值引用
 */
template <size_t Idx, typename T, size_t Size>
NEFORCE_NODISCARD constexpr const T&& get(const array<T, Size>&& arr) noexcept {
    static_assert(Idx < Size, "array index out of bounds");
    return _NEFORCE move(arr[Idx]);
}

/** @} */ // Array

/**
 * @defgroup Tuple 元组
 * @brief 元组的主模板、特化实现和辅助函数
 * @{
 */

/**
 * @brief 数组的元组大小特化
 * @tparam T 元素类型
 * @tparam Size 数组大小
 * @note 支持结构化绑定
 */
template <typename T, size_t Size>
struct tuple_size<array<T, Size>> : integral_constant<size_t, Size> {};

/**
 * @brief 数组的元组元素类型特化
 * @tparam Idx 索引位置
 * @tparam T 元素类型
 * @tparam Size 数组大小
 */
template <size_t Idx, typename T, size_t Size>
struct tuple_element<Idx, array<T, Size>> {
    static_assert(Idx < Size, "array index is in range");
    using type = T;
};

template <typename T, size_t Size>
NEFORCE_INLINE17 constexpr size_t tuple_size_v<array<T, Size>> = Size;  ///< 元组大小值

template <typename T, size_t Size>
NEFORCE_INLINE17 constexpr size_t tuple_size_v<const array<T, Size>> = Size;  ///< 常量数组的元组大小值

/** @} */ // Tuple

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_CORE_CONTAINER_ARRAY_HPP__
