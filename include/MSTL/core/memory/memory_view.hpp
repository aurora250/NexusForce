#ifndef MSTL_CORE_MEMORY_MEMORY_VIEW_HPP__
#define MSTL_CORE_MEMORY_MEMORY_VIEW_HPP__

/**
 * @file memory_view.hpp
 * @brief MSTL内存视图容器
 *
 * 此文件提供了内存视图容器的实现。
 * memory_view是一个非拥有（non-owning）的内存区域视图，提供对连续内存块的只读/读写访问，
 * 不进行内存分配，性能高效。
 */

#include "MSTL/core/container/array.hpp"
#include "MSTL/core/iterator/normal_iterator.hpp"
#include "MSTL/core/iterator/reverse_iterator.hpp"
#include "MSTL/core/numeric/numeric_traits.hpp"
#include "MSTL/core/utility/compressed_pair.hpp"
MSTL_BEGIN_NAMESPACE__

/**
 * @defgroup MemoryView 内存视图
 * @brief 非拥有连续内存视图
 * @{
 */

/// 动态范围标记，表示大小在运行时确定
MSTL_INLINE17 constexpr size_t dynamic_extent = numeric_traits<size_t>::max();

/// @cond
MSTL_BEGIN_INNER__

/**
 * @struct extent_storage
 * @brief 范围存储（静态大小版本）
 * @tparam Extent 静态大小
 */
template <size_t Extent>
struct extent_storage {
    constexpr extent_storage(size_t) noexcept {}
    static constexpr size_t extent() noexcept { return Extent; }
};

/**
 * @brief 范围存储（动态大小版本）
 */
template <>
struct extent_storage<dynamic_extent> {
private:
    size_t extent_value_;  ///< 动态大小值
public:
    constexpr extent_storage(const size_t extent_value) noexcept : extent_value_(extent_value) {}
    constexpr size_t extent() const noexcept { return extent_value_; }
};

MSTL_END_INNER__
/// @endcond


/**
 * @class memory_view
 * @brief 内存视图模板
 * @tparam Element 元素类型
 * @tparam Extent 静态范围大小，默认为dynamic_extent
 *
 * memory_view是一个非拥有的连续内存视图，针对内存操作优化。
 * 可以指向任意连续内存块。支持编译时和运行时大小确定。
 * 提供类似容器的接口，但不进行内存管理。
 */
template <typename Element, size_t Extent = dynamic_extent>
class memory_view {
public:
    using element_type      = Element;  ///< 元素类型

    using value_type        = remove_cv_t<Element>;  ///< 值类型
    using size_type         = size_t;  ///< 大小类型
    using difference_type   = ptrdiff_t;  ///< 差值类型
    using pointer           = Element*;  ///< 指针类型
    using const_pointer     = const Element*;  ///< 常量指针类型
    using reference         = element_type&;  ///< 引用类型
    using const_reference   = const element_type&;  ///< 常量引用类型
    using iterator          = normal_iterator<pointer>;  ///< 迭代器类型
    using reverse_iterator  = _MSTL reverse_iterator<iterator>;  ///< 反向迭代器类型

private:
    template <typename U, size_t ArrayExtent, enable_if_t<
        Extent == dynamic_extent || ArrayExtent == Extent, int> = 0>
    using is_compatible_array = is_array_convertible<Element, U>;

    template <typename Ref>
    using is_compatible_ref = is_array_convertible<Element, remove_reference_t<Ref>>;

    /// 压缩存储：范围存储和指针
    compressed_pair<_INNER extent_storage<Extent>, pointer> extent_pair_;


    template <size_t, size_t Count, enable_if_t<
        Count != dynamic_extent, int> = 0>
    static constexpr size_t view_extent() noexcept {
        return Count;
    }

    template <size_t Offset, size_t Count, enable_if_t<
        Count == dynamic_extent && Extent != dynamic_extent, int> = 0>
    static constexpr size_t view_extent() noexcept {
        return Extent - Offset;
    }

    template <size_t, size_t Count, enable_if_t<
        Count == dynamic_extent && Extent == dynamic_extent, int> =0>
    static constexpr size_t view_extent()  noexcept{
        return dynamic_extent;
    }
    
    template <size_t UE = Extent, enable_if_t<UE != dynamic_extent, int> = 0>
    static MSTL_ALWAYS_INLINE constexpr void check_extend(const size_t count) noexcept {
        MSTL_CONSTEXPR_ASSERT(count == Extent);
    }
    template <size_t UE = Extent, enable_if_t<UE == dynamic_extent, int> = 0>
    static MSTL_ALWAYS_INLINE constexpr void check_extend(const size_t) noexcept {}

    template <size_t UE = Extent, enable_if_t<UE != dynamic_extent, int> = 0>
    MSTL_ALWAYS_INLINE constexpr void check_count(const size_t Count) const noexcept {
        static_assert(Count <= Extent, "COunt must less than Extend");
    }
    template <size_t UE = Extent, enable_if_t<UE == dynamic_extent, int> = 0>
    MSTL_ALWAYS_INLINE constexpr void check_count(const size_t Count) const noexcept {
        MSTL_CONSTEXPR_ASSERT(Count <= size())
    }

    template <size_t Offset, size_t Count, enable_if_t<Count == dynamic_extent, int> = 0>
    constexpr decltype(auto) view_aux() const noexcept {
        using view = memory_view<element_type, view_extent<Offset, Count>()>;
        return view{data() + Offset, size() - Offset};
    }
    template <size_t Offset, size_t Count, enable_if_t<Count != dynamic_extent, int> = 0>
    constexpr decltype(auto) view_aux() const noexcept {
        using view = memory_view<element_type, view_extent<Offset, Count>()>;
        memory_view::check_count<Extent>(Count);
        memory_view::check_count<Extent>(Count + Offset);
        return view{data() + Offset, Count};
    }

public:
    /**
     * @brief 默认构造函数
     *
     * 构造空视图。仅当Extent为0或dynamic_extent时可用。
     */
    constexpr memory_view() noexcept
#ifdef MSTL_STANDARD_20__
    requires ((Extent + 1u) <= 1u)
#endif
    : extent_pair_(exact_arg_construct_tag{}, 0, nullptr) {}

    /**
     * @brief 从迭代器和大小构造
     * @tparam Iterator 迭代器类型
     * @param first 起始迭代器
     * @param count 元素数量
     *
     * 当Extent为静态大小时，count必须等于Extent。
     */
    template <typename Iterator, enable_if_t<is_cot_iter_v<Iterator>, int> = 0>
#ifdef MSTL_STANDARD_20__
    requires is_compatible_ref<iter_reference_t<Iterator>>::value
#endif
    constexpr
#ifdef MSTL_STANDARD_20__
    explicit(Extent != dynamic_extent)
#endif
    memory_view(Iterator first, size_type count) noexcept
    : extent_pair_(exact_arg_construct_tag{}, count, _MSTL to_address(first)) {
        memory_view::check_extend<Extent>(count);
    }

    /**
     * @brief 从迭代器范围构造
     * @tparam Iterator 起始迭代器类型
     * @tparam End 结束迭代器类型
     * @param first 起始迭代器
     * @param last 结束迭代器
     *
     * 当Extent为静态大小时，范围大小必须等于Extent。
     */
    template <typename Iterator, typename End, enable_if_t<is_cot_iter_v<Iterator>, int> = 0>
#ifdef MSTL_STANDARD_20__
     requires is_compatible_ref<iter_reference_t<Iterator>>::value && (!is_convertible_v<End, size_type>)
#endif
    constexpr
#ifdef MSTL_STANDARD_20__
    explicit(Extent != dynamic_extent)
#endif
    memory_view(Iterator first, End last) noexcept(noexcept(last - first))
    : extent_pair_(exact_arg_construct_tag{}, static_cast<size_type>(last - first), _MSTL to_address(first)) {
        memory_view::check_extend<Extent>(last - first);
    }

    /**
     * @brief 从C风格数组构造
     * @tparam AE 数组大小
     * @param arr C风格数组引用
     */
    template <size_t AE>
#ifdef MSTL_STANDARD_20__
    requires (Extent == dynamic_extent || AE == Extent)
#endif
    constexpr memory_view(type_identity_t<element_type> (&arr)[AE]) noexcept
    : memory_view(static_cast<pointer>(arr), AE) {}

    /**
     * @brief 从array容器构造
     * @tparam U 元素类型
     * @tparam AE 数组大小
     * @param arr array容器引用
     */
    template <typename U, size_t AE>
#ifdef MSTL_STANDARD_20__
    requires is_compatible_array<U, AE>::value
#endif
    constexpr memory_view(array<U, AE>& arr) noexcept
    : memory_view(static_cast<pointer>(arr.data()), AE) {}

    /**
     * @brief 从常量array容器构造
     * @tparam U 元素类型
     * @tparam AE 数组大小
     * @param arr 常量array容器引用
     */
    template <typename U, size_t AE>
#ifdef MSTL_STANDARD_20__
    requires is_compatible_array<const U, AE>::value
#endif
    constexpr memory_view(const array<U, AE>& arr) noexcept
    : memory_view(static_cast<pointer>(arr.data()), AE) {}

    /**
     * @brief 拷贝构造函数
     * @param other 源视图
     */
    constexpr memory_view(const memory_view& other) noexcept = default;

    /**
     * @brief 从其他元素类型的视图转换构造
     * @tparam U 源元素类型
     * @tparam OE 源范围大小
     * @param other 源视图
     *
     * 要求类型可转换且范围兼容。
     */
    template <typename U, size_t OE>
#ifdef MSTL_STANDARD_20__
    requires (Extent == dynamic_extent || OE == dynamic_extent || Extent == OE)
        && is_array_convertible_v<Element, U>
#endif
    constexpr
#ifdef MSTL_STANDARD_20__
    explicit(Extent != dynamic_extent && OE == dynamic_extent)
#endif
    memory_view(const memory_view<U, OE>& other) noexcept
    : extent_pair_(other.extent_pair_) {
        memory_view::check_extend<Extent>(other.size());
    }

    /**
     * @brief 拷贝赋值运算符
     * @param other 源视图
     * @return 自身引用
     */
    constexpr memory_view& operator =(const memory_view& other) noexcept = default;

    /**
     * @brief 析构函数
     */
    MSTL_CONSTEXPR20 ~memory_view() noexcept = default;
    
    /**
     * @brief 获取元素数量
     * @return 元素数量
     */
    MSTL_NODISCARD constexpr size_type size() const noexcept {
        return extent_pair_.get_base().extent();
    }

    /**
     * @brief 获取字节数
     * @return 总字节数
     */
    MSTL_NODISCARD constexpr size_type size_bytes() const noexcept {
        return size() * sizeof(element_type);
    }

    /**
     * @brief 检查是否为空
     * @return 是否为空
     */
    MSTL_NODISCARD constexpr bool empty() const noexcept {
        return size() == 0;
    }
    
    /**
     * @brief 访问第一个元素
     * @return 第一个元素的引用
     */
    MSTL_NODISCARD constexpr reference front() const noexcept {
        MSTL_CONSTEXPR_ASSERT(!empty());
        return *extent_pair_.value;
    }

    /**
     * @brief 访问最后一个元素
     * @return 最后一个元素的引用
     */
    MSTL_NODISCARD constexpr reference back() const noexcept {
        MSTL_CONSTEXPR_ASSERT(!empty());
        return *(extent_pair_.value + (size() - 1));
    }

    /**
     * @brief 下标访问操作符
     * @param index 索引
     * @return 指定位置的元素引用
     */
    MSTL_NODISCARD constexpr reference operator [](size_type index) const noexcept {
        MSTL_CONSTEXPR_ASSERT(index < size());
        return *(extent_pair_.value + index);
    }

    /**
     * @brief 获取数据指针
     * @return 指向内存起始位置的指针
     */
    MSTL_NODISCARD constexpr pointer data() const noexcept {
        return extent_pair_.value;
    }

    /**
     * @brief 获取起始迭代器
     * @return 指向第一个元素的迭代器
     */
    MSTL_NODISCARD constexpr iterator begin() const noexcept {
        return iterator(extent_pair_.value);
    }

    /**
     * @brief 获取结束迭代器
     * @return 指向最后一个元素之后位置的迭代器
     */
    MSTL_NODISCARD constexpr iterator end() const noexcept {
        return iterator(extent_pair_.value + size());
    }

    /**
     * @brief 获取反向起始迭代器
     * @return 指向最后一个元素的反向迭代器
     */
    MSTL_NODISCARD constexpr reverse_iterator rbegin() const noexcept {
        return reverse_iterator(end());
    }

    /**
     * @brief 获取反向结束迭代器
     * @return 指向第一个元素之前位置的反向迭代器
     */
    MSTL_NODISCARD constexpr reverse_iterator rend() const noexcept {
        return reverse_iterator(begin());
    }

    /**
     * @brief 获取前Count个元素的视图（编译时大小）
     * @tparam Count 元素数量
     * @return 新视图
     */
    template <size_t Count>
    constexpr memory_view<element_type, Count> first() const noexcept {
        memory_view::check_count<Extent>(Count);
        using view = memory_view<element_type, Count>;
        return view{ data(), Count };
    }

    /**
     * @brief 获取前count个元素的视图（运行时大小）
     * @param count 元素数量
     * @return 新视图
     */
    constexpr memory_view<element_type> first(size_type count) const noexcept {
        MSTL_CONSTEXPR_ASSERT(count <= size());
        return { data(), count };
    }

    /**
     * @brief 获取后Count个元素的视图（编译时大小）
     * @tparam Count 元素数量
     * @return 新视图
     */
    template <size_t Count>
    constexpr memory_view<element_type, Count> last() const noexcept {
        memory_view::check_count<Extent>(Count);
        using view = memory_view<element_type, Count>;
        return view{ data() + (size() - Count), Count };
    }

    /**
     * @brief 获取后count个元素的视图（运行时大小）
     * @param count 元素数量
     * @return 新视图
     */
    constexpr memory_view<element_type> last(size_type count) const noexcept {
        MSTL_CONSTEXPR_ASSERT(count <= size());
        return { data() + (size() - count), count };
    }

    /**
     * @brief 获取子视图（编译时偏移和大小）
     * @tparam Offset 偏移量
     * @tparam Count 元素数量，默认为dynamic_extent
     * @return 子视图
     */
    template <size_t Offset, size_t Count = dynamic_extent>
    constexpr auto view() const noexcept
    -> memory_view<element_type, view_extent<Offset, Count>()> {
        memory_view::check_count<Extent>(Offset);
        return this->template view_aux<Offset, Count>();
    }

    /**
     * @brief 获取子视图（运行时偏移和大小）
     * @param offset 偏移量
     * @param count 元素数量，默认为dynamic_extent（到末尾）
     * @return 子视图
     */
    constexpr memory_view<element_type>
    view(size_type offset, size_type count = dynamic_extent) const noexcept {
        MSTL_CONSTEXPR_ASSERT(offset <= size());
        if (count == dynamic_extent) {
            count = size() - offset;
        } else {
            MSTL_CONSTEXPR_ASSERT(count <= size());
            MSTL_CONSTEXPR_ASSERT(offset + count <= size());
        }
        return {data() + offset, count};
    }
};

#ifdef MSTL_SUPPORT_DEDUCTION_GUIDES__
template <typename T, size_t ArrayExtent>
memory_view(T(&)[ArrayExtent]) -> memory_view<T, ArrayExtent>;

template <typename T, size_t ArrayExtent>
memory_view(array<T, ArrayExtent>&) -> memory_view<T, ArrayExtent>;

template <typename T, size_t ArrayExtent>
memory_view(const array<T, ArrayExtent>&) -> memory_view<const T, ArrayExtent>;

#ifdef MSTL_STANDARD_20__
template <contiguous_iterator Iter, typename End>
#else
template <typename Iter, typename End>
#endif
memory_view(Iter, End) -> memory_view<remove_reference_t<iter_reference_t<Iter>>>;
#endif

/// 字节视图类型别名
using byte_view = memory_view<byte_t>;

/// 常量字节视图类型别名
using cbyte_view = memory_view<const byte_t>;

/** @} */ // MemoryView

MSTL_END_NAMESPACE__
#endif // MSTL_CORE_MEMORY_MEMORY_VIEW_HPP__
