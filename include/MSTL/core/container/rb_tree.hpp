#ifndef MSTL_CORE_CONTAINER_RB_TREE_HPP__
#define MSTL_CORE_CONTAINER_RB_TREE_HPP__
#include "MSTL/core/algorithm/compare.hpp"
#include "MSTL/core/interface/icollector.hpp"
#include "MSTL/core/interface/iiterator.hpp"
#include "MSTL/core/memory/construct.hpp"
#include "MSTL/core/memory/standard_allocator.hpp"
#include "MSTL/core/utility/compressed_pair.hpp"
#include "MSTL/core/utility/pair.hpp"
MSTL_BEGIN_NAMESPACE__

MSTL_INLINE17 constexpr bool RB_TREE_RED = false;
MSTL_INLINE17 constexpr bool RB_TREE_BLACK = true;


struct rb_tree_node_base {
    using color_type = bool;
    using base_ptr = rb_tree_node_base*;

    color_type color_ = RB_TREE_RED;
    base_ptr parent_ = nullptr;
    base_ptr left_ = nullptr;
    base_ptr right_ = nullptr;

    static base_ptr minimum(base_ptr x) noexcept {
        while (x->left_ != nullptr) {
            x = x->left_;
        }
        return x;
    }

    static base_ptr maximum(base_ptr x) noexcept {
        while (x->right_ != nullptr) {
            x = x->right_;
        }
        return x;
    }
};


MSTL_ALWAYS_INLINE_INLINE void
rb_tree_rotate_left(rb_tree_node_base* x, rb_tree_node_base*& root) noexcept {
    rb_tree_node_base* y = x->right_;
    x->right_ = y->left_;
    if (y->left_ != nullptr) {
        y->left_->parent_ = x;
    }

    y->parent_ = x->parent_;

    if (x == root) {
        root = y;
    } else if (x == x->parent_->left_) {
        x->parent_->left_ = y;
    } else {
        x->parent_->right_ = y;
    }

    y->left_ = x;
    x->parent_ = y;
}

MSTL_ALWAYS_INLINE_INLINE void
rb_tree_rotate_right(rb_tree_node_base* x, rb_tree_node_base*& root) noexcept {
    rb_tree_node_base* y = x->left_;
    x->left_ = y->right_;
    if (y->right_ != nullptr) {
        y->right_->parent_ = x;
    }
    y->parent_ = x->parent_;

    if (x == root) {
        root = y;
    } else if (x == x->parent_->right_) {
        x->parent_->right_ = y;
    } else {
        x->parent_->left_ = y;
    }

    y->right_ = x;
    x->parent_ = y;
}

MSTL_ALWAYS_INLINE_INLINE void
rb_tree_rebalance(rb_tree_node_base* x, rb_tree_node_base*& root) noexcept {
    x->color_ = RB_TREE_RED;

    while (x != root && x->parent_->color_ == RB_TREE_RED) {
        if (x->parent_ == x->parent_->parent_->left_) {
            rb_tree_node_base* y = x->parent_->parent_->right_;

            if (y != nullptr && y->color_ == RB_TREE_RED) {
                x->parent_->color_ = RB_TREE_BLACK;
                y->color_ = RB_TREE_BLACK;
                x->parent_->parent_->color_ = RB_TREE_RED;
                x = x->parent_->parent_;
            } else {
                if (x == x->parent_->right_) {
                    x = x->parent_;
                    _MSTL rb_tree_rotate_left(x, root);
                }
                x->parent_->color_ = RB_TREE_BLACK;
                x->parent_->parent_->color_ = RB_TREE_RED;
                _MSTL rb_tree_rotate_right(x->parent_->parent_, root);
            }
        } else {
            rb_tree_node_base* y = x->parent_->parent_->left_;
            if (y != nullptr && y->color_ == RB_TREE_RED) {
                x->parent_->color_ = RB_TREE_BLACK;
                y->color_ = RB_TREE_BLACK;
                x->parent_->parent_->color_ = RB_TREE_RED;
                x = x->parent_->parent_;
            } else {
                if (x == x->parent_->left_) {
                    x = x->parent_;
                    _MSTL rb_tree_rotate_right(x, root);
                }
                x->parent_->color_ = RB_TREE_BLACK;
                x->parent_->parent_->color_ = RB_TREE_RED;
                _MSTL rb_tree_rotate_left(x->parent_->parent_, root);
            }
        }
    }
    root->color_ = RB_TREE_BLACK;
}

MSTL_ALWAYS_INLINE_INLINE rb_tree_node_base*
rb_tree_rebalance_for_erase(rb_tree_node_base* z, rb_tree_node_base*& root,
                            rb_tree_node_base*& leftmost, rb_tree_node_base*& rightmost) noexcept {
    rb_tree_node_base* y = z;
    rb_tree_node_base* x;
    rb_tree_node_base* x_parent;

    if (y->left_ == nullptr) {
        x = y->right_;
    } else {
        if (y->right_ == nullptr) {
            x = y->left_;
        } else {
            y = y->right_;
            while (y->left_ != nullptr) {
                y = y->left_;
            }
            x = y->right_;
        }
    }

    if (y != z) {
        z->left_->parent_ = y;
        y->left_ = z->left_;

        if (y != z->right_) {
            x_parent = y->parent_;
            if (x != nullptr) {
                x->parent_ = y->parent_;
            }

            y->parent_->left_ = x;
            y->right_ = z->right_;
            z->right_->parent_ = y;
        } else {
            x_parent = y;
        }

        if (root == z) {
            root = y;
        } else if (z->parent_->left_ == z) {
            z->parent_->left_ = y;
        } else {
            z->parent_->right_ = y;
        }

        y->parent_ = z->parent_;
        _MSTL swap(y->color_, z->color_);
        y = z;
    } else {
        x_parent = y->parent_;
        if (x != nullptr) {
            x->parent_ = y->parent_;
        }

        if (root == z) {
            root = x;
        } else {
            if (z->parent_->left_ == z) {
                z->parent_->left_ = x;
            } else {
                z->parent_->right_ = x;
            }
        }

        if (leftmost == z) {
            if (z->right_ == nullptr) {
                leftmost = z->parent_;
            } else {
                leftmost = rb_tree_node_base::minimum(x);
            }
        }

        if (rightmost == z) {
            if (z->left_ == nullptr) {
                rightmost = z->parent_;
            } else {
                rightmost = rb_tree_node_base::maximum(x);
            }
        }
    }

    if (y->color_ != RB_TREE_RED) {
        while (x != root && (x == nullptr || x->color_ == RB_TREE_BLACK)) {
            if (x == x_parent->left_) {
                rb_tree_node_base* w = x_parent->right_;
                if (!w) continue;

                if (w->color_ == RB_TREE_RED) {
                    w->color_ = RB_TREE_BLACK;
                    x_parent->color_ = RB_TREE_RED;
                    _MSTL rb_tree_rotate_left(x_parent, root);
                    w = x_parent->right_;
                }

                if ((w->left_ == nullptr || w->left_->color_ == RB_TREE_BLACK) &&
                    (w->right_ == nullptr || w->right_->color_ == RB_TREE_BLACK)) {
                    w->color_ = RB_TREE_RED;
                    x = x_parent;
                    x_parent = x_parent->parent_;
                } else {
                    if (w->right_ == nullptr || w->right_->color_ == RB_TREE_BLACK) {
                        if (w->left_ != nullptr) w->left_->color_ = RB_TREE_BLACK;
                        w->color_ = RB_TREE_RED;
                        _MSTL rb_tree_rotate_right(w, root);
                        w = x_parent->right_;
                    }
                    w->color_ = x_parent->color_;
                    x_parent->color_ = RB_TREE_BLACK;
                    if (w->right_ != nullptr) w->right_->color_ = RB_TREE_BLACK;
                    _MSTL rb_tree_rotate_left(x_parent, root);
                    break;
                }
            } else {
                rb_tree_node_base* w = x_parent->left_;
                if (w->color_ == RB_TREE_RED) {
                    w->color_ = RB_TREE_BLACK;
                    x_parent->color_ = RB_TREE_RED;
                    _MSTL rb_tree_rotate_right(x_parent, root);
                    w = x_parent->left_;
                }

                if ((w->right_ == nullptr || w->right_->color_ == RB_TREE_BLACK) &&
                    (w->left_ == nullptr || w->left_->color_ == RB_TREE_BLACK)) {
                    w->color_ = RB_TREE_RED;
                    x = x_parent;
                    x_parent = x_parent->parent_;
                } else {
                    if (w->left_ == nullptr || w->left_->color_ == RB_TREE_BLACK) {
                        if (w->right_ != nullptr) {
                            w->right_->color_ = RB_TREE_BLACK;
                        }

                        w->color_ = RB_TREE_RED;
                        _MSTL rb_tree_rotate_left(w, root);
                        w = x_parent->left_;
                    }
                    w->color_ = x_parent->color_;
                    x_parent->color_ = RB_TREE_BLACK;
                    if (w->left_ != nullptr) {
                        w->left_->color_ = RB_TREE_BLACK;
                    }

                    _MSTL rb_tree_rotate_right(x_parent, root);
                    break;
                }
            }
        }

        if (x != nullptr) {
            x->color_ = RB_TREE_BLACK;
        }
    }
    return y;
}


template <typename T>
struct rb_tree_node : rb_tree_node_base {
    T data;

    rb_tree_node()
    noexcept(is_nothrow_default_constructible_v<T>)
    : data() {}
};


struct rb_tree_base_iterator {
public:
    using iterator_category = bidirectional_iterator_tag;

protected:
    using base_ptr = rb_tree_node_base::base_ptr;

    base_ptr node_ = nullptr;

    void increment() noexcept {
        if (node_->right_ != nullptr) {
            node_ = node_->right_;
            while (node_->left_ != nullptr) {
                node_ = node_->left_;
            }
        } else {
            base_ptr y = node_->parent_;
            while (node_ == y->right_) {
                node_ = y;
                y = y->parent_;
            }
            if (node_->right_ != y) {
                node_ = y;
            }
        }
    }

    void decrement() noexcept {
        if (node_->color_ == RB_TREE_RED &&
            node_->parent_ != nullptr &&
            node_->parent_->parent_ == node_) {
            node_ = node_->right_;
        } else if (node_->left_ != nullptr) {
            base_ptr y = node_->left_;
            while (y->right_ != nullptr) {
                y = y->right_;
            }
            node_ = y;
        } else {
            base_ptr y = node_->parent_;
            while (node_ == y->left_) {
                node_ = y;
                y = y->parent_;
            }
            node_ = y;
        }
    }
};


template <bool IsConst, typename RbTree>
struct rb_tree_iterator : rb_tree_base_iterator, iiterator<rb_tree_iterator<IsConst, RbTree>> {
public:
    using container_type	= RbTree;
    using value_type		= typename container_type::value_type;
    using size_type			= typename container_type::size_type;
    using difference_type	= typename container_type::difference_type;
    using iterator_category = rb_tree_base_iterator::iterator_category;
    using reference = conditional_t<IsConst, typename container_type::const_reference, typename container_type::reference>;
    using pointer	= conditional_t<IsConst, typename container_type::const_pointer, typename container_type::pointer>;

private:
    using base_type = rb_tree_base_iterator;
    using node_type = rb_tree_node<value_type>;
    using link_type = node_type*;

    const container_type* container_ = nullptr;

    template <typename, typename, typename, typename, typename>
    friend class rb_tree;

public:
    rb_tree_iterator() noexcept = default;
    ~rb_tree_iterator() = default;

    rb_tree_iterator(const rb_tree_iterator&) noexcept = default;
    rb_tree_iterator& operator =(const rb_tree_iterator&) noexcept = default;
    rb_tree_iterator(rb_tree_iterator&&) noexcept = default;
    rb_tree_iterator& operator =(rb_tree_iterator&&) noexcept = default;

    rb_tree_iterator(node_type* ptr, const container_type* tree) noexcept
    : container_(tree) { node_ = ptr; }

    MSTL_NODISCARD reference dereference() const noexcept {
        MSTL_DEBUG_VERIFY(node_ && container_, "Attempting to dereference on a null pointer");
        link_type link = link_type(node_);
        MSTL_DEBUG_VERIFY(
            node_ != container_->header_ && node_->parent_ != nullptr,
            "Attempting to dereference out of boundary");
        return link->data;
    }

    MSTL_CONSTEXPR20 void increment() noexcept {
        MSTL_DEBUG_VERIFY(node_ && container_, "Attempting to increment a null pointer");
        MSTL_DEBUG_VERIFY(link_type(node_) != container_->header_, "Attempting to increment out of boundary");
        base_type::increment();
    }

    MSTL_CONSTEXPR20 void decrement() noexcept {
        MSTL_DEBUG_VERIFY(node_ && container_, "Attempting to decrement a null pointer");
        MSTL_DEBUG_VERIFY(node_ != container_->header_, "Attempting to decrement out of boundary");
        base_type::decrement();
    }

    MSTL_NODISCARD bool equal(const rb_tree_iterator& rhs) const noexcept {
        MSTL_DEBUG_VERIFY(container_ == rhs.container_, "Attempting to equal to a different container");
        return node_ == rhs.node_;
    }

    MSTL_NODISCARD pointer base() const noexcept {
        return node_;
    }

    MSTL_NODISCARD const container_type* container() const noexcept {
        return container_;
    }
};


template <
    typename Key, typename Value,
    typename KeyOfValue, typename Compare,
    typename Alloc = allocator<rb_tree_node<Value>>
>
class rb_tree : icollector<rb_tree<Key, Value, KeyOfValue, Compare, Alloc>> {
    static_assert(is_allocator_v<Alloc>, "Alloc type is not a standard allocator type.");
    static_assert(is_same_v<rb_tree_node<Value>, typename Alloc::value_type>, "allocator type mismatch.");
    static_assert(is_object_v<Value>, "list only contains object types.");

private:
    using base_node = rb_tree_node_base;
    using link_node = rb_tree_node<Value>;
    using base_ptr  = base_node*;
    using link_type = link_node*;

public:
    using key_type = Key;
    using color_type = bool;

    MSTL_BUILD_TYPE_ALIAS(Value)
    using iterator                  = rb_tree_iterator<false, rb_tree>;
    using const_iterator            = rb_tree_iterator<true, rb_tree>;
    using reverse_iterator          = _MSTL reverse_iterator<iterator>;
    using const_reverse_iterator    = _MSTL reverse_iterator<const_iterator>;

    using allocator_type = Alloc;

private:
    link_type header_ = nullptr;
    Compare key_compare_{};
    KeyOfValue extracter_{};
    compressed_pair<allocator_type, size_t> size_pair_{ default_construct_tag{}, 0 };

    template <bool, typename> friend struct rb_tree_iterator;

private:
    template <typename... Args>
    link_type create_node(Args... args) {
        link_type tmp = size_pair_.get_base().allocate();
        try {
            _MSTL construct(&tmp->data, _MSTL forward<Args>(args)...);
        } catch (...) {
            rb_tree::destroy_node(tmp);
            throw_exception(memory_exception("rb tree construct node failed."));
        }
        return tmp;
    }

    link_type copy_node(link_type x) {
        link_type tmp = rb_tree::create_node(x->data);
        tmp->color_ = x->color_;
        tmp->left_ = nullptr;
        tmp->right_ = nullptr;
        return tmp;
    }

    void destroy_node(link_type p) noexcept {
        if (p == nullptr) return;
        _MSTL destroy(&p->data);
        size_pair_.get_base().deallocate(p);
    }

    link_type& root() const noexcept { return reinterpret_cast<link_type&>(header_->parent_); }
    link_type& leftmost() const noexcept { return reinterpret_cast<link_type&>(header_->left_); }
    link_type& rightmost() const noexcept { return reinterpret_cast<link_type&>(header_->right_); }

    static link_type& left(link_type x) noexcept { return reinterpret_cast<link_type&>(x->left_); }
    static link_type& right(link_type x) noexcept { return reinterpret_cast<link_type&>(x->right_); }
    static link_type& parent(link_type x) noexcept { return reinterpret_cast<link_type&>(x->parent_); }

    static const Key& key(link_type x) noexcept { return KeyOfValue()(x->data); }
    static const Key& key(const base_ptr x) noexcept { return rb_tree::key(reinterpret_cast<link_type>(x)); }

    static link_type minimum(link_type x) noexcept {
        return static_cast<link_type>(base_node::minimum(x));
    }
    static link_type maximum(link_type x) noexcept {
        return static_cast<link_type>(base_node::maximum(x));
    }

    iterator insert_node_into(base_ptr bx, base_ptr by, link_type p) {
        auto x = static_cast<link_type>(bx);
        auto y = static_cast<link_type>(by);
        if (y == header_ || x != nullptr || key_compare_(rb_tree::key(p), rb_tree::key(y))) {
            rb_tree::left(y) = p;
            if (y == header_) {
                root() = p;
                leftmost() = p;
                rightmost() = p;
            } else if (y == leftmost()) {
                leftmost() = p;
            }
        }
        else {
            rb_tree::right(y) = p;
            if (y == rightmost()) {
                rightmost() = p;
            }
        }
        rb_tree::parent(p) = y;
        rb_tree::left(p) = nullptr;
        rb_tree::right(p) = nullptr;
        _MSTL rb_tree_rebalance(p, header_->parent_);
        ++size_pair_.value;
        return iterator(p, this);
    }

    link_type copy_under_node(link_type x, link_type parent) {
        link_type top = copy_node(x);
        top->parent_ = parent;
        try{
            if (x->right_ != nullptr) {
                top->right_ = rb_tree::copy_under_node(rb_tree::right(x), top);
            }
            parent = top;
            x = rb_tree::left(x);
            while (x != nullptr) {
                link_type y = rb_tree::copy_node(x);
                parent->left_ = y;
                y->parent_ = parent;
                if (x->right_ != nullptr) {
                    y->right_ = rb_tree::copy_under_node(rb_tree::right(x), y);
                }
                parent = y;
                x = rb_tree::left(x);
            }
            if (root() != nullptr) {
                leftmost() = rb_tree::minimum(root());
                rightmost() = rb_tree::maximum(root());
            } else {
                leftmost() = header_;
                rightmost() = header_;
            }
        } catch (...) {
            rb_tree::erase_under_node(top);
            throw;
        }
        return top;
    }

    void erase_under_node(link_type x) noexcept {
        if (x == nullptr) return;
        rb_tree::erase_under_node(rb_tree::right(x));
        rb_tree::erase_under_node(rb_tree::left(x));
        rb_tree::destroy_node(x);
    }

    void header_init() {
        header_ = size_pair_.get_base().allocate();
        header_->color_ = RB_TREE_RED;
        root() = nullptr;
        leftmost() = header_;
        rightmost() = header_;
    }

    void copy_from(const rb_tree& x) {
        if (x.root() == nullptr) {
            root() = nullptr;
            leftmost() = header_;
            rightmost() = header_;
        } else {
            try {
              root() = rb_tree::copy_under_node(x.root(), header_);
            } catch (...) {
                size_pair_.get_base().deallocate(header_);
                throw;
            }
            leftmost() = rb_tree::minimum(root());
            rightmost() = rb_tree::maximum(root());
        }
        size_pair_.value = x.size_pair_.value;
    }

    pair<iterator, bool> insert_unique_node(link_type p) {
        link_type y = header_;
        link_type x = root();
        bool comp = true;
        while (x != nullptr) {
            y = x;
            comp = key_compare_(rb_tree::key(p), rb_tree::key(x));
            x = comp ? rb_tree::left(x) : rb_tree::right(x);
        }

        iterator j(y, this);
        if (comp) {
            if (j == begin()) {
                return pair<iterator, bool>(rb_tree::insert_node_into(x, y, p), true);
            }
            --j;
        }

        if (key_compare_(rb_tree::key(link_type(j.node_)), rb_tree::key(p))) {
            return pair<iterator, bool>(rb_tree::insert_node_into(x, y, p), true);
        }
        rb_tree::destroy_node(p);
        return pair<iterator, bool>(j, false);
    }

    iterator insert_equal_node(link_type p) {
        link_type y = header_;
        link_type x = root();
        while (x != nullptr) {
            y = x;
            x = key_compare_(rb_tree::key(p), rb_tree::key(x)) ?
                rb_tree::left(x) :
                rb_tree::right(x);
        }
        return rb_tree::insert_node_into(x, y, p);
    }

public:
    rb_tree() {
        header_init();
    }

    explicit rb_tree(const Compare& comp) : key_compare_(comp) {
        header_init();
    }

    rb_tree(const rb_tree& x) :
    key_compare_(x.key_compare_),
    extracter_(x.extracter_), size_pair_(x.size_pair_) {
        header_init();
        rb_tree::copy_from(x);
    }

    rb_tree& operator =(const rb_tree& x) {
        if (_MSTL addressof(x) == this) return *this;
        clear();
        rb_tree::copy_from(x);
        return *this;
    }

    rb_tree(rb_tree&& x) noexcept
    : header_(_MSTL move(x.header_)), key_compare_(_MSTL move(x.key_compare_)),
      extracter_(_MSTL move(x.extracter_)), size_pair_(_MSTL move(x.size_pair_)) {
        x.header_ = nullptr;
        x.size_pair_.value = 0;
    }

    rb_tree& operator =(rb_tree&& x) noexcept {
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

    MSTL_NODISCARD Compare key_comp() const
    noexcept(is_nothrow_copy_constructible_v<Compare>) {
        return key_compare_;
    }

    template <typename... Args>
    pair<iterator, bool> emplace_unique(Args&&... args) {
        const link_type tmp = rb_tree::create_node(_MSTL forward<Args>(args)...);
        return rb_tree::insert_unique_node(tmp);
    }

    pair<iterator, bool> insert_unique(const value_type& v) {
        return rb_tree::emplace_unique(v);
    }

    pair<iterator, bool> insert_unique(value_type&& v) {
        return rb_tree::emplace_unique(_MSTL move(v));
    }

    template <typename... Args>
    iterator emplace_unique_hint(iterator position, Args&&... args) {
        link_type tmp = rb_tree::create_node(_MSTL forward<Args>(args)...);
        if (position.node_ == header_->left_) {
            if (size() > 0 && key_compare_(rb_tree::key(tmp), rb_tree::key(position.node_))) {
                return rb_tree::insert_node_into(position.node_, position.node_, tmp);
            }
            return rb_tree::insert_unique_node(tmp).first;
        }

        if (position.node_ == header_) {
            if (key_compare_(rb_tree::key(rightmost()), rb_tree::key(tmp))) {
                return rb_tree::insert_node_into(nullptr, rightmost(), tmp);
            }
            return rb_tree::insert_unique_node(tmp).first;
        }

        iterator before = position;
        --before;
        if (key_compare_(rb_tree::key(before.node_), rb_tree::key(tmp)) &&
            key_compare_(rb_tree::key(tmp), rb_tree::key(position.node_))) {
            if (rb_tree::right(link_type(before.node_)) == nullptr) {
                return rb_tree::insert_node_into(nullptr, before.node_, tmp);
            }
            return rb_tree::insert_node_into(position.node_, position.node_, tmp);
        }

        return rb_tree::insert_unique_node(tmp).first;
    }

    iterator insert_unique(iterator position, const value_type& v) {
        return rb_tree::emplace_unique_hint(position, v);
    }

    iterator insert_unique(iterator position, value_type&& v) {
        return rb_tree::emplace_unique_hint(position, _MSTL move(v));
    }

    template <typename Iterator>
    enable_if_t<is_ranges_input_iter_v<Iterator>>
    insert_unique(Iterator first, Iterator last) {
        for (; first != last; ++first) {
            rb_tree::insert_unique(*first);
        }
        return;
    }

    template <typename... Args>
    iterator emplace_equal(Args&&... args) {
        const link_type tmp = rb_tree::create_node(_MSTL forward<Args>(args)...);
        return rb_tree::insert_equal_node(tmp);
    }

    iterator insert_equal(const value_type& v) {
        return rb_tree::emplace_equal(v);
    }

    iterator insert_equal(value_type&& v) {
        return rb_tree::emplace_equal(_MSTL move(v));
    }

    template <typename... Args>
    iterator emplace_equal_hint(iterator position, Args&&... args) {
        link_type tmp = rb_tree::create_node(_MSTL forward<Args>(args)...);

        if (position.node_ == header_->left_) {
            if (size() > 0 && key_compare_(rb_tree::key(tmp), rb_tree::key(position.node_))) {
                return rb_tree::insert_node_into(position.node_, position.node_, tmp);
            }
            return rb_tree::insert_equal_node(tmp);
        }

        if (position.node_ == header_) {
            if (!key_compare_(rb_tree::key(tmp), rb_tree::key(rightmost()))) {
                return rb_tree::insert_node_into(nullptr, rightmost(), tmp);
            }
            return rb_tree::insert_equal_node(tmp);
        }

        iterator before = position;
        --before;
        if (!key_compare_(rb_tree::key(tmp), rb_tree::key(before.node_)) &&
            !key_compare_(rb_tree::key(position.node_), rb_tree::key(tmp))) {
            if (rb_tree::right(link_type(before.node_)) == nullptr) {
                return rb_tree::insert_node_into(nullptr, before.node_, tmp);
            }
            return rb_tree::insert_node_into(position.node_, position.node_, tmp);
        }
        return rb_tree::insert_equal_node(tmp);
    }

    iterator insert_equal(iterator position, const value_type& v) {
        return rb_tree::emplace_equal_hint(position, v);
    }

    iterator insert_equal(iterator position, value_type&& v) {
        return rb_tree::emplace_equal_hint(position, _MSTL move(v));
    }

    template <typename Iterator>
    enable_if_t<is_ranges_input_iter_v<Iterator>>
    insert_equal(Iterator first, Iterator last) {
        for (; first != last; ++first) {
            rb_tree::insert_equal(*first);
        }
        return;
    }

    size_type erase(const key_type& k) noexcept {
        pair<iterator, iterator> p = rb_tree::equal_range(k);
        const size_type n = _MSTL distance(p.first, p.second);
        rb_tree::erase(p.first, p.second);
        return n;
    }
    void erase(iterator position) noexcept {
        auto y = reinterpret_cast<link_type>(_MSTL rb_tree_rebalance_for_erase(
            position.node_, header_->parent_, header_->left_, header_->right_));
        rb_tree::destroy_node(y);
        --size_pair_.value;
    }
    void erase(iterator first, iterator last) noexcept {
        if (first == begin() && last == end()) {
            clear();
        } else {
            while (first != last) {
                rb_tree::erase(first++);
            }
        }
    }

    void clear() noexcept {
        if (size_pair_.value == 0) return;
        rb_tree::erase_under_node(root());
        leftmost() = header_;
        root() = nullptr;
        rightmost() = header_;
        size_pair_.value = 0;
    }

    MSTL_NODISCARD iterator find(const key_type& k) {
        link_type y = header_;
        link_type x = root();

        while (x != nullptr) {
            if (!key_compare_(rb_tree::key(x), k)) {
                y = x;
                x = rb_tree::left(x);
            } else {
                x = rb_tree::right(x);
            }
        }

        iterator j(y, this);
        if (j == end()) {
            return end();
        }
        return key_compare_(k, rb_tree::key(y)) ? end() : j;
    }

    MSTL_NODISCARD const_iterator find(const key_type& k) const {
        link_type y = header_;
        link_type x = root();

        while (x != nullptr) {
            if (!key_compare_(rb_tree::key(x), k)) {
                y = x;
                x = rb_tree::left(x);
            } else {
                x = rb_tree::right(x);
            }
        }

        const_iterator j(y, this);
        if (j == cend()) {
            return cend();
        }
        return key_compare_(k, rb_tree::key(y)) ? cend() : j;
    }

    MSTL_NODISCARD size_type count(const key_type& k) const {
        pair<const_iterator, const_iterator> p = rb_tree::equal_range(k);
        const size_type n = _MSTL distance(p.first, p.second);
        return n;
    }

    MSTL_NODISCARD iterator lower_bound(const key_type& k) {
        link_type y = header_;
        link_type x = root();

        while (x != nullptr) {
            if (!key_compare_(rb_tree::key(x), k)) {
                y = x;
                x = rb_tree::left(x);
            } else {
                x = rb_tree::right(x);
            }
        }

        return iterator(y, this);
    }

    MSTL_NODISCARD const_iterator lower_bound(const key_type& k) const {
        link_type y = header_;
        link_type x = root();

        while (x != nullptr) {
            if (!key_compare_(rb_tree::key(x), k)) {
                y = x;
                x = rb_tree::left(x);
            } else {
                x = rb_tree::right(x);
            }
        }

        return const_iterator(y, this);
    }

    MSTL_NODISCARD iterator upper_bound(const key_type& k) {
        link_type y = header_;
        link_type x = root();

        while (x != nullptr) {
            if (key_compare_(k, rb_tree::key(x))) {
                y = x;
                x = rb_tree::left(x);
            } else {
                x = rb_tree::right(x);
            }
        }

        return iterator(y, this);
    }

    MSTL_NODISCARD const_iterator upper_bound(const key_type& k) const {
        link_type y = header_;
        link_type x = root();

        while (x != nullptr) {
            if (key_compare_(k, rb_tree::key(x))) {
                y = x;
                x = rb_tree::left(x);
            } else {
                x = rb_tree::right(x);
            }
        }

        return const_iterator(y, this);
    }

    MSTL_NODISCARD pair<iterator, iterator> equal_range(const key_type& k) {
        return pair<iterator, iterator>(rb_tree::lower_bound(k), rb_tree::upper_bound(k));
    }

    MSTL_NODISCARD pair<const_iterator, const_iterator> equal_range(const key_type& k) const {
        return pair<const_iterator, const_iterator>(rb_tree::lower_bound(k), rb_tree::upper_bound(k));
    }

    void swap(rb_tree& x)
    noexcept(
        is_nothrow_swappable_v<Compare> &&
        is_nothrow_swappable_v<KeyOfValue> &&
        noexcept(size_pair_.swap(x.size_pair_))) {
        _MSTL swap(header_, x.header_);
        _MSTL swap(size_pair_, x.size_pair_);
        _MSTL swap(key_compare_, x.key_compare_);
        _MSTL swap(extracter_, x.extracter_);
    }

    MSTL_NODISCARD bool operator ==(const rb_tree& rhs) const
    noexcept(noexcept(rb_tree::size() == rhs.size() && _MSTL equal(rb_tree::cbegin(), rb_tree::cend(), rhs.cbegin()))) {
        return rb_tree::size() == rhs.size() && _MSTL equal(rb_tree::cbegin(), rb_tree::cend(), rhs.cbegin());
    }

    MSTL_NODISCARD bool operator <(const rb_tree& rhs) const
    noexcept(noexcept(_MSTL lexicographical_compare(rb_tree::cbegin(), rb_tree::cend(), rhs.cbegin(), rhs.cend()))) {
        return _MSTL lexicographical_compare(rb_tree::cbegin(), rb_tree::cend(), rhs.cbegin(), rhs.cend());
    }
};

MSTL_END_NAMESPACE__
#endif // MSTL_CORE_CONTAINER_RB_TREE_HPP__
