#ifndef MSTL_ARRAY_HPP__
#define MSTL_ARRAY_HPP__
#include "serialize.hpp"
MSTL_BEGIN_NAMESPACE__

template <bool IsConst, size_t Size, typename Array>
struct array_iterator  {
private:
    using container_type	= Array;
    using iterator			= array_iterator<false, Size, container_type>;
    using const_iterator	= array_iterator<true, Size, container_type>;

public:
#ifdef MSTL_STANDARD_20__
    using iterator_category = contiguous_iterator_tag;
#else
    using iterator_category = random_access_iterator_tag;
#endif // MSTL_STANDARD_20__
    using value_type		= typename container_type::value_type;
    using reference			= conditional_t<IsConst, typename container_type::const_reference, typename container_type::reference>;
    using pointer			= conditional_t<IsConst, typename container_type::const_pointer, typename container_type::pointer>;
    using difference_type	= typename container_type::difference_type;
    using size_type			= typename container_type::size_type;

    using self				= array_iterator<IsConst, Size, container_type>;

private:
    pointer ptr_ = nullptr;
    size_t idx_ = 0;

public:
    constexpr array_iterator() noexcept = default;
    constexpr array_iterator(pointer ptr, const size_t off = 0) noexcept
    : ptr_(ptr), idx_(off) {}

    MSTL_NODISCARD constexpr reference operator *() const noexcept {
        return *operator->();
    }
    MSTL_NODISCARD constexpr pointer operator ->() const noexcept {
        MSTL_DEBUG_VERIFY(ptr_ && idx_ < Size, "cannot dereference out of range array iterator");
        return ptr_ + idx_;
    }

    constexpr array_iterator& operator ++() noexcept {
        MSTL_DEBUG_VERIFY(ptr_ && idx_ < Size, "cannot increment array iterator past end");
        ++idx_;
        return *this;
    }
    constexpr array_iterator operator ++(int) noexcept {
        array_iterator tmp = *this;
        ++*this;
        return tmp;
    }
    constexpr array_iterator& operator --() noexcept {
        MSTL_DEBUG_VERIFY(ptr_ && idx_ != 0, "cannot decrement array iterator before begin");
        --idx_;
        return *this;
    }
    constexpr array_iterator operator --(int) noexcept {
        array_iterator tmp = *this;
        --*this;
        return tmp;
    }

    constexpr array_iterator& operator +=(const difference_type n) noexcept {
        idx_ += static_cast<size_t>(n);
        return *this;
    }
    constexpr array_iterator& operator -=(const difference_type n) noexcept {
        return *this += -n;
    }

    MSTL_NODISCARD constexpr difference_type operator -(const array_iterator& rh) const noexcept {
        return static_cast<difference_type>(idx_ - rh.idx_);
    }
    MSTL_NODISCARD constexpr array_iterator operator -(const difference_type n) const noexcept {
        array_iterator tmp = *this;
        tmp -= n;
        return tmp;
    }
    MSTL_NODISCARD constexpr array_iterator operator +(const difference_type n) const noexcept {
        array_iterator tmp = *this;
        tmp += n;
        return tmp;
    }
    MSTL_NODISCARD friend constexpr array_iterator operator +(
        const difference_type n, array_iterator iter) noexcept {
        iter += n;
        return iter;
    }

    MSTL_NODISCARD constexpr reference operator [](const difference_type n) const noexcept {
        return *(*this + n);
    }

    MSTL_NODISCARD constexpr bool operator ==(const array_iterator& rh) const noexcept {
        return idx_ == rh.idx_;
    }
    MSTL_NODISCARD constexpr bool operator!=(const array_iterator& rh) const noexcept {
        return !(*this == rh);
    }
    MSTL_NODISCARD constexpr bool operator <(const array_iterator& rh) const noexcept {
        return idx_ < rh.idx_;
    }
    MSTL_NODISCARD constexpr bool operator >(const array_iterator& rh) const noexcept {
        return rh < *this;
    }
    MSTL_NODISCARD constexpr bool operator <=(const array_iterator& rh) const noexcept {
        return !(rh < *this);
    }
    MSTL_NODISCARD constexpr bool operator >=(const array_iterator& rh) const noexcept {
        return !(*this < rh);
    }
};

template <typename T, size_t Size>
class array : public icollector<array<T, Size>> {
    static_assert(is_object_v<T>, "array only containers of object types.");

    using self = array<T, Size>;
    using base_type = icollector<self>;

public:
    MSTL_BUILD_TYPE_ALIAS(T)
    using iterator                  = array_iterator<false, Size, self>;
    using const_iterator            = array_iterator<true, Size, self>;
    using reverse_iterator          = _MSTL reverse_iterator<iterator>;
    using const_reverse_iterator    = _MSTL reverse_iterator<const_iterator>;

private:
    T array_[Size];

public:
    constexpr array() noexcept = default;
    constexpr array(const self& rhs) noexcept = default;
    constexpr array(self&& rhs) noexcept = default;
    constexpr array(std::initializer_list<T> init) noexcept {
        size_t size = init.size() < Size ? init.size() : Size;
        _MSTL copy(init.begin(), init.begin() + size, array_);
    }
    MSTL_CONSTEXPR20 ~array() noexcept = default;

    MSTL_NODISCARD constexpr iterator begin() noexcept {
        return iterator(array_, 0);
    }
    MSTL_NODISCARD constexpr iterator end() noexcept {
        return iterator(array_, Size);
    }
    MSTL_NODISCARD constexpr const_iterator begin() const noexcept {
        return const_iterator(array_, 0);
    }
    MSTL_NODISCARD constexpr const_iterator end() const noexcept {
        return const_iterator(array_, Size);
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
        return const_iterator(array_, 0);
    }
    MSTL_NODISCARD constexpr const_iterator cend() const noexcept {
        return const_iterator(array_, Size);
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

    MSTL_NODISCARD constexpr bool operator ==(const self& rh) const noexcept {
        return _MSTL equal(this->cbegin(), this->cend(), rh.cbegin());
    }
    MSTL_NODISCARD constexpr bool operator !=(const self& rh) const noexcept {
        return !(*this == rh);
    }
    MSTL_NODISCARD constexpr bool operator <(const self& rh) const noexcept {
        return _MSTL lexicographical_compare(this->cbegin(), this->cend(), rh.cbegin(), rh.cend());
    }
    MSTL_NODISCARD constexpr bool operator >(const self& rh) const noexcept {
        return rh < *this;
    }
    MSTL_NODISCARD constexpr bool operator >=(const self& rh) const noexcept {
        return !(*this < rh);
    }
    MSTL_NODISCARD constexpr bool operator <=(const self& rh) const noexcept {
        return !(*this > rh);
    }

    MSTL_NODISCARD constexpr size_type to_hash() const noexcept {
        return base_type::default_to_hash(*this);
    }

    MSTL_NODISCARD MSTL_CONSTEXPR20 string to_string() const {
        return base_type::default_to_string(*this);
    }
};

MSTL_BEGIN_TAG__
struct empty_array_element_tag {
    constexpr explicit empty_array_element_tag() noexcept = default;
};
MSTL_END_TAG__

template <class T>
class array<T, 0> : public icollector<array<T, 0>> {
    static_assert(is_object_v<T>, "array only containers of object types.");

    using self = array<T, 0>;
    using base_type = icollector<self>;

public:
    MSTL_BUILD_TYPE_ALIAS(T)
    using iterator                  = array_iterator<false, 0, self>;
    using const_iterator            = array_iterator<true, 0, self>;
    using reverse_iterator          = _MSTL reverse_iterator<iterator>;
    using const_reverse_iterator    = _MSTL reverse_iterator<const_iterator>;

private:
    conditional_t<disjunction_v<is_default_constructible<T>, is_implicitly_default_constructible<T>>,
        T, _MSTL_TAG empty_array_element_tag> array_[1]{};

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
        Exception(StopIterator("array empty."));
        return array_[0];
    }

    MSTL_NODISCARD const_reference at(size_type) const {
        Exception(StopIterator("array empty."));
        return array_[0];
    }

    MSTL_NODISCARD reference operator [](size_type) noexcept {
        Exception(StopIterator("array index out of range"));
        return *data();
    }
    MSTL_NODISCARD const_reference operator [](size_type) const noexcept {
        Exception(StopIterator("array index out of range"));
        return *data();
    }

    MSTL_NODISCARD reference front() noexcept {
        Exception(StopIterator("array empty."));
        return *data();
    }
    MSTL_NODISCARD const_reference front() const noexcept {
        Exception(StopIterator("array empty."));
        return *data();
    }

    MSTL_NODISCARD reference back() noexcept {
        Exception(StopIterator("array empty."));
        return *data();
    }

    MSTL_NODISCARD const_reference back() const noexcept {
        Exception(StopIterator("array empty."));
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

    MSTL_NODISCARD MSTL_ALWAYS_INLINE constexpr bool operator ==(const self&) const noexcept { return true; }
    MSTL_NODISCARD MSTL_ALWAYS_INLINE constexpr bool operator !=(const self& rh) const noexcept { return !(*this == rh); }
    MSTL_NODISCARD MSTL_ALWAYS_INLINE constexpr bool operator <(const self&) const noexcept { return false; }
    MSTL_NODISCARD MSTL_ALWAYS_INLINE constexpr bool operator >(const self& rh) const noexcept { return rh < *this; }
    MSTL_NODISCARD MSTL_ALWAYS_INLINE constexpr bool operator <=(const self& rh) const noexcept { return !(rh < *this); }
    MSTL_NODISCARD MSTL_ALWAYS_INLINE constexpr bool operator >=(const self& rh) const noexcept { return !(*this < rh); }

    MSTL_NODISCARD MSTL_ALWAYS_INLINE constexpr size_type to_hash() const noexcept { return FNV_OFFSET_BASIS; }

    MSTL_NODISCARD MSTL_CONSTEXPR20 string to_string() const {
        return base_type::default_to_string(*this);
    }
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
#endif // MSTL_ARRAY_HPP__
