#ifndef MSTL_BITMAP_HPP__
#define MSTL_BITMAP_HPP__
#include "memory.hpp"
MSTL_BEGIN_NAMESPACE__

static constexpr int MSTL_WORD_BIT_SIZE = 8 * sizeof(uint32_t);

class bitmap;

struct MSTL_API bit_reference {
private:
    uint32_t* ptr_ = nullptr;
    uint32_t mask_ = 0;

public:
    MSTL_CONSTEXPR20 bit_reference() = default;
    MSTL_CONSTEXPR20 bit_reference(uint32_t* x, const uint32_t y) noexcept : ptr_(x), mask_(y) {}

    MSTL_CONSTEXPR20 operator bool() const noexcept { return *ptr_ & mask_; }

    MSTL_CONSTEXPR20 bit_reference& operator =(const bool x) noexcept {
        if (x)
            *ptr_ |= mask_;
        else
            *ptr_ &= ~mask_;
        return *this;
    }

    MSTL_CONSTEXPR20 bit_reference& operator =(const bit_reference& x) noexcept {
        return *this = static_cast<bool>(x);
    }

    MSTL_CONSTEXPR20 bool operator ==(const bit_reference& x) const noexcept {
        return static_cast<bool>(*this) == static_cast<bool>(x);
    }
    MSTL_CONSTEXPR20 bool operator <(const bit_reference& x) const noexcept {
        return static_cast<bool>(*this) < static_cast<bool>(x);
    }

    MSTL_CONSTEXPR20 void flip() const noexcept { *ptr_ ^= mask_; }
};

MSTL_CONSTEXPR20 void swap(bit_reference& x, bit_reference& y) noexcept {
    const bool tmp = x;
    x = y;
    y = tmp;
}

template <bool IsConst, typename BitMap>
struct bitmap_iterator {
private:
    using container_type	= BitMap;
    using iterator			= bitmap_iterator<false, container_type>;
    using const_iterator	= bitmap_iterator<true, container_type>;

public:
    using iterator_category = random_access_iterator_tag;
    using value_type		= typename container_type::value_type;
    using reference			= conditional_t<IsConst, typename container_type::const_reference, typename container_type::reference>;
    using pointer			= conditional_t<IsConst, typename container_type::const_pointer, typename container_type::pointer>;
    using difference_type	= typename container_type::difference_type;
    using size_type			= typename container_type::size_type;

    using self              = bitmap_iterator<IsConst, BitMap>;

private:
    uint32_t* ptr_ = nullptr;
    uint32_t off_ = 0;

    friend class bitmap;
    template <bool, typename> friend struct bitmap_iterator;

private:
    MSTL_CONSTEXPR20 void bump_up() noexcept {
        if (off_++ == MSTL_WORD_BIT_SIZE - 1) {
            off_ = 0;
            ++ptr_;
        }
    }
    MSTL_CONSTEXPR20 void bump_down() noexcept {
        if (off_-- == 0) {
            off_ = MSTL_WORD_BIT_SIZE - 1;
            --ptr_;
        }
    }

    template <typename Ref1, enable_if_t<is_boolean_v<Ref1>, int> = 0>
    MSTL_NODISCARD MSTL_CONSTEXPR20 Ref1 reference_dispatch() const noexcept {
        return (*ptr_ & (1U << off_)) != 0;
    }
    template <typename Ref1, enable_if_t<!is_boolean_v<Ref1>, int> = 0>
    MSTL_NODISCARD MSTL_CONSTEXPR20 Ref1 reference_dispatch() const noexcept {
        return Ref1(ptr_, 1U << off_);
    }

public:
    MSTL_CONSTEXPR20 bitmap_iterator() = default;
    MSTL_CONSTEXPR20 bitmap_iterator(uint32_t* x, const uint32_t y) noexcept : ptr_(x), off_(y) {}

    MSTL_CONSTEXPR20 bitmap_iterator(const iterator& other) noexcept
        : ptr_(other.ptr_), off_(other.off_) {}

    MSTL_NODISCARD MSTL_CONSTEXPR20 reference operator *() const noexcept {
        MSTL_DEBUG_VERIFY(ptr_ != nullptr, __MSTL_DEBUG_MESG_OPERATE_NULLPTR(bitmap_iterator, __MSTL_DEBUG_TAG_DEREFERENCE));
        return reference_dispatch<reference>();
    }

    MSTL_CONSTEXPR20 bitmap_iterator& operator ++() noexcept {
        bump_up();
        return *this;
    }
    MSTL_CONSTEXPR20 bitmap_iterator operator ++(int) noexcept {
        const auto tmp = *this;
        bump_up();
        return tmp;
    }
    MSTL_CONSTEXPR20 bitmap_iterator& operator --() noexcept {
        bump_down();
        return *this;
    }
    MSTL_CONSTEXPR20 bitmap_iterator operator --(int) noexcept {
        const auto tmp = *this;
        bump_down();
        return tmp;
    }

    MSTL_CONSTEXPR20 bitmap_iterator& operator +=(difference_type i) noexcept {
        difference_type n = i + off_;
        ptr_ += n / MSTL_WORD_BIT_SIZE;
        n = n % MSTL_WORD_BIT_SIZE;
        if (n < 0) {
            off_ = static_cast<uint32_t>(n) + MSTL_WORD_BIT_SIZE;
            --ptr_;
        } else
            off_ = static_cast<uint32_t>(n);
        return *this;
    }
    MSTL_CONSTEXPR20 bitmap_iterator& operator -=(const difference_type i) noexcept {
        return *this += -i;
    }

    MSTL_CONSTEXPR20 bitmap_iterator operator +(const difference_type i) const noexcept {
        auto tmp = *this;
        return tmp += i;
    }
    MSTL_CONSTEXPR20 friend bitmap_iterator operator +(const difference_type i, const bitmap_iterator& x) noexcept {
        return x + i;
    }
    MSTL_CONSTEXPR20 bitmap_iterator operator -(const difference_type i) const noexcept {
        auto tmp = *this;
        return tmp -= i;
    }
    MSTL_CONSTEXPR20 difference_type operator -(const bitmap_iterator x) const noexcept {
        return MSTL_WORD_BIT_SIZE * (ptr_ - x.ptr_) + off_ - x.off_;
    }

    MSTL_CONSTEXPR20 reference operator [](const difference_type i) const noexcept {
        return *(*this + i);
    }

    MSTL_CONSTEXPR20 bool operator ==(const bitmap_iterator& x) const noexcept {
        return ptr_ == x.ptr_ && off_ == x.off_;
    }
    MSTL_CONSTEXPR20 bool operator !=(const bitmap_iterator& x) const noexcept {
        return !(*this == x);
    }
    MSTL_CONSTEXPR20 bool operator <(const bitmap_iterator& x) const noexcept {
        return ptr_ < x.ptr_ || (ptr_ == x.ptr_ && off_ < x.off_);
    }
    MSTL_CONSTEXPR20 bool operator >(const bitmap_iterator& x) const noexcept {
        return x < *this;
    }
    MSTL_CONSTEXPR20 bool operator <=(const bitmap_iterator& x) const noexcept {
        return !(*this > x);
    }
    MSTL_CONSTEXPR20 bool operator >=(const bitmap_iterator& x) const noexcept {
        return !(*this < x);
    }
};


class MSTL_API bitmap {
public:
    using value_type         = bool;
    using pointer            = bit_reference*;
    using reference          = bit_reference;
    using const_pointer      = const bool*;
    using const_reference    = const bool;
    using size_type          = size_t;
    using difference_type    = ptrdiff_t;
    using allocator_type     = allocator<uint32_t>;
    using self               = bitmap;

    using iterator                  = bitmap_iterator<false, self>;
    using const_iterator            = bitmap_iterator<true, self>;
    using reverse_iterator          = _MSTL reverse_iterator<iterator>;
    using const_reverse_iterator    = _MSTL reverse_iterator<const_iterator>;

protected:
    iterator start_{};
    iterator finish_{};
    uint32_t* end_of_storage_ = nullptr;

    MSTL_CONSTEXPR20 static uint32_t* bit_alloc(const size_type n) {
        return allocator_type::allocate((n + MSTL_WORD_BIT_SIZE - 1) / MSTL_WORD_BIT_SIZE);
    }
    MSTL_CONSTEXPR20 void deallocate() const {
        if (start_.ptr_)
            allocator_type::deallocate(start_.ptr_, end_of_storage_ - start_.ptr_);
    }

    template <typename Iterator1, typename Iterator2>
    MSTL_CONSTEXPR20 Iterator2 bit_copy(Iterator1 first, Iterator1 last, Iterator2 result) {
        iter_dif_t<Iterator1> n = _MSTL distance(first, last);
        for (; n > 0; --n, ++first, ++result)
            *result = *first;
        return result;
    }
    template <typename Iterator1, typename Iterator2>
    MSTL_CONSTEXPR20 Iterator2 bit_copy_backward(Iterator1 first, Iterator1 last, Iterator2 result) {
        iter_dif_t<Iterator1> n = _MSTL distance(first, last);
        for (; n > 0; --n)
            *--result = *--last;
        return result;
    }

    MSTL_CONSTEXPR20 void initialize(const size_type n) {
        uint32_t* q = bit_alloc(n);
        end_of_storage_ = q + (n + MSTL_WORD_BIT_SIZE - 1) / MSTL_WORD_BIT_SIZE;
        start_ = iterator(q, 0);
        finish_ = start_ + static_cast<difference_type>(n);
    }
    MSTL_CONSTEXPR20 void insert_aux(const iterator& position, const bool x) {
        if (finish_.ptr_ != end_of_storage_) {
            bit_copy_backward(position, finish_, finish_ + 1);
            *position = x;
            ++finish_;
        }
        else {
            const size_type len = size() ? 2 * size() : MSTL_WORD_BIT_SIZE;
            uint32_t* q = bit_alloc(len);
            auto i = bit_copy(begin(), position, iterator(q, 0));
            *i++ = x;
            finish_ = bit_copy(position, end(), i);
            deallocate();
            end_of_storage_ = q + (len + MSTL_WORD_BIT_SIZE - 1)/MSTL_WORD_BIT_SIZE;
            start_ = iterator(q, 0);
        }
    }

    template <class Iterator, enable_if_t<!is_ranges_fwd_iter_v<Iterator>, int> = 0>
    MSTL_CONSTEXPR20 void initialize_range(Iterator first, Iterator last) {
        start_ = iterator();
        finish_ = iterator();
        end_of_storage_ = nullptr;
        for ( ; first != last; ++first)
            push_back(*first);
    }
    template <class Iterator, enable_if_t<is_ranges_fwd_iter_v<Iterator>, int> = 0>
    MSTL_CONSTEXPR20 void initialize_range(Iterator first, Iterator last) {
        const size_type n = _MSTL distance(first, last);
        initialize(n);
        bit_copy(first, last, start_);
    }

    template <class Iterator, enable_if_t<!is_ranges_fwd_iter_v<Iterator>, int> = 0>
    MSTL_CONSTEXPR20 void insert_range(iterator pos, Iterator first, Iterator last) {
        for ( ; first != last; ++first) {
            pos = insert(pos, *first);
            ++pos;
        }
    }
    template <class Iterator, enable_if_t<is_ranges_fwd_iter_v<Iterator>, int> = 0>
    MSTL_CONSTEXPR20 void insert_range(iterator position, Iterator first, Iterator last) {
        if (first != last) {
            size_type n = 0;
            distance(first, last, n);
            if (capacity() - size() >= n) {
                bit_copy_backward(position, end(), finish_ + static_cast<difference_type>(n));
                bit_copy(first, last, position);
                finish_ += static_cast<difference_type>(n);
            }
            else {
                const size_type len = size() + max(size(), n);
                uint32_t* q = bit_alloc(len);
                auto i = bit_copy(begin(), position, iterator(q, 0));
                i = bit_copy(first, last, i);
                finish_ = bit_copy(position, end(), i);
                deallocate();
                end_of_storage_ = q + (len + MSTL_WORD_BIT_SIZE - 1) / MSTL_WORD_BIT_SIZE;
                start_ = iterator(q, 0);
            }
        }
    }

public:
    MSTL_CONSTEXPR20 bitmap() noexcept = default;

    MSTL_CONSTEXPR20 bitmap(const size_type n, const bool value) {
        initialize(n);
        _MSTL fill(start_.ptr_, end_of_storage_, value ? ~0U : 0U);
    }
    MSTL_CONSTEXPR20 bitmap(const int n, const bool value) {
        initialize(n);
        fill(start_.ptr_, end_of_storage_, value ? ~0 : 0);
    }
    MSTL_CONSTEXPR20 bitmap(const long n, const bool value) {
        initialize(n);
        fill(start_.ptr_, end_of_storage_, value ? ~0 : 0);
    }

    MSTL_CONSTEXPR20 explicit bitmap(const size_type n) {
        initialize(n);
        fill(start_.ptr_, end_of_storage_, 0);
    }

    MSTL_CONSTEXPR20 bitmap(const bitmap& x) {
        initialize(x.size());
        bit_copy(x.cbegin(), x.cend(), start_);
    }

    MSTL_CONSTEXPR20 bitmap& operator =(const bitmap& x) {
        if (&x == this) return *this;
        if (x.size() > capacity()) {
            deallocate();
            initialize(x.size());
        }
        bit_copy(x.cbegin(), x.cend(), begin());
        finish_ = begin() + static_cast<difference_type>(x.size());
        return *this;
    }

    template <class InputIterator>
    MSTL_CONSTEXPR20 bitmap(InputIterator first, InputIterator last) {
        this->initialize_range(first, last);
    }

    MSTL_CONSTEXPR20 bitmap(const_iterator first, const_iterator last) {
        size_type n = _MSTL distance(first, last);
        initialize(n);
        bit_copy(first, last, start_);
    }

    MSTL_CONSTEXPR20 bitmap(const bool* first, const bool* last) {
        const size_type n = _MSTL distance(first, last);
        initialize(n);
        bit_copy(first, last, start_);
    }

    MSTL_CONSTEXPR20 ~bitmap() { deallocate(); }


    MSTL_NODISCARD MSTL_CONSTEXPR20 iterator begin() noexcept { return start_; }
    MSTL_NODISCARD MSTL_CONSTEXPR20 iterator end() noexcept { return finish_; }
    MSTL_NODISCARD MSTL_CONSTEXPR20 const_iterator begin() const noexcept { return cbegin(); }
    MSTL_NODISCARD MSTL_CONSTEXPR20 const_iterator end() const noexcept { return cend(); }
    MSTL_NODISCARD MSTL_CONSTEXPR20 const_iterator cbegin() const noexcept { return start_; }
    MSTL_NODISCARD MSTL_CONSTEXPR20 const_iterator cend() const noexcept { return finish_; }
    MSTL_NODISCARD MSTL_CONSTEXPR20 reverse_iterator rbegin() noexcept { return reverse_iterator(end()); }
    MSTL_NODISCARD MSTL_CONSTEXPR20 reverse_iterator rend() noexcept { return reverse_iterator(begin()); }
    MSTL_NODISCARD MSTL_CONSTEXPR20 const_reverse_iterator rbegin() const noexcept { return crbegin(); }
    MSTL_NODISCARD MSTL_CONSTEXPR20 const_reverse_iterator rend() const noexcept { return crend(); }
    MSTL_NODISCARD MSTL_CONSTEXPR20 const_reverse_iterator crbegin() const noexcept { return const_reverse_iterator(cend()); }
    MSTL_NODISCARD MSTL_CONSTEXPR20 const_reverse_iterator crend() const noexcept { return const_reverse_iterator(cbegin()); }

    MSTL_NODISCARD MSTL_CONSTEXPR20 size_type size() const noexcept { return cend() - cbegin(); }
    MSTL_NODISCARD MSTL_CONSTEXPR20 size_type max_size() const noexcept { return static_cast<size_type>(-1); }
    MSTL_NODISCARD MSTL_CONSTEXPR20 size_type capacity() const noexcept {
        return const_iterator(end_of_storage_, 0) - cbegin();
    }
    MSTL_NODISCARD MSTL_CONSTEXPR20 bool empty() const noexcept { return start_ == finish_; }

    MSTL_NODISCARD MSTL_CONSTEXPR20 reference operator [](const size_type n) {
        return *(begin() + static_cast<difference_type>(n));
    }
    MSTL_NODISCARD MSTL_CONSTEXPR20 const_reference operator [](const size_type n) const {
        return *(cbegin() + static_cast<difference_type>(n));
    }

    MSTL_NODISCARD MSTL_CONSTEXPR20 reference front() { return *begin(); }
    MSTL_NODISCARD MSTL_CONSTEXPR20 const_reference front() const { return *cbegin(); }
    MSTL_NODISCARD MSTL_CONSTEXPR20 reference back() { return *(end() - 1); }
    MSTL_NODISCARD MSTL_CONSTEXPR20 const_reference back() const { return *(cend() - 1); }


    MSTL_CONSTEXPR20 void reserve(const size_type n) {
        if (capacity() < n) {
            uint32_t* q = bit_alloc(n);
            finish_ = bit_copy(begin(), end(), iterator(q, 0));
            deallocate();
            start_ = iterator(q, 0);
            end_of_storage_ = q + (n + MSTL_WORD_BIT_SIZE - 1)/MSTL_WORD_BIT_SIZE;
        }
    }

    MSTL_CONSTEXPR20 void push_back(const bool x) {
        if (finish_.ptr_ != end_of_storage_)
            *finish_++ = x;
        else
            insert_aux(end(), x);
    }

    MSTL_CONSTEXPR20 iterator insert(const iterator& position, const bool x = bool()) {
        const difference_type n = position - begin();
        if (finish_.ptr_ != end_of_storage_ && position == end())
            *finish_++ = x;
        else
            insert_aux(position, x);
        return begin() + n;
    }

    template <class Iterator>
    MSTL_CONSTEXPR20 void insert(iterator position, Iterator first, Iterator last) {
        insert_range(position, first, last, iterator_category(first));
    }

    MSTL_CONSTEXPR20 void insert(const iterator& position, const bool* first, const bool* last) {
        if (first == last) return;
        size_type n = distance(first, last);
        if (capacity() - size() >= n) {
          bit_copy_backward(position, end(), finish_ + static_cast<difference_type>(n));
          bit_copy(first, last, position);
          finish_ += static_cast<difference_type>(n);
        }
        else {
            const size_type len = size() + max(size(), n);
            uint32_t* q = bit_alloc(len);
            auto i = bit_copy(begin(), position, iterator(q, 0));
            i = bit_copy(first, last, i);
            finish_ = bit_copy(position, end(), i);
            deallocate();
            end_of_storage_ = q + (len + MSTL_WORD_BIT_SIZE - 1) / MSTL_WORD_BIT_SIZE;
            start_ = iterator(q, 0);
        }
    }

    MSTL_CONSTEXPR20 void insert(const iterator& position, const size_type n, const bool x) {
        if (n == 0) return;
        if (capacity() - size() >= n) {
            bit_copy_backward(position, end(), finish_ + static_cast<difference_type>(n));
            _MSTL fill(position, position + static_cast<difference_type>(n), x);
            finish_ += static_cast<difference_type>(n);
        }
        else {
            const size_type len = size() + max(size(), n);
            uint32_t* q = bit_alloc(len);
            const auto i = bit_copy(begin(), position, iterator(q, 0));
            fill_n(i, n, x);
            finish_ = bit_copy(position, end(), i + static_cast<difference_type>(n));
            deallocate();
            end_of_storage_ = q + (len + MSTL_WORD_BIT_SIZE - 1)/MSTL_WORD_BIT_SIZE;
            start_ = iterator(q, 0);
        }
    }

    MSTL_CONSTEXPR20 void insert(const iterator& pos, const int n, const bool x)  { insert(pos, static_cast<size_type>(n), x); }
    MSTL_CONSTEXPR20 void insert(const iterator& pos, const long n, const bool x) { insert(pos, static_cast<size_type>(n), x); }

    MSTL_CONSTEXPR20 void pop_back() { --finish_; }

    MSTL_CONSTEXPR20 iterator erase(const iterator& position) {
      if (position + 1 != end())
          bit_copy(position + 1, end(), position);
      --finish_;
      return position;
    }
    MSTL_CONSTEXPR20 iterator erase(const iterator& first, const iterator& last) {
        finish_ = bit_copy(last, end(), first);
        return first;
    }
    MSTL_CONSTEXPR20 void resize(const size_type new_size, const bool x = bool()) {
        if (new_size < size())
            erase(begin() + static_cast<difference_type>(new_size), end());
        else
            insert(end(), new_size - size(), x);
    }
    MSTL_CONSTEXPR20 void clear() {
        erase(begin(), end());
    }

    MSTL_CONSTEXPR20 void swap(bitmap& x) noexcept {
        _MSTL swap(start_, x.start_);
        _MSTL swap(finish_, x.finish_);
        _MSTL swap(end_of_storage_, x.end_of_storage_);
    }
};

MSTL_CONSTEXPR20 void swap(bitmap& x, bitmap& y) noexcept {
    x.swap(y);
}

MSTL_END_NAMESPACE__
#endif // MSTL_BITMAP_HPP__
