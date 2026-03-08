#ifndef NEFORCE_CORE_CONTAINER_UNORDERED_MULTIMAP_HPP__
#define NEFORCE_CORE_CONTAINER_UNORDERED_MULTIMAP_HPP__

/**
 * @file unordered_multimap.hpp
 * @brief 无序多重映射容器
 *
 * 此文件提供了无序多重映射容器的实现。
 * unordered_multimap是一种基于哈希表的关联容器，包含键值对，允许重复键。
 * 元素无序存储，支持平均常数时间复杂度的查找、插入和删除操作。
 * 底层使用哈希表实现。
 */

#include "NeForce/core/container/hashtable.hpp"
NEFORCE_BEGIN_NAMESPACE__

/**
 * @defgroup UnorderedMultiMap 无序多重映射
 * @brief 基于哈希表的键值对容器
 * @{
 */

/**
 * @class unordered_multimap
 * @brief 无序多重映射容器
 * @tparam Key 键类型
 * @tparam T 值类型
 * @tparam HashFcn 哈希函数类型，默认为hash<Key>
 * @tparam EqualKey 键相等比较函数类型，默认为equal_to<Key>
 * @tparam Alloc 分配器类型
 *
 * unordered_multimap是一种关联容器，存储键值对（key-value pairs），允许重复键。
 * 元素无序存储，由哈希函数将键映射到桶中。
 * 支持快速的键查找，且允许重复键。底层使用哈希表实现。
 */
template <typename Key, typename T, typename HashFcn = hash<Key>, typename EqualKey = equal_to<Key>,
    typename Alloc = allocator<hashtable_node<pair<const Key, T>>>>
class unordered_multimap : icollector<unordered_multimap<Key, T, HashFcn, EqualKey, Alloc>> {
    static_assert(is_hash_v<HashFcn, Key>, "unordered multimap requires valid hash function.");
    static_assert(is_allocator_v<Alloc>, "Alloc type is not a standard allocator type.");
    static_assert(
        is_same_v<hashtable_node<pair<const Key, T>>, typename Alloc::value_type>,
        "allocator type mismatch.");
    static_assert(is_object_v<Key>, "unordered multimap only contains object types.");

private:
    using base_type = hashtable<pair<const Key, T>, Key, HashFcn, select1st<pair<const Key, T>>, EqualKey, Alloc>;  ///< 底层哈希表类型

public:
    using key_type          = typename base_type::key_type;  ///< 键类型
    using value_type        = typename base_type::value_type;  ///< 值类型
    using hasher            = typename base_type::hasher;  ///< 哈希函数类型
    using key_equal         = typename base_type::key_equal;  ///< 键相等比较函数类型

    using size_type         = typename base_type::size_type;  ///< 大小类型
    using difference_type   = typename base_type::difference_type;  ///< 差值类型
    using pointer           = typename base_type::pointer;  ///< 指针类型
    using const_pointer     = typename base_type::const_pointer;  ///< 常量指针类型
    using reference         = typename base_type::reference;  ///< 引用类型
    using const_reference   = typename base_type::const_reference;  ///< 常量引用类型
    using iterator          = typename base_type::iterator;  ///< 迭代器类型
    using const_iterator    = typename base_type::const_iterator;  ///< 常量迭代器类型
    using allocator_type    = typename base_type::allocator_type;  ///< 分配器类型

private:
    base_type ht_{100};  ///< 底层哈希表实例，默认初始桶数为100

public:
    /**
     * @brief 默认构造函数
     *
     * 构造一个空unordered_multimap。
     */
    unordered_multimap() = default;

    /**
     * @brief 构造函数，指定初始桶数
     * @param n 初始桶数
     */
    explicit unordered_multimap(size_type n)
    : ht_(n) {}

    /**
     * @brief 构造函数，指定初始桶数和哈希函数
     * @param n 初始桶数
     * @param hf 哈希函数
     */
    unordered_multimap(size_type n, const hasher& hf)
    : ht_(n, hf, key_equal()) {}

    /**
     * @brief 构造函数，指定初始桶数、哈希函数和键相等比较函数
     * @param n 初始桶数
     * @param hf 哈希函数
     * @param eql 键相等比较函数
     */
    unordered_multimap(size_type n, const hasher& hf, const key_equal& eql)
    : ht_(n, hf, eql) {}

    /**
     * @brief 拷贝构造函数
     * @param other 源unordered_multimap
     */
    unordered_multimap(const unordered_multimap& other)
    : ht_(other.ht_) {}

    /**
     * @brief 拷贝赋值运算符
     * @param other 源unordered_multimap
     * @return 自身引用
     */
    unordered_multimap& operator =(const unordered_multimap& other) {
        ht_ = other.ht_;
        return *this;
    }

    /**
     * @brief 移动构造函数
     * @param other 源unordered_multimap
     */
    unordered_multimap(unordered_multimap&& other)
    noexcept(is_nothrow_move_constructible_v<base_type>)
    : ht_(_NEFORCE move(other.ht_)) {}

    /**
     * @brief 移动赋值运算符
     * @param other 源unordered_multimap
     * @return 自身引用
     */
    unordered_multimap& operator =(unordered_multimap&& other)
    noexcept(is_nothrow_move_assignable_v<base_type>) {
        ht_ = _NEFORCE move(other.ht_);
        return *this;
    }

    /**
     * @brief 范围构造函数
     * @tparam Iterator 迭代器类型
     * @param first 起始迭代器
     * @param last 结束迭代器
     */
    template <typename Iterator>
    unordered_multimap(Iterator first, Iterator last)
    : ht_(100, hasher(), key_equal()) {
        ht_.insert_equal(first, last);
    }

    /**
     * @brief 范围构造函数，指定初始桶数
     * @tparam Iterator 迭代器类型
     * @param first 起始迭代器
     * @param list 结束迭代器
     * @param n 初始桶数
     */
    template <typename Iterator>
    unordered_multimap(Iterator first, Iterator list, size_type n)
    : ht_(n, hasher(), key_equal()) {
        ht_.insert_equal(first, list);
    }

    /**
     * @brief 范围构造函数，指定初始桶数和哈希函数
     * @tparam Iterator 迭代器类型
     * @param first 起始迭代器
     * @param last 结束迭代器
     * @param n 初始桶数
     * @param hf 哈希函数
     */
    template <typename Iterator>
    unordered_multimap(Iterator first, Iterator last, size_type n, const hasher& hf)
    : ht_(n, hf, key_equal()) {
        ht_.insert_equal(first, last);
    }

    /**
     * @brief 范围构造函数，指定初始桶数、哈希函数和键相等比较函数
     * @tparam Iterator 迭代器类型
     * @param first 起始迭代器
     * @param last 结束迭代器
     * @param n 初始桶数
     * @param hf 哈希函数
     * @param eql 键相等比较函数
     */
    template <typename Iterator>
    unordered_multimap(Iterator first, Iterator last, size_type n, const hasher& hf, const key_equal& eql)
    : ht_(n, hf, eql) {
        ht_.insert_equal(first, last);
    }

    /**
     * @brief 初始化列表构造函数
     * @param ilist 初始化列表
     */
    unordered_multimap(std::initializer_list<value_type> ilist)
    : unordered_multimap(ilist.begin(), ilist.end()) {}

    /**
     * @brief 初始化列表构造函数，指定初始桶数
     * @param ilist 初始化列表
     * @param n 初始桶数
     */
    unordered_multimap(std::initializer_list<value_type> ilist, size_type n)
    : unordered_multimap(ilist.begin(), ilist.end(), n) {}

    /**
     * @brief 初始化列表构造函数，指定初始桶数和哈希函数
     * @param ilist 初始化列表
     * @param n 初始桶数
     * @param hf 哈希函数
     */
    unordered_multimap(std::initializer_list<value_type> ilist, size_type n, const hasher& hf)
    : unordered_multimap(ilist.begin(), ilist.end(), n, hf) {}

    /**
     * @brief 初始化列表构造函数，指定初始桶数、哈希函数和键相等比较函数
     * @param ilist 初始化列表
     * @param n 初始桶数
     * @param hf 哈希函数
     * @param eql 键相等比较函数
     */
    unordered_multimap(std::initializer_list<value_type> ilist, size_type n, const hasher& hf, const key_equal& eql)
    : unordered_multimap(ilist.begin(), ilist.end(), n, hf, eql) {}

    /**
     * @brief 获取起始迭代器
     * @return 指向第一个元素的迭代器
     */
    NEFORCE_NODISCARD iterator begin() noexcept {
        return ht_.begin();
    }

    /**
     * @brief 获取结束迭代器
     * @return 指向最后一个元素之后位置的迭代器
     */
    NEFORCE_NODISCARD iterator end() noexcept {
        return ht_.end();
    }

    /**
     * @brief 获取常量起始迭代器
     * @return 指向第一个元素的常量迭代器
     */
    NEFORCE_NODISCARD const_iterator begin() const noexcept {
        return ht_.begin();
    }

    /**
     * @brief 获取常量结束迭代器
     * @return 指向最后一个元素之后位置的常量迭代器
     */
    NEFORCE_NODISCARD const_iterator end() const noexcept {
        return ht_.end();
    }

    /**
     * @brief 获取常量起始迭代器
     * @return 指向第一个元素的常量迭代器
     */
    NEFORCE_NODISCARD const_iterator cbegin() const noexcept {
        return ht_.cbegin();
    }

    /**
     * @brief 获取常量结束迭代器
     * @return 指向最后一个元素之后位置的常量迭代器
     */
    NEFORCE_NODISCARD const_iterator cend() const noexcept {
        return ht_.cend();
    }

    /**
     * @brief 获取元素数量
     * @return unordered_multimap中的元素数量
     */
    NEFORCE_NODISCARD size_type size() const noexcept {
        return ht_.size();
    }

    /**
     * @brief 获取最大可能大小
     * @return 最大元素数量
     */
    NEFORCE_NODISCARD size_type max_size() const noexcept {
        return ht_.max_size();
    }

    /**
     * @brief 检查是否为空
     * @return 是否为空
     */
    NEFORCE_NODISCARD bool empty() const noexcept {
        return ht_.empty();
    }

    /**
     * @brief 统计具有指定键的元素数量
     * @param key 要统计的键
     * @return 匹配的元素数量
     */
    NEFORCE_NODISCARD size_type count(const key_type& key) const
    noexcept(noexcept(ht_.count(key))) {
        return ht_.count(key);
    }

    /**
     * @brief 获取桶数量
     * @return 桶数量
     */
    NEFORCE_NODISCARD size_type buckets_size() const noexcept {
        return ht_.buckets_size();
    }

    /**
     * @brief 获取最大桶数量
     * @return 最大桶数量
     */
    NEFORCE_NODISCARD size_type buckets_max_count() const noexcept {
        return ht_.buckets_max_count();
    }

    /**
     * @brief 获取指定桶的大小
     * @param n 桶索引
     * @return 桶中的元素数量
     */
    NEFORCE_NODISCARD size_type bucket_size(size_type n) const noexcept {
        return ht_.bucket_size(n);
    }

    /**
     * @brief 获取哈希函数对象
     * @return 哈希函数对象的副本
     */
    NEFORCE_NODISCARD hasher hash_func() const
    noexcept(noexcept(ht_.hash_func())) {
        return ht_.hash_func();
    }

    /**
     * @brief 获取键相等比较函数对象
     * @return 键相等比较函数对象的副本
     */
    NEFORCE_NODISCARD key_equal key_eql() const
    noexcept(noexcept(ht_.key_eql())) {
        return ht_.key_eql();
    }

    /**
     * @brief 获取当前负载因子
     * @return 负载因子
     */
    NEFORCE_NODISCARD float load_factor() const noexcept {
        return ht_.load_factor();
    }

    /**
     * @brief 获取最大负载因子
     * @return 最大负载因子
     */
    NEFORCE_NODISCARD float max_load_factor() const noexcept {
        return ht_.max_load_factor();
    }

    /**
     * @brief 设置最大负载因子
     * @param lf 新的最大负载因子
     */
    void max_load_factor(float lf) noexcept {
        ht_.max_load_factor(lf);
    }

    /**
     * @brief 重新哈希，调整桶数量
     * @param n 目标桶数量
     */
    void rehash(size_type n) {
        ht_.rehash(n);
    }

    /**
     * @brief 预留空间
     * @param n 期望的元素数量
     *
     * 确保unordered_multimap至少能容纳n个元素而不触发rehash。
     */
    void reserve(size_type n) {
        ht_.reserve(n);
    }

    /**
     * @brief 在unordered_multimap中就地构造元素
     * @tparam Args 构造参数类型
     * @param args 构造参数
     * @return 指向插入元素的迭代器
     */
    template <typename... Args>
    iterator emplace(Args&&... args) {
        return ht_.emplace_equal(_NEFORCE forward<Args>(args)...);
    }

    /**
     * @brief 拷贝插入元素
     * @param value 要插入的值
     * @return 指向插入元素的迭代器
     */
    iterator insert(const value_type& value) {
        return ht_.insert_equal(value);
    }

    /**
     * @brief 移动插入元素
     * @param value 要插入的值
     * @return 指向插入元素的迭代器
     */
    iterator insert(value_type&& value) {
        return ht_.insert_equal(_NEFORCE move(value));
    }

    /**
     * @brief 范围插入元素
     * @tparam Iterator 迭代器类型
     * @param first 起始迭代器
     * @param last 结束迭代器
     */
    template <typename Iterator>
    void insert(Iterator first, Iterator last) {
        ht_.insert_equal(first, last);
    }

    /**
     * @brief 删除所有具有指定键的元素
     * @param key 要删除的键
     * @return 删除的元素数量
     */
    size_type erase(const key_type& key) noexcept {
        return ht_.erase(key);
    }

    /**
     * @brief 删除指定位置的元素
     * @param position 要删除的位置
     * @return 指向被删除元素之后位置的迭代器
     */
    iterator erase(iterator position) noexcept {
        return ht_.erase(position);
    }

    /**
     * @brief 删除指定范围内的元素
     * @param first 起始迭代器
     * @param last 结束迭代器
     * @return 指向最后一个被删除元素之后位置的迭代器
     */
    iterator erase(iterator first, iterator last) noexcept {
        return ht_.erase(first, last);
    }

    /**
     * @brief 删除指定位置的常量元素
     * @param position 要删除的位置
     * @return 指向被删除元素之后位置的常量迭代器
     */
    const_iterator erase(const_iterator position) noexcept {
        return ht_.erase(position);
    }

    /**
     * @brief 删除指定范围内的常量元素
     * @param first 起始迭代器
     * @param last 结束迭代器
     * @return 指向最后一个被删除元素之后位置的常量迭代器
     */
    const_iterator erase(const_iterator first, const_iterator last) noexcept {
        return ht_.erase(first, last);
    }

    /**
     * @brief 清空unordered_multimap
     */
    void clear() noexcept {
        ht_.clear();
    }

    /**
     * @brief 查找具有指定键的元素
     * @param key 要查找的键
     * @return 指向第一个匹配元素的迭代器，未找到则返回end()
     */
    NEFORCE_NODISCARD iterator find(const key_type& key) {
        return ht_.find(key);
    }

    /**
     * @brief 查找具有指定键的常量元素
     * @param key 要查找的键
     * @return 指向第一个匹配元素的常量迭代器，未找到则返回cend()
     */
    NEFORCE_NODISCARD const_iterator find(const key_type& key) const {
        return ht_.find(key);
    }

    /**
     * @brief 获取等于指定键的元素范围
     * @param key 键值
     * @return 包含范围起始和结束的pair
     */
    NEFORCE_NODISCARD pair<iterator, iterator> equal_range(const key_type& key) {
        return ht_.equal_range(key);
    }

    /**
     * @brief 获取等于指定键的常量元素范围
     * @param key 键值
     * @return 包含范围起始和结束的pair
     */
    NEFORCE_NODISCARD pair<const_iterator, const_iterator> equal_range(const key_type& key) const {
        return ht_.equal_range(key);
    }

    /**
     * @brief 交换两个unordered_multimap的内容
     * @param other 要交换的另一个unordered_multimap
     */
    void swap(unordered_multimap& other)
    noexcept(is_nothrow_swappable_v<base_type>) {
        ht_.swap(other.ht_);
    }

    /**
     * @brief 相等比较操作符
     * @param rhs 右侧unordered_multimap
     * @return 如果两个unordered_multimap大小相等且对应元素相等返回true
     */
    NEFORCE_NODISCARD bool operator ==(const unordered_multimap& rhs) const
    noexcept(noexcept(ht_ == rhs.ht_)) {
        return ht_ == rhs.ht_;
    }

    /**
     * @brief 小于比较操作符
     * @param rhs 右侧unordered_multimap
     * @return 按字典序比较结果
     */
    NEFORCE_NODISCARD bool operator <(const unordered_multimap& rhs) const
    noexcept(noexcept(ht_ < rhs.ht_)) {
        return ht_ < rhs.ht_;
    }
};

#ifdef NEFORCE_STANDARD_17
template <typename Iterator, typename HashFcn = hash<iter_map_key_t<Iterator>>,
    typename Compare = equal_to<iter_map_key_t<Iterator>>, typename Alloc>
unordered_multimap(Iterator, Iterator, HashFcn = HashFcn(), Compare = Compare(), Alloc = Alloc())
-> unordered_multimap<iter_map_key_t<Iterator>, iter_map_value_t<Iterator>, HashFcn, Compare, Alloc>;

template <typename Key, typename T, typename HashFcn = hash<Key>, typename Compare = equal_to<Key>,
    typename Alloc = allocator<pair<const Key, T>>>
unordered_multimap(std::initializer_list<pair<Key, T>>, HashFcn = HashFcn(), Compare = Compare(), Alloc = Alloc())
-> unordered_multimap<Key, T, HashFcn, Compare, Alloc>;

template <typename Iterator, typename Alloc>
unordered_multimap(Iterator, Iterator, Alloc) -> unordered_multimap<iter_map_key_t<Iterator>, iter_map_value_t<Iterator>,
    hash<iter_map_key_t<Iterator>>, equal_to<iter_map_key_t<Iterator>>, Alloc>;

template <typename Iterator, typename HashFcn, typename Alloc>
unordered_multimap(Iterator, Iterator, HashFcn, Alloc) -> unordered_multimap<iter_map_key_t<Iterator>,
    iter_map_value_t<Iterator>, HashFcn, equal_to<iter_map_key_t<Iterator>>, Alloc>;

template <typename Key, typename T, typename Alloc>
unordered_multimap(std::initializer_list<pair<Key, T>>, Alloc)
-> unordered_multimap<Key, T, hash<Key>, equal_to<Key>, Alloc>;

template <typename Key, typename T, typename HashFcn, typename Alloc>
unordered_multimap(std::initializer_list<pair<Key, T>>, HashFcn, Alloc)
-> unordered_multimap<Key, T, HashFcn, equal_to<Key>, Alloc>;
#endif

/** @} */ // UnorderedMultiMap

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_CORE_CONTAINER_UNORDERED_MULTIMAP_HPP__
