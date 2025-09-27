#include <MSTL/core/rb_tree.hpp>
MSTL_BEGIN_NAMESPACE__
MSTL_BEGIN_INNER__

__rb_tree_node_base::base_ptr __rb_tree_node_base::minimum(base_ptr x) noexcept {
    while (x->left_ != nullptr) x = x->left_;
    return x;
}

__rb_tree_node_base::base_ptr __rb_tree_node_base::maximum(base_ptr x) noexcept {
    while (x->right_ != nullptr) x = x->right_;
    return x;
}

void rb_tree_rotate_left(
    __rb_tree_node_base* x, __rb_tree_node_base*& root) noexcept {
    __rb_tree_node_base* y = x->right_;
    x->right_ = y->left_;
    if (y->left_ != nullptr) y->left_->parent_ = x;
    y->parent_ = x->parent_;
    if (x == root) root = y;
    else if (x == x->parent_->left_) x->parent_->left_ = y;
    else x->parent_->right_ = y;
    y->left_ = x;
    x->parent_ = y;
}

void rb_tree_rotate_right(
    __rb_tree_node_base* x, __rb_tree_node_base*& root) noexcept {
    __rb_tree_node_base* y = x->left_;
    x->left_ = y->right_;
    if (y->right_ != nullptr) y->right_->parent_ = x;
    y->parent_ = x->parent_;
    if (x == root) root = y;
    else if (x == x->parent_->right_) x->parent_->right_ = y;
    else x->parent_->left_ = y;
    y->right_ = x;
    x->parent_ = y;
}

void rb_tree_rebalance(
    __rb_tree_node_base* x, __rb_tree_node_base*& root) noexcept {
    x->color_ = RB_TREE_RED;
    while (x != root && x->parent_->color_ == RB_TREE_RED) {
        if (x->parent_ == x->parent_->parent_->left_) {
            __rb_tree_node_base* y = x->parent_->parent_->right_;
            if (y != nullptr && y->color_ == RB_TREE_RED) {
                x->parent_->color_ = RB_TREE_BLACK;
                y->color_ = RB_TREE_BLACK;
                x->parent_->parent_->color_ = RB_TREE_RED;
                x = x->parent_->parent_;
            }
            else {
                if (x == x->parent_->right_) {
                    x = x->parent_;
                    rb_tree_rotate_left(x, root);
                }
                x->parent_->color_ = RB_TREE_BLACK;
                x->parent_->parent_->color_ = RB_TREE_RED;
                rb_tree_rotate_right(x->parent_->parent_, root);
            }
        }
        else {
            __rb_tree_node_base* y = x->parent_->parent_->left_;
            if (y != nullptr && y->color_ == RB_TREE_RED) {
                x->parent_->color_ = RB_TREE_BLACK;
                y->color_ = RB_TREE_BLACK;
                x->parent_->parent_->color_ = RB_TREE_RED;
                x = x->parent_->parent_;
            }
            else {
                if (x == x->parent_->left_) {
                    x = x->parent_;
                    rb_tree_rotate_right(x, root);
                }
                x->parent_->color_ = RB_TREE_BLACK;
                x->parent_->parent_->color_ = RB_TREE_RED;
                rb_tree_rotate_left(x->parent_->parent_, root);
            }
        }
    }
    root->color_ = RB_TREE_BLACK;
}

__rb_tree_node_base* rb_tree_rebalance_for_erase(
    __rb_tree_node_base* z, __rb_tree_node_base*& root,
    __rb_tree_node_base*& leftmost, __rb_tree_node_base*& rightmost) noexcept {
    __rb_tree_node_base* y = z;
    __rb_tree_node_base* x;
    __rb_tree_node_base* x_parent;
    if (y->left_ == nullptr) x = y->right_;
    else {
        if (y->right_ == nullptr) x = y->left_;
        else {
            y = y->right_;
            while (y->left_ != nullptr) y = y->left_;
            x = y->right_;
        }
    }
    if (y != z) {
        z->left_->parent_ = y;
        y->left_ = z->left_;
        if (y != z->right_) {
            x_parent = y->parent_;
            if (x != nullptr) x->parent_ = y->parent_;
            y->parent_->left_ = x;
            y->right_ = z->right_;
            z->right_->parent_ = y;
        }
        else
            x_parent = y;
        if (root == z)
            root = y;
        else if (z->parent_->left_ == z)
            z->parent_->left_ = y;
        else
            z->parent_->right_ = y;
        y->parent_ = z->parent_;
        _MSTL swap(y->color_, z->color_);
        y = z;
    }
    else {
        x_parent = y->parent_;
        if (x != nullptr) x->parent_ = y->parent_;
        if (root == z) root = x;
        else {
            if (z->parent_->left_ == z) z->parent_->left_ = x;
            else z->parent_->right_ = x;
        }
        if (leftmost == z) {
            if (z->right_ == nullptr) leftmost = z->parent_;
            else leftmost = __rb_tree_node_base::minimum(x);
        }
        if (rightmost == z) {
            if (z->left_ == nullptr) rightmost = z->parent_;
            else rightmost = __rb_tree_node_base::maximum(x);
        }
    }
    if (y->color_ != RB_TREE_RED) {
        while (x != root && (x == nullptr || x->color_ == RB_TREE_BLACK)) {
            if (x == x_parent->left_) {
                __rb_tree_node_base* w = x_parent->right_;
                if (!w) continue;
                if (w->color_ == RB_TREE_RED) {
                    w->color_ = RB_TREE_BLACK;
                    x_parent->color_ = RB_TREE_RED;
                    rb_tree_rotate_left(x_parent, root);
                    w = x_parent->right_;
                }
                if ((w->left_ == nullptr || w->left_->color_ == RB_TREE_BLACK) &&
                    (w->right_ == nullptr || w->right_->color_ == RB_TREE_BLACK)) {
                    w->color_ = RB_TREE_RED;
                    x = x_parent;
                    x_parent = x_parent->parent_;
                }
                else {
                    if (w->right_ == nullptr || w->right_->color_ == RB_TREE_BLACK) {
                        if (w->left_ != nullptr) w->left_->color_ = RB_TREE_BLACK;
                        w->color_ = RB_TREE_RED;
                        rb_tree_rotate_right(w, root);
                        w = x_parent->right_;
                    }
                    w->color_ = x_parent->color_;
                    x_parent->color_ = RB_TREE_BLACK;
                    if (w->right_ != nullptr) w->right_->color_ = RB_TREE_BLACK;
                    rb_tree_rotate_left(x_parent, root);
                    break;
                }
            }
            else {
                __rb_tree_node_base* w = x_parent->left_;
                if (w->color_ == RB_TREE_RED) {
                    w->color_ = RB_TREE_BLACK;
                    x_parent->color_ = RB_TREE_RED;
                    rb_tree_rotate_right(x_parent, root);
                    w = x_parent->left_;
                }
                if ((w->right_ == nullptr || w->right_->color_ == RB_TREE_BLACK) &&
                    (w->left_ == nullptr || w->left_->color_ == RB_TREE_BLACK)) {
                    w->color_ = RB_TREE_RED;
                    x = x_parent;
                    x_parent = x_parent->parent_;
                }
                else {
                    if (w->left_ == nullptr || w->left_->color_ == RB_TREE_BLACK) {
                        if (w->right_ != nullptr) w->right_->color_ = RB_TREE_BLACK;
                        w->color_ = RB_TREE_RED;
                        rb_tree_rotate_left(w, root);
                        w = x_parent->left_;
                    }
                    w->color_ = x_parent->color_;
                    x_parent->color_ = RB_TREE_BLACK;
                    if (w->left_ != nullptr) w->left_->color_ = RB_TREE_BLACK;
                    rb_tree_rotate_right(x_parent, root);
                    break;
                }
            }
        }
        if (x != nullptr) x->color_ = RB_TREE_BLACK;
    }
    return y;
}

void __rb_tree_base_iterator::increment() noexcept {
    if (node_->right_ != nullptr) {
        node_ = node_->right_;
        while (node_->left_ != nullptr) node_ = node_->left_;
    }
    else {
        base_ptr y = node_->parent_;
        while (node_ == y->right_) {
            node_ = y;
            y = y->parent_;
        }
        if (node_->right_ != y) node_ = y;
    }
}

void __rb_tree_base_iterator::decrement() noexcept {
    if (node_->color_ == RB_TREE_RED &&
        node_->parent_ != nullptr &&
        node_->parent_->parent_ == node_) {
        node_ = node_->right_;
        }
    else if (node_->left_ != nullptr) {
        base_ptr y = node_->left_;
        while (y->right_ != nullptr) y = y->right_;
        node_ = y;
    }
    else {
        base_ptr y = node_->parent_;
        while (node_ == y->left_) {
            node_ = y;
            y = y->parent_;
        }
        node_ = y;
    }
}

MSTL_NODISCARD bool __rb_tree_base_iterator::operator ==(
    const __rb_tree_base_iterator& rh) const noexcept {
    return node_ == rh.node_;
}
MSTL_NODISCARD bool __rb_tree_base_iterator::operator !=(
    const __rb_tree_base_iterator& rh) const noexcept {
    return node_ != rh.node_;
}

MSTL_END_INNER__
MSTL_END_NAMESPACE__
