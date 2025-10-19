#ifndef MSTL_BASIC_STRING_HPP__
#define MSTL_BASIC_STRING_HPP__
#include "string_view.hpp"
#include "algo.hpp"
MSTL_BEGIN_NAMESPACE__

template <typename CharT, typename Traits, typename Alloc>
class basic_string;

template <bool IsConst, typename String>
struct basic_string_iterator {
private:
    using container_type	= String;
    using iterator			= basic_string_iterator<false, container_type>;
    using const_iterator	= basic_string_iterator<true, container_type>;

public:
#ifdef MSTL_VERSION_20__
    using iterator_category = contiguous_iterator_tag;
#else
    using iterator_category = random_access_iterator_tag;
#endif // MSTL_VERSION_20__
    using value_type		= typename container_type::value_type;
    using reference			= conditional_t<IsConst, typename container_type::const_reference, typename container_type::reference>;
    using pointer			= conditional_t<IsConst, typename container_type::const_pointer, typename container_type::pointer>;
    using difference_type	= typename container_type::difference_type;
    using size_type			= typename container_type::size_type;

    using self				= basic_string_iterator<IsConst, String>;

private:
    pointer ptr_ = nullptr;
    const container_type* str_ = nullptr;

    template <typename, typename, typename> friend class basic_string;
    template <bool, typename> friend struct basic_string_iterator;

public:
    MSTL_CONSTEXPR20 basic_string_iterator() = default;
    MSTL_CONSTEXPR20 basic_string_iterator(pointer ptr) : ptr_(ptr) {}
    MSTL_CONSTEXPR20 basic_string_iterator(pointer ptr, const container_type* vec) : ptr_(ptr), str_(vec) {}

    MSTL_CONSTEXPR20 basic_string_iterator(const iterator& x) noexcept
    : ptr_(const_cast<pointer>(x.ptr_)), str_(x.str_) {}

    MSTL_CONSTEXPR20 self& operator =(const iterator& rh) noexcept {
        if (_MSTL addressof(rh) == this) return *this;
        ptr_ = const_cast<pointer>(rh.ptr_);
        str_ = rh.str_;
        return *this;
    }

    MSTL_CONSTEXPR20 basic_string_iterator(iterator&& x) noexcept
    : ptr_(const_cast<pointer>(x.ptr_)), str_(x.str_) {
        x.ptr_ = nullptr;
        x.str_ = nullptr;
    }

    MSTL_CONSTEXPR20 self& operator =(iterator&& rh) noexcept {
        if (_MSTL addressof(rh) == this) return *this;
        ptr_ = const_cast<pointer>(rh.ptr_);
        str_ = rh.str_;
        rh.ptr_ = nullptr;
        rh.str_ = nullptr;
        return *this;
    }

    MSTL_CONSTEXPR20 basic_string_iterator(const const_iterator& x) noexcept
    : ptr_(const_cast<pointer>(x.ptr_)), str_(x.str_) {}

    MSTL_CONSTEXPR20 self& operator =(const const_iterator& rh) noexcept {
        if (_MSTL addressof(rh) == this) return *this;
        ptr_ = const_cast<pointer>(rh.ptr_);
        str_ = rh.str_;
        return *this;
    }

    MSTL_CONSTEXPR20 basic_string_iterator(const_iterator&& x) noexcept
    : ptr_(const_cast<pointer>(x.ptr_)), str_(x.str_) {
        x.ptr_ = nullptr;
        x.str_ = nullptr;
    }

    MSTL_CONSTEXPR20 self& operator =(const_iterator&& rh) noexcept {
        if (_MSTL addressof(rh) == this) return *this;
        ptr_ = const_cast<pointer>(rh.ptr_);
        str_ = rh.str_;
        rh.ptr_ = nullptr;
        rh.str_ = nullptr;
        return *this;
    }

    MSTL_CONSTEXPR20 ~basic_string_iterator() noexcept = default;

    MSTL_NODISCARD MSTL_CONSTEXPR20 reference operator *()  const noexcept {
        MSTL_DEBUG_VERIFY(ptr_ && str_, __MSTL_DEBUG_MESG_OPERATE_NULLPTR(basic_string_iterator, __MSTL_DEBUG_TAG_DEREFERENCE));
        MSTL_DEBUG_VERIFY(str_->data_ <= ptr_ && ptr_ <= str_->data_ + str_->size_,
            __MSTL_DEBUG_MESG_OUT_OF_RANGE(basic_string_iterator, __MSTL_DEBUG_TAG_DEREFERENCE));
        return *ptr_;
    }
    MSTL_NODISCARD MSTL_CONSTEXPR20 pointer operator ->() const noexcept {
        return &operator*();
    }

    MSTL_CONSTEXPR20 self& operator ++() noexcept {
        MSTL_DEBUG_VERIFY(ptr_ && str_, __MSTL_DEBUG_MESG_OPERATE_NULLPTR(basic_string_iterator, __MSTL_DEBUG_TAG_INCREMENT));
        MSTL_DEBUG_VERIFY(ptr_ < str_->data_ + str_->size_, __MSTL_DEBUG_MESG_OUT_OF_RANGE(basic_string_iterator, __MSTL_DEBUG_TAG_INCREMENT));
        ++ptr_;
        return *this;
    }
    MSTL_CONSTEXPR20 self  operator ++(int) noexcept {
        self tmp = *this;
        ++*this;
        return tmp;
    }

    MSTL_CONSTEXPR20 self& operator --() noexcept {
        MSTL_DEBUG_VERIFY(ptr_ && str_, __MSTL_DEBUG_MESG_OPERATE_NULLPTR(basic_string_iterator, __MSTL_DEBUG_TAG_DECREMENT));
        MSTL_DEBUG_VERIFY(str_->data_ < ptr_, __MSTL_DEBUG_MESG_OUT_OF_RANGE(basic_string_iterator, __MSTL_DEBUG_TAG_DECREMENT));
        --ptr_; 
        return *this;
    }
    MSTL_CONSTEXPR20 self  operator --(int) noexcept {
        self tmp = *this;
        --*this;
        return tmp;
    }

    MSTL_CONSTEXPR20 self& operator +=(difference_type n) noexcept {
        if (n < 0) {
            MSTL_DEBUG_VERIFY((ptr_ && str_) || n == 0, __MSTL_DEBUG_MESG_OPERATE_NULLPTR(basic_string_iterator, __MSTL_DEBUG_TAG_DECREMENT));
            MSTL_DEBUG_VERIFY(n >= str_->data_ - ptr_, __MSTL_DEBUG_MESG_OUT_OF_RANGE(basic_string_iterator, __MSTL_DEBUG_TAG_DECREMENT));
        }
        else if (n > 0) {
            MSTL_DEBUG_VERIFY((ptr_ && str_) || n == 0, __MSTL_DEBUG_MESG_OPERATE_NULLPTR(basic_string_iterator, __MSTL_DEBUG_TAG_INCREMENT));
            MSTL_DEBUG_VERIFY(n <= str_->data_ + str_->size_ - ptr_, __MSTL_DEBUG_MESG_OUT_OF_RANGE(basic_string_iterator, __MSTL_DEBUG_TAG_INCREMENT));
        }
        ptr_ += n;
        return *this;
    }
    MSTL_NODISCARD MSTL_CONSTEXPR20 self operator +(const difference_type n) const noexcept {
        self tmp = *this;
        tmp += n;
        return tmp;
    }
    MSTL_NODISCARD friend MSTL_CONSTEXPR20 self operator +(const difference_type n, const self& it) noexcept {
        return it + n;
    }

    MSTL_CONSTEXPR20 self& operator -=(difference_type n) noexcept {
        ptr_ += -n;
        return *this;
    }
    MSTL_NODISCARD MSTL_CONSTEXPR20 self operator -(difference_type n) const noexcept {
        auto tmp = *this;
        tmp -= n;
        return tmp;
    }
    MSTL_NODISCARD MSTL_CONSTEXPR20 difference_type operator -(const self& rh) const noexcept {
        return static_cast<difference_type>(ptr_ - rh.ptr_);
    }

    MSTL_NODISCARD MSTL_CONSTEXPR20 reference operator [](difference_type n) const noexcept {
        return *(*this + n);
    }

    MSTL_NODISCARD MSTL_CONSTEXPR20 bool operator ==(const self& rh) const noexcept {
		MSTL_DEBUG_VERIFY(str_ == rh.str_, __MSTL_DEBUG_MESG_CONTAINER_INCOMPATIBLE(vector_iterator));
        return ptr_ == rh.ptr_;
    }
    MSTL_NODISCARD MSTL_CONSTEXPR20 bool operator !=(const self& rh) const noexcept {
        return !(*this == rh);
    }
    MSTL_NODISCARD MSTL_CONSTEXPR20 bool operator <(const self& rh) const noexcept {
		MSTL_DEBUG_VERIFY(str_ == rh.str_, __MSTL_DEBUG_MESG_CONTAINER_INCOMPATIBLE(vector_iterator));
        return ptr_ < rh.ptr_;
    }
    MSTL_NODISCARD MSTL_CONSTEXPR20 bool operator >(const self& rh) const noexcept {
        return rh < *this;
    }
    MSTL_NODISCARD MSTL_CONSTEXPR20 bool operator <=(const self& rh) const noexcept {
        return !(*this > rh);
    }
    MSTL_NODISCARD MSTL_CONSTEXPR20 bool operator >=(const self& rh) const noexcept {
        return !(*this < rh);
    }
};

template <typename CharT, typename Traits = char_traits<CharT>, typename Alloc = allocator<CharT>>
class basic_string : icommon<basic_string<CharT, Traits, Alloc>> {
#ifdef MSTL_VERSION_20__	
    static_assert(is_allocator_v<Alloc>, "Alloc type is not a standard allocator type.");
#endif
    static_assert(is_same_v<CharT, typename Alloc::value_type>, "allocator type mismatch.");
    static_assert(is_same_v<CharT, typename Traits::char_type>, "trait type mismatch.");
    static_assert(!is_array_v<CharT> && is_trivial_v<CharT> && is_standard_layout_v<CharT>, 
        "basic string only contains non-array trivial standard-layout types.");

    using self = basic_string<CharT, Traits, Alloc>;
    
public:
    MSTL_BUILD_TYPE_ALIAS(CharT)
    
    using iterator                  = basic_string_iterator<false, self>;
    using const_iterator            = basic_string_iterator<true, self>;
    using reverse_iterator          = _MSTL reverse_iterator<iterator>;
    using const_reverse_iterator    = _MSTL reverse_iterator<const_iterator>;
    
    using traits_type               = Traits;
    using view_type                 = basic_string_view<CharT, Traits>;
    using allocator_type            = Alloc;

    static constexpr size_type npos = static_cast<size_type>(-1);

private:
    pointer data_ = nullptr;
    size_type size_ = 0;
    compressed_pair<allocator_type, size_type> capacity_pair_{ _MSTL_TAG default_construct_tag{}, 0 };

    template <bool, typename> friend struct basic_string_iterator;

private:
    MSTL_ALWAYS_INLINE MSTL_CONSTEXPR20 void range_check(const size_type n) const noexcept {
        MSTL_DEBUG_VERIFY(n < size_, "basic_string index out of ranges.");
    }

    MSTL_ALWAYS_INLINE MSTL_CONSTEXPR20 size_type clamp_size(
        const size_type position, const size_type size) const noexcept {
        return _MSTL min(size, size_ - position);
    }

    MSTL_CONSTEXPR20 void null_terminate() {
        if (data_ && size_ < capacity_pair_.value) {
            traits_type::assign(data_ + size_, 1, value_type());
        } else if (size_ == capacity_pair_.value) {
            this->reserve(capacity_pair_.value * 1.5);
            traits_type::assign(data_ + size_, 1, value_type());
        }
    }

    template <typename Iterator, enable_if_t<!is_ranges_fwd_iter_v<Iterator>, int> = 0>
    MSTL_CONSTEXPR20 void construct_from_iter(Iterator first, Iterator last) {
        size_type n = _MSTL distance(first, last);
        const size_type init_size = _MSTL max(MEMORY_ALIGN_THRESHHOLD, n + 1);
        pointer temp_data = nullptr;
        size_type temp_size = 0;
        try {
            temp_data = capacity_pair_.get_base().allocate(init_size);
            _MSTL uninitialized_copy(temp_data, temp_data + temp_size, first);

            data_ = temp_data;
            size_ = n;
            capacity_pair_.value = init_size;
            null_terminate();
        } catch (...) {
            if (temp_data) {
                _MSTL destroy(temp_data, temp_data + n);
                capacity_pair_.get_base().deallocate(temp_data, capacity_pair_.value);
            }
            destroy_buffer();
            throw;
        }
    }

    template <typename Iterator, enable_if_t<is_ranges_fwd_iter_v<Iterator>, int> = 0>
    MSTL_CONSTEXPR20 void construct_from_iter(Iterator first, Iterator last) {
        const size_type n = _MSTL distance(first, last);
        const size_type init_size = _MSTL max(MEMORY_ALIGN_THRESHHOLD, n + 1);
        pointer temp_data = nullptr;
        try {
            temp_data = capacity_pair_.get_base().allocate(init_size);
            size_ = n;
            capacity_pair_.value = init_size;
            _MSTL uninitialized_copy(first, last, temp_data);

            data_ = temp_data;
            size_ = n;
            capacity_pair_.value = init_size;
            null_terminate();
        }
        catch (...) {
            if (temp_data) {
                _MSTL destroy(temp_data, temp_data + n);
                capacity_pair_.get_base().deallocate(temp_data, capacity_pair_.value);
            }
            destroy_buffer();
            throw;
        }
    }

    MSTL_CONSTEXPR20 void construct_from_ptr(const_pointer str, size_type position, size_type n) {
        pointer temp_data = nullptr;
        size_type temp_capacity = 0;
        try {
            temp_capacity = _MSTL max(MEMORY_ALIGN_THRESHHOLD, n + 1);
            temp_data = capacity_pair_.get_base().allocate(temp_capacity);
            traits_type::copy(temp_data, str + position, n);

            data_ = temp_data;
            size_ = n;
            capacity_pair_.value = temp_capacity;
            null_terminate();
        } catch (...) {
            if (temp_data) {
                capacity_pair_.get_base().deallocate(temp_data, capacity_pair_.value);
            }
            data_ = nullptr;
            size_ = 0;
            capacity_pair_.value = 0;
            throw;
        }
    }

    MSTL_CONSTEXPR20 void destroy_buffer() noexcept {
        if (data_) {
            if (capacity_pair_.value > 0) {
                capacity_pair_.get_base().deallocate(data_, capacity_pair_.value);
            }
            data_ = nullptr;
        }
        capacity_pair_.value = 0;
        size_ = 0;
    }

    MSTL_CONSTEXPR20 basic_string& replace_fill(iterator first, size_type n1, const size_type n2, const value_type chr) {
        if (static_cast<size_type>(end() - first) < n1) {
            n1 = cend() - first;
        }
        if (n1 < n2) {
            const size_type diff = n2 - n1;
            MSTL_DEBUG_VERIFY(size_ + diff < max_size(), "basic_string index out of range.");
            if (size_ > capacity_pair_.value - diff)
                reallocate(diff);
            pointer raw_ptr = &*first;
            traits_type::move(raw_ptr + n2, raw_ptr + n1, end() - (first + n1));
            traits_type::assign(raw_ptr, n2, chr);
            size_ += diff;
        }
        else {
            pointer raw_ptr = &*first;
            traits_type::move(raw_ptr + n2, raw_ptr + n1, end() - (first + n1));
            traits_type::assign(raw_ptr, n2, chr);
            size_ -= n1 - n2;
        }
        null_terminate();
        return *this;
    }

    template <typename Iterator,
        enable_if_t<is_iter_v<Iterator> && is_same_v<iter_val_t<Iterator>, value_type>, int> = 0>
    MSTL_CONSTEXPR20 basic_string& replace_copy(iterator first1, iterator last1, Iterator first2, Iterator last2) {
        size_type len1 = _MSTL distance(first1, last1);
        size_type len2 = _MSTL distance(first2, last2);
        if (len1 < len2) {
            const size_type diff = len2 - len1;
            MSTL_DEBUG_VERIFY(size_ + diff < max_size(), "basic_string replace_copy index out of range.");
            if (size_ > capacity_pair_.value - diff)
                reallocate(diff);
            pointer raw_ptr = &*first1;
            traits_type::move(raw_ptr + len2, raw_ptr + len1, end() - (first1 + len1));
            traits_type::copy(raw_ptr, &*first2, len2);
            size_ += diff;
        }
        else {
            pointer raw_ptr = &*first1;
            traits_type::move(raw_ptr + len2, raw_ptr + len1, end() - (first1 + len1));
            traits_type::copy(raw_ptr, &*first2, len2);
            size_ -= len1 - len2;
        }
        null_terminate();
        return *this;
    }

    template <typename Iterator>
    MSTL_CONSTEXPR20 basic_string& replace_copy(iterator first1, const size_type n1, Iterator first2, const size_type n2) {
        return this->replace_copy(first1, first1 + n1, first2, _MSTL next(first2, n2));
    }

    MSTL_CONSTEXPR20 void reallocate(size_type n) {
        pointer new_buffer = nullptr;
        try {
            const size_t new_cap = _MSTL max(
               capacity_pair_.value + n,
               capacity_pair_.value + (capacity_pair_.value >> 1)
            ) + 1;
            new_buffer = capacity_pair_.get_base().allocate(new_cap);
            traits_type::move(new_buffer, data_, size_);

            capacity_pair_.get_base().deallocate(data_, capacity_pair_.value);
            data_ = new_buffer;
            capacity_pair_.value = new_cap;
            null_terminate();
        } catch(...) {
            if (new_buffer) {
                capacity_pair_.get_base().deallocate(new_buffer, capacity_pair_.value);
            }
            throw;
        }
    }

    MSTL_CONSTEXPR20 iterator reallocate_and_fill(iterator position, size_type n, value_type chr) {
        const difference_type diff = (&*position) - data_;
        const size_t old_cap = capacity_pair_.value;
        const size_t new_cap = _MSTL max(old_cap + n, old_cap + (old_cap >> 1));
        pointer new_buffer = capacity_pair_.get_base().allocate(new_cap);
        pointer end1 = traits_type::move(new_buffer, data_, diff) + diff;
        pointer end2 = traits_type::assign(end1, n, chr) + n;
        traits_type::move(end2, data_ + diff, size_ - diff);
        capacity_pair_.get_base().deallocate(data_, old_cap);
        data_ = new_buffer;
        size_ += n;
        capacity_pair_.value = new_cap;
        null_terminate();
        return data_ + diff;
    }

    MSTL_CONSTEXPR20 iterator reallocate_and_copy(iterator position, const_iterator first, const_iterator last) {
        const difference_type diff = position - data_;
        const size_type old_cap = capacity_pair_.value;
        const size_type n = _MSTL distance(first, last);
        const size_t new_cap = _MSTL max(old_cap + n, old_cap + (old_cap >> 1));
        pointer new_buffer = capacity_pair_.get_base().allocate(new_cap);
        pointer end1 = traits_type::move(new_buffer, data_, diff) + diff;
        pointer end2 = _MSTL uninitialized_copy_n(first, n, end1) + n;
        traits_type::move(end2, data_ + diff, size_ - diff);
        capacity_pair_.get_base().deallocate(data_, old_cap);
        data_ = new_buffer;
        size_ += n;
        capacity_pair_.value = new_cap;
        null_terminate();
        return data_ + diff;
    }

public:
    MSTL_CONSTEXPR20 basic_string() {
        pointer temp_data = nullptr;
        size_type temp_capacity = MEMORY_ALIGN_THRESHHOLD;
        try {
            temp_data = capacity_pair_.get_base().allocate(temp_capacity);

            data_ = temp_data;
            size_ = 0;
            capacity_pair_.value = temp_capacity;
            null_terminate();
        } catch (...) {
            if (temp_data) {
                capacity_pair_.get_base().deallocate(temp_data, capacity_pair_.value);
            }
            destroy_buffer();
            throw;
        }
    }

    MSTL_CONSTEXPR20 explicit basic_string(size_type n) : basic_string(n, '\0') {}

    MSTL_CONSTEXPR20 explicit basic_string(size_type n, int32_t chr)
    : basic_string(n, static_cast<value_type>(chr)) {}

    MSTL_CONSTEXPR20 explicit basic_string(size_type n, value_type chr) {
        const size_type init_size = _MSTL max(MEMORY_ALIGN_THRESHHOLD, n + 1);
        data_ = capacity_pair_.get_base().allocate(init_size);
        traits_type::assign(data_, n, chr);
        size_ = n;
        capacity_pair_.value = init_size;
        null_terminate();
    }

    MSTL_CONSTEXPR20 self& operator =(value_type chr) {
        if (capacity_pair_.value < 1) {
            pointer new_buffer = capacity_pair_.get_base().allocate(2);
            capacity_pair_.get_base().deallocate(data_);
            data_ = new_buffer;
            capacity_pair_.value = 2;
        }
        *data_ = chr;
        size_ = 1;
        null_terminate();
        return *this;
    }

    MSTL_CONSTEXPR20 basic_string(const self& str) {
        this->construct_from_ptr(str.data_, 0, str.size_);
    }

    MSTL_CONSTEXPR20 self& operator =(const self& str) {
        if (_MSTL addressof(str) == this) return *this;
        destroy_buffer();
        this->construct_from_ptr(str.data_, 0, str.size_);
        return *this;
    }

    MSTL_CONSTEXPR20 basic_string(self&& str) noexcept
    : data_(_MSTL move(str.data_)), size_(str.size_),
    capacity_pair_(_MSTL move(str.capacity_pair_)) {
        str.data_ = nullptr;
        str.size_ = 0;
        str.capacity_pair_.value = 0;
    }

    MSTL_CONSTEXPR20 self& operator =(self&& str) noexcept {
        if (_MSTL addressof(str) == this) return *this;

        destroy_buffer();
        data_ = str.data_;
        size_ = str.size_;
        capacity_pair_ = _MSTL move(str.capacity_pair_);

        str.data_ = nullptr;
        str.size_ = 0;
        str.capacity_pair_.value = 0;

        return *this;
    }

    MSTL_CONSTEXPR20 explicit basic_string(view_type str) {
        this->construct_from_ptr(str.data(), 0, str.size());
    }

    MSTL_CONSTEXPR20 basic_string(view_type str, const size_type n) {
        this->construct_from_ptr(str.data(), 0, n);
    }

    MSTL_CONSTEXPR20 self& operator =(view_type str) {
        const size_type len = str.size();
        if (capacity_pair_.value < len) {
            pointer new_buffer = capacity_pair_.get_base().allocate(len + 1);
            capacity_pair_.get_base().deallocate(data_);
            data_ = new_buffer;
            capacity_pair_.value = len + 1;
        }
        traits_type::copy(data_, str.data(), len);
        size_ = len;
        null_terminate();
        return *this;
    }

    MSTL_CONSTEXPR20 basic_string(const self& str, size_type position) {
        this->construct_from_ptr(str.data_, position, str.size_ - position);
    }

    MSTL_CONSTEXPR20 basic_string(const self& str, size_type position, size_type n) {
        this->construct_from_ptr(str.data_, position, n);
    }

    MSTL_CONSTEXPR20 basic_string(const_pointer str) {
        this->construct_from_ptr(str, 0, traits_type::length(str));
    }

    MSTL_CONSTEXPR20 basic_string(const_pointer str, const size_type n) {
        this->construct_from_ptr(str, 0, n);
    }

    MSTL_CONSTEXPR20 self& operator =(const_pointer str) {
        const size_type len = traits_type::length(str);
        if (capacity_pair_.value < len) {
            pointer new_buffer = capacity_pair_.get_base().allocate(len + 1);
            capacity_pair_.get_base().deallocate(data_);
            data_ = new_buffer;
            capacity_pair_.value = len + 1;
        }
        traits_type::copy(data_, str, len);
        size_ = len;
        null_terminate();
        return *this;
    }

    template <typename Iterator, enable_if_t<!is_convertible_v<Iterator, value_type>, int> = 0>
    MSTL_CONSTEXPR20 basic_string(Iterator first, Iterator last) {
        this->construct_from_iter(first, last);
    }

    MSTL_CONSTEXPR20 basic_string(std::initializer_list<value_type> l) : basic_string(l.begin(), l.end()) {}
    MSTL_CONSTEXPR20 self& operator =(std::initializer_list<value_type> l) {
        this->clear();
        this->insert(begin(), l.begin(), l.end());
        return *this;
    }

    MSTL_CONSTEXPR20 ~basic_string() { destroy_buffer(); }

    MSTL_NODISCARD MSTL_CONSTEXPR20 iterator begin() noexcept { return {data_, this}; }
    MSTL_NODISCARD MSTL_CONSTEXPR20 iterator end() noexcept { return {data_ + size_, this}; }
    MSTL_NODISCARD MSTL_CONSTEXPR20 const_iterator begin() const noexcept { return cbegin(); }
    MSTL_NODISCARD MSTL_CONSTEXPR20 const_iterator end() const noexcept { return cend(); }
    MSTL_NODISCARD MSTL_CONSTEXPR20 const_iterator cbegin() const noexcept { return {data_, this}; }
    MSTL_NODISCARD MSTL_CONSTEXPR20 const_iterator cend() const noexcept { return {data_ + size_, this}; }
    MSTL_NODISCARD MSTL_CONSTEXPR20 reverse_iterator rbegin() noexcept { return reverse_iterator(end()); }
    MSTL_NODISCARD MSTL_CONSTEXPR20 reverse_iterator rend() noexcept { return reverse_iterator(begin()); }
    MSTL_NODISCARD MSTL_CONSTEXPR20 const_reverse_iterator rbegin() const noexcept { return crbegin(); }
    MSTL_NODISCARD MSTL_CONSTEXPR20 const_reverse_iterator rend() const noexcept { return crend(); }
    MSTL_NODISCARD MSTL_CONSTEXPR20 const_reverse_iterator crbegin() const noexcept { return const_reverse_iterator(cend()); }
    MSTL_NODISCARD MSTL_CONSTEXPR20 const_reverse_iterator crend() const noexcept { return const_reverse_iterator(cbegin()); }

    MSTL_NODISCARD MSTL_CONSTEXPR20 size_type size() const noexcept { return size_; }
    MSTL_NODISCARD MSTL_CONSTEXPR20 size_type max_size() const noexcept { return npos; }
    MSTL_NODISCARD MSTL_CONSTEXPR20 size_type capacity() const noexcept { return capacity_pair_.value; }
    MSTL_NODISCARD MSTL_CONSTEXPR20 size_type length()   const noexcept { return size_; }
    MSTL_NODISCARD MSTL_CONSTEXPR20 bool empty() const noexcept { return size_ == 0; }

    MSTL_NODISCARD MSTL_CONSTEXPR20 allocator_type get_allocator() const noexcept { return allocator_type(); }

    MSTL_CONSTEXPR20 void reserve(const size_type n) {
        MSTL_DEBUG_VERIFY(n < max_size(), "basic_string reserve index out of range.");
        const size_type new_cap = n + 1;
        if (capacity_pair_.value >= new_cap) return;

        pointer new_buffer = capacity_pair_.get_base().allocate(new_cap);
        traits_type::move(new_buffer, data_, size_);
        capacity_pair_.get_base().deallocate(data_, capacity_pair_.value);

        data_ = new_buffer;
        capacity_pair_.value = new_cap;
        null_terminate();
    }

    MSTL_NODISCARD MSTL_CONSTEXPR20 reference operator [](const size_type n) noexcept {
        MSTL_DEBUG_VERIFY(n <= size_, "basic_string [] index out of range.");
        return *(data_ + n);
    }
    MSTL_NODISCARD MSTL_CONSTEXPR20 const_reference operator [](const size_type n) const noexcept {
        MSTL_DEBUG_VERIFY(n <= size_, "basic_string [] index out of range.");
        return *(data_ + n);
    }

    MSTL_NODISCARD MSTL_CONSTEXPR20 reference at(const size_type n) noexcept {
        range_check(n);
        return (*this)[n];
    }
    MSTL_NODISCARD MSTL_CONSTEXPR20 const_reference at(const size_type n) const noexcept {
        range_check(n);
        return (*this)[n];
    }

    MSTL_NODISCARD MSTL_CONSTEXPR20 reference front() noexcept {
        MSTL_DEBUG_VERIFY(!empty(), "front called on empty basic_string");
        return *data_;
    }
    MSTL_NODISCARD MSTL_CONSTEXPR20 const_reference front() const noexcept {
        MSTL_DEBUG_VERIFY(!empty(), "front called on empty basic_string");
        return *data_;
    }
    MSTL_NODISCARD MSTL_CONSTEXPR20 reference back() noexcept {
        MSTL_DEBUG_VERIFY(!empty(), "back called on empty basic_string");
        return *(data_ + size_ - 1);
    }
    MSTL_NODISCARD MSTL_CONSTEXPR20 const_reference back() const noexcept {
        MSTL_DEBUG_VERIFY(!empty(), "back called on empty basic_string");
        return *(data_ + size_ - 1);
    }

    MSTL_NODISCARD MSTL_CONSTEXPR20 pointer data() noexcept { return data_; }
    MSTL_NODISCARD MSTL_CONSTEXPR20 const_pointer data() const noexcept { return data_; }

    MSTL_NODISCARD MSTL_CONSTEXPR20 const_pointer c_str() const noexcept { return data_; }

    MSTL_CONSTEXPR20 iterator insert(iterator position, value_type chr) {
        if (size_ == capacity_pair_.value)
            return this->reallocate_and_fill(position, 1, chr);
        traits_type::move(position + 1, position, end() - position);
        ++size_;
        *position = chr;
        null_terminate();
        return position;
    }

    MSTL_CONSTEXPR20 self& insert(size_type position, size_type n, value_type chr) {
        this->insert(begin() + position, n, chr);
        return *this;
    }

    MSTL_CONSTEXPR20 iterator insert(iterator position, size_type n, value_type chr) {
        if (n == 0) return position;
        if (capacity_pair_.value - size_ < n)
            return this->reallocate_and_fill(position, n, chr);
        if (position == end()) {
            traits_type::assign(&*end(), n, chr);
            size_ += n;
            return position;
        }
        traits_type::move(&*(position + n), &*position, n);
        traits_type::assign(&*position, n, chr);
        size_ += n;
        null_terminate();
        return position;
    }

    template <typename Iterator>
    MSTL_CONSTEXPR20 iterator insert(iterator position, Iterator first, Iterator last) {
        const size_type len = _MSTL distance(first, last);
        if (len == 0) return position;

        if (capacity_pair_.value - size_ < len)
            return reallocate_and_copy(position, first, last);
        if (position == end()) {
            _MSTL uninitialized_copy(first, last, end());
            size_ += len;
            return position;
        }
        traits_type::move(position + len, position, len);
        _MSTL uninitialized_copy(first, last, position);
        size_ += len;
        null_terminate();
        return position;
    }

    MSTL_CONSTEXPR20 void push_back(value_type chr) {
        this->append(1, chr);
    }

    MSTL_CONSTEXPR20 void pop_back() noexcept {
        MSTL_DEBUG_VERIFY(!empty(), "pop_back called on empty basic_string");
        --size_;
        null_terminate();
    }

    MSTL_CONSTEXPR20 self& append(size_type n, value_type chr) {
        MSTL_DEBUG_VERIFY(size_ + n < max_size(), "basic_string append iterator out of ranges.");
        if (capacity_pair_.value - size_ <= n) reallocate(n);
        traits_type::assign(data_ + size_, n, chr);
        size_ += n;
        null_terminate();
        return *this;
    }
    MSTL_CONSTEXPR20 self& append(value_type chr) { return this->append(1, chr); }

    MSTL_CONSTEXPR20 self& append(const self& str, size_type position, size_type n) {
        MSTL_DEBUG_VERIFY(size_ + n < max_size(), "basic_string append iterator out of ranges.");
        if (n == 0) return *this;

        if (capacity_pair_.value - size_ <= n) reallocate(n);
        traits_type::copy(data_ + size_, str.data_ + position, n);
        size_ += n;
        null_terminate();
        return *this;
    }
    MSTL_CONSTEXPR20 self& append(const self& str) {
        return this->append(str, 0, str.size_);
    }
    MSTL_CONSTEXPR20 self& append(const self& str, size_type position) {
        return this->append(str, position, str.size_ - position);
    }

    MSTL_CONSTEXPR20 self& append(self&& str, size_type position, size_type n) {
        MSTL_DEBUG_VERIFY(size_ + n < max_size(), "basic_string append iterator out of ranges.");
        if (n == 0) return *this;

        if (capacity_pair_.value - size_ <= n) reallocate(n);
        traits_type::move(data_ + size_, str.data_ + position, n);
        size_ += n;
        null_terminate();
        str.destroy_buffer();
        return *this;
    }
    MSTL_CONSTEXPR20 self& append(self&& str) {
        return this->append(_MSTL move(str), 0, str.size_);
    }
    MSTL_CONSTEXPR20 self& append(self&& str, size_type position) {
        return this->append(_MSTL move(str), position, str.size_ - position);
    }

    MSTL_CONSTEXPR20 self& append(view_type str, size_type n) {
        return this->append(str.data(), n);
    }
    MSTL_CONSTEXPR20 self& append(view_type str) {
        return this->append(str.data(), str.size());
    }

    MSTL_CONSTEXPR20 self& append(const_pointer str, size_type n) {
        MSTL_DEBUG_VERIFY(size_ + n < max_size(), "basic_string append iterator out of ranges.");
        if (capacity_pair_.value - size_ <= n) reallocate(n);
        traits_type::copy(data_ + size_, str, n);
        size_ += n;
        null_terminate();
        return *this;
    }
    MSTL_CONSTEXPR20 self& append(const_pointer str) {
        return this->append(str, traits_type::length(str));
    }

    template <typename Iterator, enable_if_t<is_iter_v<Iterator>, int> = 0>
    MSTL_CONSTEXPR20 self& append(Iterator first, Iterator last) {
        const size_type n = _MSTL distance(first, last);
        MSTL_DEBUG_VERIFY(size_ + n < max_size(), "basic_string append iterator out of ranges.");

        if (capacity_pair_.value - size_ <= n) reallocate(n);
        _MSTL uninitialized_copy_n(first, n, data_ + size_);
        size_ += n;
        null_terminate();
        return *this;
    }

    MSTL_CONSTEXPR20 self& append(std::initializer_list<value_type> l) {
        return this->append(l.begin(), l.end());
    }

    MSTL_CONSTEXPR20 self& assign(const self& str) { return *this = str; }
    MSTL_CONSTEXPR20 self& assign(self&& str) { return *this = _MSTL move(str); }
    MSTL_CONSTEXPR20 self& assign(const_pointer str) { return *this = str; }
    MSTL_CONSTEXPR20 self& assign(const_pointer str, const size_type n) {
        this->clear();
        return this->append(str, n);
    }
    MSTL_CONSTEXPR20 self& assign(const size_type n, value_type chr) {
        this->clear();
        return this->append(n, chr);
    }
    MSTL_CONSTEXPR20 self& assign(std::initializer_list<value_type> ilist) {
        return *this = ilist;
    }
    MSTL_CONSTEXPR20 self& assign(const view_type& view) {
        return *this = view;
    }
    template <typename Iterator>
    MSTL_CONSTEXPR20 self& assign(Iterator first, Iterator last) {
        this->clear();
        return this->append(first, last);
    }

    MSTL_CONSTEXPR20 self& operator +=(const self& str) { return this->append(str); }
    MSTL_CONSTEXPR20 self& operator +=(self&& str) { return this->append(_MSTL move(str)); }
    MSTL_CONSTEXPR20 self& operator +=(value_type chr) { return this->append(chr); }
    MSTL_CONSTEXPR20 self& operator +=(const_pointer str) { return this->append(str, str + traits_type::length(str)); }
    MSTL_CONSTEXPR20 self& operator +=(std::initializer_list<value_type> lls) { return this->append(lls); }
    MSTL_CONSTEXPR20 self& operator +=(view_type view) { return this->append(view); }

    MSTL_CONSTEXPR20 iterator erase(iterator position) noexcept {
        MSTL_DEBUG_VERIFY(position != end(), "");
        traits_type::move(position, position + 1, end() - position - 1);
        --size_;
        null_terminate();
        return position;
    }

    MSTL_CONSTEXPR20 self& erase(size_type first = 0, size_type n = npos) noexcept {
        this->erase(begin() + first, n);
        return *this;
    }

    MSTL_CONSTEXPR20 iterator erase(iterator first, size_type n) noexcept {
        return this->erase(first, first + n);
    }

    MSTL_CONSTEXPR20 iterator erase(iterator first, iterator last) noexcept {
        if (first == begin() && last == end()) {
            this->clear();
            return end();
        }
        const size_type n = end() - last;
        traits_type::move(&*first, &*last, n);
        size_ -= (last - first);
        null_terminate();
        return first;
    }


    MSTL_CONSTEXPR20 void resize(size_type count, value_type chr) {
        if (count < size_)
            this->erase({data_ + count, this}, {data_ + size_, this});
        else
            this->append(count - size_, chr);
    }

    MSTL_CONSTEXPR20 void resize(const size_type count) {
        this->resize(count, value_type());
    }

    template <typename Operation>
    MSTL_CONSTEXPR20 void resize_and_overwrite(size_type count, Operation op) {
        if (count + 1 > capacity_pair_.value) {
            this->reserve(count + 1);
        }
        const size_type actual_size = op(data_, count);
        MSTL_DEBUG_VERIFY(actual_size <= count, "resize_and_overwrite: operation returned size larger than requested");
        size_ = actual_size;
        null_terminate();
    }


    MSTL_CONSTEXPR20 void clear() noexcept {
        size_ = 0;
        null_terminate();
    }

    MSTL_CONSTEXPR20 void shrink_to_fit() {
        if (size_ + 1 >= capacity_pair_.value) return;
        pointer new_buffer = capacity_pair_.get_base().allocate(size_ + 1);
        traits_type::move(new_buffer, data_, size_);
        capacity_pair_.get_base().deallocate(data_, capacity_pair_.value);
        data_ = new_buffer;
        capacity_pair_.value = size_ + 1;
        null_terminate();
    }


    MSTL_NODISCARD MSTL_CONSTEXPR20 self substr(const size_type off = 0, size_type count = npos) const {
        range_check(off);
        count = clamp_size(off, count);
        return self(data_ + off, count);
    }

    MSTL_NODISCARD constexpr view_type view(const size_type off = 0, size_type count = npos) const noexcept {
        range_check(off);
        count = clamp_size(off, count);
        return view_type(data_ + off, count);
    }

    MSTL_CONSTEXPR20 size_type copy(pointer dest, size_type count, size_type position = 0) const {
        MSTL_DEBUG_VERIFY(position <= size_, "basic_string::copy: position out of range");

        const size_type len = _MSTL min(count, size_ - position);
        traits_type::copy(dest, data_ + position, len);
        return len;
    }


    MSTL_NODISCARD MSTL_CONSTEXPR20 int compare(const self& str) const noexcept {
        return (char_traits_compare<Traits>)(data_, size_, str.data_, str.size_);
    }
    MSTL_NODISCARD MSTL_CONSTEXPR20 int compare(const size_type off, const size_type n, const self& str) const {
        return substr(off, n).compare(str);
    }
    MSTL_NODISCARD MSTL_CONSTEXPR20 int compare(const size_type off, const size_type n, const self& str,
        const size_type roff, const size_type count) const {
        return substr(off, n).compare(str.substr(roff, count));
    }
    MSTL_NODISCARD MSTL_CONSTEXPR20 int compare(const CharT* const str) const noexcept {
        return this->compare(self(str));
    }
    MSTL_NODISCARD MSTL_CONSTEXPR20 int compare(const string_view& str) const noexcept {
        return this->compare(self(str));
    }
    MSTL_NODISCARD MSTL_CONSTEXPR20 int compare(const size_type off, const size_type n, const CharT* const str) const {
        return substr(off, n).compare(self(str));
    }
    MSTL_NODISCARD MSTL_CONSTEXPR20 int compare(const size_type off, const size_type n,
        const CharT* const str, const size_type count) const {
        return substr(off, n).compare(self(str, count));
    }

    MSTL_CONSTEXPR20 self& replace(const size_type position, const size_type n, const self& str) {
        range_check(position);
        return this->replace_copy(begin() + position, n, str.data_, str.size_);
    }
    MSTL_CONSTEXPR20 self& replace(iterator first, iterator last, const self& str) {
        MSTL_DEBUG_VERIFY(begin() <= first && last <= end() && first <= last, "basic_string replace iterator out of ranges.");
        return this->replace_copy(first, last - first, str.data_, str.size_);
    }
    MSTL_CONSTEXPR20 self& replace(const size_type position, const size_type n, const_pointer str) {
        range_check(position);
        return this->replace_copy({data_ + position, this}, n, str, traits_type::length(str));
    }
    MSTL_CONSTEXPR20 self& replace(iterator first, iterator last, const_pointer str) {
        MSTL_DEBUG_VERIFY(begin() <= first && last <= end() && first <= last, "basic_string replace iterator out of ranges.");
        return this->replace_copy(first, last - first, str, traits_type::length(str));
    }
    MSTL_CONSTEXPR20 self& replace(const size_type position, const size_type n1, const_pointer str, const size_type n2) {
        range_check(position);
        return this->replace_copy(data_ + position, n1, str, n2);
    }
    MSTL_CONSTEXPR20 self& replace(iterator first, iterator last, const_pointer str, const size_type n) {
        MSTL_DEBUG_VERIFY(begin() <= first && last <= end() && first <= last, "basic_string replace iterator out of ranges.");
        return this->replace_copy(first, last - first, str, n);
    }
    MSTL_CONSTEXPR20 self& replace(const size_type position, const size_type n1, const size_type n2, const value_type chr) {
        range_check(position);
        return this->replace_fill(data_ + position, n1, n2, chr);
    }
    MSTL_CONSTEXPR20 self& replace(iterator first, iterator last, const size_type n, const value_type chr) {
        MSTL_DEBUG_VERIFY(begin() <= first && last <= end() && first <= last, "basic_string replace iterator out of ranges.");
        return this->replace_fill(first, static_cast<size_type>(last - first), n, chr);
    }
    MSTL_CONSTEXPR20 self& replace(const size_type position1, const size_type n1, const self& str,
        const size_type position2, const size_type n2 = npos) {
        range_check(position1);
        range_check(position2);
        return this->replace_copy(data_ + position1, n1, str.data_ + position2, n2);
    }
    template <typename Iterator>
    MSTL_CONSTEXPR20 self& replace(iterator first, iterator last, Iterator first2, Iterator last2) {
        MSTL_DEBUG_VERIFY(begin() <= first && last <= end() && first <= last, "basic_string replace iterator out of ranges.");
        return this->replace_copy(first, last, first2, last2);
    }

    MSTL_CONSTEXPR20 void reverse() noexcept {
        if (size() < 2) return;
        for (iterator first = begin(), last = end(); first < last; ) _MSTL iter_swap(first++, --last);
    }

    MSTL_NODISCARD MSTL_CONSTEXPR20 size_type find(const self& str, const size_type n = 0) const noexcept {
        return (char_traits_find<Traits>)(data_, size_, n, str.data_, str.size_);
    }
    MSTL_NODISCARD MSTL_CONSTEXPR20 size_type find(const CharT chr, const size_type n = 0) const noexcept {
        return (char_traits_find_char<Traits>)(data_, size_, n, chr);
    }
    MSTL_NODISCARD MSTL_CONSTEXPR20 size_type find(const CharT* const str,
        const size_type off, const size_type count) const noexcept {
        return (char_traits_find<Traits>)(data_, size_, off, str, count);
    }
    MSTL_NODISCARD MSTL_CONSTEXPR20 size_type find(const CharT* const str, const size_type off = 0) const noexcept {
        return (char_traits_find<Traits>)(data_, size_, off, str, Traits::length(str));
    }
    MSTL_NODISCARD MSTL_CONSTEXPR20 size_type find(
        const string_view& str, const size_type off, const size_type count) const noexcept {
        return (char_traits_find<Traits>)(data_, size_, off, str.data(), count);
    }
    MSTL_NODISCARD MSTL_CONSTEXPR20 size_type find(const string_view& str, const size_type off = 0) const noexcept {
        return (char_traits_find<Traits>)(data_, size_, off, str.data(), str.size());
    }

    MSTL_NODISCARD MSTL_CONSTEXPR20 size_type rfind(const self& str, const size_type off = npos) const noexcept {
        return (char_traits_rfind<Traits>)(data_, size_, off, str.data_, str.size_);
    }
    MSTL_NODISCARD MSTL_CONSTEXPR20 size_type rfind(const CharT chr, const size_type n = npos) const noexcept {
        return (char_traits_rfind_char<Traits>)(data_, size_, n, chr);
    }
    MSTL_NODISCARD MSTL_CONSTEXPR20 size_type rfind(const CharT* const str, const size_type off,
        const size_type n) const noexcept {
        return (char_traits_rfind<Traits>)(data_, size_, off, str, n);
    }
    MSTL_NODISCARD MSTL_CONSTEXPR20 size_type rfind(const CharT* const str, const size_type off = npos) const noexcept {
        return (char_traits_rfind<Traits>)(data_, size_, off, str, Traits::length(str));
    }
    MSTL_NODISCARD MSTL_CONSTEXPR20 size_type rfind(
        const string_view& str, const size_type off, const size_type count) const noexcept {
        return (char_traits_rfind<Traits>)(data_, size_, off, str.data(), count);
    }
    MSTL_NODISCARD MSTL_CONSTEXPR20 size_type rfind(const string_view& str, const size_type off = 0) const noexcept {
        return (char_traits_rfind<Traits>)(data_, size_, off, str.data(), str.size());
    }

    MSTL_NODISCARD MSTL_CONSTEXPR20 size_type find_first_of(const self& str, const size_type off = 0) const noexcept {
        return (char_traits_find_first_of<Traits>)(data_, size_, off, str.data_, str.size_);
    }
    MSTL_NODISCARD MSTL_CONSTEXPR20 size_type find_first_of(const CharT chr, const size_type off = 0) const noexcept {
        return (char_traits_find_char<Traits>)(data_, size_, off, chr);
    }
    MSTL_NODISCARD MSTL_CONSTEXPR20 size_type find_first_of(const CharT* const str, const size_type off,
        const size_type n) const noexcept {
        return (char_traits_find_first_of<Traits>)(data_, size_, off, str, n);
    }
    MSTL_NODISCARD MSTL_CONSTEXPR20 size_type find_first_of(const CharT* const str, const size_type off = 0) const noexcept {
        return (char_traits_find_first_of<Traits>)(data_, size_, off, str, Traits::length(str));
    }
    MSTL_NODISCARD MSTL_CONSTEXPR20 size_type find_first_of(
        const string_view& str, const size_type off, const size_type n) const noexcept {
        return (char_traits_find_first_of<Traits>)(data_, size_, off, str.data(), n);
    }
    MSTL_NODISCARD MSTL_CONSTEXPR20 size_type find_first_of(const string_view& str, const size_type off = 0) const noexcept {
        return (char_traits_find_first_of<Traits>)(data_, size_, off, str.data(), str.size());
    }

    MSTL_NODISCARD MSTL_CONSTEXPR20 size_type find_last_of(const self& str, const size_type off = npos) const noexcept {
        return (char_traits_find_last_of<Traits>)(data_, size_, off, str.data_, str.size_);
    }
    MSTL_NODISCARD MSTL_CONSTEXPR20 size_type find_last_of(const CharT chr, const size_type off = npos) const noexcept {
        return (char_traits_rfind_char<Traits>)(data_, size_, off, chr);
    }
    MSTL_NODISCARD MSTL_CONSTEXPR20 size_type find_last_of(const CharT* const str, const size_type off,
        const size_type n) const noexcept {
        return (char_traits_find_last_of<Traits>)(data_, size_, off, str, n);
    }
    MSTL_NODISCARD MSTL_CONSTEXPR20 size_type find_last_of(const CharT* const str, const size_type off = npos) const noexcept {
        return (char_traits_find_last_of<Traits>)(data_, size_, off, str, Traits::length(str));
    }
    MSTL_NODISCARD MSTL_CONSTEXPR20 size_type find_last_of(
        const string_view& str, const size_type off, const size_type n) const noexcept {
        return (char_traits_find_last_of<Traits>)(data_, size_, off, str.data(), n);
    }
    MSTL_NODISCARD MSTL_CONSTEXPR20 size_type find_last_of(const string_view& str, const size_type off = npos) const noexcept {
        return (char_traits_find_last_of<Traits>)(data_, size_, off, str.data(), str.size());
    }

    MSTL_NODISCARD MSTL_CONSTEXPR20 size_type find_first_not_of(const self& str, const size_type off = 0) const noexcept {
        return (char_traits_find_first_not_of<Traits>)(data_, size_, off, str.data_, str.size_);
    }
    MSTL_NODISCARD MSTL_CONSTEXPR20 size_type find_first_not_of(const CharT chr, const size_type off = 0) const noexcept {
        return (char_traits_find_not_char<Traits>)(data_, size_, off, chr);
    }
    MSTL_NODISCARD MSTL_CONSTEXPR20 size_type find_first_not_of(const CharT* const str, const size_type off,
        const size_type n) const noexcept {
        return (char_traits_find_first_not_of<Traits>)(data_, size_, off, str, n);
    }
    MSTL_NODISCARD MSTL_CONSTEXPR20 size_type find_first_not_of(const CharT* const str, const size_type off = 0) const noexcept {
        return (char_traits_find_first_not_of<Traits>)(data_, size_, off, str, Traits::length(str));
    }
    MSTL_NODISCARD MSTL_CONSTEXPR20 size_type find_first_not_of(
        const string_view& str, const size_type off, const size_type n) const noexcept {
        return (char_traits_find_first_not_of<Traits>)(data_, size_, off, str.data(), n);
    }
    MSTL_NODISCARD MSTL_CONSTEXPR20 size_type find_first_not_of(const string_view& str, const size_type off = 0) const noexcept {
        return (char_traits_find_first_not_of<Traits>)(data_, size_, off, str.data(), str.size());
    }

    MSTL_NODISCARD MSTL_CONSTEXPR20 size_type find_last_not_of(const self& str, const size_type off = npos) const noexcept {
        return (char_traits_find_last_not_of<Traits>)(data_, size_, off, str.data_, str.size_);
    }
    MSTL_NODISCARD MSTL_CONSTEXPR20 size_type find_last_not_of(const CharT chr, const size_type off = npos) const noexcept {
        return (char_traits_rfind_not_char<Traits>)(data_, size_, off, chr);
    }
    MSTL_NODISCARD MSTL_CONSTEXPR20 size_type find_last_not_of(
        const CharT* const str, const size_type off, const size_type n) const noexcept {
        return (char_traits_find_last_not_of<Traits>)(data_, size_, off, str, n);
    }
    MSTL_NODISCARD MSTL_CONSTEXPR20 size_type find_last_not_of(const CharT* const str,
        const size_type off = npos) const noexcept {
        return (char_traits_find_last_not_of<Traits>)(data_, size_, off, str, Traits::length(str));
    }
    MSTL_NODISCARD MSTL_CONSTEXPR20 size_type find_last_not_of(
        const string_view& str, const size_type off, const size_type n) const noexcept {
        return (char_traits_find_last_not_of<Traits>)(data_, size_, off, str.data(), n);
    }
    MSTL_NODISCARD MSTL_CONSTEXPR20 size_type find_last_not_of(
        const string_view& str, const size_type off = npos) const noexcept {
        return (char_traits_find_last_not_of<Traits>)(data_, size_, off, str.data(), str.size());
    }


    MSTL_CONSTEXPR20 size_type count(value_type chr, size_type position = 0) const noexcept {
        size_type n = 0;
        for (size_type idx = position; idx < size_; ++idx) {
            if (*(data_ + idx) == chr) ++n;
        }
        return n;
    }

    MSTL_NODISCARD MSTL_CONSTEXPR20 bool starts_with(view_type view) const noexcept {
        return view.size() <= size_ && traits_type::compare(data_, view.data(), view.size()) == 0;
    }
    MSTL_NODISCARD MSTL_CONSTEXPR20 bool starts_with(value_type chr) const noexcept {
        return !empty() && traits_type::eq(front(), chr);
    }
    MSTL_NODISCARD MSTL_CONSTEXPR20 bool starts_with(const_pointer str) const noexcept {
        return this->starts_with(view_type(str));
    }

    MSTL_NODISCARD MSTL_CONSTEXPR20 bool ends_with(view_type view) const noexcept {
        const size_type view_size = view.size();
        return view_size <= size_ && traits_type::compare(data_ + size_ - view_size, view.data(), view_size) == 0;
    }
    MSTL_NODISCARD MSTL_CONSTEXPR20 bool ends_with(value_type chr) const noexcept {
        return !empty() && traits_type::eq(back(), chr);
    }
    MSTL_NODISCARD MSTL_CONSTEXPR20 bool ends_with(const_pointer str) const noexcept {
        return this->ends_with(view_type(str));
    }

    MSTL_NODISCARD MSTL_CONSTEXPR20 bool contains(view_type view) const noexcept {
        return this->find(view) != npos;
    }
    MSTL_NODISCARD MSTL_CONSTEXPR20 bool contains(value_type chr) const noexcept {
        return this->find(chr) != npos;
    }
    MSTL_NODISCARD MSTL_CONSTEXPR20 bool contains(const_pointer str) const noexcept {
        return this->find(str) != npos;
    }


    MSTL_CONSTEXPR20 self& trim_left() noexcept {
        return this->trim_left_if([](value_type ch) { return _MSTL is_space(ch); });
    }
    MSTL_CONSTEXPR20 self& trim_right() noexcept {
        return this->trim_right_if([](value_type ch) { return _MSTL is_space(ch); });
    }
    MSTL_CONSTEXPR20 self& trim() noexcept { return this->trim_left().trim_right(); }

    template <typename Predicate>
    MSTL_CONSTEXPR20 self& trim_left_if(Predicate pred)
    noexcept(noexcept(pred(*cbegin()))) {
        if (empty()) return *this;

        const_iterator it = cbegin();
        while (it != cend() && pred(*it))
            ++it;

        if (it != cbegin())
            this->erase(begin(), it - begin());

        return *this;
    }

    template <typename Predicate>
    MSTL_CONSTEXPR20 self& trim_right_if(Predicate pred)
    noexcept(noexcept(pred(*crbegin()))) {
        if (empty()) return *this;

        const_reverse_iterator rit = crbegin();
        while (rit != crend() && pred(*rit))
            ++rit;

        if (rit != crbegin())
            this->erase(end() - (rit - crbegin()), end());

        return *this;
    }

    template <typename Predicate>
    MSTL_CONSTEXPR20 self& trim_if(Predicate pred)
    noexcept(noexcept(this->trim_right_if(pred)) && noexcept(this->trim_left_if(pred))) {
        return this->trim_left_if(pred).trim_right_if(pred);
    }

    MSTL_CONSTEXPR20 bool equal_to(const self& str) const noexcept {
        return (char_traits_equal<Traits>)(data_, size_, str.data_, str.size_);
    }
    MSTL_CONSTEXPR20 bool equal_to(const view_type str) const noexcept {
        return (char_traits_equal<Traits>)(data_, size_, str.data(), str.size());
    }
    MSTL_CONSTEXPR20 bool equal_to(const CharT* str) const noexcept {
        return (char_traits_equal<Traits>)(data_, size_, str, Traits::length(str));
    }

    MSTL_CONSTEXPR20 void lowercase()
    noexcept(noexcept(_MSTL transform(begin(), end(), begin(), _MSTL to_lowercase<CharT>))) {
        _MSTL transform(begin(), end(), begin(), _MSTL to_lowercase<CharT>);
    }
    MSTL_CONSTEXPR20 void uppercase()
    noexcept(noexcept(_MSTL transform(begin(), end(), begin(), _MSTL to_uppercase<CharT>))) {
        _MSTL transform(begin(), end(), begin(), _MSTL to_uppercase<CharT>);
    }

    MSTL_CONSTEXPR20 void swap(self& x) noexcept {
        if (_MSTL addressof(x) == this) return;
        _MSTL swap(data_, x.data_);
        _MSTL swap(size_, x.size_);
        _MSTL swap(capacity_pair_, x.capacity_pair_);
    }

    MSTL_NODISCARD constexpr bool operator ==(const self& rh) const noexcept { return this->equal_to(rh); }
    MSTL_NODISCARD constexpr bool operator !=(const self& rh) const noexcept { return !(*this == rh); }
    MSTL_NODISCARD constexpr bool operator <(const self& rh) const noexcept { return this->compare(rh) < 0; }
    MSTL_NODISCARD constexpr bool operator >(const self& rh) const noexcept { return rh < *this; }
    MSTL_NODISCARD constexpr bool operator <=(const self& rh) const noexcept { return !(rh < *this); }
    MSTL_NODISCARD constexpr bool operator >=(const self& rh) const noexcept { return !(*this < rh); }

    MSTL_NODISCARD constexpr size_t to_hash() const noexcept {
        return _INNER FNV_hash_string(this->data(), this->length());
    }
};
#ifdef MSTL_SUPPORT_DEDUCTION_GUIDES__
template <typename Iterator, typename Alloc = allocator<iter_val_t<Iterator>>>
basic_string(Iterator, Iterator, Alloc = Alloc())
-> basic_string<iter_val_t<Iterator>, char_traits<iter_val_t<Iterator>>, Alloc>;

template <typename CharT, typename Traits, typename Alloc = allocator<CharT>>
explicit basic_string(basic_string_view<CharT, Traits>, const Alloc & = Alloc())
-> basic_string<CharT, Traits, Alloc>;

template <typename CharT, typename Traits, typename Alloc = allocator<CharT>>
basic_string(basic_string_view<CharT, Traits>, alloc_size_t<Alloc>, alloc_size_t<Alloc>, const Alloc & = Alloc())
-> basic_string<CharT, Traits, Alloc>;
#endif

template <typename CharT, typename Traits, typename Alloc>
MSTL_CONSTEXPR20 basic_string<CharT, Traits, Alloc> operator +(
    const basic_string<CharT, Traits, Alloc>& lh, const basic_string<CharT, Traits, Alloc>& rh) {
    basic_string<CharT, Traits, Alloc> tmp(lh);
    tmp.append(rh);
    return _MSTL move(tmp);
}

template <typename CharT, typename Traits, typename Alloc>
MSTL_CONSTEXPR20 basic_string<CharT, Traits, Alloc> operator +(
    const CharT* lh, const basic_string<CharT, Traits, Alloc>& rh) {
    basic_string<CharT, Traits, Alloc> tmp(lh);
    tmp.append(rh);
    return _MSTL move(tmp);
}
template <typename CharT, typename Traits, typename Alloc>
MSTL_CONSTEXPR20 basic_string<CharT, Traits, Alloc> operator +(
    const basic_string<CharT, Traits, Alloc>& lh, const CharT* rh) {
    basic_string<CharT, Traits, Alloc> tmp(lh);
    tmp.append(rh);
    return _MSTL move(tmp);
}

template <typename CharT, typename Traits, typename Alloc>
MSTL_CONSTEXPR20 basic_string<CharT, Traits, Alloc> operator +(
    const string_view& lh, const basic_string<CharT, Traits, Alloc>& rh) {
    basic_string<CharT, Traits, Alloc> tmp(lh);
    tmp.append(rh);
    return _MSTL move(tmp);
}
template <typename CharT, typename Traits, typename Alloc>
MSTL_CONSTEXPR20 basic_string<CharT, Traits, Alloc> operator +(
    const basic_string<CharT, Traits, Alloc>& lh, const string_view& rh) {
    basic_string<CharT, Traits, Alloc> tmp(lh);
    tmp.append(rh);
    return _MSTL move(tmp);
}

template <typename CharT, typename Traits, typename Alloc>
MSTL_CONSTEXPR20 basic_string<CharT, Traits, Alloc> operator +(
    CharT lh, const basic_string<CharT, Traits, Alloc>& rh) {
    basic_string<CharT, Traits, Alloc> tmp(1, lh);
    tmp.append(rh);
    return _MSTL move(tmp);
}
template <typename CharT, typename Traits, typename Alloc>
MSTL_CONSTEXPR20 basic_string<CharT, Traits, Alloc> operator +(
    const basic_string<CharT, Traits, Alloc>& lh, CharT rh) {
    basic_string<CharT, Traits, Alloc> tmp(lh);
    tmp.append(1, rh);
    return _MSTL move(tmp);
}

template <typename CharT, typename Traits, typename Alloc>
MSTL_CONSTEXPR20 basic_string<CharT, Traits, Alloc> operator +(
    basic_string<CharT, Traits, Alloc>&& lh, const basic_string<CharT, Traits, Alloc>& rh) {
    return _MSTL move(lh.append(rh));
}
template <typename CharT, typename Traits, typename Alloc>
MSTL_CONSTEXPR20 basic_string<CharT, Traits, Alloc> operator +(
    const basic_string<CharT, Traits, Alloc>& lh, basic_string<CharT, Traits, Alloc>&& rh) {
    basic_string<CharT, Traits, Alloc> tmp(lh);
    tmp.append(_MSTL move(rh));
    return _MSTL move(tmp);
}

template <typename CharT, typename Traits, typename Alloc>
MSTL_CONSTEXPR20 basic_string<CharT, Traits, Alloc> operator +(
    basic_string<CharT, Traits, Alloc>&& lh, basic_string<CharT, Traits, Alloc>&& rh) {
    MSTL_DEBUG_VERIFY(_MSTL addressof(lh) != _MSTL addressof(rh),
        "cannot concatenate the same moved string to itself.");
    basic_string<CharT, Traits, Alloc> tmp(_MSTL move(lh));
    tmp.append(_MSTL move(rh));
    return _MSTL move(tmp);
}

template <typename CharT, typename Traits, typename Alloc>
MSTL_CONSTEXPR20 basic_string<CharT, Traits, Alloc> operator +(
    const CharT* lh, basic_string<CharT, Traits, Alloc>&& rh) {
    basic_string<CharT, Traits, Alloc> tmp(lh);
    tmp.append(_MSTL move(rh));
    return _MSTL move(tmp);
}
template <typename CharT, typename Traits, typename Alloc>
MSTL_CONSTEXPR20 basic_string<CharT, Traits, Alloc> operator +(
    basic_string<CharT, Traits, Alloc>&& lh, const CharT* rh) {
    return _MSTL move(lh.append(rh));
}

template <typename CharT, typename Traits, typename Alloc>
MSTL_CONSTEXPR20 basic_string<CharT, Traits, Alloc> operator +(
    CharT lh, basic_string<CharT, Traits, Alloc>&& rh) {
    basic_string<CharT, Traits, Alloc> tmp(1, lh);
    tmp.append(_MSTL move(rh));
    return _MSTL move(tmp);
}
template <typename CharT, typename Traits, typename Alloc>
MSTL_CONSTEXPR20 basic_string<CharT, Traits, Alloc> operator +(
    basic_string<CharT, Traits, Alloc>&& lh, CharT rh) {
    return _MSTL move(lh.append(rh));
}

template <typename CharT, typename Traits, typename Alloc>
MSTL_NODISCARD MSTL_CONSTEXPR20 bool operator ==(
    const CharT* const lh,
    const basic_string<CharT, Traits, Alloc>& rh) noexcept {
    return rh.equal_to(lh);
}
template <typename CharT, typename Traits, typename Alloc>
MSTL_NODISCARD MSTL_CONSTEXPR20 bool operator ==(
    const basic_string<CharT, Traits, Alloc>& lh, 
    const CharT* const rh) noexcept {
    return lh.equal_to(rh);
}
template <typename CharT, typename Traits, typename Alloc>
MSTL_NODISCARD MSTL_CONSTEXPR20 bool operator ==(
    const basic_string_view<CharT, Traits>& lh,
    const basic_string<CharT, Traits, Alloc>& rh) noexcept {
    return rh.equal_to(lh);
}
template <typename CharT, typename Traits, typename Alloc>
MSTL_NODISCARD MSTL_CONSTEXPR20 bool operator ==(
    const basic_string<CharT, Traits, Alloc>& lh,
    const basic_string_view<CharT, Traits>& rh) noexcept {
    return lh.equal_to(rh);
}

template <typename CharT, typename Traits, typename Alloc>
MSTL_NODISCARD MSTL_CONSTEXPR20 bool operator !=(
    const CharT* const lh,
    const basic_string<CharT, Traits, Alloc>& rh) noexcept {
    return !(lh == rh);
}
template <typename CharT, typename Traits, typename Alloc>
MSTL_NODISCARD MSTL_CONSTEXPR20 bool operator !=(
    const basic_string<CharT, Traits, Alloc>& lh,
    const CharT* const rh) noexcept {
    return !(lh == rh);
}
template <typename CharT, typename Traits, typename Alloc>
MSTL_NODISCARD MSTL_CONSTEXPR20 bool operator !=(
    const basic_string_view<CharT, Traits>& lh,
    const basic_string<CharT, Traits, Alloc>& rh) noexcept {
    return !(lh == rh);
}
template <typename CharT, typename Traits, typename Alloc>
MSTL_NODISCARD MSTL_CONSTEXPR20 bool operator !=(
    const basic_string<CharT, Traits, Alloc>& lh,
    const basic_string_view<CharT, Traits>& rh) noexcept {
    return !(lh == rh);
}

template <typename CharT, typename Traits, typename Alloc>
MSTL_NODISCARD MSTL_CONSTEXPR20 bool operator <(
    const CharT* const lh,
    const basic_string<CharT, Traits, Alloc>& rh) noexcept {
    return 0 < rh.compare(lh);
}
template <typename CharT, typename Traits, typename Alloc>
MSTL_NODISCARD MSTL_CONSTEXPR20 bool operator <(
    const basic_string<CharT, Traits, Alloc>& lh,
    const CharT* const rh) noexcept {
    return lh.compare(rh) < 0;
}
template <typename CharT, typename Traits, typename Alloc>
MSTL_NODISCARD MSTL_CONSTEXPR20 bool operator <(
    const basic_string_view<CharT, Traits>& lh,
    const basic_string<CharT, Traits, Alloc>& rh) noexcept {
    return 0 < rh.compare(lh);
}
template <typename CharT, typename Traits, typename Alloc>
MSTL_NODISCARD MSTL_CONSTEXPR20 bool operator <(
    const basic_string<CharT, Traits, Alloc>& lh,
    const basic_string_view<CharT, Traits>& rh) noexcept {
    return lh.compare(rh) < 0;
}

template <typename CharT, typename Traits, typename Alloc>
MSTL_NODISCARD MSTL_CONSTEXPR20 bool operator >(
    const CharT* const lh,
    const basic_string<CharT, Traits, Alloc>& rh) noexcept {
    return rh < lh;
}
template <typename CharT, typename Traits, typename Alloc>
MSTL_NODISCARD MSTL_CONSTEXPR20 bool operator >(
    const basic_string<CharT, Traits, Alloc>& lh,
    const CharT* const rh) noexcept {
    return rh < lh;
}
template <typename CharT, typename Traits, typename Alloc>
MSTL_NODISCARD MSTL_CONSTEXPR20 bool operator >(
    const basic_string_view<CharT, Traits>& lh,
    const basic_string<CharT, Traits, Alloc>& rh) noexcept {
    return rh < lh;
}
template <typename CharT, typename Traits, typename Alloc>
MSTL_NODISCARD MSTL_CONSTEXPR20 bool operator >(
    const basic_string<CharT, Traits, Alloc>& lh,
    const basic_string_view<CharT, Traits>& rh) noexcept {
    return rh < lh;
}

template <typename CharT, typename Traits, typename Alloc>
MSTL_NODISCARD MSTL_CONSTEXPR20 bool operator <=(
    const CharT* const lh,
    const basic_string<CharT, Traits, Alloc>& rh) noexcept {
    return !(lh > rh);
}
template <typename CharT, typename Traits, typename Alloc>
MSTL_NODISCARD MSTL_CONSTEXPR20 bool operator <=(
    const basic_string<CharT, Traits, Alloc>& lh,
    const CharT* const rh) noexcept {
    return !(lh > rh);
}
template <typename CharT, typename Traits, typename Alloc>
MSTL_NODISCARD MSTL_CONSTEXPR20 bool operator <=(
    const basic_string_view<CharT, Traits>& lh,
    const basic_string<CharT, Traits, Alloc>& rh) noexcept {
    return !(lh > rh);
}
template <typename CharT, typename Traits, typename Alloc>
MSTL_NODISCARD MSTL_CONSTEXPR20 bool operator <=(
    const basic_string<CharT, Traits, Alloc>& lh,
    const basic_string_view<CharT, Traits>& rh) noexcept {
    return !(lh > rh);
}

template <typename CharT, typename Traits, typename Alloc>
MSTL_NODISCARD MSTL_CONSTEXPR20 bool operator >=(
    const CharT* const lh,
    const basic_string<CharT, Traits, Alloc>& rh) noexcept {
    return !(rh < lh);
}
template <typename CharT, typename Traits, typename Alloc>
MSTL_NODISCARD MSTL_CONSTEXPR20 bool operator >=(
    const basic_string<CharT, Traits, Alloc>& lh,
    const CharT* const rh) noexcept {
    return !(rh < lh);
}
template <typename CharT, typename Traits, typename Alloc>
MSTL_NODISCARD MSTL_CONSTEXPR20 bool operator >=(
    const basic_string_view<CharT, Traits>& lh,
    const basic_string<CharT, Traits, Alloc>& rh) noexcept {
    return !(rh < lh);
}
template <typename CharT, typename Traits, typename Alloc>
MSTL_NODISCARD MSTL_CONSTEXPR20 bool operator >=(
    const basic_string<CharT, Traits, Alloc>& lh,
    const basic_string_view<CharT, Traits>& rh) noexcept {
    return !(rh < lh);
}

MSTL_END_NAMESPACE__
#endif // MSTL_BASIC_STRING_HPP__
