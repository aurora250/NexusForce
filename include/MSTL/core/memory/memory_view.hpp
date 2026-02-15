#ifndef MSTL_CORE_MEMORY_MEMORY_VIEW_HPP__
#define MSTL_CORE_MEMORY_MEMORY_VIEW_HPP__
#include "MSTL/core/container/array.hpp"
#include "MSTL/core/iterator/normal_iterator.hpp"
#include "MSTL/core/iterator/reverse_iterator.hpp"
#include "MSTL/core/numeric/numeric_traits.hpp"
#include "MSTL/core/utility/compressed_pair.hpp"
MSTL_BEGIN_NAMESPACE__

MSTL_INLINE17 constexpr size_t dynamic_extent = numeric_traits<size_t>::max();


MSTL_BEGIN_INNER__

template <size_t Extent>
struct extent_storage {
    constexpr extent_storage(size_t) noexcept {}
    static constexpr size_t extent() noexcept { return Extent; }
};

template <>
struct extent_storage<dynamic_extent> {
private:
    size_t extent_value_;
public:
    constexpr extent_storage(const size_t extent_value) noexcept : extent_value_(extent_value) {}
    constexpr size_t extent() const noexcept { return extent_value_; }
};

MSTL_END_INNER__


template <typename Element, size_t Extent = dynamic_extent>
class memory_view {
public:
    using element_type = Element;
    
    using value_type        = remove_cv_t<Element>;
    using size_type         = size_t;
    using difference_type   = ptrdiff_t;
    using pointer           = Element*;
    using const_pointer     = const Element*;
    using reference         = element_type&;
    using const_reference   = const element_type&;
    using iterator          = normal_iterator<pointer>;
    using reverse_iterator  = _MSTL reverse_iterator<iterator>;

private:
    template <typename U, size_t ArrayExtent, enable_if_t<
        Extent == dynamic_extent || ArrayExtent == Extent, int> = 0>
    using is_compatible_array = is_array_convertible<Element, U>;

    template <typename Ref>
    using is_compatible_ref = is_array_convertible<Element, remove_reference_t<Ref>>;
    
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
    constexpr memory_view() noexcept
#ifdef MSTL_STANDARD_20__
    requires ((Extent + 1u) <= 1u)
#endif
    : extent_pair_(exact_arg_construct_tag{}, 0, nullptr) {}

#ifdef MSTL_STANDARD_20__
    template <contiguous_iterator Iter>
    requires is_compatible_ref<iter_reference_t<Iter>>::value
#else
    template <typename Iter, enable_if_t<is_cot_iter_v<Iter>, int> = 0>
#endif
    constexpr
#ifdef MSTL_STANDARD_20__
    explicit(Extent != dynamic_extent)
#endif
    memory_view(Iter first, size_type count) noexcept
    : extent_pair_(exact_arg_construct_tag{}, count, _MSTL to_address(first)) {
        memory_view::check_extend<Extent>(count);
    }

#ifdef MSTL_STANDARD_20__
     template <contiguous_iterator Iter, sized_sentinel_for<Iter> End>
     requires is_compatible_ref<iter_reference_t<Iter>>::value && (!is_convertible_v<End, size_type>)
#else
    template <typename Iter, typename End, enable_if_t<is_cot_iter_v<Iter>, int> = 0>
#endif
    constexpr
#ifdef MSTL_STANDARD_20__
    explicit(Extent != dynamic_extent)
#endif
    memory_view(Iter first, End last) noexcept(noexcept(last - first))
    : extent_pair_(exact_arg_construct_tag{}, static_cast<size_type>(last - first), _MSTL to_address(first)) {
        memory_view::check_extend<Extent>(last - first);
    }

    template <size_t AE>
#ifdef MSTL_STANDARD_20__
    requires (Extent == dynamic_extent || AE == Extent)
#endif
    constexpr memory_view(type_identity_t<element_type> (&arr)[AE]) noexcept
    : memory_view(static_cast<pointer>(arr), AE) {}

    template <typename U, size_t AE>
#ifdef MSTL_STANDARD_20__
    requires is_compatible_array<U, AE>::value
#endif
    constexpr memory_view(array<U, AE>& arr) noexcept
    : memory_view(static_cast<pointer>(arr.data()), AE) {}

    template <typename U, size_t AE>
#ifdef MSTL_STANDARD_20__
    requires is_compatible_array<const U, AE>::value
#endif
    constexpr memory_view(const array<U, AE>& arr) noexcept
    : memory_view(static_cast<pointer>(arr.data()), AE) {}

    constexpr memory_view(const memory_view&) noexcept = default;

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

    constexpr memory_view& operator =(const memory_view&) noexcept = default;
    
    MSTL_CONSTEXPR20 ~memory_view() noexcept = default;
    

    MSTL_NODISCARD constexpr size_type size() const noexcept {
        return extent_pair_.get_base().extent();
    }
    
    MSTL_NODISCARD constexpr size_type size_bytes() const noexcept {
        return size() * sizeof(element_type);
    }
    
    MSTL_NODISCARD constexpr bool empty() const noexcept {
        return size() == 0;
    }
    

    MSTL_NODISCARD constexpr reference front() const noexcept {
        MSTL_CONSTEXPR_ASSERT(!empty());
        return *extent_pair_.value;
    }

    MSTL_NODISCARD constexpr reference back() const noexcept {
        MSTL_CONSTEXPR_ASSERT(!empty());
        return *(extent_pair_.value + (size() - 1));
    }

    MSTL_NODISCARD constexpr reference operator [](size_type index) const noexcept {
        MSTL_CONSTEXPR_ASSERT(index < size());
        return *(extent_pair_.value + index);
    }

    MSTL_NODISCARD constexpr pointer data() const noexcept {
        return extent_pair_.value;
    }

    MSTL_NODISCARD constexpr iterator begin() const noexcept {
        return iterator(extent_pair_.value);
    }
    MSTL_NODISCARD constexpr iterator end() const noexcept {
        return iterator(extent_pair_.value + size());
    }

    MSTL_NODISCARD constexpr reverse_iterator rbegin() const noexcept {
        return reverse_iterator(end());
    }
    MSTL_NODISCARD constexpr reverse_iterator rend() const noexcept {
        return reverse_iterator(begin());
    }

    template <size_t Count>
    constexpr memory_view<element_type, Count> first() const noexcept {
        memory_view::check_count<Extent>(Count);
        using view = memory_view<element_type, Count>;
        return view{ data(), Count };
    }

    constexpr memory_view<element_type> first(size_type count) const noexcept {
        MSTL_CONSTEXPR_ASSERT(count <= size());
        return { data(), count };
    }

    template <size_t Count>
    constexpr memory_view<element_type, Count> last() const noexcept {
        memory_view::check_count<Extent>(Count);
        using view = memory_view<element_type, Count>;
        return view{ data() + (size() - Count), Count };
    }

    constexpr memory_view<element_type> last(size_type count) const noexcept {
        MSTL_CONSTEXPR_ASSERT(count <= size());
        return { data() + (size() - count), count };
    }

    template <size_t Offset, size_t Count = dynamic_extent>
    constexpr auto view() const noexcept
    -> memory_view<element_type, view_extent<Offset, Count>()> {
        memory_view::check_count<Extent>(Offset);
        return this->template view_aux<Offset, Count>();
    }

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

using byte_view = memory_view<byte_t>;
using cbyte_view = memory_view<const byte_t>;

MSTL_END_NAMESPACE__
#endif // MSTL_CORE_MEMORY_MEMORY_VIEW_HPP__
