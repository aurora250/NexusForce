#ifndef NEFORCE_CORE_CONTAINER_MULTISET_HPP__
#define NEFORCE_CORE_CONTAINER_MULTISET_HPP__

/**
 * @file multiset.hpp
 * @brief 多重集合容器
 *
 * 此文件提供了多重集合容器的实现。
 * multiset是一种有序关联容器，包含键的集合，允许重复键。
 * 键按严格弱序排序，支持对数时间复杂度的查找、插入和删除操作。
 * 底层使用红黑树实现。
 */

#include "NeForce/core/container/rb_tree.hpp"
NEFORCE_BEGIN_NAMESPACE__

/**
 * @defgroup Container 标准容器
 * @brief 支持标准算法的容器的实现
 * @{
 */

/**
 * @class multiset
 * @brief 多重集合容器
 * @tparam Key 键类型
 * @tparam Compare 键比较函数类型，默认为less<Key>
 * @tparam Alloc 分配器类型
 *
 * multiset是一种关联容器，存储唯一的键值，允许重复键。
 * 元素按照键的顺序自动排序，排序标准由Compare函数对象指定。
 * 支持快速的键查找，且允许重复键。底层使用红黑树实现。
 */
template <typename Key, typename Compare = less<Key>, typename Alloc = allocator<rb_tree_node<Key>>>
class multiset : icollector<multiset<Key, Compare, Alloc>> {
    static_assert(is_allocator_v<Alloc>, "Alloc type is not a standard allocator type.");
    static_assert(
        is_same_v<rb_tree_node<Key>, typename Alloc::value_type>,
        "allocator type mismatch.");
    static_assert(is_object_v<Key>, "multiset only contains object types.");

public:
    using key_type          = Key;  ///< 键类型
    using value_type        = Key;  ///< 值类型
    using key_compare       = Compare;  ///< 键比较函数类型
    using value_compare     = Compare;  ///< 值比较函数类型

private:
    using base_type = rb_tree<Key, Key, identity<Key>, Compare, Alloc>;  ///< 底层红黑树类型

public:
    using size_type             = typename base_type::size_type;  ///< 大小类型
    using difference_type       = typename base_type::difference_type;  ///< 差值类型
    using pointer               = typename base_type::pointer;  ///< 指针类型
    using const_pointer         = typename base_type::const_pointer;  ///< 常量指针类型
    using reference             = typename base_type::reference;  ///< 引用类型
    using const_reference       = typename base_type::const_reference;  ///< 常量引用类型
    using iterator                      = typename base_type::iterator;  ///< 迭代器类型
    using const_iterator                = typename base_type::const_iterator;  ///< 常量迭代器类型
    using reverse_iterator              = typename base_type::reverse_iterator;  ///< 反向迭代器类型
    using const_reverse_iterator        = typename base_type::const_reverse_iterator;  ///< 常量反向迭代器类型
    using allocator_type                = typename base_type::allocator_type;  ///< 分配器类型

private:
    base_type tree_;  ///< 底层红黑树实例

public:
    /**
     * @brief 默认构造函数
     *
     * 构造一个空multiset，使用默认的比较函数。
     */
    multiset()
    : tree_(Compare()) {}

    /**
     * @brief 构造函数，指定比较函数
     * @param comp 比较函数对象
     */
    explicit multiset(const key_compare& comp)
    : tree_(comp) {}

    /**
     * @brief 拷贝构造函数
     * @param other 源multiset
     */
    multiset(const multiset& other)
    : tree_(other.tree_) {}

    /**
     * @brief 拷贝赋值运算符
     * @param other 源multiset
     * @return 自身引用
     */
    multiset& operator =(const multiset& other) {
        tree_ = other.tree_;
        return *this;
    }

    /**
     * @brief 移动构造函数
     * @param other 源multiset
     */
    multiset(multiset&& other)
    noexcept(is_nothrow_move_constructible_v<base_type>)
    : tree_(_NEFORCE move(other.tree_)) {}

    /**
     * @brief 移动赋值运算符
     * @param other 源multiset
     * @return 自身引用
     */
    multiset& operator =(multiset&& other)
    noexcept(is_nothrow_move_assignable_v<base_type>) {
        tree_ = _NEFORCE move(other.tree_);
        return *this;
    }

    /**
     * @brief 范围构造函数
     * @tparam Iterator 迭代器类型
     * @param first 起始迭代器
     * @param last 结束迭代器
     */
    template <typename Iterator>
    multiset(Iterator first, Iterator last)
    : tree_(Compare()) {
        tree_.insert_equal(first, last);
    }

    /**
     * @brief 范围构造函数，指定比较函数
     * @tparam Iterator 迭代器类型
     * @param first 起始迭代器
     * @param last 结束迭代器
     * @param comp 比较函数对象
     */
    template <typename Iterator>
    multiset(Iterator first, Iterator last, const key_compare& comp)
    : tree_(comp) {
        tree_.insert_equal(first, last);
    }

    /**
     * @brief 初始化列表构造函数
     * @param ilist 初始化列表
     */
    multiset(std::initializer_list<value_type> ilist)
    : multiset(ilist.begin(), ilist.end()) {}

    /**
     * @brief 初始化列表构造函数，指定比较函数
     * @param ilist 初始化列表
     * @param comp 比较函数对象
     */
    multiset(std::initializer_list<value_type> ilist, const key_compare& comp)
    : multiset(ilist.begin(), ilist.end(), comp) {}

    /**
     * @brief 初始化列表赋值运算符
     * @param ilist 初始化列表
     * @return 自身引用
     */
    multiset& operator =(std::initializer_list<value_type> ilist) {
        clear();
        insert(ilist.begin(), ilist.end());
        return *this;
    }

    /**
     * @brief 析构函数
     */
    ~multiset() = default;

    /**
     * @brief 获取起始迭代器
     * @return 指向最小元素的迭代器
     */
    NEFORCE_NODISCARD iterator begin() noexcept {
     return tree_.begin();
    }

    /**
     * @brief 获取结束迭代器
     * @return 指向最后一个元素之后位置的迭代器
     */
    NEFORCE_NODISCARD iterator end() noexcept {
     return tree_.end();
    }

    /**
     * @brief 获取常量起始迭代器
     * @return 指向最小元素的常量迭代器
     */
    NEFORCE_NODISCARD const_iterator cbegin() const noexcept {
     return tree_.cbegin();
    }

    /**
     * @brief 获取常量结束迭代器
     * @return 指向最后一个元素之后位置的常量迭代器
     */
    NEFORCE_NODISCARD const_iterator cend() const noexcept {
     return tree_.cend();
    }

    /**
     * @brief 获取反向起始迭代器
     * @return 指向最大元素的反向迭代器
     */
    NEFORCE_NODISCARD reverse_iterator rbegin() noexcept {
     return reverse_iterator(tree_.begin()); }

    /**
     * @brief 获取反向结束迭代器
     * @return 指向最小元素之前位置的反向迭代器
     */
    NEFORCE_NODISCARD reverse_iterator rend() noexcept {
     return reverse_iterator(tree_.end());
    }

    /**
     * @brief 获取常量反向起始迭代器
     * @return 指向最大元素的常量反向迭代器
     */
    NEFORCE_NODISCARD const_reverse_iterator crbegin() const noexcept {
     return const_reverse_iterator(tree_.cbegin());
    }

    /**
     * @brief 获取常量反向结束迭代器
     * @return 指向最小元素之前位置的常量反向迭代器
     */
    NEFORCE_NODISCARD const_reverse_iterator crend() const noexcept {
     return const_reverse_iterator(tree_.cend());
    }

    /**
     * @brief 获取元素数量
     * @return multiset中的元素数量
     */
    NEFORCE_NODISCARD size_type size() const noexcept {
     return tree_.size();
    }

    /**
     * @brief 获取最大可能大小
     * @return 最大元素数量
     */
    NEFORCE_NODISCARD size_type max_size() const noexcept {
     return tree_.max_size();
    }

    /**
     * @brief 检查是否为空
     * @return 是否为空
     */
    NEFORCE_NODISCARD bool empty() const noexcept {
     return tree_.empty();
    }

    /**
     * @brief 获取键比较函数对象
     * @return 键比较函数对象的副本
     */
    NEFORCE_NODISCARD key_compare key_comp() const noexcept {
     return tree_.key_comp();
    }

    /**
     * @brief 获取值比较函数对象
     * @return 值比较函数对象的副本
     */
    NEFORCE_NODISCARD value_compare value_comp() const noexcept {
     return value_compare(tree_.key_comp());
    }

    /**
     * @brief 在multiset中就地构造元素
     * @tparam Args 构造参数类型
     * @param args 构造参数
     * @return 指向插入元素的迭代器
     */
    template <typename... Args>
    iterator emplace(Args&&... args) {
        return tree_.emplace_equal(_NEFORCE forward<Args>(args)...);
    }

    /**
     * @brief 拷贝插入元素
     * @param value 要插入的值
     * @return 指向插入元素的迭代器
     */
    iterator insert(const value_type& value) {
        return tree_.insert_equal(value);
    }

    /**
     * @brief 移动插入元素
     * @param value 要插入的值
     * @return 指向插入元素的迭代器
     */
    iterator insert(value_type&& value) {
        return tree_.insert_equal(_NEFORCE move(value));
    }

    /**
     * @brief 在提示位置附近就地构造元素
     * @tparam Args 构造参数类型
     * @param position 插入位置提示
     * @param args 构造参数
     * @return 指向插入元素的迭代器
     */
    template <typename... Args>
    iterator emplace_hint(iterator position, Args&&... args) {
        return tree_.emplace_equal_hint(position, _NEFORCE forward<Args>(args)...);
    }

    /**
     * @brief 在提示位置附近拷贝插入元素
     * @param position 插入位置提示
     * @param value 要插入的值
     * @return 指向插入元素的迭代器
     */
    iterator insert(iterator position, const value_type& value) {
        return tree_.insert_equal(position, value);
    }

    /**
     * @brief 在提示位置附近移动插入元素
     * @param position 插入位置提示
     * @param value 要插入的值
     * @return 指向插入元素的迭代器
     */
    iterator insert(iterator position, value_type&& value) {
        return tree_.insert_equal(position, _NEFORCE move(value));
    }

    /**
     * @brief 范围插入元素
     * @tparam Iterator 迭代器类型
     * @param first 起始迭代器
     * @param last 结束迭代器
     */
    template <typename Iterator>
    void insert(Iterator first, Iterator last) {
        tree_.insert_equal(first, last);
    }

    /**
     * @brief 删除指定位置的元素
     * @param position 要删除的位置
     */
    void erase(iterator position)
    noexcept(noexcept(tree_.erase(position))) {
        tree_.erase(position);
    }

    /**
     * @brief 删除所有具有指定键的元素
     * @param key 要删除的键
     * @return 删除的元素数量
     */
    size_type erase(const key_type& key)
    noexcept(noexcept(tree_.erase(key))) {
        return tree_.erase(key);
    }

    /**
     * @brief 删除指定范围内的元素
     * @param first 起始迭代器
     * @param last 结束迭代器
     */
    void erase(iterator first, iterator last)
    noexcept(noexcept(tree_.erase(first, last))) {
        tree_.erase(first, last);
    }

    /**
     * @brief 清空multiset
     */
    void clear()
    noexcept(noexcept(tree_.clear())) {
        tree_.clear();
    }

    /**
     * @brief 查找具有指定键的元素
     * @param key 要查找的键
     * @return 指向第一个匹配元素的迭代器，未找到则返回end()
     */
    NEFORCE_NODISCARD iterator find(const key_type& key) { return tree_.find(key); }

    /**
     * @brief 查找具有指定键的元素（常量版本）
     * @param key 要查找的键
     * @return 指向第一个匹配元素的常量迭代器，未找到则返回cend()
     */
    NEFORCE_NODISCARD const_iterator find(const key_type& key) const { return tree_.find(key); }

    /**
     * @brief 统计具有指定键的元素数量
     * @param key 要统计的键
     * @return 匹配的元素数量
     */
    NEFORCE_NODISCARD size_type count(const key_type& key) const { return tree_.count(key); }

    /**
     * @brief 获取第一个不小于指定键的元素位置
     * @param key 键值
     * @return 指向第一个不小于key的元素的迭代器
     */
    NEFORCE_NODISCARD iterator lower_bound(const key_type& key) { return tree_.lower_bound(key); }

    /**
     * @brief 获取第一个不小于指定键的常量元素位置
     * @param key 键值
     * @return 指向第一个不小于key的元素的常量迭代器
     */
    NEFORCE_NODISCARD const_iterator lower_bound(const key_type& key) const { return tree_.lower_bound(key); }

    /**
     * @brief 获取第一个大于指定键的元素位置
     * @param key 键值
     * @return 指向第一个大于key的元素的迭代器
     */
    NEFORCE_NODISCARD iterator upper_bound(const key_type& key) { return tree_.upper_bound(key); }

    /**
     * @brief 获取第一个大于指定键的常量元素位置
     * @param key 键值
     * @return 指向第一个大于key的元素的常量迭代器
     */
    NEFORCE_NODISCARD const_iterator upper_bound(const key_type& key) const { return tree_.upper_bound(key); }

    /**
     * @brief 获取等于指定键的元素范围
     * @param key 键值
     * @return 包含lower_bound和upper_bound的pair
     */
    NEFORCE_NODISCARD pair<iterator, iterator> equal_range(const key_type& key) {
        return tree_.equal_range(key);
    }

    /**
     * @brief 获取等于指定键的常量元素范围
     * @param key 键值
     * @return 包含lower_bound和upper_bound的pair
     */
    NEFORCE_NODISCARD pair<const_iterator, const_iterator> equal_range(const key_type& key) const {
        return tree_.equal_range(key);
    }

    /**
     * @brief 交换两个multiset的内容
     * @param other 要交换的另一个multiset
     */
    void swap(multiset& other)
    noexcept(is_nothrow_swappable_v<base_type>) {
        tree_.swap(other.tree_);
    }

    /**
     * @brief 相等比较操作符
     * @param rhs 右侧multiset
     * @return 如果两个multiset大小相等且对应元素相等返回true
     */
    NEFORCE_NODISCARD bool operator ==(const multiset& rhs) const
    noexcept(noexcept(tree_ == rhs.tree_)) {
        return tree_ == rhs.tree_;
    }

    /**
     * @brief 小于比较操作符
     * @param rhs 右侧multiset
     * @return 按字典序比较结果
     */
    NEFORCE_NODISCARD bool operator <(const multiset& rhs) const
    noexcept(noexcept(tree_ < rhs.tree_)) {
        return tree_ < rhs.tree_;
    }
};

#ifdef NEFORCE_STANDARD_17
template <typename Iterator, typename Compare = less<iter_value_t<Iterator>>,
	typename Alloc = allocator<iter_value_t<Iterator>>>
multiset(Iterator, Iterator, Compare = Compare(), Alloc = Alloc())
-> multiset<iter_value_t<Iterator>, Compare, Alloc>;

template <typename Key, typename Compare = less<Key>, typename Alloc = allocator<Key>>
multiset(std::initializer_list<Key>, Compare = Compare(), Alloc = Alloc()) -> multiset<Key, Compare, Alloc>;

template <typename Iterator, typename Alloc>
multiset(Iterator, Iterator, Alloc) -> multiset<iter_value_t<Iterator>, less<iter_value_t<Iterator>>, Alloc>;

template <typename Key, typename Alloc>
multiset(std::initializer_list<Key>, Alloc) -> multiset<Key, less<Key>, Alloc>;
#endif

/** @} */ // Container

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_CORE_CONTAINER_MULTISET_HPP__
