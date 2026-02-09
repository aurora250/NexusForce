#ifndef MSTL_CORE_CONTAINER_LIST_HPP__
#define MSTL_CORE_CONTAINER_LIST_HPP__
#include "MSTL/core/interface/icollector.hpp"
#include "MSTL/core/interface/iiterator.hpp"
MSTL_BEGIN_NAMESPACE__

template <typename T, typename Alloc>
class list;


template <typename T>
struct list_node {
    T data;
    list_node* prev = nullptr;
    list_node* next = nullptr;

    list_node()
    noexcept(is_nothrow_default_constructible_v<T>)
    : data() {}

    explicit list_node(T&& value)
    noexcept(is_nothrow_constructible_v<T, T&&>)
    : data(_MSTL forward<T>(value)) {}
};


template <bool IsConst, typename List>
struct list_iterator : iiterator<list_iterator<IsConst, List>> {
public:
    using container_type	= List;
    using value_type		= typename container_type::value_type;
    using size_type			= typename container_type::size_type;
    using difference_type	= typename container_type::difference_type;
    using iterator_category = bidirectional_iterator_tag;
    using reference = conditional_t<IsConst, typename container_type::const_reference, typename container_type::reference>;
    using pointer	= conditional_t<IsConst, typename container_type::const_pointer, typename container_type::pointer>;

private:
    using node_type = list_node<value_type>;

    node_type* current_ = nullptr;
    const container_type* container_ = nullptr;

    template <typename, typename>
    friend class list;

public:
    list_iterator() noexcept = default;
    ~list_iterator() = default;

    list_iterator(const list_iterator&) noexcept = default;
    list_iterator& operator =(const list_iterator&) noexcept = default;
    list_iterator(list_iterator&&) noexcept = default;
    list_iterator& operator =(list_iterator&&) noexcept = default;

    list_iterator(node_type* ptr, const container_type* list) noexcept
    : current_(ptr), container_(list) {}

    MSTL_NODISCARD reference dereference() const noexcept {
        MSTL_DEBUG_VERIFY(current_ && container_, "Attempting to dereference on a null pointer");
        MSTL_DEBUG_VERIFY(current_ != container_->head_, "Attempting to dereference out of boundary");
        return current_->data;
    }

    void increment() noexcept {
        MSTL_DEBUG_VERIFY(current_ && container_, "Attempting to increment a null pointer");
        MSTL_DEBUG_VERIFY(current_ != container_->head_, "Attempting to increment out of boundary");
        current_ = current_->next;
    }

    void decrement() noexcept {
        MSTL_DEBUG_VERIFY(current_ && container_, "Attempting to decrement a null pointer");
        MSTL_DEBUG_VERIFY(current_->prev != container_->head_, "Attempting to decrement out of boundary");
        current_ = current_->prev;
    }

    MSTL_NODISCARD bool equal(const list_iterator& rhs) const noexcept {
        MSTL_DEBUG_VERIFY(container_ == rhs.container_, "Attempting to equal to a different container");
        return current_ == rhs.current_;
    }

    MSTL_NODISCARD pointer base() const noexcept {
        return current_;
    }

    MSTL_NODISCARD const container_type* container() const noexcept {
        return container_;
    }
};


template <typename T, typename Alloc = allocator<list_node<T>>>
class list : public icollector<list<T, Alloc>> {
#ifdef MSTL_STANDARD_20__
    static_assert(is_allocator_v<Alloc>, "Alloc type is not a standard allocator type.");
#endif
    static_assert(is_same_v<list_node<T>, typename Alloc::value_type>, "allocator type mismatch.");
    static_assert(is_object_v<T>, "list only contains object types.");

public:
    MSTL_BUILD_TYPE_ALIAS(T)
    using allocator_type            = Alloc;
    using iterator                  = list_iterator<false, list>;
    using const_iterator            = list_iterator<true, list>;
    using reverse_iterator          = _MSTL reverse_iterator<iterator>;
    using const_reverse_iterator    = _MSTL reverse_iterator<const_iterator>;

private:
    using node_type = list_node<T>;
    using link_type = node_type*;

    link_type head_ = nullptr;
    compressed_pair<allocator_type, size_type> pair_{ default_construct_tag{}, 0 };

    template <bool, typename> friend struct list_iterator;

private:
    template <typename... Args>
    link_type create_node(Args&&... args) {
        link_type p = pair_.get_base().allocate();
        _MSTL construct(&p->data, _MSTL forward<Args>(args)...);
        return p;
    }
    void destroy_node(link_type p) noexcept {
        _MSTL destroy(p);
        pair_.get_base().deallocate(p);
    }

    void empty_init() {
        head_ = create_node();
        head_->prev = head_->next = head_;
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

    list& operator =(std::initializer_list<T> l) {
        clear();
        insert(begin(), l.begin(), l.end());
        return *this;
    }

    list(const list& x) : list(x.cbegin(), x.cend()) {}

    list& operator =(const list& x) {
        if (_MSTL addressof(x) == this) return *this;
        clear();
        link_type p = x.head_->next;
        while (p != x.head_) {
            link_type q = this->create_node(p->data);
            q->prev = head_->prev;
            q->next = head_;
            head_->prev->next = q;
            head_->prev = q;
            p = p->next;
        }
        pair_.value = x.pair_.value;
        return *this;
    }

    list(list&& x) noexcept {
        empty_init();
        this->swap(x);
    }
    list& operator =(list&& x) noexcept {
        if (_MSTL addressof(x) == this) return *this;
        this->swap(x);
        return *this;
    }

    ~list() {
        link_type p = head_->next;
        while (p != head_) {
            link_type q = p;
            p = p->next;
            this->destroy_node(q);
        }
        this->destroy_node(head_);
    }

    MSTL_NODISCARD iterator begin() noexcept { return {head_->next, this}; }
    MSTL_NODISCARD iterator end() noexcept { return {head_, this}; }
    MSTL_NODISCARD const_iterator begin() const noexcept { return cbegin(); }
    MSTL_NODISCARD const_iterator end() const noexcept { return cend(); }
    MSTL_NODISCARD const_iterator cbegin() const noexcept { return {head_->next, this}; }
    MSTL_NODISCARD const_iterator cend() const noexcept { return {head_, this}; }
    MSTL_NODISCARD reverse_iterator rbegin() noexcept { return reverse_iterator(end()); }
    MSTL_NODISCARD reverse_iterator rend() noexcept { return reverse_iterator(begin()); }
    MSTL_NODISCARD const_reverse_iterator rbegin() const noexcept { return crbegin(); }
    MSTL_NODISCARD const_reverse_iterator rend() const noexcept { return crend(); }
    MSTL_NODISCARD const_reverse_iterator crbegin() const noexcept { return const_reverse_iterator(cend()); }
    MSTL_NODISCARD const_reverse_iterator crend() const noexcept { return const_reverse_iterator(cbegin()); }

    MSTL_NODISCARD size_type size() const noexcept { return pair_.value; }
    MSTL_NODISCARD size_type max_size() const noexcept { return static_cast<size_type>(-1); }
    MSTL_NODISCARD bool empty() const noexcept { return head_->next == head_; }

    MSTL_NODISCARD allocator_type get_allocator() { return allocator_type(); }

    MSTL_NODISCARD reference front() noexcept {
        MSTL_DEBUG_VERIFY(!empty(), "front called on empty list");
        return head_->next->data;
    }
    MSTL_NODISCARD const_reference front() const noexcept {
        MSTL_DEBUG_VERIFY(!empty(), "front called on empty list");
        return head_->next->data;
    }
    MSTL_NODISCARD reference back() noexcept {
        MSTL_DEBUG_VERIFY(!empty(), "back called on empty list");
        return head_->prev->data;
    }
    MSTL_NODISCARD const_reference back() const noexcept {
        MSTL_DEBUG_VERIFY(!empty(), "back called on empty list");
        return head_->prev->data;
    }

    template <typename... U>
    iterator emplace(iterator position, U&&... args) {
        link_type temp = (create_node)(_MSTL forward<U>(args)...);
        temp->next = position.current_;
        temp->prev = position.current_->prev;
        position.current_->prev->next = temp;
        position.current_->prev = temp;
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
    void pop_back() noexcept { erase({head_->prev, this}); }

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
        for (--last; first != last; --last)
            position = insert(position, *last);
        insert(position, *last);
    }
    void insert(iterator position, std::initializer_list<T> l) {
        insert(position, l.begin(), l.end());
    }

    void insert(iterator position, size_type n, const T& x) {
        link_type prev = position.current_->prev;
        while (n--) {
            link_type temp = this->create_node(x);
            temp->prev = prev;
            temp->next = prev->next;
            prev->next->prev = temp;
            prev->next = temp;
            prev = temp;
            ++pair_.value;
        }
    }

    iterator erase(iterator position) noexcept {
        if (empty()) return end();
        link_type ret = position.current_->next;
        position.current_->prev->next = position.current_->next;
        position.current_->next->prev = position.current_->prev;
        destroy_node(position.current_);
        --pair_.value;
        return {ret, this};
    }
    iterator erase(iterator first, iterator last) noexcept {
        while (first != last) first = erase(first);
        return first;
    }
    void clear() noexcept {
        link_type cur = head_->next;
        while (cur != head_) {
            link_type temp = cur;
            cur = cur->next;
            destroy_node(temp);
            --pair_.value;
        }
        head_->prev = head_;
        head_->next = head_;
    }

    void swap(list& x) noexcept {
        _MSTL swap(head_, x.head_);
        _MSTL swap(pair_, x.pair_);
    }

    void transfer(iterator position, iterator first, iterator last) {
        if (position == last) return;
        last.current_->prev->next = position.current_;
        first.current_->prev->next = last.current_;
        position.current_->prev->next = first.current_;
        link_type tmp = position.current_->prev;
        position.current_->prev = last.current_->prev;
        last.current_->prev = first.current_->prev;
        first.current_->prev = tmp;
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

    void splice(iterator position, list& x) {
        if (!x.empty()) 
            transfer(position, x.begin(), x.end());
    }
    void splice(iterator position, list&, iterator i) {
        iterator j = i;
        ++j;
        if (i == position || j == position) return;
        transfer(position, i, j);
    }
    void splice(iterator position, list&, iterator first, iterator last) {
        if (first != last) transfer(position, first, last);
    }

    template <typename Pred>
    void merge(list& x, Pred pred) {
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
    void merge(list& x) {
        this->merge(x, _MSTL less<T>());
    }
    template <typename Pred>
    void merge(list&& x, Pred pred) {
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
    void merge(list&& x) {
        this->merge(_MSTL forward<list>(x), _MSTL less<T>());
    }

    void reverse() noexcept {
        if (empty()) return;
        link_type current = head_;
        do {
            _MSTL swap(current->prev, current->next);
            current = current->prev;
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
        link_type p = head_->next->next;
        while (p != head_) {
            T temp = p->data;
            link_type prev = p->prev;
            while (prev != head_ && pred(temp, prev->data)) {
                prev->next->data = prev->data;
                prev = prev->prev;
            }
            prev->next->data = temp;
            p = p->next;
        }
    }
    void sort() {
        sort(_MSTL less<T>());
    }

    MSTL_NODISCARD const_reference at(size_type position) const {
        const_iterator iter = cbegin();
        while (position--) ++iter;
        return iter.current_->data;
    }
    MSTL_NODISCARD reference at(const size_type position) {
        return const_cast<reference>(
            static_cast<const list*>(this)->at(position)
            );
    }
    MSTL_NODISCARD const_reference operator [](const size_type position) const {
        return this->at(position);
    }
    MSTL_NODISCARD reference operator [](const size_type position) {
        return this->at(position);
    }

    MSTL_NODISCARD bool operator ==(const list& rhs) const
    noexcept(noexcept(this->size() == rhs.size() && _MSTL equal(this->cbegin(), this->cend(), rhs.cbegin()))) {
        return this->size() == rhs.size() && _MSTL equal(this->cbegin(), this->cend(), rhs.cbegin());
    }

    MSTL_NODISCARD bool operator <(const list& rhs) const
    noexcept(noexcept(_MSTL lexicographical_compare(this->cbegin(), this->cend(), rhs.cbegin(), rhs.cend()))) {
        return _MSTL lexicographical_compare(this->cbegin(), this->cend(), rhs.cbegin(), rhs.cend());
    }
};
#if MSTL_SUPPORT_DEDUCTION_GUIDES__
template <typename Iterator, typename Alloc>
list(Iterator, Iterator, Alloc = Alloc()) -> list<iter_value_t<Iterator>, Alloc>;
#endif

MSTL_END_NAMESPACE__
#endif // MSTL_CORE_CONTAINER_LIST_HPP__
