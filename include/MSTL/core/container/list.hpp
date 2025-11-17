#ifndef MSTL_LIST_HPP__
#define MSTL_LIST_HPP__
#include "../string/serialize.hpp"
MSTL_BEGIN_NAMESPACE__

template <typename T, typename Alloc>
class list;
template <bool IsConst, typename List>
struct list_iterator;

template <typename T>
struct list_node {
private:
    using value_type    = T;
    using node_type     = list_node<value_type>;

    value_type data_{};
    node_type* prev_ = nullptr;
    node_type* next_ = nullptr;

    template <typename, typename> friend class list;
    template <bool, typename> friend struct list_iterator;
};

template <bool IsConst, typename List>
struct list_iterator {
private:
    using container_type	= List;
    using iterator			= list_iterator<false, container_type>;
    using const_iterator	= list_iterator<true, container_type>;

public:
    using iterator_category = bidirectional_iterator_tag;
    using value_type		= typename container_type::value_type;
    using reference			= conditional_t<IsConst, typename container_type::const_reference, typename container_type::reference>;
    using pointer			= conditional_t<IsConst, typename container_type::const_pointer, typename container_type::pointer>;
    using difference_type	= typename container_type::difference_type;
    using size_type			= typename container_type::size_type;

    using self              = list_iterator<IsConst, container_type>;

private:
    using node_type         = list_node<value_type>;

    node_type* node_ = nullptr;
    const container_type* list_ = nullptr;

    template <typename, typename> friend class list;
    template <bool, typename> friend struct list_iterator;

public:
    list_iterator() noexcept = default;
    list_iterator(node_type* x, const container_type* list) noexcept
    : node_(x), list_(list) {}

    list_iterator(const iterator& x) noexcept
    : node_(x.node_), list_(x.list_) {}

    self& operator =(const iterator& x) noexcept {
    	if(_MSTL addressof(x) == this) return *this;
		node_ = x.node_;
        list_ = x.list_;
		return *this;
	}

    list_iterator(const const_iterator& x) noexcept
    : node_(x.node_), list_(x.list_) {}

    self& operator =(const const_iterator& x) noexcept {
        if(_MSTL addressof(x) == this) return *this;
        node_ = x.node_;
        list_ = x.list_;
        return *this;
    }

    list_iterator(iterator&& x) noexcept : node_(x.node_), list_(x.list_) {
        x.node_ = nullptr;
        x.list_ = nullptr;
    }

    self& operator =(iterator&& x) noexcept {
        if(_MSTL addressof(x) == this) return *this;
        node_ = x.node_;
        list_ = x.list_;
        x.node_ = nullptr;
        x.list_ = nullptr;
        return *this;
    }

    list_iterator(const_iterator&& x) noexcept
    : node_(x.node_), list_(x.list_) {
        x.node_ = nullptr;
        x.list_ = nullptr;
    }

    self& operator =(const_iterator&& x) noexcept {
        if(_MSTL addressof(x) == this) return *this;
        node_ = x.node_;
        list_ = x.list_;
        x.node_ = nullptr;
        x.list_ = nullptr;
        return *this;
    }

    ~list_iterator() = default;

    MSTL_NODISCARD reference operator *() const noexcept {
        MSTL_DEBUG_VERIFY(list_ && node_ && node_ != list_->head_,
            __MSTL_DEBUG_MESG_OPERATE_NULLPTR(list_iterator, __MSTL_DEBUG_TAG_DEREFERENCE));
        return node_->data_;
    }
    MSTL_NODISCARD pointer operator->() const noexcept {
        return &operator*();
    }

    self& operator ++() noexcept {
        MSTL_DEBUG_VERIFY(list_ && node_, __MSTL_DEBUG_MESG_OPERATE_NULLPTR(list_iterator, __MSTL_DEBUG_TAG_INCREMENT));
        MSTL_DEBUG_VERIFY(node_ != list_->head_, __MSTL_DEBUG_MESG_OUT_OF_RANGE(list_iterator, __MSTL_DEBUG_TAG_INCREMENT));
        node_ = node_->next_;
        return *this;
    }
    self operator ++(int) noexcept {
        self old = *this;
        ++*this;
        return old;
    }
    self& operator --() noexcept {
        MSTL_DEBUG_VERIFY(list_ && node_, __MSTL_DEBUG_MESG_OPERATE_NULLPTR(list_iterator, __MSTL_DEBUG_TAG_DECREMENT));
        MSTL_DEBUG_VERIFY(node_->prev_ != list_->head_, __MSTL_DEBUG_MESG_OUT_OF_RANGE(list_iterator, __MSTL_DEBUG_TAG_DECREMENT));
        node_ = node_->prev_;
        return *this;
    }
    self operator --(int) noexcept {
        self old = *this;
        --*this;
        return old;
    }

    MSTL_NODISCARD bool operator ==(const self& x) noexcept {
		MSTL_DEBUG_VERIFY(list_ == x.list_, __MSTL_DEBUG_MESG_CONTAINER_INCOMPATIBLE(list_iterator));
        return node_ == x.node_;
    }
    MSTL_NODISCARD bool operator !=(const self& x) noexcept {
        return !(*this == x);
    }
};


template <typename T, typename Alloc = allocator<list_node<T>>>
class list : public icollector<list<T, Alloc>> {
#ifdef MSTL_STANDARD_20__
    static_assert(is_allocator_v<Alloc>, "Alloc type is not a standard allocator type.");
#endif
    static_assert(is_same_v<list_node<T>, typename Alloc::value_type>, "allocator type mismatch.");
    static_assert(is_object_v<T>, "list only contains object types.");

    using self = list<T, Alloc>;
    using super = icollector<self>;

public:
    MSTL_BUILD_TYPE_ALIAS(T)
    using allocator_type            = Alloc;
    using iterator                  = list_iterator<false, self>;
    using const_iterator            = list_iterator<true, self>;
    using reverse_iterator          = _MSTL reverse_iterator<iterator>;
    using const_reverse_iterator    = _MSTL reverse_iterator<const_iterator>;

private:
    using node_type = list_node<T>;
    using link_type = node_type*;

    link_type head_ = nullptr;
    compressed_pair<allocator_type, size_type> pair_{ _MSTL_TAG default_construct_tag{}, 0 };

    template <bool, typename> friend struct list_iterator;

private:
    template <typename... Args>
    link_type create_node(Args&&... args) {
        link_type p = pair_.get_base().allocate();
        _MSTL construct(&p->data_, _MSTL forward<Args>(args)...);
        return p;
    }
    void destroy_node(link_type p) noexcept {
        _MSTL destroy(p);
        pair_.get_base().deallocate(p);
    }

    void empty_init() {
        head_ = create_node();
        head_->prev_ = head_->next_ = head_;
    }

public:
    list() {
        empty_init();
    }
    explicit list(size_type n) {
        empty_init();
        while (n--) push_back(value_type());
    };

    list(size_type n, const T& x) {
        empty_init();
        while (n--) push_back(x);
    };

    template <typename Iterator, enable_if_t<is_ranges_input_iter_v<Iterator>, int> = 0>
    list(Iterator first, Iterator last) {
        empty_init();
        while (first != last) {
            push_back(*first);
            ++first;
        }
    };

    list(std::initializer_list<T> l) : list(l.begin(), l.end()) {}

    self& operator =(std::initializer_list<T> l) {
        clear();
        insert(begin(), l.begin(), l.end());
        return *this;
    }

    list(const self& x) : list(x.cbegin(), x.cend()) {}

    self& operator =(const self& x) {
        if (_MSTL addressof(x) == this) return *this;
        clear();
        link_type p = x.head_->next_;
        while (p != x.head_) {
            link_type q = this->create_node(p->data_);
            q->prev_ = head_->prev_;
            q->next_ = head_;
            head_->prev_->next_ = q;
            head_->prev_ = q;
            p = p->next_;
        }
        pair_.value = x.pair_.value;
        return *this;
    }

    list(self&& x) noexcept {
        empty_init();
        this->swap(x);
    }
    self& operator =(self&& x) noexcept {
        if (_MSTL addressof(x) == this) return *this;
        this->swap(x);
        return *this;
    }

    ~list() {
        link_type p = head_->next_;
        while (p != head_) {
            link_type q = p;
            p = p->next_;
            this->destroy_node(q);
        }
        this->destroy_node(head_);
    }

    MSTL_NODISCARD iterator begin() noexcept { return {head_->next_, this}; }
    MSTL_NODISCARD iterator end() noexcept { return {head_, this}; }
    MSTL_NODISCARD const_iterator begin() const noexcept { return cbegin(); }
    MSTL_NODISCARD const_iterator end() const noexcept { return cend(); }
    MSTL_NODISCARD const_iterator cbegin() const noexcept { return {head_->next_, this}; }
    MSTL_NODISCARD const_iterator cend() const noexcept { return {head_, this}; }
    MSTL_NODISCARD reverse_iterator rbegin() noexcept { return reverse_iterator(end()); }
    MSTL_NODISCARD reverse_iterator rend() noexcept { return reverse_iterator(begin()); }
    MSTL_NODISCARD const_reverse_iterator rbegin() const noexcept { return crbegin(); }
    MSTL_NODISCARD const_reverse_iterator rend() const noexcept { return crend(); }
    MSTL_NODISCARD const_reverse_iterator crbegin() const noexcept { return const_reverse_iterator(cend()); }
    MSTL_NODISCARD const_reverse_iterator crend() const noexcept { return const_reverse_iterator(cbegin()); }

    MSTL_NODISCARD size_type size() const noexcept { return pair_.value; }
    MSTL_NODISCARD size_type max_size() const noexcept { return static_cast<size_type>(-1); }
    MSTL_NODISCARD bool empty() const noexcept { return head_->next_ == head_; }

    MSTL_NODISCARD allocator_type get_allocator() { return allocator_type(); }

    MSTL_NODISCARD reference front() noexcept {
        MSTL_DEBUG_VERIFY(!empty(), "front called on empty list");
        return head_->next_->data_;
    }
    MSTL_NODISCARD const_reference front() const noexcept {
        MSTL_DEBUG_VERIFY(!empty(), "front called on empty list");
        return head_->next_->data_;
    }
    MSTL_NODISCARD reference back() noexcept {
        MSTL_DEBUG_VERIFY(!empty(), "back called on empty list");
        return head_->prev_->data_;
    }
    MSTL_NODISCARD const_reference back() const noexcept {
        MSTL_DEBUG_VERIFY(!empty(), "back called on empty list");
        return head_->prev_->data_;
    }

    template <typename... U>
    iterator emplace(iterator position, U&&... args) {
        link_type temp = (create_node)(_MSTL forward<U>(args)...);
        temp->next_ = position.node_;
        temp->prev_ = position.node_->prev_;
        position.node_->prev_->next_ = temp;
        position.node_->prev_ = temp;
        ++pair_.value;
        return {temp, this};
    }

    template <typename... Args>
    iterator emplace_back(Args&&... args) {
        return (emplace)(end(), _MSTL forward<Args>(args)...);
    }
    template <typename... Args>
    iterator emplace_front(Args&&... args) {
        return (emplace)(begin(), _MSTL forward<Args>(args)...);
    }

    void push_front(const T& x) { insert(begin(), x); }
    void push_front(T&& x) { insert(begin(), _MSTL forward<T>(x)); }
    void push_back(const T& x) { insert(end(), x); }
    void push_back(T&& x) { insert(end(), _MSTL forward<T>(x)); }

    void pop_front() noexcept { erase(begin()); }
    void pop_back() noexcept { erase({head_->prev_, this}); }

    void assign(size_type count, const T& value) {
        clear();
        insert(begin(), count, value);
    }

    template <typename Iterator, enable_if_t<is_ranges_input_iter_v<Iterator>, int> = 0>
    void assign(Iterator first, Iterator last) {
        clear();
        insert(begin(), first, last);
    }

    void assign(std::initializer_list<T> l) {
        assign(l.begin(), l.end());
    }

    iterator insert(iterator position, const T& x) {
        return (emplace)(position, x);
    }
    iterator insert(iterator position, T&& x) {
        return (emplace)(position, _MSTL forward<T>(x));
    }

    template <typename InputIterator>
    void insert(iterator position, InputIterator first, InputIterator last) {
        range_check(position);
        Exception(_MSTL distance(first, last) >= 0, StopIterator("deque insert out of ranges."));
        for (--last; first != last; --last)
            position = insert(position, *last);
        insert(position, *last);
    }
    void insert(iterator position, std::initializer_list<T> l) {
        insert(position, l.begin(), l.end());
    }

    void insert(iterator position, size_type n, const T& x) {
        range_check(position);
        link_type prev = position.node_->prev_;
        while (n--) {
            link_type temp = this->create_node(x);
            temp->prev_ = prev;
            temp->next_ = prev->next_;
            prev->next_->prev_ = temp;
            prev->next_ = temp;
            prev = temp;
            ++pair_.value;
        }
    }

    iterator erase(iterator position) noexcept {
        if (empty()) return end();
        link_type ret = position.node_->next_;
        position.node_->prev_->next_ = position.node_->next_;
        position.node_->next_->prev_ = position.node_->prev_;
        destroy_node(position.node_);
        --pair_.value;
        return {ret, this};
    }
    iterator erase(iterator first, iterator last) noexcept {
        while (first != last) first = erase(first);
        return first;
    }
    void clear() noexcept {
        link_type cur = head_->next_;
        while (cur != head_) {
            link_type temp = cur;
            cur = cur->next_;
            destroy_node(temp);
            --pair_.value;
        }
        head_->prev_ = head_;
        head_->next_ = head_;
    }

    void swap(self& x) noexcept {
        _MSTL swap(head_, x.head_);
        _MSTL swap(pair_, x.pair_);
    }

    void transfer(iterator position, iterator first, iterator last) {
        range_check(position);
        range_check(first);
        range_check(last);
        Exception(_MSTL distance(first, last) >= 0, StopIterator("list transfer index out of ranges."));
        if (position == last) return;
        last.node_->prev_->next_ = position.node_;
        first.node_->prev_->next_ = last.node_;
        position.node_->prev_->next_ = first.node_;
        link_type tmp = position.node_->prev_;
        position.node_->prev_ = last.node_->prev_;
        last.node_->prev_ = first.node_->prev_;
        first.node_->prev_ = tmp;
    }

    template <typename Pred>
    void remove_if(Pred pred) {
        iterator iter = begin(), last = end();
        while (iter != last) {
            if (pred(*iter)) iter = erase(iter);
            else ++iter;
        }
    }
    void remove(const T& x) {
        return this->remove_if([&](const T& oth) -> bool { return oth == x; });
    }

    void splice(iterator position, self& x) {
        if (!x.empty()) 
            transfer(position, x.begin(), x.end());
    }
    void splice(iterator position, self&, iterator i) {
        iterator j = i;
        ++j;
        if (i == position || j == position) return;
        transfer(position, i, j);
    }
    void splice(iterator position, self&, iterator first, iterator last) {
        if (first != last) transfer(position, first, last);
    }

    template <typename Pred>
    void merge(self& x, Pred pred) {
        iterator first1 = begin(), first2 = x.begin();
        iterator last1 = end(), last2 = x.end();
        while (first1 != last1 && first2 != last2) {
            if (!pred(*first2, *first1)) {
                ++first1;
            }
            else {
                iterator temp = first2;
                ++temp;
                transfer(first1, first2, temp);
                first2 = temp;
            }
        }
        if (first2 != last2) {
            transfer(last1, first2, last2);
        }
    }
    void merge(self& x) {
        this->merge(x, _MSTL less<T>());
    }
    template <typename Pred>
    void merge(self&& x, Pred pred) {
        iterator first1 = begin(), first2 = x.begin();
        iterator last1 = end(), last2 = x.end();
        while (first1 != last1 && first2 != last2) {
            if (!pred(*first2, *first1)) {
                ++first1;
            }
            else {
                iterator temp = first2;
                ++temp;
                transfer(first1, first2, temp);
                first2 = temp;
            }
        }
        if (first2 != last2) {
            transfer(last1, first2, last2);
        }
        x.clear();
    }
    void merge(self&& x) {
        this->merge(_MSTL forward<self>(x), _MSTL less<T>());
    }

    void reverse() noexcept {
        if (empty()) return;
        link_type current = head_;
        do {
            _MSTL swap(current->prev_, current->next_);
            current = current->prev_;
        } while (current != head_);
    }

    template <typename Pred>
    void unique(Pred pred) noexcept {
        if (empty()) return;
        iterator current = begin();
        iterator next = current;
        while (++next != end()) {
            if (pred(*current, *next)) {
                this->erase(next);
                next = current;
            } else {
                current = next;
            }
        }
    }
    void unique() noexcept {
        unique(_MSTL equal_to<T>());
    }

    template <typename Pred>
    void sort(Pred pred) {
        if (empty()) return;
        link_type p = head_->next_->next_;
        while (p != head_) {
            T temp = p->data_;
            link_type prev = p->prev_;
            while (prev != head_ && pred(temp, prev->data_)) {
                prev->next_->data_ = prev->data_;
                prev = prev->prev_;
            }
            prev->next_->data_ = temp;
            p = p->next_;
        }
    }
    void sort() {
        sort(_MSTL less<T>());
    }

    MSTL_NODISCARD const_reference at(size_type position) const {
        const_iterator iter = cbegin();
        while (position--) ++iter;
        return iter.node_->data_;
    }
    MSTL_NODISCARD reference at(const size_type position) {
        return const_cast<reference>(
            static_cast<const self*>(this)->at(position)
            );
    }
    MSTL_NODISCARD const_reference operator [](const size_type position) const {
        return this->at(position);
    }
    MSTL_NODISCARD reference operator [](const size_type position) {
        return this->at(position);
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
list(Iterator, Iterator, Alloc = Alloc()) -> list<iter_value_t<Iterator>, Alloc>;
#endif

MSTL_END_NAMESPACE__
#endif // MSTL_LIST_HPP__
