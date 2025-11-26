#ifndef MSTL_CORE_CONTAINER_DEQUE_HPP__
#define MSTL_CORE_CONTAINER_DEQUE_HPP__
#include "../string/serialize.hpp"
MSTL_BEGIN_NAMESPACE__

MSTL_INLINE17 constexpr size_t MAX_DEQUE_BUFFER_THRESHHOLD = 256;
MSTL_INLINE17 constexpr size_t MAX_DEQUE_BUFFER_SIZE = 4096;
MSTL_INLINE17 constexpr size_t DEQUE_INIT_MAP_SIZE = 8;

constexpr size_t deque_buf_size(const size_t n, const size_t sz) noexcept {
    return n != 0 ? n : sz < MAX_DEQUE_BUFFER_THRESHHOLD ? MAX_DEQUE_BUFFER_SIZE / sz : 16;
}


template <typename T, typename Alloc, size_t BufSize>
class deque;

template <bool IsConst, typename Deque, size_t BufSize = 0>
struct deque_iterator {
private:
    using container_type	= Deque;
    using iterator			= deque_iterator<false, container_type>;
    using const_iterator	= deque_iterator<true, container_type>;

public:
    using iterator_category = random_access_iterator_tag;
    using value_type		= typename container_type::value_type;
    using reference			= conditional_t<IsConst, typename container_type::const_reference, typename container_type::reference>;
    using pointer			= conditional_t<IsConst, typename container_type::const_pointer, typename container_type::pointer>;
    using difference_type	= typename container_type::difference_type;
    using size_type			= typename container_type::size_type;

    using self              = deque_iterator<IsConst, Deque, BufSize>;

private:
    pointer cur_ = nullptr;
    pointer first_ = nullptr;
    pointer last_ = nullptr;
    pointer* node_ = nullptr;
    const container_type* deq_ = nullptr;

    static constexpr difference_type buffer_size_ = _MSTL deque_buf_size(BufSize, sizeof(value_type));

    template <typename, typename, size_t> friend class deque;
    template <bool, typename, size_t> friend struct deque_iterator;

private:
    void change_buff(pointer* new_map) noexcept {
        node_ = new_map;
        first_ = *new_map;
        last_ = first_ + buffer_size_;
    }

public:
    deque_iterator() noexcept = default;

    deque_iterator(pointer cur, pointer* map, const container_type* deq)
        : cur_(cur), first_(*map), last_(*map + buffer_size_), node_(map), deq_(deq) {}

    deque_iterator(pointer cur, pointer first, pointer last, pointer* node, const container_type* deq) noexcept
    : cur_(const_cast<pointer>(cur)), first_(const_cast<pointer>(first)),
    last_(const_cast<pointer>(last)), node_(const_cast<pointer*>(node)), deq_(deq) {}

    deque_iterator(const iterator& x) noexcept
    : cur_(const_cast<pointer>(x.cur_)), first_(const_cast<pointer>(x.first_)),
    last_(const_cast<pointer>(x.last_)), node_(const_cast<pointer*>(x.node_)), deq_(x.deq_) {}

    self& operator =(const iterator& x) noexcept {
        if (_MSTL addressof(x) == this) return *this;
        cur_ = const_cast<pointer>(x.cur_);
        first_ = const_cast<pointer>(x.first_);
        last_ = const_cast<pointer>(x.last_);
        node_ = const_cast<pointer*>(x.node_);
        deq_ = x.deq_;
        return *this;
    }

    deque_iterator(iterator&& x) noexcept
    : cur_(const_cast<pointer>(x.cur_)), first_(const_cast<pointer>(x.first_)),
    last_(const_cast<pointer>(x.last_)), node_(const_cast<pointer*>(x.node_)), deq_(x.deq_) {
        x.cur_ = nullptr;
        x.first_ = nullptr;
        x.last_ = nullptr;
        x.node_ = nullptr;
        x.deq_ = nullptr;
    }

    self& operator =(iterator&& x) noexcept {
        if (_MSTL addressof(x) == this) return *this;
        swap(x);
        return *this;
    }

    deque_iterator(const const_iterator& x) noexcept
    : cur_(const_cast<pointer>(x.cur_)), first_(const_cast<pointer>(x.first_)),
    last_(const_cast<pointer>(x.last_)), node_(const_cast<pointer*>(x.node_)), deq_(x.deq_) {}

    self& operator =(const const_iterator& x) noexcept {
        if (_MSTL addressof(x) == this) return *this;
        cur_ = const_cast<pointer>(x.cur_);
        first_ = const_cast<pointer>(x.first_);
        last_ = const_cast<pointer>(x.last_);
        node_ = const_cast<pointer*>(x.node_);
        deq_ = x.deq_;
        return *this;
    }

    deque_iterator(const_iterator&& x) noexcept
    : cur_(const_cast<pointer>(x.cur_)), first_(const_cast<pointer>(x.first_)),
    last_(const_cast<pointer>(x.last_)), node_(const_cast<pointer*>(x.node_)), deq_(x.deq_) {
        x.cur_ = nullptr;
        x.first_ = nullptr;
        x.last_ = nullptr;
        x.node_ = nullptr;
        x.deq_ = nullptr;
    }
    self& operator =(const_iterator&& x) noexcept {
        if (_MSTL addressof(x) == this) return *this;
        cur_ = const_cast<pointer>(x.cur_);
        first_ = const_cast<pointer>(x.first_);
        last_ = const_cast<pointer>(x.last_);
        node_ = const_cast<pointer*>(x.node_);
        deq_ = x.deq_;
        x.cur_ = nullptr;
        x.first_ = nullptr;
        x.last_ = nullptr;
        x.node_ = nullptr;
        x.deq_ = nullptr;
        return *this;
    }

    ~deque_iterator() noexcept = default;

    MSTL_NODISCARD reference operator *() const noexcept {
        MSTL_DEBUG_VERIFY(cur_ && deq_, __MSTL_DEBUG_MESG_OPERATE_NULLPTR(deque_iterator, __MSTL_DEBUG_TAG_DEREFERENCE));
        MSTL_DEBUG_VERIFY(first_ <= cur_ && cur_ < last_,
            __MSTL_DEBUG_MESG_OUT_OF_RANGE(deque_iterator, __MSTL_DEBUG_TAG_DEREFERENCE));
        return *cur_;
    }

    MSTL_NODISCARD pointer operator ->() const noexcept {
        MSTL_DEBUG_VERIFY(cur_ && deq_, __MSTL_DEBUG_MESG_OPERATE_NULLPTR(deque_iterator, __MSTL_DEBUG_TAG_DEREFERENCE));
        return cur_;
    }

    self& operator ++() noexcept {
        MSTL_DEBUG_VERIFY(cur_ && deq_, __MSTL_DEBUG_MESG_OPERATE_NULLPTR(deque_iterator, __MSTL_DEBUG_TAG_INCREMENT));
        ++cur_;
        if (cur_ == last_) {
            this->change_buff(node_ + 1);
            cur_ = first_;
        }
        return *this;
    }

    self operator ++(int) noexcept {
        self tmp = *this;
        ++*this;
        return tmp;
    }

    self& operator --() noexcept {
        MSTL_DEBUG_VERIFY(cur_ && deq_, __MSTL_DEBUG_MESG_OPERATE_NULLPTR(deque_iterator, __MSTL_DEBUG_TAG_DECREMENT));
        if (cur_ == first_) {
            this->change_buff(node_ - 1);
            cur_ = last_;
        }
        --cur_;
        return *this;
    }
    
    self operator --(int) noexcept {
        self tmp = *this;
        --*this;
        return tmp;
    }

    self& operator +=(const difference_type n) noexcept {
        MSTL_DEBUG_VERIFY(cur_ && deq_, __MSTL_DEBUG_MESG_OPERATE_NULLPTR(deque_iterator, __MSTL_DEBUG_TAG_INCREMENT));
        const difference_type offset = n + (cur_ - first_);
        if (offset >= 0 && offset < buffer_size_) {
            cur_ += n;
        }
        else {
            difference_type node_offset = offset > 0 ?
                offset / buffer_size_ :
                -static_cast<difference_type>((-offset - 1) / buffer_size_) - 1;
            this->change_buff(node_ + node_offset);
            cur_ = first_ + (offset - node_offset * buffer_size_);
        }
        return *this;
    }
    MSTL_NODISCARD self operator +(const difference_type n) const noexcept {
        auto tmp = *this;
        return tmp += n;
    }
    MSTL_NODISCARD friend self operator +(const difference_type n, const self& it) {
        return it + n;
    }

    self& operator -=(const difference_type n) noexcept {
        return *this += -n;
    }
    MSTL_NODISCARD self operator -(const difference_type n) const noexcept {
        self temp = *this;
        return temp -= n;
    }
    difference_type operator -(const self& x) const noexcept {
		MSTL_DEBUG_VERIFY(deq_ == x.deq_, __MSTL_DEBUG_MESG_CONTAINER_INCOMPATIBLE(deque_iterator));
        return (node_ - x.node_) * buffer_size_ + (cur_ - first_) - (x.cur_ - x.first_);
    }

    MSTL_NODISCARD reference operator [](const difference_type n) noexcept { return *(*this + n); }

    MSTL_NODISCARD bool operator ==(const self& x) const noexcept {
        MSTL_DEBUG_VERIFY(deq_ == x.deq_, __MSTL_DEBUG_MESG_CONTAINER_INCOMPATIBLE(deque_iterator));
        return cur_ == x.cur_;
    }
    MSTL_NODISCARD bool operator !=(const self& x) const noexcept { return !(*this == x); }
    MSTL_NODISCARD bool operator <(const self& x) const noexcept {
        MSTL_DEBUG_VERIFY(deq_ == x.deq_, __MSTL_DEBUG_MESG_CONTAINER_INCOMPATIBLE(deque_iterator));
        return node_ == x.node_ ? cur_ < x.cur_ : node_ < x.node_;
    }
    MSTL_NODISCARD bool operator >(const self& x) const noexcept { return x < *this; }
    MSTL_NODISCARD bool operator <=(const self& x) const noexcept { return !(*this > x); }
    MSTL_NODISCARD bool operator >=(const self& x) const noexcept { return !(*this < x); }

    void swap(self& x) noexcept {
        cur_ = x.cur_;
        first_ = x.first_;
        last_ = x.last_;
        node_ = x.node_;
        deq_ = x.deq_;
        x.cur_ = nullptr;
        x.first_ = nullptr;
        x.last_ = nullptr;
        x.node_ = nullptr;
        x.deq_ = nullptr;
    }

    MSTL_NODISCARD pointer base() const noexcept {
        return cur_;
    }
};
template <bool IsConst, typename Deque, size_t BufSize>
void swap (
    deque_iterator<IsConst, Deque, BufSize>& lh,
    deque_iterator<IsConst, Deque, BufSize>& rh) noexcept {
    lh.swap(rh);
}


// let BufSize = 0 to use default deque size, or manually set it as you wish.
template <typename T, typename Alloc = allocator<T>, size_t BufSize = 0>
class deque : public icollector<deque<T, Alloc>> {
#ifdef MSTL_STANDARD_20__
    static_assert(is_allocator_v<Alloc>, "Alloc type is not a standard allocator type.");
#endif
    static_assert(is_same_v<T, typename Alloc::value_type>, "allocator type mismatch.");
    static_assert(is_object_v<T>, "deque only contains object types.");

    using self = deque<T, Alloc, BufSize>;
    using super = icollector<self>;

public:
    MSTL_BUILD_TYPE_ALIAS(T)
    using allocator_type            = Alloc;

    using iterator                  = deque_iterator<false, self, BufSize>;
    using const_iterator            = deque_iterator<true, self, BufSize>;
    using reverse_iterator          = _MSTL reverse_iterator<iterator>;
    using const_reverse_iterator    = _MSTL reverse_iterator<const_iterator>;

private:
    using map_pointer   = pointer*;
    using map_allocator = typename Alloc::template rebind<pointer>::other;

    iterator start_{};
    iterator finish_{};
    // allocator, map_size_
    compressed_pair<allocator_type, size_type> map_size_pair_{
        default_construct_tag{}, 0
    };
    // map_allocator, map_
    compressed_pair<map_allocator, map_pointer> map_pair_{
        default_construct_tag{}, nullptr
    };

    static constexpr difference_type buffer_size_ = iterator::buffer_size_;

    template <bool, typename, size_t> friend struct deque_iterator;

private:
    map_pointer create_map(const size_t size) {
        map_pointer map = nullptr;
        map = map_pair_.get_base().allocate(size);
        for (size_t i = 0; i < size; ++i) {
            *(map + i) = nullptr;
        }
        return map;
    }

    void create_nodes(map_pointer nstart, map_pointer nfinish) {
        map_pointer cur;
        try {
            for (cur = nstart; cur <= nfinish; ++cur)
                *cur = map_size_pair_.get_base().allocate(buffer_size_);
        }
        catch (...) {
            while (cur != nstart) {
                --cur;
                map_size_pair_.get_base().deallocate(*cur, buffer_size_);
                *cur = nullptr;
            }
            throw;
        }
    }

    void destroy_nodes(map_pointer nstart, map_pointer nfinish) noexcept {
        for (map_pointer cur = nstart; cur <= nfinish; ++cur) {
            if (*cur == nullptr) continue;
            map_size_pair_.get_base().deallocate(*cur, buffer_size_);
            *cur = nullptr;
        }
    }

    void create_map_and_nodes(const size_type n) {
        size_type node_nums = n / buffer_size_ + (n % buffer_size_ ? 1 : 0);
        map_size_pair_.value = _MSTL max(DEQUE_INIT_MAP_SIZE, node_nums + 2);

        try {
            map_pair_.value = this->create_map(map_size_pair_.value);
        }
        catch (...) {
            map_pair_.value = nullptr;
            map_size_pair_.value = 0;
            throw;
        }

        map_pointer nstart = map_pair_.value + (map_size_pair_.value - node_nums) / 2;
        map_pointer nfinish = node_nums == 0 ? nstart : nstart + node_nums - 1;

        try {
            this->create_nodes(nstart, nfinish);
        }
        catch (...) {
            this->destroy_nodes(map_pair_.value, map_pair_.value + map_size_pair_.value - 1);
            map_pair_.get_base().deallocate(map_pair_.value, map_size_pair_.value);
            map_pair_.value = nullptr;
            map_size_pair_.value = 0;
            throw;
        }

        start_.change_buff(nstart);
        finish_.change_buff(nfinish);
        start_.cur_ = start_.first_;
        finish_.cur_ = n == 0 ?
            start_.first_ :
            finish_.first_ + (n % buffer_size_ ? n % buffer_size_ : buffer_size_);
        start_.deq_ = this;
        finish_.deq_ = this;
    }

    void fill_initialize(const size_type n, const T& x) {
        this->create_map_and_nodes(n);
        if (n == 0) return;
        for (map_pointer cur = start_.node_; cur < finish_.node_; ++cur)
            _MSTL uninitialized_fill(*cur, *cur + buffer_size_, x);
        _MSTL uninitialized_fill(finish_.first_, finish_.cur_, x);
    }

    template <typename Iterator, enable_if_t<is_ranges_fwd_iter_v<Iterator>, int> = 0>
    void copy_initialize(Iterator first, Iterator last) {
        this->create_map_and_nodes(_MSTL distance(first, last));
        for (map_pointer cur = start_.node_; cur < finish_.node_; ++cur) {
            Iterator next = _MSTL next(first, buffer_size_);
            _MSTL uninitialized_copy(first, next, *cur);
            first = next;
        }
        _MSTL uninitialized_copy(first, last, finish_.first_);
    }

    template <typename Iterator, enable_if_t<!is_ranges_fwd_iter_v<Iterator>, int> = 0>
    void copy_initialize(Iterator first, Iterator last) {
        this->create_map_and_nodes(_MSTL distance(first, last));
        for (; first != last; ++first) {
            this->emplace_back(*first);
        }
    }

    void assign_aux_n(size_type n, const T& value) {
        if (n > size()) {
            _MSTL fill(begin(), end(), value);
            this->insert(end(), n - size(), value);
        }
        else {
            this->erase(begin() + n, end());
            _MSTL fill(begin(), end(), value);
        }
    }

    template <typename Iterator>
    void assign_ranges(Iterator first, Iterator last) {
        auto first1 = begin();
        auto last1 = end();
        for (; first != last && first1 != last1; ++first, ++first1)
            *first1 = *first;
        if (first1 != last1)
            this->erase(first1, last1);
        else
            insert(first1, last1);
    }

    void reallocate_map(const size_type n, const bool add_at_front) {
        const auto begin_left = static_cast<size_type>(start_.cur_ - start_.first_);
        if (add_at_front && begin_left < n) {
            const size_t needed = (n - begin_left) / buffer_size_ + 1;
            if (needed > static_cast<size_type>(start_.node_ - map_pair_.value)) {
                const size_type new_size = _MSTL max(
                    map_size_pair_.value << 1,
                    map_size_pair_.value + needed + DEQUE_INIT_MAP_SIZE);
                map_pointer map = this->create_map(new_size);
                const size_type old_buf = finish_.node_ - start_.node_ + 1;
                const size_type new_buf = needed + old_buf;

                auto begin = map + (new_size - new_buf) / 2;
                auto mid = begin + needed;
                auto end = mid + old_buf;
                this->create_nodes(begin, mid - 1);
                for (auto begin1 = mid, begin2 = start_.node_;
                    begin1 != end; ++begin1, ++begin2) {
                    *begin1 = *begin2;
                }

                map_pair_.get_base().deallocate(map_pair_.value, map_size_pair_.value);
                map_pair_.value = map;
                map_size_pair_.value = new_size;
                start_ = iterator(*mid + (start_.cur_ - start_.first_), mid, this);
                finish_ = iterator(*(end - 1) + (finish_.cur_ - finish_.first_), end - 1, this);
                return;
            }
            this->create_nodes(start_.node_ - needed, start_.node_ - 1);
            return;
        }
        const auto end_left = static_cast<size_type>(finish_.last_ - finish_.cur_ - 1);
        if (!add_at_front && end_left < n) {
            const size_type needed = (n - end_left) / buffer_size_ + 1;
            if (needed > static_cast<size_type>((map_pair_.value + map_size_pair_.value) - finish_.node_ - 1)) {
                const size_type new_size = _MSTL max(
                    map_size_pair_.value << 1,
                    map_size_pair_.value + needed + DEQUE_INIT_MAP_SIZE);
                map_pointer map = this->create_map(new_size);
                const size_type old_buf = finish_.node_ - start_.node_ + 1;
                const size_type new_buf = needed + old_buf;

                auto begin = map + (new_size - new_buf) / 2;
                auto mid = begin + old_buf;
                auto end = mid + needed;
                for (auto begin1 = begin, begin2 = start_.node_;
                    begin1 != mid; ++begin1, ++begin2) {
                    *begin1 = *begin2;
                }
                this->create_nodes(mid, end - 1);

                map_pair_.get_base().deallocate(map_pair_.value, map_size_pair_.value);
                map_pair_.value = map;
                map_size_pair_.value = new_size;
                start_ = iterator(*begin + (start_.cur_ - start_.first_), begin, this);
                finish_ = iterator(*(mid - 1) + (finish_.cur_ - finish_.first_), mid - 1, this);
                return;
            }
            this->create_nodes(finish_.node_ + 1, finish_.node_ + needed);
        }
    }

    template <typename Iterator>
    void insert_ranges_n(iterator position, Iterator first, Iterator last, size_type n) {
        difference_type dist_before = position - start_;
        if (dist_before < static_cast<difference_type>(size() / 2)) {
            this->reallocate_map(n, true);
            auto old_start = start_;
            auto new_start = start_ - n;
            position = start_ + dist_before;

            try {
                if (dist_before >= n) {
                    iterator start_n = start_ + n;
                    _MSTL uninitialized_copy(start_, start_n, new_start);
                    start_ = new_start;
                    _MSTL copy(start_n, position, old_start);
                    _MSTL copy(first, last, position - n);
                }
                else {
                    auto mid = _MSTL next(first, n - dist_before);
                    _MSTL uninitialized_copy(first, mid,
                        _MSTL uninitialized_copy(start_, position, new_start));
                    start_ = new_start;
                    _MSTL copy(mid, last, old_start);
                }
            }
            catch(...) {
                if (new_start.node_ != start_.node_) {
                    this->destroy_nodes(new_start.node_, start_.node_ - 1);
                }
                throw;
            }
        }
        else {
            this->reallocate_map(n, false);
            auto old_finish = finish_;
            auto new_finish = finish_ + n;
            const size_type dist_after = size() - dist_before;
            position = finish_ - dist_after;

            try {
                if (dist_after > n) {
                    auto finish_n = finish_ - n;
                    _MSTL uninitialized_copy(finish_n, finish_, finish_);
                    finish_ = new_finish;
                    _MSTL copy_backward(position, finish_n, old_finish);
                    _MSTL copy(first, last, position);
                }
                else {
                    auto mid = _MSTL next(first, dist_after);
                    _MSTL uninitialized_copy(position, finish_,
                        _MSTL uninitialized_copy(mid, last, finish_));
                    finish_ = new_finish;
                    _MSTL copy(first, mid, position);
                }
            }
            catch(...) {
                if (new_finish.node_ != finish_.node_) {
                    this->destroy_nodes(finish_.node_ + 1, new_finish.node_);
                }
                throw;
            }
        }
    }

    template <typename Iterator, enable_if_t<!is_ranges_fwd_iter_v<Iterator>, int> = 0>
    void insert_ranges(iterator position, Iterator first, Iterator last) {
        if (last <= first) return;
        const size_type n = _MSTL distance(first, last);
        const size_type dist_before = position - start_;
        if (dist_before < size() / 2) {
            this->reallocate_map(n, true);
        }
        else {
            this->reallocate_map(n, false);
        }
        position = start_ + dist_before;
        auto cur = --last;
        for(size_type i = 0; i < n; ++i, --cur) {
            this->insert(position, *cur);
        }
    }

    template <typename Iterator, enable_if_t<is_ranges_fwd_iter_v<Iterator>, int> = 0>
    void insert_ranges(iterator position, Iterator first, Iterator last) {
        if (last <= first) return;
        const size_type n = _MSTL distance(first, last);

        if (position.cur_ == start_.cur_) {
            this->reallocate_map(n, true);
            auto new_start = start_ - n;
            try {
                _MSTL uninitialized_copy(first, last, new_start);
                start_ = new_start;
            }
            catch(...) {
                if (new_start.node_ != start_.node_) {
                    this->destroy_nodes(new_start.node_, start_.node_ - 1);
                }
                throw;
            }
        }
        else if (position.cur_ == finish_.cur_) {
            this->reallocate_map(n, false);
            auto new_finish = finish_ + n;
            try {
                _MSTL uninitialized_copy(first, last, finish_);
                finish_ = new_finish;
            }
            catch (...) {
                if (new_finish.node_ != finish_.node_) {
                    this->destroy_nodes(finish_.node_ + 1, new_finish.node_);
                }
                throw;
            }
        }
        else {
            this->insert_ranges_n(position, first, last, n);
        }
    }

    void insert_n_aux(iterator position, size_type n, const T& x) {
        difference_type dist_before = position - start_;
        if (dist_before < static_cast<difference_type>(size() / 2)) {
            this->reallocate_map(n, true);
            auto old_start = start_;
            auto new_start = start_ - n;
            position = start_ + dist_before;

            try {
                if (dist_before >= n) {
                    iterator start_n = start_ + n;
                    _MSTL uninitialized_copy(start_, start_n, new_start);
                    start_ = new_start;
                    _MSTL copy(start_n, position, old_start);
                    _MSTL fill(position - n, position, x);
                }
                else {
                    _MSTL uninitialized_fill(
                        _MSTL uninitialized_copy(start_, position, new_start), start_, x);
                    start_ = new_start;
                    _MSTL fill(old_start, position, x);
                }
            }
            catch(...) {
                if (new_start.node_ != start_.node_) {
                    this->destroy_nodes(new_start.node_, start_.node_ - 1);
                }
                throw;
            }
        }
        else {
            this->reallocate_map(n, false);
            auto old_finish = finish_;
            auto new_finish = finish_ + n;
            const size_type dist_after = size() - dist_before;
            position = finish_ - dist_after;

            try {
                if (dist_after > n) {
                    auto finish_n = finish_ - n;
                    _MSTL uninitialized_copy(finish_n, finish_, finish_);
                    finish_ = new_finish;
                    _MSTL copy_backward(position, finish_n, old_finish);
                    _MSTL fill(position, position + n, x);
                }
                else {
                    _MSTL uninitialized_fill(finish_, position + n, x);
                    _MSTL uninitialized_copy(position, finish_, position + n);
                    finish_ = new_finish;
                    _MSTL fill(position, old_finish, x);
                }
            }
            catch(...) {
                if (new_finish.node_ != finish_.node_) {
                    this->destroy_nodes(finish_.node_ + 1, new_finish.node_);
                }
                throw;
            }
        }
    }

    template <typename... Args>
    iterator insert_aux(iterator position, Args&&... args)
        noexcept(is_nothrow_move_assignable_v<value_type>) {
        size_type index = position - start_;
        if (index < size() / 2) {
            this->emplace_front(this->front());
            iterator front1 = start_;
            ++front1;
            iterator front2 = front1;
            ++front2;
            position = start_ + index;
            iterator pos1 = position;
            ++pos1;
            _MSTL copy(front2, pos1, front1);
        }
        else {
            this->push_back(this->back());
            iterator back1 = finish_;
            --back1;
            iterator back2 = back1;
            --back2;
            position = start_ + index;
            _MSTL copy_backward(position, back2, back1);
        }
        value_type val(_MSTL forward<Args>(args)...);
        *position = _MSTL move(val);
        return position;
    }

public:
    deque() {
        this->fill_initialize(0, _MSTL move(T()));
    }
    explicit deque(const size_type n) {
        this->fill_initialize(n, T());
    }
    deque(const size_type n, const T& x) {
        this->fill_initialize(n, x);
    }

    template <typename Iterator, enable_if_t<is_input_iter_v<Iterator>, int> = 0>
    deque(Iterator first, Iterator last) {
        this->copy_initialize(first, last);
    }

    deque(std::initializer_list<T> l) {
        this->copy_initialize(l.begin(), l.end());
    }

    self& operator =(std::initializer_list<T> l) {
        self tmp(l);
        this->swap(tmp);
        return *this;
    }

    deque(const self& x) {
        this->copy_initialize(x.cbegin(), x.cend());
    }

    self& operator =(const self& x) {
        if (_MSTL addressof(x) == this) return *this;

        const size_t len = size();
        if (len >= x.size()) {
            this->erase(_MSTL copy(x.start_, x.finish_, start_), finish_);
        }
        else {
            iterator mid = x.cbegin() + len;
            _MSTL copy(x.start_, mid, start_);
            this->insert(finish_, mid, x.finish_);
        }
        return *this;
    }

    deque(self&& x) noexcept {
        this->swap(x);
    }

    self& operator =(self&& x) noexcept {
        if (_MSTL addressof(x) == this) return *this;
        this->clear();
        this->swap(x);
        return *this;
    }

    ~deque() {
        if (map_pair_.value == nullptr) return;
        this->clear();

        if (map_pair_.value != nullptr) {
            for (size_type i = 0; i < map_size_pair_.value; ++i) {
                if (map_pair_.value[i] != nullptr) {
                    map_size_pair_.get_base().deallocate(map_pair_.value[i], buffer_size_);
                    map_pair_.value[i] = nullptr;
                }
            }

            map_pair_.get_base().deallocate(map_pair_.value, map_size_pair_.value);
            map_pair_.value = nullptr;
        }
    }

    MSTL_NODISCARD iterator begin() noexcept { return iterator(start_); }
    MSTL_NODISCARD iterator end() noexcept { return iterator(finish_); }
    MSTL_NODISCARD const_iterator begin() const noexcept { return cbegin(); }
    MSTL_NODISCARD const_iterator end() const noexcept { return cend(); }
    MSTL_NODISCARD const_iterator cbegin() const noexcept { return const_iterator(start_); }
    MSTL_NODISCARD const_iterator cend() const noexcept { return const_iterator(finish_); }
    MSTL_NODISCARD reverse_iterator rbegin() noexcept { return reverse_iterator(end()); }
    MSTL_NODISCARD reverse_iterator rend() noexcept { return reverse_iterator(begin()); }
    MSTL_NODISCARD const_reverse_iterator rbegin() const noexcept { return crbegin(); }
    MSTL_NODISCARD const_reverse_iterator rend() const noexcept { return crend(); }
    MSTL_NODISCARD const_reverse_iterator crbegin() const noexcept { return const_reverse_iterator(cend()); }
    MSTL_NODISCARD const_reverse_iterator crend() const noexcept { return const_reverse_iterator(cbegin()); }

    MSTL_NODISCARD size_type size() const noexcept { return finish_ - start_; }
    MSTL_NODISCARD size_type max_size() const noexcept { return static_cast<size_type>(-1); }
    MSTL_NODISCARD bool empty() const noexcept { return finish_ == start_; }

    MSTL_NODISCARD allocator_type get_allocator() const noexcept { return allocator_type(); }

    MSTL_NODISCARD reference front() noexcept {
        MSTL_DEBUG_VERIFY(!empty(), "front called on empty deque");
        return *start_;
    }
    MSTL_NODISCARD const_reference front() const noexcept {
        MSTL_DEBUG_VERIFY(!empty(), "front called on empty deque");
        return *start_;
    }
    MSTL_NODISCARD reference back() noexcept {
        MSTL_DEBUG_VERIFY(!empty(), "back called on empty deque");
        return *(finish_ - 1);
    }
    MSTL_NODISCARD const_reference back() const noexcept {
        MSTL_DEBUG_VERIFY(!empty(), "back called on empty deque");
        return *(finish_ - 1);
    }

    void resize(size_type new_size, const T& x) {
        const auto size = this->size();
        if (new_size < size)
            this->erase(start_ + new_size, finish_);
        else
            this->insert(finish_, new_size - size, x);
    }
    void resize(const size_type new_size) {
        this->resize(new_size, T());
    }

    template <typename... Args>
    iterator emplace(iterator position, Args&& ...args) {
        if (position.cur_ == start_.cur_) {
            this->emplace_front(_MSTL forward<Args>(args)...);
            return start_;
        }
        if (position.cur_ == finish_.cur_) {
            this->emplace_back(_MSTL forward<Args>(args)...);
            return finish_ - 1;
        }
        return this->insert_aux(position, _MSTL forward<Args>(args)...);
    }

    template <typename... Args>
    void emplace_back(Args&&... args) {
        if (finish_.cur_ != finish_.last_ - 1) {
            _MSTL construct(finish_.cur_, _MSTL forward<Args>(args)...);
            ++finish_.cur_;
        }
        else {
            this->reallocate_map(1, false);
            _MSTL construct(finish_.cur_, _MSTL forward<Args>(args)...);
            ++finish_;
        }
    }

    template <typename... Args>
    void emplace_front(Args&&... args) {
        if (start_.cur_ != start_.first_) {
            _MSTL construct(start_.cur_ - 1, _MSTL forward<Args>(args)...);
            --start_.cur_;
        }
        else {
            this->reallocate_map(1, true);
            --start_;
            _MSTL construct(start_.cur_, _MSTL forward<Args>(args)...);
        }
    }

    void push_back(const T& x) {
        if (finish_.cur_ != finish_.last_ - 1) {
            _MSTL construct(finish_.cur_, x);
            ++finish_.cur_;
        }
        else {
            reallocate_map(1, false);
            _MSTL construct(finish_.cur_, x);
            ++finish_;
        }
    }

    void push_front(const T& x) {
        if (start_.cur_ != start_.first_) {
            _MSTL construct(start_.cur_ - 1, x);
            --start_.cur_;
        }
        else {
            this->reallocate_map(1, true);
            --start_;
            _MSTL construct(start_.cur_, x);
        }
    }

    void push_back(T&& x) {
        this->emplace_back(_MSTL forward<T>(x));
    }

    void push_front(T&& x) {
        this->emplace_front(_MSTL forward<T>(x));
    }

    void pop_back() noexcept {
        MSTL_DEBUG_VERIFY(!empty(), "pop_back called on empty deque");
        if (finish_.cur_ != finish_.first_) {
            --finish_.cur_;
            _MSTL destroy(finish_.cur_);
        }
        else {
            auto cur = finish_.node_;
            --finish_;
            _MSTL destroy(finish_.cur_);
            map_size_pair_.get_base().deallocate(*cur, buffer_size_);
            *cur = nullptr;
        }
    }

    void pop_front() noexcept {
        MSTL_DEBUG_VERIFY(!empty(), "pop_front called on empty deque");
        if (start_.cur_ != start_.last_ - 1) {
            _MSTL destroy(start_.cur_);
            ++start_.cur_;
        }
        else {
            _MSTL destroy(start_.cur_);
            auto cur = start_.node_;
            map_size_pair_.get_base().deallocate(start_.first_, buffer_size_);
            *cur = nullptr;
            ++start_;
        }
    }

    void assign(const size_type count, const T& value) {
        this->assign_aux_n(count, value);
    }

    template <typename Iterator, enable_if_t<is_iter_v<Iterator>, int> = 0>
    void assign(Iterator first, Iterator last) {
        this->assign_ranges(first, last);
    }

    void assign(std::initializer_list<T> l) {
        this->assign_ranges(l.begin(), l.end());
    }

    iterator insert(iterator position, const T& x) {
        if (position.cur_ == start_.cur_) {
            this->push_front(x);
            return start_;
        }
        if (position.cur_ == finish_.cur_) {
            this->push_back(x);
            return _MSTL prev(finish_);
        }
        return this->insert_aux(position, x);
    }

    iterator insert(iterator position, T&& x) {
        if (position.cur_ == start_.cur_) {
            this->emplace_front(_MSTL move(x));
            return start_;
        }
        if (position.cur_ == finish_.cur_) {
            this->emplace_back(_MSTL move(x));
            return _MSTL prev(finish_);
        }
        return this->insert_aux(position, _MSTL move(x));
    }

    template <typename Iterator, enable_if_t<is_ranges_input_iter_v<Iterator>, int> = 0>
    void insert(iterator position, Iterator first, Iterator last) {
        this->insert_ranges(position, first, last);
    }

    iterator insert(iterator position, std::initializer_list<T> l) {
        return this->insert(position, l.begin(), l.end());
    }

    void insert(iterator position, const size_t n, const T& x) {
        if (position.cur_ == start_.cur_) {
            this->reallocate_map(n, true);
            auto new_start = start_ - n;
            _MSTL uninitialized_fill_n(new_start, n, x);
            start_ = new_start;
        }
        else if (position.cur_ == finish_.cur_) {
            this->reallocate_map(n, false);
            auto new_finish = finish_ + n;
            _MSTL uninitialized_fill_n(finish_, n, x);
            finish_ = new_finish;
        }
        else
            return this->insert_n_aux(position, n, x);
    }

    iterator erase(iterator position) noexcept {
        iterator next = _MSTL next(position);
        const size_type dest_before = position - start_;
        if (dest_before < size() / 2) {
            _MSTL copy_backward(start_, position, next);
            this->pop_front();
        }
        else {
            _MSTL copy(next, finish_, position);
            this->pop_back();
        }
        return start_ + dest_before;
    }

    iterator erase(iterator first, iterator last) noexcept {
        if (first == start_ && last == finish_) {
            this->clear();
            return finish_;
        }

        const size_type len = last - first;
        const size_type dist_before = first - start_;
        if (dist_before < (size() - len) / 2) {
            _MSTL copy_backward(start_, first, last);
            iterator new_start = start_ + len;
            _MSTL destroy(start_.cur_, new_start.cur_);
            start_ = new_start;
        }
        else {
            _MSTL copy(last, finish_, first);
            iterator new_finish = finish_ - len;
            _MSTL destroy(new_finish.cur_, finish_.cur_);
            finish_ = new_finish;
        }
        return start_ + dist_before;
    }

    void shrink_to_fit() {
        for (map_pointer cur = map_pair_.value; cur < start_.node_; ++cur) {
            if (*cur == nullptr) continue;
            map_size_pair_.get_base().deallocate(*cur, buffer_size_);
            *cur = nullptr;
        }
        for (map_pointer cur = finish_.node_ + 1; cur < map_pair_.value + map_size_pair_.value; ++cur) {
            if (*cur == nullptr) continue;
            map_size_pair_.get_base().deallocate(*cur, buffer_size_);
            *cur = nullptr;
        }
    }

    void clear() noexcept {
        for (map_pointer cur = start_.node_ + 1; cur < finish_.node_; ++cur) {
            _MSTL destroy(*cur, *cur + buffer_size_);
        }

        if (start_.node_ == finish_.node_)
            _MSTL destroy(start_.cur_, finish_.cur_);
        else {
            _MSTL destroy(start_.cur_, start_.last_);
            _MSTL destroy(finish_.first_, finish_.cur_);
        }

        shrink_to_fit();
        finish_ = start_;
    }

    MSTL_NODISCARD const_reference at(size_type position) const {
        return const_iterator(start_)[position];
    }
    MSTL_NODISCARD reference at(const size_type position) {
        return const_cast<reference>(static_cast<const self*>(this)->at(position));
    }
    MSTL_NODISCARD const_reference operator [](const size_type position) const {
        return this->at(position);
    }
    MSTL_NODISCARD reference operator [](const size_type position) {
        return this->at(position);
    }

    void swap(self& x) noexcept {
        if (_MSTL addressof(x) == this) return;
        _MSTL swap(start_, x.start_);
        _MSTL swap(finish_, x.finish_);
        _MSTL swap(map_pair_, x.map_pair_);
        _MSTL swap(map_size_pair_, x.map_size_pair_);
    }

    MSTL_NODISCARD bool operator ==(const self& rh) const
    noexcept(noexcept(this->size() == rh.size() && _MSTL equal(this->cbegin(), this->cend(), rh.cbegin()))) {
        return this->size() == rh.size() && _MSTL equal(this->cbegin(), this->cend(), rh.cbegin());
    }
    MSTL_NODISCARD bool operator !=(const self& rh) const
    noexcept(noexcept(!(*this == rh))) {
        return !(*this == rh);
    }
    MSTL_NODISCARD bool operator <(const self& rh) const
    noexcept(noexcept(_MSTL lexicographical_compare(this->cbegin(), this->cend(), rh.cbegin(), rh.cend()))) {
        return _MSTL lexicographical_compare(this->cbegin(), this->cend(), rh.cbegin(), rh.cend());
    }
    MSTL_NODISCARD bool operator >(const self& rh) const
    noexcept(noexcept(rh < *this)) {
        return rh < *this;
    }
    MSTL_NODISCARD bool operator >=(const self& rh) const
    noexcept(noexcept(!(*this < rh))) {
        return !(*this < rh);
    }
    MSTL_NODISCARD bool operator <=(const self& rh) const
    noexcept(noexcept(!(*this > rh))) {
        return !(*this > rh);
    }

    MSTL_NODISCARD size_type to_hash() const noexcept {
        return super::default_to_hash(*this);
    }

    MSTL_NODISCARD string to_string() const {
        return super::default_to_string(*this);
    }
};
#if MSTL_SUPPORT_DEDUCTION_GUIDES__
template <typename Iterator, typename Alloc>
deque(Iterator, Iterator, Alloc = Alloc()) -> deque<iter_value_t<Iterator>, Alloc>;
#endif

MSTL_END_NAMESPACE__
#endif // MSTL_CORE_CONTAINER_DEQUE_HPP__
