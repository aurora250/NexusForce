#ifndef MSTL_CORE_CONTAINER_RB_TREE_HPP__
#define MSTL_CORE_CONTAINER_RB_TREE_HPP__
#include "../string/serialize.hpp"
MSTL_BEGIN_NAMESPACE__

MSTL_INLINE17 constexpr bool RB_TREE_RED = false;
MSTL_INLINE17 constexpr bool RB_TREE_BLACK = true;

MSTL_BEGIN_INNER__
struct __rb_tree_node_base;
struct __rb_tree_base_iterator;
MSTL_END_INNER__

template <bool IsConst, typename RB_TREE>
struct rb_tree_iterator;
template <typename Key, typename Value, typename KeyOfValue, typename Compare, typename Alloc>
class rb_tree;

MSTL_BEGIN_INNER__

MSTL_API void rb_tree_rotate_left(__rb_tree_node_base* x, __rb_tree_node_base*& root) noexcept;
MSTL_API void rb_tree_rotate_right(__rb_tree_node_base* x, __rb_tree_node_base*& root) noexcept;
MSTL_API void rb_tree_rebalance(__rb_tree_node_base* x, __rb_tree_node_base*& root) noexcept;
MSTL_API __rb_tree_node_base* rb_tree_rebalance_for_erase(
    __rb_tree_node_base* z, __rb_tree_node_base*& root,
    __rb_tree_node_base*& leftmost, __rb_tree_node_base*& rightmost
    ) noexcept;


struct MSTL_API __rb_tree_node_base {
public:
    using color_type = bool;

protected:
    using base_ptr = __rb_tree_node_base*;

    color_type color_ = RB_TREE_RED;
    base_ptr parent_ = nullptr;
    base_ptr left_ = nullptr;
    base_ptr right_ = nullptr;

    friend _INNER __rb_tree_base_iterator;
    template <bool, typename> friend struct _MSTL rb_tree_iterator;
    template <typename, typename, typename, typename, typename> friend class _MSTL rb_tree;
    friend MSTL_API void _INNER rb_tree_rotate_left(__rb_tree_node_base*, __rb_tree_node_base*&) noexcept;
    friend MSTL_API void _INNER rb_tree_rotate_right(__rb_tree_node_base*, __rb_tree_node_base*&) noexcept;
    friend MSTL_API void _INNER rb_tree_rebalance(__rb_tree_node_base*, __rb_tree_node_base*&) noexcept;
    friend MSTL_API _INNER __rb_tree_node_base* _INNER rb_tree_rebalance_for_erase(
        _INNER __rb_tree_node_base*, _INNER __rb_tree_node_base*&,
        _INNER __rb_tree_node_base*&, _INNER __rb_tree_node_base*&) noexcept;

    static base_ptr minimum(base_ptr x) noexcept {
        while (x->left_ != nullptr) x = x->left_;
        return x;
    }
    static base_ptr maximum(base_ptr x) noexcept {
        while (x->right_ != nullptr) x = x->right_;
        return x;
    }
};

MSTL_END_INNER__


template <typename T>
struct rb_tree_node : _INNER __rb_tree_node_base {
private:
    T data_{};

    template <typename, typename, typename, typename, typename> friend class rb_tree;
    template <bool, typename> friend struct rb_tree_iterator;

public:
    rb_tree_node() = default;
    ~rb_tree_node() = default;
};


MSTL_BEGIN_INNER__
struct MSTL_API __rb_tree_base_iterator {
public:
    using iterator_category = bidirectional_iterator_tag;

protected:
    using base_ptr = __rb_tree_node_base::base_ptr;
    base_ptr node_ = nullptr;

    void increment() noexcept;
    void decrement() noexcept;

public:
    MSTL_NODISCARD bool operator ==(const __rb_tree_base_iterator& rh) const noexcept {
        return node_ == rh.node_;
    }
    MSTL_NODISCARD bool operator !=(const __rb_tree_base_iterator& rh) const noexcept {
        return node_ != rh.node_;
    }
};
MSTL_END_INNER__


template <bool IsConst, typename RB_TREE>
struct rb_tree_iterator : _INNER __rb_tree_base_iterator {
private:
    using container_type	= RB_TREE;
    using iterator			= rb_tree_iterator<false, container_type>;
    using const_iterator	= rb_tree_iterator<true, container_type>;

public:
    using value_type		= typename container_type::value_type;
    using reference			= conditional_t<IsConst, typename container_type::const_reference, typename container_type::reference>;
    using pointer			= conditional_t<IsConst, typename container_type::const_pointer, typename container_type::pointer>;
    using difference_type	= typename container_type::difference_type;
    using size_type			= typename container_type::size_type;

    using self              = rb_tree_iterator<IsConst, container_type>;

private:
    using link_type         = rb_tree_node<value_type>*;

    const container_type* tree_ = nullptr;

    template <typename, typename, typename, typename, typename> friend class rb_tree;
    template <bool, typename> friend struct rb_tree_iterator;

public:
    rb_tree_iterator() = default;
    rb_tree_iterator(link_type x, const container_type* cont)
    : tree_(cont) {
        node_ = x;
    }

    rb_tree_iterator(const iterator& it) {
        node_ = it.node_;
        tree_ = it.tree_;
    }
    self& operator =(const iterator& it) {
        if(_MSTL addressof(it) == this) return *this;
        node_ = it.node_;
        tree_ = it.tree_;
        return *this;
    }

    rb_tree_iterator(iterator&& it) noexcept {
        node_ = it.node_;
        tree_ = it.tree_;
        it.node_ = nullptr;
        it.tree_ = nullptr;
    }
    self& operator =(iterator&& it) {
        if(_MSTL addressof(it) == this) return *this;
        node_ = it.node_;
        tree_ = it.tree_;
        it.node_ = nullptr;
        it.tree_ = nullptr;
        return *this;
    }

    rb_tree_iterator(const const_iterator& it) {
        node_ = it.node_;
        tree_ = it.tree_;
    }
    self& operator =(const const_iterator& it) {
        if(_MSTL addressof(it) == this) return *this;
        node_ = it.node_;
        tree_ = it.tree_;
        return *this;
    }

    rb_tree_iterator(const_iterator&& it) {
        node_ = it.node_;
        tree_ = it.tree_;
        it.node_ = nullptr;
        it.tree_ = nullptr;
    }
    self& operator =(const_iterator&& it) {
        if(_MSTL addressof(it) == this) return *this;
        node_ = it.node_;
        tree_ = it.tree_;
        it.node_ = nullptr;
        it.tree_ = nullptr;
        return *this;
    }

    ~rb_tree_iterator() = default;

    MSTL_NODISCARD reference operator *() const noexcept {
        MSTL_DEBUG_VERIFY(node_ && tree_, __MSTL_DEBUG_MESG_OPERATE_NULLPTR(rb_tree_iterator, __MSTL_DEBUG_TAG_DEREFERENCE));
        link_type link = link_type(node_);
        MSTL_DEBUG_VERIFY(node_ != tree_->header_ && node_->parent_ != nullptr,
            __MSTL_DEBUG_MESG_OUT_OF_RANGE(rb_tree_iterator, __MSTL_DEBUG_TAG_DEREFERENCE));
        return link->data_;
    }
    MSTL_NODISCARD pointer operator ->() const noexcept {
        return &operator*();
    }

    self& operator ++() noexcept {
        MSTL_DEBUG_VERIFY(node_ && tree_, __MSTL_DEBUG_MESG_OPERATE_NULLPTR(rb_tree_iterator, __MSTL_DEBUG_TAG_INCREMENT));
        MSTL_DEBUG_VERIFY(link_type(node_) != tree_->header_,
            __MSTL_DEBUG_MESG_OUT_OF_RANGE(rb_tree_iterator, __MSTL_DEBUG_TAG_INCREMENT));
        increment();
        return *this;
    }
    self operator ++(int) noexcept {
        self tmp = *this;
        ++*this;
        return tmp;
    }
    self& operator --() noexcept {
        MSTL_DEBUG_VERIFY(node_ && tree_, __MSTL_DEBUG_MESG_OPERATE_NULLPTR(rb_tree_iterator, __MSTL_DEBUG_TAG_DECREMENT));
        MSTL_DEBUG_VERIFY(node_ != tree_->header_,
            __MSTL_DEBUG_MESG_OUT_OF_RANGE(rb_tree_iterator, __MSTL_DEBUG_TAG_DECREMENT));
        decrement();
        return *this;
    }
    self operator --(int) noexcept {
        self tmp = *this;
        --*this;
        return tmp;
    }

    MSTL_NODISCARD bool operator ==(const rb_tree_iterator& rh) const noexcept {
        MSTL_DEBUG_VERIFY(node_ && tree_, __MSTL_DEBUG_MESG_OPERATE_NULLPTR(rb_tree_iterator, __MSTL_DEBUG_TAG_DEREFERENCE));
		MSTL_DEBUG_VERIFY(tree_ == rh.tree_, __MSTL_DEBUG_MESG_CONTAINER_INCOMPATIBLE(rb_tree_iterator));
        return __rb_tree_base_iterator::operator ==(rh);
    }
    MSTL_NODISCARD bool operator !=(const rb_tree_iterator& rh) const noexcept {
        MSTL_DEBUG_VERIFY(node_ && tree_, __MSTL_DEBUG_MESG_OPERATE_NULLPTR(rb_tree_iterator, __MSTL_DEBUG_TAG_DEREFERENCE));
		MSTL_DEBUG_VERIFY(tree_ == rh.tree_, __MSTL_DEBUG_MESG_CONTAINER_INCOMPATIBLE(rb_tree_iterator));
        return __rb_tree_base_iterator::operator !=(rh);
    }
};


template <typename Key, typename Value, typename KeyOfValue, typename Compare, 
    typename Alloc = allocator<rb_tree_node<Value>>>
class rb_tree : icollector<rb_tree<Key, Value, KeyOfValue, Compare, Alloc>> {
#ifdef MSTL_STANDARD_20__
    static_assert(is_allocator_v<Alloc>, "Alloc type is not a standard allocator type.");
#endif
    static_assert(is_same_v<rb_tree_node<Value>, typename Alloc::value_type>, "allocator type mismatch.");
    static_assert(is_object_v<Value>, "list only contains object types.");

    using self = rb_tree<Key, Value, KeyOfValue, Compare, Alloc>;
    using super = icollector<self>;

    using base_node  = _INNER __rb_tree_node_base;
    using link_node = rb_tree_node<Value>;
    using base_ptr  = base_node*;
    using link_type = link_node*;

public:
    using key_type = Key;
    using color_type = bool;

    MSTL_BUILD_TYPE_ALIAS(Value)

    using iterator                  = rb_tree_iterator<false, self>;
    using const_iterator            = rb_tree_iterator<true, self>;
    using reverse_iterator          = _MSTL reverse_iterator<iterator>;
    using const_reverse_iterator    = _MSTL reverse_iterator<const_iterator>;

    using allocator_type = Alloc;

private:
    link_type header_ = nullptr;
    Compare key_compare_{};
    KeyOfValue extracter_{};
    compressed_pair<allocator_type, size_t> size_pair_{ _MSTL_TAG default_construct_tag{}, 0 }; // size

    template <bool, typename> friend struct rb_tree_iterator;

private:
    template <typename... Args>
    link_type create_node(Args... args) {
        link_type tmp = size_pair_.get_base().allocate();
        try {
            _MSTL construct(&tmp->data_, _MSTL forward<Args>(args)...);
        }
        catch (...) {
            destroy_node(tmp);
            throw_exception(memory_exception("rb tree construct node failed."));
        }
        return tmp;
    }
    link_type copy_node(link_type x) {
        link_type tmp = (create_node)(x->data_);
        tmp->color_ = x->color_;
        tmp->left_ = nullptr;
        tmp->right_ = nullptr;
        return tmp;
    }
    void destroy_node(link_type p) noexcept {
        if (p == nullptr) return;
        _MSTL destroy(&p->data_);
        size_pair_.get_base().deallocate(p);
    }

    link_type& root() const noexcept { return reinterpret_cast<link_type&>(header_->parent_); }
    link_type& leftmost() const noexcept { return reinterpret_cast<link_type&>(header_->left_); }
    link_type& rightmost() const noexcept { return reinterpret_cast<link_type&>(header_->right_); }

    static link_type& left(link_type x) noexcept { return reinterpret_cast<link_type&>(x->left_); }
    static link_type& right(link_type x) noexcept { return reinterpret_cast<link_type&>(x->right_); }
    static link_type& parent(link_type x) noexcept { return reinterpret_cast<link_type&>(x->parent_); }
    static const Key& key(link_type x) noexcept { return KeyOfValue()(x->data_); }
    static const Key& key(const base_ptr x) noexcept { return self::key(reinterpret_cast<link_type>(x)); }

    static link_type minimum(link_type x) noexcept {
        return static_cast<link_type>(base_node::minimum(x));
    }
    static link_type maximum(link_type x) noexcept {
        return static_cast<link_type>(base_node::maximum(x));
    }

    iterator insert_node_into(base_ptr bx, base_ptr by, link_type p) {
        auto x = static_cast<link_type>(bx);
        auto y = static_cast<link_type>(by);
        if (y == header_ || x != nullptr || key_compare_(key(p), key(y))) {
            left(y) = p;
            if (y == header_) {
                root() = p;
                leftmost() = p;
                rightmost() = p;
            }
            else if (y == leftmost()) leftmost() = p;
        }
        else {
            right(y) = p;
            if (y == rightmost()) rightmost() = p;
        }
        parent(p) = y;
        left(p) = nullptr;
        right(p) = nullptr;
        _INNER rb_tree_rebalance(p, header_->parent_);
        ++size_pair_.value;
        return iterator(p, this);
    }

    link_type copy_under_node(link_type x, link_type parent) {
        link_type top = copy_node(x);
        top->parent_ = parent;
        try{
            if (x->right_ != nullptr) 
                top->right_ = copy_under_node(right(x), top);
            parent = top;
            x = left(x);
            while (x != nullptr) {
                link_type y = copy_node(x);
                parent->left_ = y;
                y->parent_ = parent;
                if (x->right_ != nullptr) 
                    y->right_ = copy_under_node(right(x), y);
                parent = y;
                x = left(x);
            }
            if (root() != nullptr) {
                leftmost() = minimum(root());
                rightmost() = maximum(root());
            } else {
                leftmost() = header_;
                rightmost() = header_;
            }
        }
        catch (...) {
            this->erase_under_node(top);
            throw;
        }
        return top;
    }

    void erase_under_node(link_type x) noexcept {
        if (x == nullptr) return;
        this->erase_under_node(this->right(x));
        this->erase_under_node(this->left(x));
        this->destroy_node(x);
    }

    void header_init() {
        header_ = size_pair_.get_base().allocate();
        header_->color_ = RB_TREE_RED;
        root() = nullptr;
        leftmost() = header_;
        rightmost() = header_;
    }

    void copy_from(const self& x) {
        if (x.root() == nullptr) {
            root() = nullptr;
            leftmost() = header_;
            rightmost() = header_;
        }
        else {
            try {
              root() = copy_under_node(x.root(), header_);
            }
            catch (...) {
                size_pair_.get_base().deallocate(header_);
                throw;
            }
            leftmost() = minimum(root());
            rightmost() = maximum(root());
        }
        size_pair_.value = x.size_pair_.value;
    }

    pair<iterator, bool> insert_unique_node(link_type p) {
        link_type y = header_;
        link_type x = root();
        bool comp = true;
        while (x != nullptr) {
            y = x;
            comp = key_compare_(key(p), key(x));
            x = comp ? left(x) : right(x);
        }
        iterator j(y, this);
        if (comp) {
            if (j == begin())
                return pair<iterator, bool>(insert_node_into(x, y, p), true);
            --j;
        }
        if (key_compare_(key(link_type(j.node_)), key(p)))
            return pair<iterator, bool>(insert_node_into(x, y, p), true);
        destroy_node(p);
        return pair<iterator, bool>(j, false);
    }

    iterator insert_equal_node(link_type p) {
        link_type y = header_;
        link_type x = root();
        while (x != nullptr) {
            y = x;
            x = key_compare_(key(p), key(x)) ? left(x) : right(x);
        }
        return insert_node_into(x, y, p);
    }

public:
    rb_tree() {
        header_init();
    }

    explicit rb_tree(const Compare& comp) : key_compare_(comp) {
        header_init();
    }

    rb_tree(const self& x) :
    key_compare_(x.key_compare_),
    extracter_(x.extracter_), size_pair_(x.size_pair_) {
        header_init();
        copy_from(x);
    }
    self& operator =(const self& x) {
        if (_MSTL addressof(x) == this) return *this;
        clear();
        copy_from(x);
        return *this;
    }

    rb_tree(self&& x) noexcept :
    header_(_MSTL move(x.header_)), key_compare_(_MSTL move(x.key_compare_)),
    extracter_(_MSTL move(x.extracter_)), size_pair_(_MSTL move(x.size_pair_)) {
        x.header_ = nullptr;
        x.size_pair_.value = 0;
    }

    self& operator =(self&& x) noexcept {
        if (_MSTL addressof(x) == this) return *this;
        clear();
        size_pair_.get_base().deallocate(header_);
        header_ = x.header_;
        key_compare_ = _MSTL move(x.key_compare_);
        size_pair_ = _MSTL move(x.size_pair_);
        x.header_ = nullptr;
        x.size_pair_.value = 0;
        return *this;
    }

    ~rb_tree() {
        clear();
        if (header_) {
            size_pair_.get_base().deallocate(header_);
        }
    }

    MSTL_NODISCARD iterator begin() noexcept { return {leftmost(), this}; }
    MSTL_NODISCARD iterator end() noexcept { return {header_, this}; }
    MSTL_NODISCARD const_iterator begin() const noexcept { return cbegin(); }
    MSTL_NODISCARD const_iterator end() const noexcept { return cend(); }
    MSTL_NODISCARD const_iterator cbegin() const noexcept { return {leftmost(), this}; }
    MSTL_NODISCARD const_iterator cend() const noexcept { return {header_, this}; }
    MSTL_NODISCARD reverse_iterator rbegin() noexcept { return reverse_iterator(end()); }
    MSTL_NODISCARD reverse_iterator rend() noexcept { return reverse_iterator(begin()); }
    MSTL_NODISCARD const_reverse_iterator rbegin() const noexcept { return crbegin(); }
    MSTL_NODISCARD const_reverse_iterator rend() const noexcept { return crend(); }
    MSTL_NODISCARD const_reverse_iterator crbegin() const noexcept { return const_reverse_iterator(cend()); }
    MSTL_NODISCARD const_reverse_iterator crend() const noexcept { return const_reverse_iterator(cbegin()); }

    MSTL_NODISCARD size_type size() const noexcept { return size_pair_.value; }
    MSTL_NODISCARD size_type max_size() const noexcept { return static_cast<size_type>(-1); }
    MSTL_NODISCARD bool empty() const noexcept { return size_pair_.value == 0; }

    MSTL_NODISCARD allocator_type get_allocator() const noexcept { return allocator_type(); }

    MSTL_NODISCARD Compare key_comp() const noexcept(is_nothrow_copy_constructible_v<Compare>) {
        return key_compare_;
    }

    template <typename... Args>
    pair<iterator, bool> emplace_unique(Args&&... args) {
        const link_type tmp = (create_node)(_MSTL forward<Args>(args)...);
        return (insert_unique_node)(tmp);
    }
    pair<iterator, bool> insert_unique(const value_type& v) {
        return (emplace_unique)(v);
    }
    pair<iterator, bool> insert_unique(value_type&& v) {
        return (emplace_unique)(_MSTL move(v));
    }
    template <typename... Args>
    iterator emplace_unique_hint(iterator position, Args&&... args) {
        link_type tmp = (create_node)(_MSTL forward<Args>(args)...);
        if (position.node_ == header_->left_) {
            if (size() > 0 && key_compare_(key(tmp), key(position.node_)))
                return insert_node_into(position.node_, position.node_, tmp);
            return insert_unique_node(tmp).first;
        }
        if (position.node_ == header_) {
            if (key_compare_(key(rightmost()), key(tmp)))
                return insert_node_into(nullptr, rightmost(), tmp);
            return insert_unique_node(tmp).first;
        }
        iterator before = position;
        --before;
        if (key_compare_(key(before.node_), key(tmp)) &&
            key_compare_(key(tmp), key(position.node_))) {
            if (right(link_type(before.node_)) == nullptr)
                return insert_node_into(nullptr, before.node_, tmp);
            return insert_node_into(position.node_, position.node_, tmp);
        }
        return insert_unique_node(tmp).first;
    }
    iterator insert_unique(iterator position, const value_type& v) {
        return (emplace_unique_hint)(position, v);
    }
    iterator insert_unique(iterator position, value_type&& v) {
        return (emplace_unique_hint)(position, _MSTL move(v));
    }
    template <typename Iterator, enable_if_t<is_ranges_input_iter_v<Iterator>, int> = 0>
    void insert_unique(Iterator first, Iterator last) {
        for (; first != last; ++first) insert_unique(*first);
    }

    template <typename... Args>
    iterator emplace_equal(Args&&... args) {
        const link_type tmp = (create_node)(_MSTL forward<Args>(args)...);
        return (insert_equal_node)(tmp);
    }
    iterator insert_equal(const value_type& v) {
        return (emplace_equal)(v);
    }
    iterator insert_equal(value_type&& v) {
        return (emplace_equal)(_MSTL move(v));
    }
    template <typename... Args>
    iterator emplace_equal_hint(iterator position, Args&&... args) {
        link_type tmp = (create_node)(_MSTL forward<Args>(args)...);
        if (position.node_ == header_->left_) {
            if (size() > 0 && key_compare_(key(tmp), key(position.node_)))
                return insert_node_into(position.node_, position.node_, tmp);
            return insert_equal_node(tmp);
        }
        if (position.node_ == header_) {
            if (!key_compare_(key(tmp), key(rightmost())))
                return insert_node_into(nullptr, rightmost(), tmp);
            return insert_equal_node(tmp);
        }
        iterator before = position;
        --before;
        if (!key_compare_(key(tmp), key(before.node_)) &&
            !key_compare_(key(position.node_), key(tmp))) {
            if (right(link_type(before.node_)) == nullptr)
                return insert_node_into(nullptr, before.node_, tmp);
            return insert_node_into(position.node_, position.node_, tmp);
        }
        return insert_equal_node(tmp);
    }
    iterator insert_equal(iterator position, const value_type& v) {
        return (emplace_equal_hint)(position, v);
    }
    iterator insert_equal(iterator position, value_type&& v) {
        return (emplace_equal_hint)(position, _MSTL move(v));
    }
    template <typename Iterator, enable_if_t<is_ranges_input_iter_v<Iterator>, int> = 0>
    void insert_equal(Iterator first, Iterator last) {
        for (; first != last; ++first) insert_equal(*first);
    }

    size_type erase(const key_type& k) noexcept {
        pair<iterator, iterator> p = equal_range(k);
        const size_type n = _MSTL distance(p.first, p.second);
        erase(p.first, p.second);
        return n;
    }
    void erase(iterator position) noexcept {
        auto y = reinterpret_cast<link_type>(rb_tree_rebalance_for_erase(
            position.node_, header_->parent_, header_->left_, header_->right_));
        destroy_node(y);
        --size_pair_.value;
    }
    void erase(iterator first, iterator last) noexcept {
        if (first == begin() && last == end()) 
            clear();
        else 
            while (first != last) erase(first++);
    }

    void clear() noexcept {
        if (size_pair_.value == 0) return;
        this->erase_under_node(root());
        leftmost() = header_;
        root() = nullptr;
        rightmost() = header_;
        size_pair_.value = 0;
    }

    MSTL_NODISCARD iterator find(const key_type& k) {
        link_type y = header_;
        link_type x = root();
        while (x != nullptr) {
            if (!key_compare_(key(x), k)) {
                y = x;
                x = left(x);
            }
            else x = right(x);
        }
        iterator j(y, this);
        if (j == end())
            return end();
        return key_compare_(k, key(y)) ? end() : j;
    }
    MSTL_NODISCARD const_iterator find(const key_type& k) const {
        link_type y = header_;
        link_type x = root();
        while (x != nullptr) {
            if (!key_compare_(key(x), k)) {
                y = x;
                x = left(x);
            }
            else x = right(x);
        }
        const_iterator j(y, this);
        if (j == cend())
            return cend();
        return key_compare_(k, key(y)) ? cend() : j;
    }

    MSTL_NODISCARD size_type count(const key_type& k) const {
        pair<const_iterator, const_iterator> p = equal_range(k);
        const size_type n = _MSTL distance(p.first, p.second);
        return n;
    }

    MSTL_NODISCARD iterator lower_bound(const key_type& k) {
        link_type y = header_;
        link_type x = root();
        while (x != nullptr) {
            if (!key_compare_(key(x), k)) {
                y = x;
                x = left(x);
            }
            else x = right(x);
        }
        return iterator(y, this);
    }
    MSTL_NODISCARD const_iterator lower_bound(const key_type& k) const {
        link_type y = header_;
        link_type x = root();
        while (x != nullptr) {
            if (!key_compare_(key(x), k)) {
                y = x;
                x = left(x);
            }
            else x = right(x);
        }
        return const_iterator(y, this);
    }

    MSTL_NODISCARD iterator upper_bound(const key_type& k) {
        link_type y = header_;
        link_type x = root();
        while (x != nullptr) {
            if (key_compare_(k, key(x))) {
                y = x;
                x = left(x);
            }
            else x = right(x);
        }
        return iterator(y, this);
    }
    MSTL_NODISCARD const_iterator upper_bound(const key_type& k) const {
        link_type y = header_;
        link_type x = root();
        while (x != nullptr) {
            if (key_compare_(k, key(x))) {
                y = x;
                x = left(x);
            }
            else x = right(x);
        }
        return const_iterator(y, this);
    }

    MSTL_NODISCARD pair<iterator, iterator> equal_range(const key_type& k) {
        return pair<iterator, iterator>(this->lower_bound(k), this->upper_bound(k));
    }
    MSTL_NODISCARD pair<const_iterator, const_iterator> equal_range(const key_type& k) const {
        return pair<const_iterator, const_iterator>(this->lower_bound(k), this->upper_bound(k));
    }

    void swap(self& x)
    noexcept(is_nothrow_swappable_v<Compare> &&
    is_nothrow_swappable_v<KeyOfValue> &&
    noexcept(size_pair_.swap(x.size_pair_))) {
        _MSTL swap(header_, x.header_);
        _MSTL swap(size_pair_, x.size_pair_);
        _MSTL swap(key_compare_, x.key_compare_);
        _MSTL swap(extracter_, x.extracter_);
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

MSTL_END_NAMESPACE__
#endif // MSTL_CORE_CONTAINER_RB_TREE_HPP__
