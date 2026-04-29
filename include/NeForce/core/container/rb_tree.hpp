#ifndef NEFORCE_CORE_CONTAINER_RB_TREE_HPP__
#define NEFORCE_CORE_CONTAINER_RB_TREE_HPP__

/**
 * @file rb_tree.hpp
 * @brief 红黑树容器
 *
 * 此文件提供了红黑树容器的实现。
 * 红黑树是一种自平衡二叉搜索树，保证了插入、删除和查找操作的对数时间复杂度。
 * 作为关联式容器的底层实现。
 */

#include "NeForce/core/algorithm/compare.hpp"
#include "NeForce/core/interface/icollector.hpp"
#include "NeForce/core/interface/iiterator.hpp"
#include "NeForce/core/memory/construct.hpp"
#include "NeForce/core/memory/standard_allocator.hpp"
#include "NeForce/core/utility/compressed_pair.hpp"
#include "NeForce/core/utility/pair.hpp"
NEFORCE_BEGIN_NAMESPACE__

/**
 * @defgroup RBTree 红黑树
 * @brief 自平衡二叉搜索树实现
 *
 * @section standards 遵循的国际标准与文献参考
 * 本实现严格遵循以下计算机科学文献：
 *
 * - **Rudolf Bayer (1972)**："Symmetric Binary B-Trees: Data Structure and Maintenance Algorithms"
 *   Acta Informatica, Vol. 1, pp. 290–306.
 *   https://doi.org/10.1007/BF00289509
 * - **Leo J. Guibas & Robert Sedgewick (1978)**："A Dichromatic Framework for Balanced Trees"
 *   Proceedings of the 19th Annual Symposium on Foundations of Computer Science, pp. 8–21.
 *   https://doi.org/10.1109/SFCS.1978.3
 *
 * @section rb_tree_properties 红黑树性质
 * 根据 Guibas & Sedgewick (1978) 的定义，红黑树满足以下五条性质：
 *
 * 1. **节点颜色**：每个节点要么是红色，要么是黑色。
 * 2. **根节点颜色**：根节点是黑色的。
 * 3. **叶子节点颜色**：每个叶子节点（NIL）是黑色的。
 * 4. **红色节点限制**：如果一个节点是红色的，则它的两个子节点都是黑色的。
 * 5. **黑色高度**：对于每个节点，从该节点到其所有后代叶子节点的简单路径上，
 *    包含相同数量的黑色节点。
 *
 * @section complexity_guarantees 复杂度保证
 * 根据红黑树性质，最坏情况下的时间复杂度保证：
 *
 * | 操作               | 时间复杂度 | 说明                               |
 * |--------------------|------------|------------------------------------|
 * | 插入               | O(log n)   | 最多 2 次旋转                      |
 * | 删除               | O(log n)   | 最多 3 次旋转                      |
 * | 查找               | O(log n)   | 二叉搜索树查找                     |
 * | 最小/最大          | O(log n)   | 沿最左/最右路径查找                |
 * | 前驱/后继          | O(log n)   | 中序遍历相邻节点                    |
 * | 范围迭代           | O(k)       | k 为范围内元素数量                  |
 *
 * 树的高度上界：`h ≤ 2 log₂(n + 1)`。
 *
 * @note 本实现提供了两个插入策略：
 *       - `insert_unique`：键必须唯一，重复键插入失败并返回已存在元素迭代器
 *       - `insert_equal`：允许重复键，按插入顺序存储
 *
 * @warning 根据 ISO/IEC 14882:2020 §22.2.6：
 *          - 对关联容器的修改操作不会使任何迭代器失效，除非删除该迭代器指向的元素
 *          - 比较函数对象（Compare）必须提供严格的弱序关系
 *          - 键的不可变性：修改已插入元素的键会导致未定义行为
 *
 * @see https://en.cppreference.com/w/cpp/container/map
 * @see https://en.cppreference.com/w/cpp/container/set
 * @see https://doi.org/10.1109/SFCS.1978.3 (Guibas & Sedgewick)
 * @{
 */

/// 红黑树节点颜色常量：红色
NEFORCE_INLINE17 constexpr bool RB_TREE_RED = false;

/// 红黑树节点颜色常量：黑色
NEFORCE_INLINE17 constexpr bool RB_TREE_BLACK = true;


/**
 * @struct rb_tree_node_base
 * @brief 红黑树节点基类
 *
 * 包含红黑树节点所需的指针和颜色信息，不包含具体数据。
 * 提供最小值和最大值的静态辅助函数。
 */
struct rb_tree_node_base {
    using color_type = bool;             ///< 颜色类型
    using base_ptr = rb_tree_node_base*; ///< 基类指针类型

    color_type color_ = RB_TREE_RED; ///< 节点颜色，默认为红色
    base_ptr parent_ = nullptr;      ///< 父节点指针
    base_ptr left_ = nullptr;        ///< 左子节点指针
    base_ptr right_ = nullptr;       ///< 右子节点指针

    /**
     * @brief 获取子树中的最小节点
     * @param root 子树根节点
     * @return 最小节点指针
     */
    static base_ptr minimum(base_ptr root) noexcept {
        while (root->left_ != nullptr) {
            root = root->left_;
        }
        return root;
    }

    /**
     * @brief 获取子树中的最大节点
     * @param root 子树根节点
     * @return 最大节点指针
     */
    static base_ptr maximum(base_ptr root) noexcept {
        while (root->right_ != nullptr) {
            root = root->right_;
        }
        return root;
    }
};


/**
 * @brief 红黑树左旋转
 * @param axis 旋转轴节点
 * @param root 树根节点引用
 *
 * 执行左旋转操作，保持红黑树性质。
 */
NEFORCE_ALWAYS_INLINE_INLINE void rb_tree_rotate_left(rb_tree_node_base* axis, rb_tree_node_base*& root) noexcept {
    rb_tree_node_base* y = axis->right_;
    axis->right_ = y->left_;
    if (y->left_ != nullptr) {
        y->left_->parent_ = axis;
    }

    y->parent_ = axis->parent_;

    if (axis == root) {
        root = y;
    } else if (axis == axis->parent_->left_) {
        axis->parent_->left_ = y;
    } else {
        axis->parent_->right_ = y;
    }

    y->left_ = axis;
    axis->parent_ = y;
}

/**
 * @brief 红黑树右旋转
 * @param axis 旋转轴节点
 * @param root 树根节点引用
 *
 * 执行右旋转操作，保持红黑树性质。
 */
NEFORCE_ALWAYS_INLINE_INLINE void rb_tree_rotate_right(rb_tree_node_base* axis, rb_tree_node_base*& root) noexcept {
    rb_tree_node_base* y = axis->left_;
    axis->left_ = y->right_;
    if (y->right_ != nullptr) {
        y->right_->parent_ = axis;
    }
    y->parent_ = axis->parent_;

    if (axis == root) {
        root = y;
    } else if (axis == axis->parent_->right_) {
        axis->parent_->right_ = y;
    } else {
        axis->parent_->left_ = y;
    }

    y->right_ = axis;
    axis->parent_ = y;
}

/**
 * @brief 插入节点后重新平衡红黑树
 * @param insert 新插入的节点
 * @param root 树根节点引用
 *
 * 通过旋转和重新着色恢复红黑树的平衡性质。
 */
NEFORCE_ALWAYS_INLINE_INLINE void rb_tree_insert_rebalance(rb_tree_node_base* insert,
                                                           rb_tree_node_base*& root) noexcept {
    insert->color_ = RB_TREE_RED;

    while (insert != root && insert->parent_->color_ == RB_TREE_RED) {
        if (insert->parent_ == insert->parent_->parent_->left_) {
            rb_tree_node_base* y = insert->parent_->parent_->right_;

            if (y != nullptr && y->color_ == RB_TREE_RED) {
                insert->parent_->color_ = RB_TREE_BLACK;
                y->color_ = RB_TREE_BLACK;
                insert->parent_->parent_->color_ = RB_TREE_RED;
                insert = insert->parent_->parent_;
            } else {
                if (insert == insert->parent_->right_) {
                    insert = insert->parent_;
                    _NEFORCE rb_tree_rotate_left(insert, root);
                }
                insert->parent_->color_ = RB_TREE_BLACK;
                insert->parent_->parent_->color_ = RB_TREE_RED;
                _NEFORCE rb_tree_rotate_right(insert->parent_->parent_, root);
            }
        } else {
            rb_tree_node_base* y = insert->parent_->parent_->left_;
            if (y != nullptr && y->color_ == RB_TREE_RED) {
                insert->parent_->color_ = RB_TREE_BLACK;
                y->color_ = RB_TREE_BLACK;
                insert->parent_->parent_->color_ = RB_TREE_RED;
                insert = insert->parent_->parent_;
            } else {
                if (insert == insert->parent_->left_) {
                    insert = insert->parent_;
                    _NEFORCE rb_tree_rotate_right(insert, root);
                }
                insert->parent_->color_ = RB_TREE_BLACK;
                insert->parent_->parent_->color_ = RB_TREE_RED;
                _NEFORCE rb_tree_rotate_left(insert->parent_->parent_, root);
            }
        }
    }
    root->color_ = RB_TREE_BLACK;
}

/**
 * @brief 删除节点后重新平衡红黑树
 * @param erase 要删除的节点
 * @param root 树根节点引用
 * @param leftmost 最左节点引用
 * @param rightmost 最右节点引用
 * @return 实际被删除的节点
 *
 * 执行节点删除后的重新平衡操作，并更新最左最右节点。
 */
NEFORCE_ALWAYS_INLINE_INLINE rb_tree_node_base* rb_tree_erase_rebalance(rb_tree_node_base* erase,
                                                                        rb_tree_node_base*& root,
                                                                        rb_tree_node_base*& leftmost,
                                                                        rb_tree_node_base*& rightmost) noexcept {
    rb_tree_node_base* y = erase;
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

    if (y != erase) {
        erase->left_->parent_ = y;
        y->left_ = erase->left_;

        if (y != erase->right_) {
            x_parent = y->parent_;
            if (x != nullptr) {
                x->parent_ = y->parent_;
            }

            y->parent_->left_ = x;
            y->right_ = erase->right_;
            erase->right_->parent_ = y;
        } else {
            x_parent = y;
        }

        if (root == erase) {
            root = y;
        } else if (erase->parent_->left_ == erase) {
            erase->parent_->left_ = y;
        } else {
            erase->parent_->right_ = y;
        }

        y->parent_ = erase->parent_;
        _NEFORCE swap(y->color_, erase->color_);
        y = erase;
    } else {
        x_parent = y->parent_;
        if (x != nullptr) {
            x->parent_ = y->parent_;
        }

        if (root == erase) {
            root = x;
        } else {
            if (erase->parent_->left_ == erase) {
                erase->parent_->left_ = x;
            } else {
                erase->parent_->right_ = x;
            }
        }

        if (leftmost == erase) {
            if (erase->right_ == nullptr) {
                leftmost = erase->parent_;
            } else {
                leftmost = rb_tree_node_base::minimum(x);
            }
        }

        if (rightmost == erase) {
            if (erase->left_ == nullptr) {
                rightmost = erase->parent_;
            } else {
                rightmost = rb_tree_node_base::maximum(x);
            }
        }
    }

    if (y->color_ != RB_TREE_RED) {
        while (x != root && (x == nullptr || x->color_ == RB_TREE_BLACK)) {
            if (x == x_parent->left_) {
                rb_tree_node_base* w = x_parent->right_;
                NEFORCE_DEBUG_VERIFY(w != nullptr, "RB-tree structure corrupted: right sibling is null");
                if (w == nullptr) {
                    x = x_parent;
                    x_parent = x_parent->parent_;
                    continue;
                }

                if (w->color_ == RB_TREE_RED) {
                    w->color_ = RB_TREE_BLACK;
                    x_parent->color_ = RB_TREE_RED;
                    _NEFORCE rb_tree_rotate_left(x_parent, root);
                    w = x_parent->right_;
                }

                if ((w->left_ == nullptr || w->left_->color_ == RB_TREE_BLACK) &&
                    (w->right_ == nullptr || w->right_->color_ == RB_TREE_BLACK)) {
                    w->color_ = RB_TREE_RED;
                    x = x_parent;
                    x_parent = x_parent->parent_;
                } else {
                    if (w->right_ == nullptr || w->right_->color_ == RB_TREE_BLACK) {
                        if (w->left_ != nullptr) {
                            w->left_->color_ = RB_TREE_BLACK;
                        }
                        w->color_ = RB_TREE_RED;
                        _NEFORCE rb_tree_rotate_right(w, root);
                        w = x_parent->right_;
                    }
                    w->color_ = x_parent->color_;
                    x_parent->color_ = RB_TREE_BLACK;
                    if (w->right_ != nullptr) {
                        w->right_->color_ = RB_TREE_BLACK;
                    }
                    _NEFORCE rb_tree_rotate_left(x_parent, root);
                    break;
                }
            } else {
                rb_tree_node_base* w = x_parent->left_;
                NEFORCE_DEBUG_VERIFY(w != nullptr, "RB-tree structure corrupted: left sibling is null");
                if (w == nullptr) {
                    x = x_parent;
                    x_parent = x_parent->parent_;
                    continue;
                }

                if (w->color_ == RB_TREE_RED) {
                    w->color_ = RB_TREE_BLACK;
                    x_parent->color_ = RB_TREE_RED;
                    _NEFORCE rb_tree_rotate_right(x_parent, root);
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
                        _NEFORCE rb_tree_rotate_left(w, root);
                        w = x_parent->left_;
                    }
                    w->color_ = x_parent->color_;
                    x_parent->color_ = RB_TREE_BLACK;
                    if (w->left_ != nullptr) {
                        w->left_->color_ = RB_TREE_BLACK;
                    }

                    _NEFORCE rb_tree_rotate_right(x_parent, root);
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


/**
 * @struct rb_tree_node
 * @brief 红黑树数据节点
 * @tparam T 数据类型
 *
 * 继承自节点基类，添加数据成员。
 */
template <typename T>
struct rb_tree_node : rb_tree_node_base {
    T data; ///< 节点存储的数据

    /**
     * @brief 默认构造函数
     */
    rb_tree_node() noexcept(is_nothrow_default_constructible_v<T>) :
    data() {}
};


/**
 * @struct rb_tree_base_iterator
 * @brief 红黑树迭代器基类
 *
 * 提供递增和递减操作的基本实现。
 */
struct rb_tree_base_iterator {
public:
    using iterator_category = bidirectional_iterator_tag; ///< 双向迭代器

protected:
    using base_ptr = rb_tree_node_base::base_ptr; ///< 基类指针类型

    base_ptr node_ = nullptr; ///< 当前节点指针

    /**
     * @brief 递增操作（移动到中序遍历的后继节点）
     */
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

    /**
     * @brief 递减操作（移动到中序遍历的前驱节点）
     */
    void decrement() noexcept {
        if (node_->color_ == RB_TREE_RED && node_->parent_ != nullptr && node_->parent_->parent_ == node_) {
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


/**
 * @struct rb_tree_iterator
 * @brief 红黑树迭代器
 * @tparam IsConst 是否常量迭代器
 * @tparam RbTree 红黑树类型
 *
 * 提供对红黑树元素的迭代访问。
 */
template <bool IsConst, typename RbTree>
struct rb_tree_iterator : rb_tree_base_iterator, iiterator<rb_tree_iterator<IsConst, RbTree>> {
public:
    using container_type = RbTree;                                      ///< 容器类型
    using value_type = typename container_type::value_type;             ///< 值类型
    using size_type = typename container_type::size_type;               ///< 大小类型
    using difference_type = typename container_type::difference_type;   ///< 差值类型
    using iterator_category = rb_tree_base_iterator::iterator_category; ///< 迭代器类别（双向）
    using reference = conditional_t<IsConst, typename container_type::const_reference,
                                    typename container_type::reference>; ///< 引用类型
    using pointer = conditional_t<IsConst, typename container_type::const_pointer,
                                  typename container_type::pointer>; ///< 指针类型

private:
    using base_type = rb_tree_base_iterator;    ///< 基类类型
    using node_type = rb_tree_node<value_type>; ///< 节点类型
    using link_type = node_type*;               ///< 节点指针类型

    const container_type* container_ = nullptr; ///< 关联容器指针

    template <typename, typename, typename, typename, typename>
    friend class rb_tree;

public:
    rb_tree_iterator() noexcept = default;
    ~rb_tree_iterator() = default;

    rb_tree_iterator(const rb_tree_iterator&) noexcept = default;
    rb_tree_iterator& operator=(const rb_tree_iterator&) noexcept = default;
    rb_tree_iterator(rb_tree_iterator&&) noexcept = default;
    rb_tree_iterator& operator=(rb_tree_iterator&&) noexcept = default;

    /**
     * @brief 构造函数
     * @param ptr 节点指针
     * @param tree 容器指针
     */
    rb_tree_iterator(node_type* ptr, const container_type* tree) noexcept :
    container_(tree) {
        node_ = ptr;
    }

    /**
     * @brief 解引用操作
     * @return 当前元素的引用
     */
    NEFORCE_NODISCARD reference dereference() const noexcept {
        NEFORCE_DEBUG_VERIFY(node_ && container_, "Attempting to dereference on a null pointer");
        link_type link = link_type(node_);
        NEFORCE_DEBUG_VERIFY(node_ != container_->header_ && node_->parent_ != nullptr,
                             "Attempting to dereference out of boundary");
        return link->data;
    }

    /**
     * @brief 递增操作
     */
    NEFORCE_CONSTEXPR20 void increment() noexcept {
        NEFORCE_DEBUG_VERIFY(node_ && container_, "Attempting to increment a null pointer");
        NEFORCE_DEBUG_VERIFY(link_type(node_) != container_->header_, "Attempting to increment out of boundary");
        base_type::increment();
    }

    /**
     * @brief 递减操作
     */
    NEFORCE_CONSTEXPR20 void decrement() noexcept {
        NEFORCE_DEBUG_VERIFY(node_ && container_, "Attempting to decrement a null pointer");
        NEFORCE_DEBUG_VERIFY(node_ != container_->header_, "Attempting to decrement out of boundary");
        base_type::decrement();
    }

    /**
     * @brief 相等比较
     * @param rhs 右侧迭代器
     * @return 是否相等
     */
    NEFORCE_NODISCARD bool equal(const rb_tree_iterator& rhs) const noexcept {
        NEFORCE_DEBUG_VERIFY(container_ == rhs.container_, "Attempting to equal to a different container");
        return node_ == rhs.node_;
    }

    /**
     * @brief 获取底层指针
     * @return 当前节点指针
     */
    NEFORCE_NODISCARD pointer base() const noexcept { return node_; }

    /**
     * @brief 获取关联容器
     * @return 关联容器指针
     */
    NEFORCE_NODISCARD const container_type* container() const noexcept { return container_; }
};


/**
 * @class rb_tree
 * @brief 红黑树容器
 * @tparam Key 键类型
 * @tparam Value 值类型
 * @tparam KeyOfValue 从值中提取键的函数对象
 * @tparam Compare 键比较函数对象
 * @tparam Alloc 分配器类型
 *
 * 红黑树是一种自平衡二叉搜索树，作为关联式容器的底层实现。
 */
template <typename Key, typename Value, typename KeyOfValue, typename Compare,
          typename Alloc = allocator<rb_tree_node<Value>>>
class rb_tree : icollector<rb_tree<Key, Value, KeyOfValue, Compare, Alloc>> {
    static_assert(is_allocator_v<Alloc>, "Alloc type is not a standard allocator type.");
    static_assert(is_same_v<rb_tree_node<Value>, typename Alloc::value_type>, "allocator type mismatch.");
    static_assert(is_object_v<Value>, "list only contains object types.");

public:
    using key_type = Key;    ///< 键类型
    using color_type = bool; ///< 颜色类型

    using value_type = Value;                                                 ///< 值类型
    using pointer = Value*;                                                   ///< 指针类型
    using reference = Value&;                                                 ///< 引用类型
    using const_pointer = const Value*;                                       ///< 常量指针类型
    using const_reference = const Value&;                                     ///< 常量引用类型
    using size_type = size_t;                                                 ///< 大小类型
    using difference_type = ptrdiff_t;                                        ///< 差值类型
    using iterator = rb_tree_iterator<false, rb_tree>;                        ///< 迭代器类型
    using const_iterator = rb_tree_iterator<true, rb_tree>;                   ///< 常量迭代器类型
    using reverse_iterator = _NEFORCE reverse_iterator<iterator>;             ///< 反向迭代器类型
    using const_reverse_iterator = _NEFORCE reverse_iterator<const_iterator>; ///< 常量反向迭代器类型
    using allocator_type = Alloc;                                             ///< 分配器类型

private:
    using base_node = rb_tree_node_base;   ///< 节点基类类型
    using link_node = rb_tree_node<Value>; ///< 数据节点类型
    using base_ptr = base_node*;           ///< 基类指针类型
    using link_type = link_node*;          ///< 数据节点指针类型

    link_type header_ = nullptr;                                                    ///< 头节点
    Compare key_compare_{};                                                         ///< 键比较函数对象
    KeyOfValue extracter_{};                                                        ///< 值提取键函数对象
    compressed_pair<allocator_type, size_t> size_pair_{default_construct_tag{}, 0}; ///< 压缩存储分配器和大小

    template <bool, typename>
    friend struct rb_tree_iterator;

private:
    struct node_guard {
    private:
        rb_tree* tree_;
        link_type node_;
        bool released_ = false;

    public:
        node_guard(rb_tree* t, link_type n) noexcept :
        tree_(t),
        node_(n) {}

        ~node_guard() {
            if (!released_) {
                tree_->destroy_node(node_);
            }
        }

        link_type release() noexcept {
            released_ = true;
            return node_;
        }
    };

    /**
     * @brief 获取根节点
     * @return 根节点引用
     */
    link_type& root() const noexcept { return reinterpret_cast<link_type&>(header_->parent_); }

    /**
     * @brief 获取最左节点
     * @return 最左节点引用
     */
    link_type& leftmost() const noexcept { return reinterpret_cast<link_type&>(header_->left_); }

    /**
     * @brief 获取最右节点
     * @return 最右节点引用
     */
    link_type& rightmost() const noexcept { return reinterpret_cast<link_type&>(header_->right_); }

    /**
     * @brief 获取节点的左子节点
     * @param ptr 节点指针
     * @return 左子节点引用
     */
    static link_type& left(link_type ptr) noexcept { return reinterpret_cast<link_type&>(ptr->left_); }

    /**
     * @brief 获取节点的右子节点
     * @param ptr 节点指针
     * @return 右子节点引用
     */
    static link_type& right(link_type ptr) noexcept { return reinterpret_cast<link_type&>(ptr->right_); }

    /**
     * @brief 获取节点的父节点
     * @param ptr 节点指针
     * @return 父节点引用
     */
    static link_type& parent(link_type ptr) noexcept { return reinterpret_cast<link_type&>(ptr->parent_); }

    /**
     * @brief 从节点提取键
     * @param ptr 节点指针
     * @return 节点的键
     */
    static const Key& key(link_type ptr) noexcept { return KeyOfValue()(ptr->data); }

    /**
     * @brief 从基类指针节点提取键
     * @param ptr 基类指针
     * @return 节点的键
     */
    static const Key& key(base_ptr ptr) noexcept { return rb_tree::key(reinterpret_cast<link_type>(ptr)); }

    /**
     * @brief 获取子树的最小节点
     * @param ptr 子树根节点
     * @return 最小节点指针
     */
    static link_type minimum(link_type ptr) noexcept { return static_cast<link_type>(base_node::minimum(ptr)); }

    /**
     * @brief 获取子树的最大节点
     * @param ptr 子树根节点
     * @return 最大节点指针
     */
    static link_type maximum(link_type ptr) noexcept { return static_cast<link_type>(base_node::maximum(ptr)); }

    /**
     * @brief 创建节点
     * @tparam Args 构造参数类型
     * @param args 构造参数
     * @return 新创建的节点指针
     */
    template <typename... Args>
    link_type create_node(Args&&... args) {
        link_type tmp = size_pair_.get_base().allocate();
        try {
            _NEFORCE construct(&tmp->data, _NEFORCE forward<Args>(args)...);
        } catch (...) {
            rb_tree::destroy_node(tmp);
            throw;
        }
        return tmp;
    }

    /**
     * @brief 拷贝节点
     * @param ptr 源节点指针
     * @return 新拷贝的节点指针
     */
    link_type copy_node(link_type ptr) {
        link_type tmp = rb_tree::create_node(ptr->data);
        tmp->color_ = ptr->color_;
        tmp->left_ = nullptr;
        tmp->right_ = nullptr;
        return tmp;
    }

    /**
     * @brief 销毁节点
     * @param ptr 要销毁的节点指针
     */
    void destroy_node(link_type ptr) noexcept(is_nothrow_destructible_v<link_node>) {
        if (ptr == nullptr) {
            return;
        }
        _NEFORCE destroy(&ptr->data);
        size_pair_.get_base().deallocate(ptr);
    }

    /**
     * @brief 将节点插入到指定位置
     * @param child 插入位置的子节点
     * @param parent 插入位置的父节点
     * @param ptr 要插入的节点
     * @return 指向插入位置的迭代器
     */
    iterator insert_into(base_ptr child, base_ptr parent, link_type ptr) {
        auto x = static_cast<link_type>(child);
        auto y = static_cast<link_type>(parent);
        if (y == header_ || x != nullptr || key_compare_(rb_tree::key(ptr), rb_tree::key(y))) {
            rb_tree::left(y) = ptr;
            if (y == header_) {
                root() = ptr;
                leftmost() = ptr;
                rightmost() = ptr;
            } else if (y == leftmost()) {
                leftmost() = ptr;
            }
        } else {
            rb_tree::right(y) = ptr;
            if (y == rightmost()) {
                rightmost() = ptr;
            }
        }
        rb_tree::parent(ptr) = y;
        rb_tree::left(ptr) = nullptr;
        rb_tree::right(ptr) = nullptr;
        _NEFORCE rb_tree_insert_rebalance(ptr, header_->parent_);
        ++size_pair_.value;
        return iterator(ptr, this);
    }

    /**
     * @brief 递归拷贝子树
     * @param src_root 源子树根节点
     * @param parent 父节点指针
     * @return 拷贝得到的子树根节点
     */
    link_type copy_under(link_type src_root, link_type parent) {
        link_type top = rb_tree::copy_node(src_root);
        top->parent_ = parent;
        try {
            if (src_root->right_ != nullptr) {
                top->right_ = rb_tree::copy_under(rb_tree::right(src_root), top);
            }
            parent = top;
            src_root = rb_tree::left(src_root);
            while (src_root != nullptr) {
                link_type y = rb_tree::copy_node(src_root);
                parent->left_ = y;
                y->parent_ = parent;
                if (src_root->right_ != nullptr) {
                    y->right_ = rb_tree::copy_under(rb_tree::right(src_root), y);
                }
                parent = y;
                src_root = rb_tree::left(src_root);
            }
        } catch (...) {
            rb_tree::erase_under(top);
            throw;
        }
        return top;
    }

    /**
     * @brief 递归销毁子树
     * @param root 子树根节点
     */
    void erase_under(link_type root) noexcept(is_nothrow_destructible_v<link_node>) {
        if (root == nullptr) {
            return;
        }
        rb_tree::erase_under(rb_tree::right(root));
        rb_tree::erase_under(rb_tree::left(root));
        rb_tree::destroy_node(root);
    }

    /**
     * @brief 初始化头节点
     */
    void header_init() {
        header_ = size_pair_.get_base().allocate(1);
        header_->color_ = RB_TREE_RED;
        root() = nullptr;
        leftmost() = header_;
        rightmost() = header_;
    }

    /**
     * @brief 从另一个红黑树拷贝
     * @param other 源红黑树
     */
    void copy_from(const rb_tree& other) {
        if (other.root() == nullptr) {
            root() = nullptr;
            leftmost() = header_;
            rightmost() = header_;
        } else {
            try {
                root() = rb_tree::copy_under(other.root(), header_);
            } catch (...) {
                size_pair_.get_base().deallocate(header_);
                header_ = nullptr;
                throw;
            }
            leftmost() = rb_tree::minimum(root());
            rightmost() = rb_tree::maximum(root());
        }
        size_pair_.value = other.size_pair_.value;
    }

    /**
     * @brief 插入唯一键节点
     * @param node 要插入的节点
     * @return 插入结果（迭代器和是否成功）
     */
    pair<iterator, bool> insert_unique(link_type node) {
        link_type y = header_;
        link_type x = root();
        bool comp = true;
        while (x != nullptr) {
            y = x;
            comp = key_compare_(rb_tree::key(node), rb_tree::key(x));
            x = comp ? rb_tree::left(x) : rb_tree::right(x);
        }

        iterator j(y, this);
        if (comp) {
            if (j == begin()) {
                return pair<iterator, bool>(rb_tree::insert_into(x, y, node), true);
            }
            --j;
        }

        if (key_compare_(rb_tree::key(link_type(j.node_)), rb_tree::key(node))) {
            return pair<iterator, bool>(rb_tree::insert_into(x, y, node), true);
        }
        rb_tree::destroy_node(node);
        return pair<iterator, bool>(j, false);
    }

    /**
     * @brief 插入允许重复键的节点
     * @param node 要插入的节点
     * @return 指向插入位置的迭代器
     */
    iterator insert_equal(link_type node) {
        link_type y = header_;
        link_type x = root();
        while (x != nullptr) {
            y = x;
            x = key_compare_(rb_tree::key(node), rb_tree::key(x)) ? rb_tree::left(x) : rb_tree::right(x);
        }
        return rb_tree::insert_into(x, y, node);
    }

public:
    /**
     * @brief 默认构造函数
     */
    rb_tree() { header_init(); }

    /**
     * @brief 构造函数，指定比较函数
     * @param comp 比较函数对象
     */
    explicit rb_tree(const Compare& comp) :
    key_compare_(comp) {
        header_init();
    }

    /**
     * @brief 拷贝构造函数
     * @param other 源红黑树
     */
    rb_tree(const rb_tree& other) :
    key_compare_(other.key_compare_),
    extracter_(other.extracter_),
    size_pair_(other.size_pair_) {
        header_init();
        rb_tree::copy_from(other);
    }

    /**
     * @brief 拷贝赋值运算符
     * @param other 源红黑树
     * @return 自身引用
     */
    rb_tree& operator=(const rb_tree& other) {
        if (_NEFORCE addressof(other) == this) {
            return *this;
        }
        rb_tree tmp(other);
        rb_tree::swap(tmp);
        return *this;
    }

    /**
     * @brief 移动构造函数
     * @param other 源红黑树
     */
    rb_tree(rb_tree&& other) noexcept(is_nothrow_move_constructible_v<Compare> &&
                                      is_nothrow_move_constructible_v<KeyOfValue> &&
                                      is_nothrow_move_constructible_v<allocator_type>) :
    header_(_NEFORCE move(other.header_)),
    key_compare_(_NEFORCE move(other.key_compare_)),
    extracter_(_NEFORCE move(other.extracter_)),
    size_pair_(_NEFORCE move(other.size_pair_)) {
        other.header_ = nullptr;
        other.size_pair_.value = 0;
    }

    /**
     * @brief 移动赋值运算符
     * @param other 源红黑树
     * @return 自身引用
     */
    rb_tree& operator=(rb_tree&& other) noexcept(is_nothrow_move_constructible_v<rb_tree>) {
        if (_NEFORCE addressof(other) == this) {
            return *this;
        }
        rb_tree tmp(_NEFORCE move(other));
        rb_tree::swap(tmp);
        return *this;
    }

    /**
     * @brief 析构函数
     */
    ~rb_tree() {
        clear();
        if (header_) {
            size_pair_.get_base().deallocate(header_);
        }
    }

    /**
     * @brief 获取起始迭代器
     * @return 指向最小元素的迭代器
     */
    NEFORCE_NODISCARD iterator begin() noexcept { return {leftmost(), this}; }

    /**
     * @brief 获取结束迭代器
     * @return 指向头节点的迭代器
     */
    NEFORCE_NODISCARD iterator end() noexcept { return {header_, this}; }

    /**
     * @brief 获取常量起始迭代器
     * @return 指向最小元素的常量迭代器
     */
    NEFORCE_NODISCARD const_iterator begin() const noexcept { return cbegin(); }

    /**
     * @brief 获取常量结束迭代器
     * @return 指向头节点的常量迭代器
     */
    NEFORCE_NODISCARD const_iterator end() const noexcept { return cend(); }

    /**
     * @brief 获取常量起始迭代器
     * @return 指向最小元素的常量迭代器
     */
    NEFORCE_NODISCARD const_iterator cbegin() const noexcept { return {leftmost(), this}; }

    /**
     * @brief 获取常量结束迭代器
     * @return 指向头节点的常量迭代器
     */
    NEFORCE_NODISCARD const_iterator cend() const noexcept { return {header_, this}; }

    /**
     * @brief 获取反向起始迭代器
     * @return 指向最大元素的反向迭代器
     */
    NEFORCE_NODISCARD reverse_iterator rbegin() noexcept { return reverse_iterator(end()); }

    /**
     * @brief 获取反向结束迭代器
     * @return 指向最小元素之前位置的反向迭代器
     */
    NEFORCE_NODISCARD reverse_iterator rend() noexcept { return reverse_iterator(begin()); }

    /**
     * @brief 获取常量反向起始迭代器
     * @return 指向最大元素的常量反向迭代器
     */
    NEFORCE_NODISCARD const_reverse_iterator rbegin() const noexcept { return crbegin(); }

    /**
     * @brief 获取常量反向结束迭代器
     * @return 指向最小元素之前位置的常量反向迭代器
     */
    NEFORCE_NODISCARD const_reverse_iterator rend() const noexcept { return crend(); }

    /**
     * @brief 获取常量反向起始迭代器
     * @return 指向最大元素的常量反向迭代器
     */
    NEFORCE_NODISCARD const_reverse_iterator crbegin() const noexcept { return const_reverse_iterator(cend()); }

    /**
     * @brief 获取常量反向结束迭代器
     * @return 指向最小元素之前位置的常量反向迭代器
     */
    NEFORCE_NODISCARD const_reverse_iterator crend() const noexcept { return const_reverse_iterator(cbegin()); }

    /**
     * @brief 获取元素数量
     * @return 树中元素数量
     */
    NEFORCE_NODISCARD size_type size() const noexcept { return size_pair_.value; }

    /**
     * @brief 获取最大可能大小
     * @return 最大元素数量
     */
    NEFORCE_NODISCARD size_type max_size() const noexcept { return static_cast<size_type>(-1); }

    /**
     * @brief 检查是否为空
     * @return 是否为空
     */
    NEFORCE_NODISCARD bool empty() const noexcept { return size_pair_.value == 0; }

    /**
     * @brief 获取键比较函数对象
     * @return 键比较函数对象的副本
     */
    NEFORCE_NODISCARD Compare key_compare() const noexcept(is_nothrow_copy_constructible_v<Compare>) {
        return key_compare_;
    }

    /**
     * @brief 在树中构造元素（唯一键版本）
     * @tparam Args 构造参数类型
     * @param args 构造参数
     * @return 插入结果（迭代器和是否成功）
     */
    template <typename... Args>
    pair<iterator, bool> emplace_unique(Args&&... args) {
        const link_type tmp = rb_tree::create_node(_NEFORCE forward<Args>(args)...);
        return rb_tree::insert_unique(tmp);
    }

    /**
     * @brief 插入元素（唯一键版本）
     * @param value 要插入的值
     * @return 插入结果（迭代器和是否成功）
     */
    pair<iterator, bool> insert_unique(const value_type& value) { return rb_tree::emplace_unique(value); }

    /**
     * @brief 移动插入元素（唯一键版本）
     * @param value 要插入的值
     * @return 插入结果（迭代器和是否成功）
     */
    pair<iterator, bool> insert_unique(value_type&& value) { return rb_tree::emplace_unique(_NEFORCE move(value)); }

    /**
     * @brief 在提示位置附近构造元素（唯一键版本）
     * @tparam Args 构造参数类型
     * @param position 插入位置提示
     * @param args 构造参数
     * @return 指向插入元素的迭代器
     */
    template <typename... Args>
    iterator emplace_unique_hint(iterator position, Args&&... args) {
        link_type tmp = rb_tree::create_node(_NEFORCE forward<Args>(args)...);
        node_guard guard(this, tmp);

        if (position.node_ == header_->left_) {
            if (size() > 0 && key_compare_(rb_tree::key(tmp), rb_tree::key(position.node_))) {
                guard.release();
                return rb_tree::insert_into(position.node_, position.node_, tmp);
            }
            guard.release();
            return rb_tree::insert_unique(tmp).first;
        }

        if (position.node_ == header_) {
            if (key_compare_(rb_tree::key(rightmost()), rb_tree::key(tmp))) {
                guard.release();
                return rb_tree::insert_into(nullptr, rightmost(), tmp);
            }
            guard.release();
            return rb_tree::insert_unique(tmp).first;
        }

        iterator before = position;
        --before;

        if (key_compare_(rb_tree::key(before.node_), rb_tree::key(tmp)) &&
            key_compare_(rb_tree::key(tmp), rb_tree::key(position.node_))) {
            if (rb_tree::right(link_type(before.node_)) == nullptr) {
                guard.release();
                return rb_tree::insert_into(nullptr, before.node_, tmp);
            }
            if (rb_tree::left(link_type(position.node_)) == nullptr) {
                guard.release();
                return rb_tree::insert_into(position.node_, position.node_, tmp);
            }
        }

        guard.release();
        return rb_tree::insert_unique(tmp).first;
    }

    /**
     * @brief 在提示位置附近插入元素（唯一键版本）
     * @param position 插入位置提示
     * @param value 要插入的值
     * @return 指向插入元素的迭代器
     */
    iterator insert_unique(iterator position, const value_type& value) {
        return rb_tree::emplace_unique_hint(position, value);
    }

    /**
     * @brief 在提示位置附近移动插入元素（唯一键版本）
     * @param position 插入位置提示
     * @param value 要插入的值
     * @return 指向插入元素的迭代器
     */
    iterator insert_unique(iterator position, value_type&& value) {
        return rb_tree::emplace_unique_hint(position, _NEFORCE move(value));
    }

    /**
     * @brief 范围插入元素（唯一键版本）
     * @tparam Iterator 迭代器类型
     * @param first 起始迭代器
     * @param last 结束迭代器
     */
    template <typename Iterator, enable_if_t<is_iter_v<Iterator>, int> = 0>
    void insert_unique(Iterator first, Iterator last) {
        for (; first != last; ++first) {
            rb_tree::insert_unique(*first);
        }
    }

    /**
     * @brief 在树中构造元素（允许重复键版本）
     * @tparam Args 构造参数类型
     * @param args 构造参数
     * @return 指向插入元素的迭代器
     */
    template <typename... Args>
    iterator emplace_equal(Args&&... args) {
        const link_type tmp = rb_tree::create_node(_NEFORCE forward<Args>(args)...);
        return rb_tree::insert_equal(tmp);
    }

    /**
     * @brief 插入元素（允许重复键版本）
     * @param value 要插入的值
     * @return 指向插入元素的迭代器
     */
    iterator insert_equal(const value_type& value) { return rb_tree::emplace_equal(value); }

    /**
     * @brief 移动插入元素（允许重复键版本）
     * @param value 要插入的值
     * @return 指向插入元素的迭代器
     */
    iterator insert_equal(value_type&& value) { return rb_tree::emplace_equal(_NEFORCE move(value)); }

    /**
     * @brief 在提示位置附近构造元素（允许重复键版本）
     * @tparam Args 构造参数类型
     * @param position 插入位置提示
     * @param args 构造参数
     * @return 指向插入元素的迭代器
     */
    template <typename... Args>
    iterator emplace_equal_hint(iterator position, Args&&... args) {
        link_type tmp = rb_tree::create_node(_NEFORCE forward<Args>(args)...);
        node_guard guard(this, tmp);

        if (position.node_ == header_->left_) {
            if (size() > 0 && key_compare_(rb_tree::key(tmp), rb_tree::key(position.node_))) {
                guard.release();
                return rb_tree::insert_into(position.node_, position.node_, tmp);
            }
            guard.release();
            return rb_tree::insert_equal(tmp);
        }

        if (position.node_ == header_) {
            if (!key_compare_(rb_tree::key(tmp), rb_tree::key(rightmost()))) {
                guard.release();
                return rb_tree::insert_into(nullptr, rightmost(), tmp);
            }
            guard.release();
            return rb_tree::insert_equal(tmp);
        }

        iterator before = position;
        --before;

        if (!key_compare_(rb_tree::key(tmp), rb_tree::key(before.node_)) &&
            !key_compare_(rb_tree::key(position.node_), rb_tree::key(tmp))) {
            if (rb_tree::right(link_type(before.node_)) == nullptr) {
                guard.release();
                return rb_tree::insert_into(nullptr, before.node_, tmp);
            }
            guard.release();
            return rb_tree::insert_into(position.node_, position.node_, tmp);
        }

        guard.release();
        return rb_tree::insert_equal(tmp);
    }

    /**
     * @brief 在提示位置附近插入元素（允许重复键版本）
     * @param position 插入位置提示
     * @param value 要插入的值
     * @return 指向插入元素的迭代器
     */
    iterator insert_equal(iterator position, const value_type& value) {
        return rb_tree::emplace_equal_hint(position, value);
    }

    /**
     * @brief 在提示位置附近移动插入元素（允许重复键版本）
     * @param position 插入位置提示
     * @param value 要插入的值
     * @return 指向插入元素的迭代器
     */
    iterator insert_equal(iterator position, value_type&& value) {
        return rb_tree::emplace_equal_hint(position, _NEFORCE move(value));
    }

    /**
     * @brief 范围插入元素（允许重复键版本）
     * @tparam Iterator 迭代器类型
     * @param first 起始迭代器
     * @param last 结束迭代器
     */
    template <typename Iterator, enable_if_t<is_iter_v<Iterator>, int> = 0>
    void insert_equal(Iterator first, Iterator last) {
        for (; first != last; ++first) {
            rb_tree::insert_equal(*first);
        }
    }

    /**
     * @brief 删除所有具有指定键的元素
     * @param key 要删除的键
     * @return 删除的元素数量
     */
    size_type erase(const key_type& key) noexcept(is_nothrow_destructible_v<link_node>) {
        pair<iterator, iterator> p = rb_tree::equal_range(key);
        const size_type n = _NEFORCE distance(p.first, p.second);
        rb_tree::erase(p.first, p.second);
        return n;
    }

    /**
     * @brief 删除指定位置的元素
     * @param position 要删除的位置
     */
    void erase(iterator position) noexcept(is_nothrow_destructible_v<link_node>) {
        auto y = reinterpret_cast<link_type>(
                _NEFORCE rb_tree_erase_rebalance(position.node_, header_->parent_, header_->left_, header_->right_));
        rb_tree::destroy_node(y);
        --size_pair_.value;
    }

    /**
     * @brief 删除指定范围内的元素
     * @param first 起始迭代器
     * @param last 结束迭代器
     */
    void erase(iterator first, iterator last) noexcept(is_nothrow_destructible_v<link_node>) {
        if (first == begin() && last == end()) {
            clear();
        } else {
            while (first != last) {
                rb_tree::erase(first++);
            }
        }
    }

    /**
     * @brief 清空树
     */
    void clear() noexcept(is_nothrow_destructible_v<link_node>) {
        if (header_ == nullptr) {
            return;
        }
        if (size_pair_.value == 0) {
            return;
        }
        rb_tree::erase_under(root());
        leftmost() = header_;
        root() = nullptr;
        rightmost() = header_;
        size_pair_.value = 0;
    }

    /**
     * @brief 查找具有指定键的元素
     * @param key 要查找的键
     * @return 指向第一个匹配元素的迭代器，未找到则返回end()
     */
    NEFORCE_NODISCARD iterator find(const key_type& key) {
        link_type y = header_;
        link_type x = root();

        while (x != nullptr) {
            if (!key_compare_(rb_tree::key(x), key)) {
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
        return key_compare_(key, rb_tree::key(y)) ? end() : j;
    }

    /**
     * @brief 查找具有指定键的元素（常量版本）
     * @param key 要查找的键
     * @return 指向第一个匹配元素的常量迭代器，未找到则返回cend()
     */
    NEFORCE_NODISCARD const_iterator find(const key_type& key) const {
        link_type y = header_;
        link_type x = root();

        while (x != nullptr) {
            if (!key_compare_(rb_tree::key(x), key)) {
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
        return key_compare_(key, rb_tree::key(y)) ? cend() : j;
    }

    /**
     * @brief 统计具有指定键的元素数量
     * @param key 要统计的键
     * @return 匹配的元素数量
     */
    NEFORCE_NODISCARD size_type count(const key_type& key) const {
        pair<const_iterator, const_iterator> p = rb_tree::equal_range(key);
        const size_type n = _NEFORCE distance(p.first, p.second);
        return n;
    }

    /**
     * @brief 获取第一个不小于指定键的元素位置
     * @param key 键值
     * @return 指向第一个不小于key的元素的迭代器
     */
    NEFORCE_NODISCARD iterator lower_bound(const key_type& key) {
        link_type y = header_;
        link_type x = root();

        while (x != nullptr) {
            if (!key_compare_(rb_tree::key(x), key)) {
                y = x;
                x = rb_tree::left(x);
            } else {
                x = rb_tree::right(x);
            }
        }

        return iterator(y, this);
    }

    /**
     * @brief 获取第一个不小于指定键的元素位置（常量版本）
     * @param key 键值
     * @return 指向第一个不小于key的元素的常量迭代器
     */
    NEFORCE_NODISCARD const_iterator lower_bound(const key_type& key) const {
        link_type y = header_;
        link_type x = root();

        while (x != nullptr) {
            if (!key_compare_(rb_tree::key(x), key)) {
                y = x;
                x = rb_tree::left(x);
            } else {
                x = rb_tree::right(x);
            }
        }

        return const_iterator(y, this);
    }

    /**
     * @brief 获取第一个大于指定键的元素位置
     * @param key 键值
     * @return 指向第一个大于key的元素的迭代器
     */
    NEFORCE_NODISCARD iterator upper_bound(const key_type& key) {
        link_type y = header_;
        link_type x = root();

        while (x != nullptr) {
            if (key_compare_(key, rb_tree::key(x))) {
                y = x;
                x = rb_tree::left(x);
            } else {
                x = rb_tree::right(x);
            }
        }

        return iterator(y, this);
    }

    /**
     * @brief 获取第一个大于指定键的元素位置（常量版本）
     * @param key 键值
     * @return 指向第一个大于key的元素的常量迭代器
     */
    NEFORCE_NODISCARD const_iterator upper_bound(const key_type& key) const {
        link_type y = header_;
        link_type x = root();

        while (x != nullptr) {
            if (key_compare_(key, rb_tree::key(x))) {
                y = x;
                x = rb_tree::left(x);
            } else {
                x = rb_tree::right(x);
            }
        }

        return const_iterator(y, this);
    }

    /**
     * @brief 获取等于指定键的元素范围
     * @param key 键值
     * @return 包含lower_bound和upper_bound的pair
     */
    NEFORCE_NODISCARD pair<iterator, iterator> equal_range(const key_type& key) {
        return pair<iterator, iterator>(rb_tree::lower_bound(key), rb_tree::upper_bound(key));
    }

    /**
     * @brief 获取等于指定键的元素范围（常量版本）
     * @param key 键值
     * @return 包含lower_bound和upper_bound的pair
     */
    NEFORCE_NODISCARD pair<const_iterator, const_iterator> equal_range(const key_type& key) const {
        return pair<const_iterator, const_iterator>(rb_tree::lower_bound(key), rb_tree::upper_bound(key));
    }

    /**
     * @brief 交换两个红黑树的内容
     * @param other 要交换的另一个红黑树
     */
    void swap(rb_tree& other) noexcept(is_nothrow_swappable_v<Compare> && is_nothrow_swappable_v<KeyOfValue> &&
                                       is_nothrow_swappable_v<allocator_type>) {
        _NEFORCE swap(header_, other.header_);
        _NEFORCE swap(size_pair_, other.size_pair_);
        _NEFORCE swap(key_compare_, other.key_compare_);
        _NEFORCE swap(extracter_, other.extracter_);
    }

    /**
     * @brief 相等比较操作符
     * @param rhs 右侧红黑树
     * @return 如果两个红黑树大小相等且对应元素相等返回true
     */
    NEFORCE_NODISCARD bool equal_to(const rb_tree& rhs) const
            noexcept(noexcept(_NEFORCE equal(cbegin(), cend(), rhs.cbegin()))) {
        return size() == rhs.size() && _NEFORCE equal(cbegin(), cend(), rhs.cbegin());
    }

    /**
     * @brief 小于比较操作符
     * @param rhs 右侧红黑树
     * @return 按字典序比较结果
     */
    NEFORCE_NODISCARD bool less_than(const rb_tree& rhs) const
            noexcept(noexcept(_NEFORCE lexicographical_compare(cbegin(), cend(), rhs.cbegin(), rhs.cend()))) {
        return _NEFORCE lexicographical_compare(cbegin(), cend(), rhs.cbegin(), rhs.cend());
    }
};

/** @} */ // RBTree

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_CORE_CONTAINER_RB_TREE_HPP__
