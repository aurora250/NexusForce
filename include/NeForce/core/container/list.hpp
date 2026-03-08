#ifndef NEFORCE_CORE_CONTAINER_LIST_HPP__
#define NEFORCE_CORE_CONTAINER_LIST_HPP__

/**
 * @file list.hpp
 * @brief 双向链表容器
 *
 * 此文件提供了双向链表容器的实现。
 * 支持常数时间的插入和删除操作，不支持随机访问。
 */

#include "NeForce/core/algorithm/compare.hpp"
#include "NeForce/core/interface/icollector.hpp"
#include "NeForce/core/interface/iiterator.hpp"
#include "NeForce/core/memory/standard_allocator.hpp"
#include "NeForce/core/utility/compressed_pair.hpp"
NEFORCE_BEGIN_NAMESPACE__

/**
 * @defgroup List 双向链表
 * @brief 双向链表容器实现
 * @{
 */

/**
 * @struct list_node
 * @brief 链表节点结构
 * @tparam T 节点存储的数据类型
 *
 * 作为list容器的基本节点单元，包含数据域和前后指针。
 */
template <typename T>
struct list_node {
    T data;                    ///< 节点存储的数据
    list_node* prev = nullptr; ///< 指向前一个节点的指针
    list_node* next = nullptr; ///< 指向后一个节点的指针

    /**
     * @brief 默认构造函数
     *
     * 使用默认构造方式初始化数据成员。
     */
    list_node()
    noexcept(is_nothrow_default_constructible_v<T>)
    : data() {}

    /**
     * @brief 参数构造
     * @tparam Args 构造参数类型
     * @param args 参数
     *
     * 使用参数初始化节点数据。
     */
    template <typename... Args>
    explicit list_node(Args&&... args)
    noexcept(is_nothrow_constructible_v<T, Args...>)
    : data(_NEFORCE forward<Args>(args)...) {}
};


/**
 * @struct list_iterator
 * @brief 链表迭代器
 * @tparam IsConst 是否常量迭代器
 * @tparam List 链表类型
 *
 * 为list提供双向迭代器支持，包含边界检查和调试验证。
 */
template <bool IsConst, typename List>
struct list_iterator : iiterator<list_iterator<IsConst, List>> {
public:
    using container_type    = List;                                    ///< 容器类型
    using value_type        = typename container_type::value_type;    ///< 值类型
    using size_type         = typename container_type::size_type;     ///< 大小类型
    using difference_type   = typename container_type::difference_type; ///< 差值类型
    using iterator_category = bidirectional_iterator_tag;             ///< 迭代器类别
    using reference = conditional_t<IsConst, typename container_type::const_reference, typename container_type::reference>; ///< 引用类型
    using pointer   = conditional_t<IsConst, typename container_type::const_pointer, typename container_type::pointer>;   ///< 指针类型

private:
    using node_type = list_node<value_type>;  ///< 节点类型别名

    node_type* current_ = nullptr;  ///< 当前指向的节点指针
    const container_type* container_ = nullptr;  ///< 关联容器指针

public:
    list_iterator() noexcept = default;
    ~list_iterator() = default;

    list_iterator(const list_iterator&) noexcept = default;
    list_iterator& operator =(const list_iterator&) noexcept = default;
    list_iterator(list_iterator&&) noexcept = default;
    list_iterator& operator =(list_iterator&&) noexcept = default;

    /**
     * @brief 构造函数
     * @param ptr 初始节点指针
     * @param list 关联容器指针
     */
    list_iterator(node_type* ptr, const container_type* list) noexcept
    : current_(ptr), container_(list) {}

    /**
     * @brief 解引用操作实现
     * @return 当前节点的数据引用
     */
    NEFORCE_NODISCARD reference dereference() const noexcept {
        NEFORCE_DEBUG_VERIFY(current_ && container_, "Attempting to dereference on a null pointer");
        NEFORCE_DEBUG_VERIFY(current_ != container_->head_, "Attempting to dereference out of boundary");
        return current_->data;
    }

    /**
     * @brief 递增操作
     */
    void increment() noexcept {
        NEFORCE_DEBUG_VERIFY(current_ && container_, "Attempting to increment a null pointer");
        NEFORCE_DEBUG_VERIFY(current_ != container_->head_, "Attempting to increment out of boundary");
        current_ = current_->next;
    }

    /**
     * @brief 递减操作
     */
    void decrement() noexcept {
        NEFORCE_DEBUG_VERIFY(current_ && container_, "Attempting to decrement a null pointer");
        NEFORCE_DEBUG_VERIFY(current_->prev != container_->head_, "Attempting to decrement out of boundary");
        current_ = current_->prev;
    }

    /**
     * @brief 相等比较
     * @param rhs 右侧迭代器
     * @return 是否指向同一节点
     */
    NEFORCE_NODISCARD bool equal(const list_iterator& rhs) const noexcept {
        NEFORCE_DEBUG_VERIFY(container_ == rhs.container_, "Attempting to equal to a different container");
        return current_ == rhs.current_;
    }

    /**
     * @brief 获取底层节点指针
     * @return 当前节点指针
     */
    NEFORCE_NODISCARD node_type* base() const noexcept {
        return current_;
    }

    /**
     * @brief 获取关联容器
     * @return 关联容器指针
     */
    NEFORCE_NODISCARD const container_type* container() const noexcept {
        return container_;
    }
};


/**
 * @class list
 * @brief 双向链表容器
 * @tparam T 元素类型
 * @tparam Alloc 分配器类型
 *
 * 双向链表容器，提供常数时间的插入和删除操作。
 * 不支持随机访问，但支持双向迭代。
 */
template <typename T, typename Alloc = allocator<list_node<T>>>
class list : public icollector<list<T, Alloc>> {
    static_assert(is_allocator_v<Alloc>, "Alloc type is not a standard allocator type.");
    static_assert(is_same_v<list_node<T>, typename Alloc::value_type>, "allocator type mismatch.");
    static_assert(is_object_v<T>, "list only contains object types.");

public:
    using pointer			= T*;  ///< 指针类型
    using reference			= T&;  ///< 引用类型
    using const_pointer		= const T*;  ///< 常量指针类型
    using const_reference	= const T&;  ///< 常量引用类型
    using value_type		= T;  ///< 值类型
    using size_type			= size_t;  ///< 大小类型
    using difference_type	= ptrdiff_t;  ///< 差值类型
    using iterator                  = list_iterator<false, list>; ///< 迭代器类型
    using const_iterator            = list_iterator<true, list>;  ///< 常量迭代器类型
    using reverse_iterator          = _NEFORCE reverse_iterator<iterator>;        ///< 反向迭代器类型
    using const_reverse_iterator    = _NEFORCE reverse_iterator<const_iterator>;  ///< 常量反向迭代器类型
    using allocator_type            = Alloc;  ///< 分配器类型

private:
    using node_type = list_node<T>; ///< 节点类型
    using link_type = node_type*;   ///< 节点指针类型

    link_type head_ = nullptr;  ///< 头节点指针
    compressed_pair<allocator_type, size_type> pair_{ default_construct_tag{}, 0 };  ///< 压缩存储的分配器和大小

    template <bool, typename> friend struct list_iterator;

private:
    /**
     * @brief 创建新节点
     * @tparam Args 构造参数类型
     * @param args 构造参数
     * @return 新创建的节点指针
     *
     * 分配内存并构造节点，如果构造失败则释放内存。
     */
    template <typename... Args>
    link_type create_node(Args&&... args) {
        link_type p = pair_.get_base().allocate();
        try {
            _NEFORCE construct(&p->data, _NEFORCE forward<Args>(args)...);
        } catch (...) {
            pair_.get_base().deallocate(p);
            throw;
        }
        return p;
    }

    /**
     * @brief 销毁节点
     * @param p 要销毁的节点指针
     *
     * 析构节点数据并释放内存。
     */
    void destroy_node(link_type p)
    noexcept(is_nothrow_destructible_v<node_type>) {
        _NEFORCE destroy(p);
        pair_.get_base().deallocate(p);
    }

    /**
     * @brief 初始化头节点
     *
     * 创建并初始化哨兵节点，其prev和next都指向自身。
     */
    void init_header() {
        head_ = pair_.get_base().allocate();
        try {
            _NEFORCE construct(head_);
        } catch (...) {
            pair_.get_base().deallocate(head_);
            throw;
        }
        head_->prev = head_->next = head_;
    }

public:
    /**
     * @brief 默认构造函数
     *
     * 构造一个空链表。
     */
    list() {
        list::init_header();
    }

    /**
     * @brief 构造包含n个默认构造元素的链表
     * @param n 元素数量
     */
    explicit list(size_type n)
    : list(n, _NEFORCE initialize<T>()) {}

    /**
     * @brief 构造包含n个指定值元素的链表
     * @param n 元素数量
     * @param value 初始值
     */
    list(size_type n, T&& value) {
        list::init_header();
        iterator pos = end();
        try {
            while (n--) {
                pos = list::emplace(pos, _NEFORCE forward<T>(value));
            }
        } catch (...) {
            clear();
            throw;
        }
    }

    /**
     * @brief 范围构造函数
     * @tparam Iterator 迭代器类型
     * @param first 起始迭代器
     * @param last 结束迭代器
     */
    template <typename Iterator, enable_if_t<is_iter_v<Iterator>, int> = 0>
    list(Iterator first, Iterator last) {
        list::init_header();
        iterator pos = end();
        try {
            while (first != last) {
                pos = list::emplace(pos, *first);
                ++first;
            }
        } catch (...) {
            clear();
            throw;
        }
    }

    /**
     * @brief 初始化列表构造函数
     * @param ilist 初始化列表
     */
    list(std::initializer_list<T> ilist)
    : list(ilist.begin(), ilist.end()) {}

    /**
     * @brief 初始化列表赋值运算符
     * @param ilist 初始化列表
     * @return 自身引用
     */
    list& operator =(std::initializer_list<T> ilist) {
        clear();
        list::insert(begin(), ilist.begin(), ilist.end());
        return *this;
    }

    /**
     * @brief 拷贝构造函数
     * @param other 源链表
     */
    list(const list& other)
    : list(other.begin(), other.end()) {}

    /**
     * @brief 拷贝赋值运算符
     * @param other 源链表
     * @return 自身引用
     */
    list& operator =(const list& other) {
        if (_NEFORCE addressof(other) == this) return *this;
        list tmp(other);
        list::swap(tmp);
        return *this;
    }

    /**
     * @brief 移动构造函数
     * @param other 源链表
     */
    list(list&& other)
    noexcept(is_nothrow_swappable_v<compressed_pair<allocator_type, size_type>>) {
        init_header();
        list::swap(other);
    }

    /**
     * @brief 移动赋值运算符
     * @param other 源链表
     * @return 自身引用
     */
    list& operator =(list&& other)
    noexcept(
        is_nothrow_swappable_v<compressed_pair<allocator_type, size_type>> &&
        is_nothrow_destructible_v<node_type>) {
        if (_NEFORCE addressof(other) == this) return *this;
        clear();
        list::swap(other);
        return *this;
    }

    /**
     * @brief 析构函数
     *
     * 销毁所有元素并释放内存。
     */
    ~list() {
        link_type p = head_->next;
        while (p != head_) {
            link_type q = p;
            p = p->next;
            list::destroy_node(q);
        }
        list::destroy_node(head_);
    }

    /**
     * @brief 获取起始迭代器
     * @return 指向第一个元素的迭代器
     */
    NEFORCE_NODISCARD iterator begin() noexcept {
        return iterator{head_->next, this};
    }

    /**
     * @brief 获取结束迭代器
     * @return 指向无效元素的迭代器
     */
    NEFORCE_NODISCARD iterator end() noexcept {
        return iterator{head_, this};
    }

    /**
     * @brief 获取常量起始迭代器
     * @return 指向第一个元素的常量迭代器
     */
    NEFORCE_NODISCARD const_iterator begin() const noexcept {
        return cbegin();
    }

    /**
     * @brief 获取常量结束迭代器
     * @return 指向无效元素的常量迭代器
     */
    NEFORCE_NODISCARD const_iterator end() const noexcept {
        return cend();
    }

    /**
     * @brief 获取反向起始迭代器
     * @return 指向无效元素的反向迭代器
     */
    NEFORCE_NODISCARD reverse_iterator rbegin() noexcept {
        return reverse_iterator(end());
    }

    /**
     * @brief 获取反向结束迭代器
     * @return 指向第一个元素的反向迭代器
     */
    NEFORCE_NODISCARD reverse_iterator rend() noexcept {
        return reverse_iterator(begin());
    }

    /**
     * @brief 获取常量反向起始迭代器
     * @return 指向无效元素的常量反向迭代器
     */
    NEFORCE_NODISCARD const_reverse_iterator rbegin() const noexcept {
        return crbegin();
    }

    /**
     * @brief 获取常量反向结束迭代器
     * @return 指向第一个元素的常量反向迭代器
     */
    NEFORCE_NODISCARD const_reverse_iterator rend() const noexcept {
        return crend();
    }

    /**
     * @brief 获取常量起始迭代器
     * @return 指向第一个元素的常量迭代器
     */
    NEFORCE_NODISCARD const_iterator cbegin() const noexcept {
        return const_iterator{head_->next, this};
    }

    /**
     * @brief 获取常量结束迭代器
     * @return 指向无效元素的常量迭代器
     */
    NEFORCE_NODISCARD const_iterator cend() const noexcept {
        return const_iterator{head_, this};
    }

    /**
     * @brief 获取常量反向起始迭代器
     * @return 指向无效元素的常量反向迭代器
     */
    NEFORCE_NODISCARD const_reverse_iterator crbegin() const noexcept {
        return const_reverse_iterator(cend());
    }

    /**
     * @brief 获取常量反向结束迭代器
     * @return 指向第一个元素的常量反向迭代器
     */
    NEFORCE_NODISCARD const_reverse_iterator crend() const noexcept {
        return const_reverse_iterator(cbegin());
    }

    /**
     * @brief 获取当前元素数量
     * @return 元素数量
     */
    NEFORCE_NODISCARD size_type size() const noexcept {
        return pair_.value;
    }

    /**
     * @brief 获取最大可能大小
     * @return 最大元素数量
     */
    NEFORCE_NODISCARD size_type max_size() const noexcept {
        return static_cast<size_type>(-1);
    }

    /**
     * @brief 检查是否为空
     * @return 是否为空
     */
    NEFORCE_NODISCARD bool empty() const noexcept {
        return head_->next == head_;
    }

    /**
     * @brief 访问第一个元素
     * @return 第一个元素的引用
     */
    NEFORCE_NODISCARD reference front() noexcept {
        NEFORCE_DEBUG_VERIFY(!empty(), "front called on empty list");
        return head_->next->data;
    }

    /**
     * @brief 访问第一个常量元素
     * @return 第一个元素的常量引用
     */
    NEFORCE_NODISCARD const_reference front() const noexcept {
        NEFORCE_DEBUG_VERIFY(!empty(), "front called on empty list");
        return head_->next->data;
    }

    /**
     * @brief 访问最后一个元素
     * @return 最后一个元素的引用
     */
    NEFORCE_NODISCARD reference back() noexcept {
        NEFORCE_DEBUG_VERIFY(!empty(), "back called on empty list");
        return head_->prev->data;
    }

    /**
     * @brief 访问最后一个常量元素
     * @return 最后一个元素的常量引用
     */
    NEFORCE_NODISCARD const_reference back() const noexcept {
        NEFORCE_DEBUG_VERIFY(!empty(), "back called on empty list");
        return head_->prev->data;
    }

    /**
     * @brief 在指定位置构造元素
     * @tparam Args 构造参数类型
     * @param position 插入位置
     * @param args 构造参数
     * @return 指向插入元素的迭代器
     */
    template <typename... Args>
    iterator emplace(iterator position, Args&&... args) {
        link_type temp = list::create_node(_NEFORCE forward<Args>(args)...);
        temp->next = position.base();
        temp->prev = position.base()->prev;
        position.base()->prev->next = temp;
        position.base()->prev = temp;
        ++pair_.value;
        return {temp, this};
    }

    /**
     * @brief 在末尾构造元素
     * @tparam Args 构造参数类型
     * @param args 构造参数
     * @return 指向插入元素的迭代器
     */
    template <typename... Args>
    iterator emplace_back(Args&&... args) {
        return list::emplace(end(), _NEFORCE forward<Args>(args)...);
    }

    /**
     * @brief 在开头构造元素
     * @tparam Args 构造参数类型
     * @param args 构造参数
     * @return 指向插入元素的迭代器
     */
    template <typename... Args>
    iterator emplace_front(Args&&... args) {
        return list::emplace(begin(), _NEFORCE forward<Args>(args)...);
    }

    /**
     * @brief 在开头拷贝插入元素
     * @param value 要插入的值
     */
    void push_front(const T& value) {
        list::insert(begin(), value);
    }

    /**
     * @brief 在开头移动插入元素
     * @param value 要插入的值
     */
    void push_front(T&& value) {
        list::insert(begin(), _NEFORCE forward<T>(value));
    }

    /**
     * @brief 在末尾拷贝插入元素
     * @param value 要插入的值
     */
    void push_back(const T& value) {
        list::insert(end(), value);
    }

    /**
     * @brief 在末尾移动插入元素
     * @param value 要插入的值
     */
    void push_back(T&& value) {
        list::insert(end(), _NEFORCE forward<T>(value));
    }

    /**
     * @brief 移除开头元素
     */
    void pop_front() noexcept {
        list::erase(begin());
    }

    /**
     * @brief 移除末尾元素
     */
    void pop_back() noexcept {
        list::erase({head_->prev, this});
    }

    /**
     * @brief 赋值n个指定值的元素
     * @param n 元素数量
     * @param value 要赋的值
     */
    void assign(const size_type n, const T& value) {
        clear();
        list::insert(begin(), n, value);
    }

    /**
     * @brief 范围赋值
     * @tparam Iterator 迭代器类型
     * @param first 起始迭代器
     * @param last 结束迭代器
     */
    template <typename Iterator, enable_if_t<is_iter_v<Iterator>, int> = 0>
    void assign(Iterator first, Iterator last) {
        clear();
        list::insert(begin(), first, last);
    }

    /**
     * @brief 初始化列表赋值
     * @param ilist 初始化列表
     */
    void assign(std::initializer_list<T> ilist) {
        list::assign(ilist.begin(), ilist.end());
    }

    /**
     * @brief 在指定位置拷贝插入元素
     * @param position 插入位置
     * @param value 要插入的值
     * @return 指向插入元素的迭代器
     */
    iterator insert(iterator position, const T& value) {
        return list::emplace(position, value);
    }

    /**
     * @brief 在指定位置移动插入元素
     * @param position 插入位置
     * @param value 要插入的值
     * @return 指向插入元素的迭代器
     */
    iterator insert(iterator position, T&& value) {
        return list::emplace(position, _NEFORCE move(value));
    }

    /**
     * @brief 范围插入
     * @tparam Iterator 迭代器类型
     * @param position 插入位置
     * @param first 起始迭代器
     * @param last 结束迭代器
     *
     * 将[first, last)范围内的元素插入到position之前。
     * 如果插入过程中发生异常，会回滚已插入的元素。
     */
    template <typename Iterator, enable_if_t<is_iter_v<Iterator>, int> = 0>
    void insert(iterator position, Iterator first, Iterator last) {
        if (first == last) return;

        link_type original_prev = position.base()->prev;
        link_type current_prev = original_prev;
        link_type first_inserted = nullptr;

        try {
            while (first != last) {
                link_type temp = list::create_node(*first);
                temp->prev = current_prev;
                temp->next = position.base();
                current_prev->next = temp;
                position.base()->prev = temp;
                if (first_inserted == nullptr) {
                    first_inserted = temp;
                }
                current_prev = temp;
                ++pair_.value;
                ++first;
            }
        } catch (...) {
            if (first_inserted != nullptr) {
                link_type to_delete = first_inserted;
                while (to_delete != position.base()) {
                    link_type next = to_delete->next;
                    list::destroy_node(to_delete);
                    --pair_.value;
                    to_delete = next;
                }
                original_prev->next = position.base();
                position.base()->prev = original_prev;
            }
            throw;
        }
    }

    /**
     * @brief 初始化列表插入
     * @param position 插入位置
     * @param ilist 初始化列表
     */
    void insert(iterator position, std::initializer_list<T> ilist) {
        list::insert(position, ilist.begin(), ilist.end());
    }

    /**
     * @brief 插入n个指定值的元素
     * @param position 插入位置
     * @param n 元素数量
     * @param value 要插入的值
     *
     * 如果插入过程中发生异常，会回滚已插入的元素。
     */
    void insert(iterator position, size_type n, const T& value) {
        if (n == 0) return;

        link_type original_prev = position.base()->prev;
        link_type current_prev = original_prev;
        link_type first_inserted = nullptr;

        try {
            while (n--) {
                link_type temp = list::create_node(value);
                temp->prev = current_prev;
                temp->next = position.base();
                current_prev->next = temp;
                position.base()->prev = temp;
                if (first_inserted == nullptr) {
                    first_inserted = temp;
                }
                current_prev = temp;
                ++pair_.value;
            }
        } catch (...) {
            if (first_inserted != nullptr) {
                link_type to_delete = first_inserted;
                while (to_delete != position.base()) {
                    link_type next = to_delete->next;
                    list::destroy_node(to_delete);
                    --pair_.value;
                    to_delete = next;
                }
                original_prev->next = position.base();
                position.base()->prev = original_prev;
            }
            throw;
        }
    }

    /**
     * @brief 删除指定位置的元素
     * @param position 要删除的位置
     * @return 指向被删除元素之后位置的迭代器
     */
    iterator erase(iterator position)
    noexcept(is_nothrow_destructible_v<node_type>) {
        if (empty()) return end();
        link_type ret = position.base()->next;
        position.base()->prev->next = position.base()->next;
        position.base()->next->prev = position.base()->prev;
        list::destroy_node(position.base());
        --pair_.value;
        return iterator{ret, this};
    }

    /**
     * @brief 删除指定范围内的元素
     * @param first 起始迭代器
     * @param last 结束迭代器
     * @return 指向最后一个被删除元素之后位置的迭代器
     */
    iterator erase(iterator first, iterator last)
    noexcept(is_nothrow_destructible_v<node_type>) {
        while (first != last) {
            first = list::erase(first);
        }
        return first;
    }

    /**
     * @brief 清空链表
     *
     * 销毁所有元素，但保留哨兵节点。
     */
    void clear()
    noexcept(is_nothrow_destructible_v<node_type>) {
        link_type cur = head_->next;
        while (cur != head_) {
            link_type temp = cur;
            cur = cur->next;
            list::destroy_node(temp);
            --pair_.value;
        }
        head_->prev = head_;
        head_->next = head_;
    }

    /**
     * @brief 交换两个链表的内容
     * @param other 要交换的另一个链表
     */
    void swap(list& other)
	noexcept(is_nothrow_swappable_v<allocator_type>) {
        _NEFORCE swap(head_, other.head_);
        _NEFORCE swap(pair_, other.pair_);
    }

    /**
     * @brief 传输元素
     * @param position 目标位置
     * @param first 起始迭代器
     * @param last 结束迭代器
     *
     * 将[first, last)范围内的元素从当前链表传输到position之前。
     * 此操作为常数时间，不涉及元素构造和析构。
     */
    void transfer(iterator position, iterator first, iterator last) {
        if (position == last) return;
        last.base()->prev->next = position.base();
        first.base()->prev->next = last.base();
        position.base()->prev->next = first.base();
        link_type tmp = position.base()->prev;
        position.base()->prev = last.base()->prev;
        last.base()->prev = first.base()->prev;
        first.base()->prev = tmp;
    }

    /**
     * @brief 根据谓词移除元素
     * @tparam Pred 谓词类型
     * @param pred 一元谓词，返回true的元素将被移除
     */
    template <typename Pred>
    void remove_if(Pred pred) {
        iterator iter = begin(), last = end();
        while (iter != last) {
            if (pred(*iter)) {
                iter = list::erase(iter);
            } else {
                ++iter;
            }
        }
    }

    /**
     * @brief 移除指定值的元素
     * @param value 要移除的值
     */
    void remove(const T& value) {
        return list::remove_if([&](const T& other) -> bool {
            return other == value;
        });
    }

    /**
     * @brief 拼接整个链表
     * @param position 目标位置
     * @param other 源链表
     *
     * 将other链表的所有元素拼接到position之前，other变为空链表。
     */
    void splice(iterator position, list& other) {
        if (!other.empty()) {
            size_type n = other.pair_.value;
            list::transfer(position, other.begin(), other.end());
            pair_.value += n;
            other.pair_.value = 0;
        }
    }

    /**
     * @brief 拼接单个元素
     * @param position 目标位置
     * @param other 源链表
     * @param iter 指向要拼接元素的迭代器
     *
     * 将other链表中的iter指向的元素拼接到position之前。
     */
    void splice(iterator position, list& other, iterator iter) {
        iterator j = iter;
        ++j;
        if (iter == position || j == position) return;
        list::transfer(position, iter, j);
        ++pair_.value;
        --other.pair_.value;
    }

    /**
     * @brief 拼接范围内的元素
     * @param position 目标位置
     * @param other 源链表
     * @param first 起始迭代器
     * @param last 结束迭代器
     *
     * 将other链表中[first, last)范围内的元素拼接到position之前。
     */
    void splice(iterator position, list& other, iterator first, iterator last) {
        if (first == last) return;
        size_type n = 0;
        for (iterator it = first; it != last; ++it) ++n;
        list::transfer(position, first, last);
        pair_.value += n;
        other.pair_.value -= n;
    }

    /**
     * @brief 合并两个有序链表
     * @tparam Pred 比较谓词类型
     * @param other 要合并的链表
     * @param pred 二元比较谓词
     *
     * 将有序链表other合并到当前有序链表中，合并后仍保持有序。
     * 使用pred作为比较准则。
     */
    template <typename Pred>
    void merge_if(list& other, Pred pred) {
        iterator first1 = begin(), first2 = other.begin();
        iterator last1 = end(), last2 = other.end();

        while (first1 != last1 && first2 != last2) {
            if (!pred(*first2, *first1)) {
                ++first1;
            } else {
                iterator temp = first2;
                ++temp;
                list::transfer(first1, first2, temp);
                first2 = temp;
                ++pair_.value;
                --other.pair_.value;
            }
        }

        if (first2 != last2) {
            size_type n = other.pair_.value;
            list::transfer(last1, first2, last2);
            pair_.value += n;
            other.pair_.value = 0;
        }
    }

    /**
     * @brief 合并两个有序链表（默认使用小于比较）
     * @param other 要合并的链表
     */
    void merge(list& other) {
        list::merge_if(other, _NEFORCE less<T>());
    }

    /**
     * @brief 反转链表
     */
    void reverse() noexcept {
        if (empty()) return;
        link_type current = head_;
        do {
            _NEFORCE swap(current->prev, current->next);
            current = current->prev;
        } while (current != head_);
    }

    /**
     * @brief 移除连续的重复元素
     * @tparam Pred 二元谓词类型
     * @param pred 用于判断两个元素是否相等的谓词
     *
     * 移除链表中所有连续重复的元素，只保留第一个。
     */
    template <typename Pred>
    void unique_if(Pred pred) noexcept {
        if (empty()) return;
        iterator current = begin();
        iterator next = current;
        while (++next != end()) {
            if (pred(*current, *next)) {
                list::erase(next);
                next = current;
            } else {
                current = next;
            }
        }
    }

    /**
     * @brief 移除连续的重复元素（默认使用等于比较）
     */
    void unique() noexcept {
        list::unique_if(_NEFORCE equal_to<T>());
    }

    /**
     * @brief 对链表进行排序
     * @tparam Pred 比较谓词类型
     * @param pred 二元比较谓词
     *
     * 使用插入排序算法对链表进行排序。
     */
    template <typename Pred>
    void sort_if(Pred pred) {
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

    /**
     * @brief 对链表进行排序（默认使用小于比较）
     */
    void sort() {
        list::sort_if(_NEFORCE less<T>());
    }

    /**
     * @brief 常量索引访问
     * @param position 索引位置
     * @return 指定位置元素的常量引用
     */
    NEFORCE_NODISCARD const_reference at(size_type position) const {
        const_iterator iter = cbegin();
        while (position--) ++iter;
        return iter.base()->data;
    }

    /**
     * @brief 索引访问
     * @param position 索引位置
     * @return 指定位置元素的引用
     */
    NEFORCE_NODISCARD reference at(size_type position) {
        const_iterator iter = cbegin();
        while (position--) ++iter;
        return iter.base()->data;
    }

    /**
     * @brief 常量下标访问操作符
     * @param position 索引位置
     * @return 指定位置元素的常量引用
     */
    NEFORCE_NODISCARD const_reference operator [](const size_type position) const {
        return at(position);
    }

    /**
     * @brief 下标访问操作符
     * @param position 索引位置
     * @return 指定位置元素的引用
     */
    NEFORCE_NODISCARD reference operator [](const size_type position) {
        return at(position);
    }

    /**
     * @brief 相等比较操作符
     * @param rhs 右侧链表
     * @return 如果两个链表大小相等且对应元素相等返回true
     */
    NEFORCE_NODISCARD bool operator ==(const list& rhs) const
    noexcept(noexcept(_NEFORCE equal(cbegin(), cend(), rhs.cbegin()))) {
        return size() == rhs.size() && _NEFORCE equal(cbegin(), cend(), rhs.cbegin());
    }

    /**
     * @brief 小于比较操作符
     * @param rhs 右侧链表
     * @return 按字典序比较结果
     */
    NEFORCE_NODISCARD bool operator <(const list& rhs) const
    noexcept(noexcept(_NEFORCE lexicographical_compare(cbegin(), cend(), rhs.cbegin(), rhs.cend()))) {
        return _NEFORCE lexicographical_compare(cbegin(), cend(), rhs.cbegin(), rhs.cend());
    }
};

#ifdef NEFORCE_STANDARD_17
template <typename Iterator, typename Alloc>
list(Iterator, Iterator, Alloc = Alloc()) -> list<iter_value_t<Iterator>, Alloc>;
#endif

/** @} */ // List

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_CORE_CONTAINER_LIST_HPP__
