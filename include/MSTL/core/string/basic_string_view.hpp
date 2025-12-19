#ifndef MSTL_CORE_STRING_BASIC_STRING_VIEW_HPP__
#define MSTL_CORE_STRING_BASIC_STRING_VIEW_HPP__
#include "../iterator/reverse_iterator.hpp"
#include "char_traits.hpp"
MSTL_BEGIN_NAMESPACE__

template <typename CharT, typename Traits = char_traits<CharT>>
class basic_string_view;


template <typename Traits>
class string_view_iterator {
private:
    using container_type	= basic_string_view<typename Traits::char_type, Traits>;
    using iterator			= string_view_iterator<Traits>;
    using const_iterator	= string_view_iterator<Traits>;

public:
#ifdef MSTL_STANDARD_20__
    using iterator_category = contiguous_iterator_tag;
#else
    using iterator_category = random_access_iterator_tag;
#endif // MSTL_STANDARD_20__
    using value_type		= typename container_type::value_type;
    using reference			= typename container_type::const_reference;
    using pointer			= typename container_type::const_pointer;
    using difference_type	= typename container_type::difference_type;
    using size_type			= typename container_type::size_type;

private:
    pointer data_ = nullptr;
    size_t size_ = 0;
    size_t idx_ = 0;

    friend basic_string_view<value_type, Traits>;

public:
    constexpr string_view_iterator() noexcept = default;

    constexpr string_view_iterator(const pointer data, const size_t size, const size_t off) noexcept
        : data_(data), size_(size), idx_(off) {}

    MSTL_NODISCARD constexpr reference operator *() const noexcept {
        MSTL_DEBUG_VERIFY(data_, __MSTL_DEBUG_MESG_OPERATE_NULLPTR(string_view_iterator, __MSTL_DEBUG_TAG_DEREFERENCE));
        MSTL_DEBUG_VERIFY(idx_ < size_, __MSTL_DEBUG_MESG_OUT_OF_RANGE(string_view_iterator, __MSTL_DEBUG_TAG_DEREFERENCE));
        return data_[idx_];
    }

    MSTL_NODISCARD constexpr pointer operator ->() const noexcept {
        return &operator*();
    }

    constexpr string_view_iterator& operator ++() noexcept {
        MSTL_DEBUG_VERIFY(data_, __MSTL_DEBUG_MESG_OPERATE_NULLPTR(string_view_iterator, __MSTL_DEBUG_TAG_INCREMENT));
        MSTL_DEBUG_VERIFY(idx_ < size_, __MSTL_DEBUG_MESG_OUT_OF_RANGE(string_view_iterator, __MSTL_DEBUG_TAG_INCREMENT));
        ++idx_;
        return *this;
    }

    constexpr string_view_iterator operator ++(int) noexcept {
        string_view_iterator tmp(*this);
        ++*this;
        return tmp;
    }

    constexpr string_view_iterator& operator --() noexcept {
        MSTL_DEBUG_VERIFY(data_, __MSTL_DEBUG_MESG_OPERATE_NULLPTR(string_view_iterator, __MSTL_DEBUG_TAG_DECREMENT));
        MSTL_DEBUG_VERIFY(idx_ != 0, __MSTL_DEBUG_MESG_OUT_OF_RANGE(string_view_iterator, __MSTL_DEBUG_TAG_DECREMENT));
        --idx_;
        return *this;
    }

    constexpr string_view_iterator operator --(int) noexcept {
        string_view_iterator tmp(*this);
        --*this;
        return tmp;
    }

    constexpr string_view_iterator& operator +=(const difference_type n) noexcept {
        if (n < 0) {
            MSTL_DEBUG_VERIFY(data_ || n == 0, __MSTL_DEBUG_MESG_OPERATE_NULLPTR(vector_iterator, __MSTL_DEBUG_TAG_DECREMENT));
            MSTL_DEBUG_VERIFY(idx_ >= static_cast<size_t>(0) - static_cast<size_t>(n),
                __MSTL_DEBUG_MESG_OUT_OF_RANGE(vector_iterator, __MSTL_DEBUG_TAG_DECREMENT));
        }
        else if (n > 0) {
            MSTL_DEBUG_VERIFY(data_ || n == 0, __MSTL_DEBUG_MESG_OPERATE_NULLPTR(vector_iterator, __MSTL_DEBUG_TAG_INCREMENT));
            MSTL_DEBUG_VERIFY(size_ - idx_ >= static_cast<size_t>(n),
                __MSTL_DEBUG_MESG_OUT_OF_RANGE(vector_iterator, __MSTL_DEBUG_TAG_INCREMENT));
        }
        idx_ += n;
        return *this;
    }
    MSTL_NODISCARD constexpr string_view_iterator operator +(const difference_type n) const noexcept {
        string_view_iterator tmp = *this;
        tmp += n;
        return tmp;
    }
    MSTL_NODISCARD friend constexpr string_view_iterator operator +(const difference_type n, const string_view_iterator& iter) noexcept {
        return iter + n;
    }

    constexpr string_view_iterator& operator -=(const difference_type n) noexcept {
        idx_ += -n;
        return *this;
    }
    MSTL_NODISCARD constexpr string_view_iterator operator -(const difference_type n) const noexcept {
        string_view_iterator tmp = *this;
        tmp -= n;
        return tmp;
    }
    MSTL_NODISCARD constexpr difference_type operator -(const string_view_iterator& iter) const noexcept {
        MSTL_DEBUG_VERIFY(data_ == iter.data_ && size_ == iter.size_,
            __MSTL_DEBUG_MESG_CONTAINER_INCOMPATIBLE(string_view_iterator));
        return static_cast<difference_type>(idx_ - iter.idx_);
    }

    MSTL_NODISCARD constexpr reference operator [](const difference_type n) const noexcept {
        return *(*this + n);
    }

    MSTL_NODISCARD constexpr bool operator ==(const string_view_iterator& iter) const noexcept {
        MSTL_DEBUG_VERIFY(data_ == iter.data_ && size_ == iter.size_,
            __MSTL_DEBUG_MESG_CONTAINER_INCOMPATIBLE(string_view_iterator));
        return idx_ == iter.idx_;
    }
    MSTL_NODISCARD constexpr bool operator !=(const string_view_iterator& iter) const noexcept {
        return !(*this == iter);
    }
    MSTL_NODISCARD constexpr bool operator <(const string_view_iterator& iter) const noexcept {
        MSTL_DEBUG_VERIFY(data_ == iter.data_ && size_ == iter.size_,
            __MSTL_DEBUG_MESG_CONTAINER_INCOMPATIBLE(string_view_iterator));
        return idx_ < iter.idx_;
    }
    MSTL_NODISCARD constexpr bool operator >(const string_view_iterator& iter) const noexcept {
        return iter < *this;
    }
    MSTL_NODISCARD constexpr bool operator <=(const string_view_iterator& iter) const noexcept {
        return !(iter < *this);
    }
    MSTL_NODISCARD constexpr bool operator >=(const string_view_iterator& iter) const noexcept {
        return !(*this < iter);
    }
};


template <typename CharT, typename Traits>
class basic_string_view : public icommon<basic_string_view<CharT, Traits>> {
    static_assert(is_same_v<CharT, typename Traits::char_type>,
        "char type of basic string view should be same with char traits.");
    static_assert(!is_array_v<CharT> && is_trivial_v<CharT> && is_standard_layout_v<CharT>,
        "basic string view only contains non-array trivial standard-layout types.");

public:
    MSTL_BUILD_TYPE_ALIAS(CharT)
    using traits_type               = Traits;

    using const_iterator            = string_view_iterator<Traits>;
    using iterator                  = const_iterator;
    using const_reverse_iterator    = _MSTL reverse_iterator<const_iterator>;
    using reverse_iterator          = const_reverse_iterator;

    static constexpr auto npos = static_cast<size_type>(-1);

private:
    const_pointer data_ = nullptr;
    size_type size_ = 0;

    MSTL_NODISCARD MSTL_ALWAYS_INLINE
    constexpr size_type clamp_size(const size_type position, const size_type size) const noexcept {
        return _MSTL min(size, size_ - position);
    }

public:
    constexpr basic_string_view() noexcept = default;

    constexpr basic_string_view(const basic_string_view&) noexcept = default;
    constexpr basic_string_view& operator =(const basic_string_view&) noexcept = default;

    constexpr basic_string_view(const_pointer str) noexcept
        : data_(str), size_(Traits::length(str)) {}
    constexpr basic_string_view(const_pointer str, const size_type n) noexcept
        : data_(str), size_(n) {}

    template <typename Iterator, enable_if_t<is_same_v<iter_value_t<Iterator>, value_type>, int> = 0>
    constexpr basic_string_view(Iterator start, Iterator finish)
    : data_(&*start), size_(_MSTL distance(start, finish)) {}

    MSTL_CONSTEXPR20 ~basic_string_view() noexcept = default;

    MSTL_NODISCARD constexpr const_iterator begin() const noexcept { return const_iterator(data_, size_, 0); }
    MSTL_NODISCARD constexpr const_iterator end() const noexcept { return const_iterator(data_, size_, size_); }
    MSTL_NODISCARD constexpr const_iterator cbegin() const noexcept { return begin(); }
    MSTL_NODISCARD constexpr const_iterator cend() const noexcept { return end(); }
    MSTL_NODISCARD constexpr const_reverse_iterator rbegin() const noexcept { return const_reverse_iterator(end()); }
    MSTL_NODISCARD constexpr const_reverse_iterator rend() const noexcept { return const_reverse_iterator(begin()); }
    MSTL_NODISCARD constexpr const_reverse_iterator crbegin() const noexcept { return rbegin(); }
    MSTL_NODISCARD constexpr const_reverse_iterator crend() const noexcept { return rend(); }

    MSTL_NODISCARD constexpr size_type size() const noexcept { return size_; }
    MSTL_NODISCARD constexpr size_type max_size() const noexcept {
        return (npos - sizeof(size_type) - POINTER_SIZE) / sizeof(value_type) / 4;
    }
    MSTL_NODISCARD constexpr size_type length() const noexcept { return size_; }
    MSTL_NODISCARD constexpr bool empty() const noexcept { return size_ == 0; }

    MSTL_NODISCARD constexpr const_pointer data() const noexcept { return data_; }
    MSTL_NODISCARD constexpr const_pointer to_cstring() const noexcept { return this->data(); }

    MSTL_NODISCARD constexpr const_reference front() const noexcept {
        MSTL_DEBUG_VERIFY(!empty(), "cannot call front on empty string_view");
        return data_[0];
    }
    MSTL_NODISCARD constexpr const_reference back() const noexcept {
        MSTL_DEBUG_VERIFY(!empty(), "cannot call back on empty string_view");
        return data_[size_ - 1];
    }

    MSTL_NODISCARD constexpr const_reference operator [](const size_type n) const noexcept {
        MSTL_DEBUG_VERIFY(n < size_, "basic string view index out of ranges.");
        return data_[n];
    }
    MSTL_NODISCARD constexpr const_reference at(const size_type n) const {
        MSTL_DEBUG_VERIFY(n < size_, "basic string view index out of ranges.");
        return data_[n];
    }

    constexpr void remove_prefix(const size_type n) noexcept {
        MSTL_DEBUG_VERIFY(size_ >= n, "cannot remove prefix longer than total size");
        data_ += n;
        size_ -= n;
    }
    constexpr void remove_suffix(const size_type n) noexcept {
        MSTL_DEBUG_VERIFY(size_ >= n, "cannot remove suffix longer than total size");
        size_ -= n;
    }

    constexpr size_type copy(CharT* const str, size_type count, const size_type off = 0) const {
        MSTL_DEBUG_VERIFY(off < size_, "basic string view index out of ranges.");
        count = clamp_size(off, count);
        Traits::copy(str, data_ + off, count);
        return count;
    }

    MSTL_NODISCARD constexpr basic_string_view substr(const size_type off = 0, size_type count = npos) const {
        MSTL_DEBUG_VERIFY(off < size_, "basic string view index out of ranges.");
        count = clamp_size(off, count);
        return basic_string_view(data_ + off, count);
    }
    
    MSTL_NODISCARD constexpr basic_string_view view(const size_type off, size_type count = npos) const {
        return substr(off, count);
    }

    MSTL_NODISCARD constexpr int compare(const basic_string_view view) const noexcept {
        return (char_traits_compare<Traits>)(data_, size_, view.data_, view.size_);
    }
    MSTL_NODISCARD constexpr int compare(const size_type off, const size_type n, const basic_string_view view) const {
        return substr(off, n).compare(view);
    }
    MSTL_NODISCARD constexpr int compare(const size_type off, const size_type n, const basic_string_view view,
        const size_type roff, const size_type count) const {
        return substr(off, n).compare(view.substr(roff, count));
    }
    MSTL_NODISCARD constexpr int compare(const CharT* const str) const noexcept {
        return compare(basic_string_view(str));
    }
    MSTL_NODISCARD constexpr int compare(const size_type off, const size_type n, const CharT* const str) const {
        return substr(off, n).compare(basic_string_view(str));
    }
    MSTL_NODISCARD constexpr int compare(const size_type off, const size_type n,
        const CharT* const str, const size_type count) const {
        return substr(off, n).compare(basic_string_view(str, count));
    }

    MSTL_NODISCARD constexpr size_type find(const basic_string_view view, const size_type n = 0) const noexcept {
        return (char_traits_find<Traits>)(data_, size_, n, view.data_, view.size_);
    }
    MSTL_NODISCARD constexpr size_type find(const CharT chr, const size_type n = 0) const noexcept {
        return (char_traits_find_char<Traits>)(data_, size_, n, chr);
    }
    MSTL_NODISCARD constexpr size_type find(const CharT* const str,
        const size_type off, const size_type count) const noexcept {
        return (char_traits_find<Traits>)(data_, size_, off, str, count);
    }
    MSTL_NODISCARD constexpr size_type find(const CharT* const str, const size_type off = 0) const noexcept {
        return (char_traits_find<Traits>)(data_, size_, off, str, Traits::length(str));
    }

    MSTL_NODISCARD constexpr size_type rfind(const basic_string_view view, const size_type off = npos) const noexcept {
        return (char_traits_rfind<Traits>)(data_, size_, off, view.data_, view.size_);
    }
    MSTL_NODISCARD constexpr size_type rfind(const CharT chr, const size_type n = npos) const noexcept {
        return (char_traits_rfind_char<Traits>)(data_, size_, n, chr);
    }
    MSTL_NODISCARD constexpr size_type rfind(const CharT* const str, const size_type off,
        const size_type n) const noexcept {
        return (char_traits_rfind<Traits>)(data_, size_, off, str, n);
    }
    MSTL_NODISCARD constexpr size_type rfind(const CharT* const str, const size_type off = npos) const noexcept {
        return (char_traits_rfind<Traits>)(data_, size_, off, str, Traits::length(str));
    }

    MSTL_NODISCARD constexpr size_type find_first_of(const basic_string_view view, const size_type off = 0) const noexcept {
        return (char_traits_find_first_of<Traits>)(data_, size_, off, view.data_, view.size_);
    }
    MSTL_NODISCARD constexpr size_type find_first_of(const CharT chr, const size_type off = 0) const noexcept {
        return (char_traits_find_char<Traits>)(data_, size_, off, chr);
    }
    MSTL_NODISCARD constexpr size_type find_first_of(const CharT* const str, const size_type off,
        const size_type n) const noexcept {
        return (char_traits_find_first_of<Traits>)(data_, size_, off, str, n);
    }
    MSTL_NODISCARD constexpr size_type find_first_of(const CharT* const str, const size_type off = 0) const noexcept {
        return (char_traits_find_first_of<Traits>)(data_, size_, off, str, Traits::length(str));
    }

    MSTL_NODISCARD constexpr size_type find_last_of(const basic_string_view view, const size_type off = npos) const noexcept {
        return (char_traits_find_last_of<Traits>)(data_, size_, off, view.data_, view.size_);
    }
    MSTL_NODISCARD constexpr size_type find_last_of(const CharT chr, const size_type off = npos) const noexcept {
        return (char_traits_rfind_char<Traits>)(data_, size_, off, chr);
    }
    MSTL_NODISCARD constexpr size_type find_last_of(const CharT* const str, const size_type off,
        const size_type n) const noexcept {
        return (char_traits_find_last_of<Traits>)(data_, size_, off, str, n);
    }
    MSTL_NODISCARD constexpr size_type find_last_of(const CharT* const str, const size_type off = npos) const noexcept {
        return (char_traits_find_last_of<Traits>)(data_, size_, off, str, Traits::length(str));
    }

    MSTL_NODISCARD constexpr size_type find_first_not_of(const basic_string_view view,
        const size_type off = 0) const noexcept {
        return (char_traits_find_first_not_of<Traits>)(data_, size_, off, view.data_, view.size_);
    }
    MSTL_NODISCARD constexpr size_type find_first_not_of(const CharT chr, const size_type off = 0) const noexcept {
        return (char_traits_find_not_char<Traits>)(data_, size_, off, chr);
    }
    MSTL_NODISCARD constexpr size_type find_first_not_of(const CharT* const str, const size_type off,
        const size_type n) const noexcept {
        return (char_traits_find_first_not_of<Traits>)(data_, size_, off, str, n);
    }
    MSTL_NODISCARD constexpr size_type find_first_not_of(const CharT* const str, const size_type off = 0) const noexcept {
        return (char_traits_find_first_not_of<Traits>)(data_, size_, off, str, Traits::length(str));
    }

    MSTL_NODISCARD constexpr size_type find_last_not_of(const basic_string_view view,
        const size_type off = npos) const noexcept {
        return (char_traits_find_last_not_of<Traits>)(data_, size_, off, view.data_, view.size_);
    }
    MSTL_NODISCARD constexpr size_type find_last_not_of(const CharT chr, const size_type off = npos) const noexcept {
        return (char_traits_rfind_not_char<Traits>)(data_, size_, off, chr);
    }
    MSTL_NODISCARD constexpr size_type find_last_not_of(const CharT* const str, const size_type off,
        const size_type n) const noexcept {
        return (char_traits_find_last_not_of<Traits>)(data_, size_, off, str, n);
    }
    MSTL_NODISCARD constexpr size_type find_last_not_of(const CharT* const str,
        const size_type off = npos) const noexcept {
        return (char_traits_find_last_not_of<Traits>)(data_, size_, off, str, Traits::length(str));
    }

    MSTL_CONSTEXPR20 size_type count(value_type chr, const size_type position = 0) const noexcept {
        size_type n = 0;
        for (size_type idx = position; idx < size_; ++idx) {
            if (*(data() + idx) == chr) ++n;
        }
        return n;
    }

    MSTL_NODISCARD MSTL_CONSTEXPR20 bool starts_with(const basic_string_view view) const noexcept {
        return view.size() <= size_ && traits_type::compare(data(), view.data(), view.size()) == 0;
    }
    MSTL_NODISCARD MSTL_CONSTEXPR20 bool starts_with(value_type chr) const noexcept {
        return !empty() && traits_type::eq(front(), chr);
    }
    MSTL_NODISCARD MSTL_CONSTEXPR20 bool starts_with(const_pointer str) const noexcept {
        return this->starts_with(view_type(str));
    }

    MSTL_NODISCARD MSTL_CONSTEXPR20 bool ends_with(const basic_string_view view) const noexcept {
        const size_type view_size = view.size();
        return view_size <= size_ && traits_type::compare(data_ + size_ - view_size, view.data(), view_size) == 0;
    }
    MSTL_NODISCARD MSTL_CONSTEXPR20 bool ends_with(value_type chr) const noexcept {
        return !empty() && traits_type::eq(back(), chr);
    }
    MSTL_NODISCARD MSTL_CONSTEXPR20 bool ends_with(const_pointer str) const noexcept {
        return this->ends_with(view_type(str));
    }

    MSTL_NODISCARD MSTL_CONSTEXPR20 bool contains(const basic_string_view view) const noexcept {
        return this->find(view) != npos;
    }
    MSTL_NODISCARD MSTL_CONSTEXPR20 bool contains(value_type chr) const noexcept {
        return this->find(chr) != npos;
    }
    MSTL_NODISCARD MSTL_CONSTEXPR20 bool contains(const_pointer str) const noexcept {
        return this->find(str) != npos;
    }

    MSTL_NODISCARD MSTL_CONSTEXPR20 basic_string_view trim_left() const noexcept {
        return this->trim_left_if([](value_type ch) { return _MSTL is_space(ch); });
    }
    MSTL_NODISCARD MSTL_CONSTEXPR20 basic_string_view trim_right() const noexcept {
        return this->trim_right_if([](value_type ch) { return _MSTL is_space(ch); });
    }
    MSTL_NODISCARD MSTL_CONSTEXPR20 basic_string_view trim() const noexcept {
        return this->trim_left().trim_right(); 
    }

    template <typename Predicate>
    MSTL_NODISCARD MSTL_CONSTEXPR20 basic_string_view trim_left_if(Predicate pred) const
    noexcept(noexcept(pred(*cbegin()))) {
        if (empty()) return *this;

        const_iterator it = cbegin();
        while (it != cend() && pred(*it))
            ++it;

        if (it != cbegin())
            return basic_string_view(data_ + (it - cbegin()), size_ - (it - cbegin()));
        
        return *this;
    }

    template <typename Predicate>
    MSTL_NODISCARD MSTL_CONSTEXPR20 basic_string_view trim_right_if(Predicate pred) const
    noexcept(noexcept(pred(*crbegin()))) {
        if (empty()) return *this;

        const_reverse_iterator rit = crbegin();
        while (rit != crend() && pred(*rit))
            ++rit;

        if (rit != crbegin())
            return basic_string_view(data_, size_ - (rit - crbegin()));
        
        return *this;
    }

    template <typename Predicate>
    MSTL_NODISCARD MSTL_CONSTEXPR20 basic_string_view trim_if(Predicate pred) const
    noexcept(noexcept(this->trim_right_if(pred)) && noexcept(this->trim_left_if(pred))) {
        return this->trim_left_if(pred).trim_right_if(pred);
    }

    MSTL_NODISCARD MSTL_CONSTEXPR20 bool equal_to(const basic_string_view str) const noexcept {
        return (char_traits_equal<Traits>)(data_, size_, str.data_, str.size_);
    }
    
    MSTL_NODISCARD MSTL_CONSTEXPR20 bool equal_to(const CharT* str) const noexcept {
        return equal_to(view_type(str));
    }

    constexpr void swap(basic_string_view& view) noexcept {
        const basic_string_view tmp(view);
        view = *this;
        *this = tmp;
    }

    MSTL_NODISCARD constexpr bool operator ==(const basic_string_view& rh) const noexcept { return this->equal_to(rh); }
    MSTL_NODISCARD constexpr bool operator <(const basic_string_view& rh) const noexcept { return this->compare(rh) < 0; }

    MSTL_NODISCARD constexpr size_t to_hash() const noexcept {
        return _INNER FNV_hash_string(this->data(), this->length());
    }
};

MSTL_END_NAMESPACE__
#endif // MSTL_CORE_STRING_BASIC_STRING_VIEW_HPP__
