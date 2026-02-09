#ifndef MSTL_CORE_CONTAINER_ARRAY_HPP__
#define MSTL_CORE_CONTAINER_ARRAY_HPP__
#include "MSTL/core/algorithm/compare.hpp"
#include "MSTL/core/algorithm/shift.hpp"
#include "MSTL/core/interface/icollector.hpp"
#include "MSTL/core/interface/iiterator.hpp"
MSTL_BEGIN_NAMESPACE__

template <bool IsConst, size_t Size, typename Array>
struct array_iterator : iiterator<array_iterator<IsConst, Size, Array>> {
public:
    using container_type	= Array;
    using value_type		= typename container_type::value_type;
    using size_type			= typename container_type::size_type;
    using difference_type	= typename container_type::difference_type;
    using iterator_category = contiguous_iterator_tag;
    using reference = conditional_t<IsConst, typename container_type::const_reference, typename container_type::reference>;
    using pointer	= conditional_t<IsConst, typename container_type::const_pointer, typename container_type::pointer>;

private:
    pointer current_ = nullptr;
    const container_type* container_ = nullptr;

public:
    constexpr array_iterator() noexcept = default;
    constexpr ~array_iterator() = default;

    constexpr array_iterator(const array_iterator&) noexcept = default;
    constexpr array_iterator& operator =(const array_iterator&) noexcept = default;
    constexpr array_iterator(array_iterator&&) noexcept = default;
    constexpr array_iterator& operator =(array_iterator&&) noexcept = default;

    constexpr array_iterator(pointer ptr, const container_type* vec) noexcept
    : current_(ptr), container_(vec) {}

    MSTL_NODISCARD constexpr reference dereference() const noexcept {
        MSTL_DEBUG_VERIFY(current_ && container_, "Attempting to dereference on a null pointer");
        MSTL_DEBUG_VERIFY(
            current_ >= container_->data() && current_ < container_->data() + Size,
            "Attempting to dereference out of boundary");
        return *current_;
    }

    constexpr void increment() noexcept {
        MSTL_DEBUG_VERIFY(current_ && container_, "Attempting to increment a null pointer");
        MSTL_DEBUG_VERIFY(
            current_ < container_->data() + Size - 1,
            "Attempting to increment out of boundary");
        ++current_;
    }

    constexpr void decrement() noexcept {
        MSTL_DEBUG_VERIFY(current_ && container_, "Attempting to decrement a null pointer");
        MSTL_DEBUG_VERIFY(
            current_ > container_->data(),
            "Attempting to decrement out of boundary");
        --current_;
    }

    constexpr void advance(difference_type off) noexcept {
        MSTL_DEBUG_VERIFY((current_ && container_) || off == 0, "Attempting to advance a null pointer");
        MSTL_DEBUG_VERIFY(
            current_ + off >= container_->data() && current_ + off <= container_->data() + Size,
            "Attempting to advance out of boundary");
        current_ += off;
    }

    MSTL_NODISCARD constexpr difference_type distance_to(const array_iterator& other) const noexcept {
        MSTL_DEBUG_VERIFY(container_ == other.container_, "Attempting to distance to a different container");
        return static_cast<difference_type>(other.current_ - current_);
    }

    MSTL_NODISCARD constexpr reference operator [](const difference_type off) const noexcept {
        return *(*this + off);
    }

    MSTL_NODISCARD constexpr bool equal(const array_iterator& rhs) const noexcept {
        MSTL_DEBUG_VERIFY(container_ == rhs.container_, "Attempting to equal to a different container");
        return current_ == rhs.current_;
    }

    MSTL_NODISCARD constexpr bool less_than(const array_iterator& rhs) const noexcept {
        MSTL_DEBUG_VERIFY(container_ == rhs.container_, "Attempting to less than a different container");
        return current_ < rhs.current_;
    }

    MSTL_NODISCARD constexpr pointer base() const noexcept {
        return current_;
    }

    MSTL_NODISCARD constexpr const container_type* container() const noexcept {
        return container_;
    }
};


template <typename T, size_t Size>
class array : public icollector<array<T, Size>> {
    static_assert(is_object_v<T>, "array only containers of object types.");

    using base_type = icollector<array>;

public:
    MSTL_BUILD_TYPE_ALIAS(T)
    using iterator                  = array_iterator<false, Size, array>;
    using const_iterator            = array_iterator<true, Size, array>;
    using reverse_iterator          = _MSTL reverse_iterator<iterator>;
    using const_reverse_iterator    = _MSTL reverse_iterator<const_iterator>;

private:
    T array_[Size];

public:
    constexpr array() noexcept = default;
    MSTL_CONSTEXPR20 ~array() noexcept = default;

    constexpr array(const array& rhs) noexcept = default;
    constexpr array& operator =(const array& rhs) noexcept = default;
    constexpr array(array&& rhs) noexcept = default;
    constexpr array& operator =(array&& rhs) noexcept = default;

    constexpr array(std::initializer_list<T> init) noexcept {
        size_t size = init.size() < Size ? init.size() : Size;
        _MSTL copy(init.begin(), init.begin() + size, array_);
    }

    MSTL_NODISCARD constexpr iterator begin() noexcept {
        return iterator(array_, this);
    }
    MSTL_NODISCARD constexpr iterator end() noexcept {
        return iterator(array_, this);
    }
    MSTL_NODISCARD constexpr const_iterator begin() const noexcept {
        return const_iterator(array_, this);
    }
    MSTL_NODISCARD constexpr const_iterator end() const noexcept {
        return const_iterator(array_, this);
    }
    MSTL_NODISCARD constexpr reverse_iterator rbegin() noexcept {
        return reverse_iterator(end());
    }
    MSTL_NODISCARD constexpr reverse_iterator rend() noexcept {
        return reverse_iterator(begin());
    }
    MSTL_NODISCARD constexpr const_reverse_iterator rbegin() const noexcept {
        return const_reverse_iterator(end());
    }
    MSTL_NODISCARD constexpr const_reverse_iterator rend() const noexcept {
        return const_reverse_iterator(begin());
    }
    MSTL_NODISCARD constexpr const_iterator cbegin() const noexcept {
        return const_iterator(array_, this);
    }
    MSTL_NODISCARD constexpr const_iterator cend() const noexcept {
        return const_iterator(array_, this);
    }
    MSTL_NODISCARD constexpr const_reverse_iterator crbegin() const noexcept {
        return reverse_iterator(cend());
    }
    MSTL_NODISCARD constexpr const_reverse_iterator crend() const noexcept {
        return reverse_iterator(cbegin());
    }

    MSTL_NODISCARD MSTL_CONST_FUNCTION MSTL_ALWAYS_INLINE
    constexpr size_type size() const noexcept {
        return Size;
    }
    MSTL_NODISCARD MSTL_CONST_FUNCTION MSTL_ALWAYS_INLINE
    constexpr size_type max_size() const noexcept {
        return Size;
    }
    MSTL_NODISCARD MSTL_CONST_FUNCTION MSTL_ALWAYS_INLINE
    constexpr bool empty() const noexcept {
        return false;
    }

    MSTL_NODISCARD constexpr reference at(size_type n) {
        MSTL_DEBUG_VERIFY(n < Size, "array subscript out of range");
        return array_[n];
    }
    MSTL_NODISCARD constexpr const_reference at(size_type n) const {
        MSTL_DEBUG_VERIFY(n < Size, "array subscript out of range");
        return array_[n];
    }
    MSTL_NODISCARD constexpr reference operator[](size_type n) noexcept {
        MSTL_DEBUG_VERIFY(n < Size, "array subscript out of range");
        return array_[n];
    }
    MSTL_NODISCARD constexpr const_reference operator[](size_type n) const noexcept {
        MSTL_DEBUG_VERIFY(n < Size, "array subscript out of range");
        return array_[n];
    }

    MSTL_NODISCARD constexpr reference front() noexcept {
        return array_[0];
    }
    MSTL_NODISCARD constexpr const_reference front() const noexcept {
        return array_[0];
    }
    MSTL_NODISCARD constexpr reference back() noexcept {
        return array_[Size - 1];
    }
    MSTL_NODISCARD constexpr const_reference back() const noexcept {
        return array_[Size - 1];
    }
    MSTL_NODISCARD MSTL_CONST_FUNCTION MSTL_ALWAYS_INLINE constexpr
    T* data() noexcept {
        return array_;
    }
    MSTL_NODISCARD constexpr const T* data() const noexcept {
        return array_;
    }

    constexpr void fill(const T& value) {
        _MSTL fill_n(array_, Size, value);
    }

    constexpr void swap(array& x) noexcept(is_nothrow_swappable_v<T>) {
        _MSTL swap(array_, x.array_);
    }

    MSTL_NODISCARD constexpr bool operator ==(const array& rhs) const noexcept {
        return _MSTL equal(this->cbegin(), this->cend(), rhs.cbegin());
    }
    MSTL_NODISCARD constexpr bool operator <(const array& rhs) const noexcept {
        return _MSTL lexicographical_compare(this->cbegin(), this->cend(), rhs.cbegin(), rhs.cend());
    }
};

struct empty_array_element_tag {
    constexpr explicit empty_array_element_tag() noexcept = default;
};

template <typename T>
class array<T, 0> : public icollector<array<T, 0>> {
    static_assert(is_object_v<T>, "array only containers of object types.");

    using base_type = icollector<array>;

public:
    MSTL_BUILD_TYPE_ALIAS(T)
    using iterator                  = array_iterator<false, 0, array>;
    using const_iterator            = array_iterator<true, 0, array>;
    using reverse_iterator          = _MSTL reverse_iterator<iterator>;
    using const_reverse_iterator    = _MSTL reverse_iterator<const_iterator>;

private:
    conditional_t<disjunction_v<is_default_constructible<T>, is_implicitly_default_constructible<T>>,
        T, empty_array_element_tag> array_[1]{};

public:
    MSTL_NODISCARD MSTL_CONST_FUNCTION MSTL_ALWAYS_INLINE
    constexpr iterator begin() noexcept {
        return iterator{};
    }
    MSTL_NODISCARD MSTL_CONST_FUNCTION MSTL_ALWAYS_INLINE
    constexpr iterator end() noexcept {
        return iterator{};
    }
    MSTL_NODISCARD MSTL_CONST_FUNCTION MSTL_ALWAYS_INLINE
    constexpr const_iterator begin() const noexcept {
        return const_iterator{};
    }
    MSTL_NODISCARD MSTL_CONST_FUNCTION MSTL_ALWAYS_INLINE
    constexpr const_iterator end() const noexcept {
        return const_iterator{};
    }
    MSTL_NODISCARD MSTL_CONST_FUNCTION MSTL_ALWAYS_INLINE
    constexpr reverse_iterator rbegin() noexcept {
        return reverse_iterator(end());
    }
    MSTL_NODISCARD MSTL_CONST_FUNCTION MSTL_ALWAYS_INLINE
    constexpr reverse_iterator rend() noexcept {
        return reverse_iterator(begin());
    }
    MSTL_NODISCARD MSTL_CONST_FUNCTION MSTL_ALWAYS_INLINE
    constexpr const_reverse_iterator rbegin() const noexcept {
        return const_reverse_iterator(end());
    }
    MSTL_NODISCARD MSTL_CONST_FUNCTION MSTL_ALWAYS_INLINE
    constexpr const_reverse_iterator rend() const noexcept {
        return const_reverse_iterator(begin());
    }
    MSTL_NODISCARD MSTL_ALWAYS_INLINE
    constexpr const_iterator cbegin() const noexcept {
        return begin();
    }
    MSTL_NODISCARD MSTL_ALWAYS_INLINE
    constexpr const_iterator cend() const noexcept {
        return end();
    }
    MSTL_NODISCARD MSTL_ALWAYS_INLINE
    constexpr const_reverse_iterator crbegin() const noexcept {
        return rbegin();
    }
    MSTL_NODISCARD MSTL_ALWAYS_INLINE
    constexpr const_reverse_iterator crend() const noexcept {
        return rend();
    }

    MSTL_NODISCARD MSTL_CONST_FUNCTION MSTL_ALWAYS_INLINE
    constexpr size_type size() const noexcept {
        return 0;
    }
    MSTL_NODISCARD MSTL_CONST_FUNCTION MSTL_ALWAYS_INLINE
    constexpr size_type max_size() const noexcept {
        return 0;
    }
    MSTL_NODISCARD MSTL_CONST_FUNCTION MSTL_ALWAYS_INLINE
    constexpr bool empty() const noexcept {
        return true;
    }

    MSTL_NODISCARD reference at(size_type) {
        throw_exception(iterator_exception("array empty."));
        return array_[0];
    }

    MSTL_NODISCARD const_reference at(size_type) const {
        throw_exception(iterator_exception("array empty."));
        return array_[0];
    }

    MSTL_NODISCARD reference operator [](size_type) noexcept {
        throw_exception(iterator_exception("array index out of range"));
        return *data();
    }
    MSTL_NODISCARD const_reference operator [](size_type) const noexcept {
        throw_exception(iterator_exception("array index out of range"));
        return *data();
    }

    MSTL_NODISCARD reference front() noexcept {
        throw_exception(iterator_exception("array empty."));
        return *data();
    }
    MSTL_NODISCARD const_reference front() const noexcept {
        throw_exception(iterator_exception("array empty."));
        return *data();
    }

    MSTL_NODISCARD reference back() noexcept {
        throw_exception(iterator_exception("array empty."));
        return *data();
    }

    MSTL_NODISCARD const_reference back() const noexcept {
        throw_exception(iterator_exception("array empty."));
        return *data();
    }

    MSTL_NODISCARD MSTL_CONST_FUNCTION MSTL_ALWAYS_INLINE
    constexpr T* data() noexcept {
        return nullptr;
    }
    MSTL_NODISCARD MSTL_CONST_FUNCTION MSTL_ALWAYS_INLINE
    constexpr const T* data() const noexcept {
        return nullptr;
    }

    MSTL_ALWAYS_INLINE constexpr void fill(const T&) {}
    MSTL_ALWAYS_INLINE constexpr void swap(array&) noexcept {}

    MSTL_NODISCARD MSTL_ALWAYS_INLINE constexpr bool operator ==(const array&) const noexcept { return true; }
    MSTL_NODISCARD MSTL_ALWAYS_INLINE constexpr bool operator <(const array&) const noexcept { return false; }
};
#if MSTL_SUPPORT_DEDUCTION_GUIDES__
MSTL_BEGIN_INNER__
template <typename First, typename... Rest>
struct __array_same {
    static_assert(conjunction_v<is_same<First, Rest>...>, "array types mismatch.");
    using type = First;
};
MSTL_END_INNER__

template <typename First, typename... Rest>
array(First, Rest...) -> array<typename _INNER __array_same<First, Rest...>::type, 1 + sizeof...(Rest)>;
#endif // MSTL_SUPPORT_DEDUCTION_GUIDES__


template <size_t Idx, class T, size_t Size>
MSTL_NODISCARD constexpr T& get(array<T, Size>& arr) noexcept {
    static_assert(Idx < Size, "array index out of bounds");
    return arr[Idx];
}
template <size_t Idx, class T, size_t Size>
MSTL_NODISCARD constexpr const T& get(const array<T, Size>& arr) noexcept {
    static_assert(Idx < Size, "array index out of bounds");
    return arr[Idx];
}
template <size_t Idx, class T, size_t Size>
MSTL_NODISCARD constexpr T&& get(array<T, Size>&& arr) noexcept {
    static_assert(Idx < Size, "array index out of bounds");
    return _MSTL move(arr[Idx]);
}
template <size_t Idx, class T, size_t Size>
MSTL_NODISCARD constexpr const T&& get(const array<T, Size>&& arr) noexcept {
    static_assert(Idx < Size, "array index out of bounds");
    return _MSTL move(arr[Idx]);
}


template <typename T, size_t Size>
struct tuple_size<array<T, Size>> : integral_constant<size_t, Size> { };

template <size_t Idx, typename T, size_t Size>
struct tuple_element<Idx, array<T, Size>> {
    static_assert(Idx < Size, "array index is in range");
    using type = T;
};

template <typename T, size_t Size>
MSTL_INLINE17 constexpr size_t tuple_size_v<array<T, Size>> = Size;

template <typename T, size_t Size>
MSTL_INLINE17 constexpr size_t tuple_size_v<const array<T, Size>> = Size;

MSTL_END_NAMESPACE__
#endif // MSTL_CORE_CONTAINER_ARRAY_HPP__
