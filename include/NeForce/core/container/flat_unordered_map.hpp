#ifndef NEFORCE_CORE_CONTAINER_FLAT_UNORDERED_MAP_HPP__
#define NEFORCE_CORE_CONTAINER_FLAT_UNORDERED_MAP_HPP__

/**
 * @file flat_unordered_map.hpp
 * @brief 平坦无序映射容器
 *
 * 此文件提供了平坦无序映射容器的实现。
 * flat_unordered_map 是一种基于开放寻址哈希表的关联容器，包含具有唯一键的键值对。
 * 元素无序存储，支持平均常数时间复杂度的查找、插入和删除操作。
 * 底层使用平坦哈希表实现。
 */

#include "NeForce/core/container/flat_hashtable.hpp"
NEFORCE_BEGIN_NAMESPACE__

/**
 * @defgroup Container 标准容器
 * @brief 支持标准算法的容器的实现
 * @{
 */

/**
 * @class flat_unordered_map
 * @brief 平坦无序映射容器
 * @tparam Key 键类型
 * @tparam T 值类型
 * @tparam HashFcn 哈希函数类型，默认为 hash<Key>
 * @tparam EqualKey 键相等比较函数类型，默认为 equal_to<Key>
 * @tparam Alloc 分配器类型
 *
 * flat_unordered_map 是一种关联容器，存储键值对（key-value pairs），每个键在容器中唯一。
 * 元素无序存储，由哈希函数将键映射到槽位。
 * 底层使用开放寻址平坦哈希表，提供优于链地址法的缓存局部性。
 *
 * @note 与 unordered_map 不同，本容器使用 pair<Key, T>（非 const Key）作为值类型，
 *       以适应平坦存储的元素搬移需求。
 */
template <typename Key, typename T, typename HashFcn = hash<Key>, typename EqualKey = equal_to<Key>,
          typename Alloc = allocator<pair<Key, T>>>
class flat_unordered_map : public icollector<flat_unordered_map<Key, T, HashFcn, EqualKey, Alloc>> {
    static_assert(is_hash_v<HashFcn, Key>, "flat_unordered_map requires valid hash function.");
    static_assert(is_allocator_v<Alloc>, "Alloc type is not a standard allocator type.");
    static_assert(is_object_v<Key>, "flat_unordered_map only contains object types.");

    using base_type = flat_hashtable<pair<Key, T>, Key, HashFcn, select1st<pair<Key, T>>, EqualKey,
                                     Alloc>; ///< 底层平坦哈希表类型

public:
    using key_type = typename base_type::key_type;   ///< 键类型
    using mapped_type = T;                           ///< 映射值类型
    using value_type = pair<Key, T>;                 ///< 值类型
    using hasher = typename base_type::hasher;       ///< 哈希函数类型
    using key_equal = typename base_type::key_equal; ///< 键相等比较函数类型

    using size_type = typename base_type::size_type;             ///< 大小类型
    using difference_type = typename base_type::difference_type; ///< 差值类型
    using pointer = value_type*;                                 ///< 指针类型
    using const_pointer = const value_type*;                     ///< 常量指针类型
    using reference = value_type&;                               ///< 引用类型
    using const_reference = const value_type&;                   ///< 常量引用类型
    using iterator = typename base_type::iterator;               ///< 迭代器类型
    using const_iterator = typename base_type::const_iterator;   ///< 常量迭代器类型
    using allocator_type = typename base_type::allocator_type;   ///< 分配器类型

private:
    base_type ht_; ///< 底层平坦哈希表实例

public:
    /**
     * @brief 默认构造函数
     *
     * 构造一个空 flat_unordered_map。
     */
    flat_unordered_map() = default;

    /**
     * @brief 构造函数，指定初始容量
     * @param n 初始容量提示
     */
    explicit flat_unordered_map(const size_type n) :
    ht_(n) {}

    /**
     * @brief 构造函数，指定初始容量和哈希函数
     * @param n 初始容量提示
     * @param hf 哈希函数
     */
    flat_unordered_map(const size_type n, const hasher& hf) :
    ht_(n, hf) {}

    /**
     * @brief 构造函数，指定初始容量、哈希函数和键相等比较函数
     * @param n 初始容量提示
     * @param hf 哈希函数
     * @param eql 键相等比较函数
     */
    flat_unordered_map(const size_type n, const hasher& hf, const key_equal& eql) :
    ht_(n, hf, eql) {}

    /**
     * @brief 拷贝构造函数
     * @param other 源 flat_unordered_map
     */
    flat_unordered_map(const flat_unordered_map& other) :
    ht_(other.ht_) {}

    /**
     * @brief 拷贝赋值运算符
     * @param other 源 flat_unordered_map
     * @return 自身引用
     */
    flat_unordered_map& operator=(const flat_unordered_map& other) {
        ht_ = other.ht_;
        return *this;
    }

    /**
     * @brief 移动构造函数
     * @param other 源 flat_unordered_map
     */
    flat_unordered_map(flat_unordered_map&& other) noexcept(is_nothrow_move_constructible_v<base_type>) :
    ht_(_NEFORCE move(other.ht_)) {}

    /**
     * @brief 移动赋值运算符
     * @param other 源 flat_unordered_map
     * @return 自身引用
     */
    flat_unordered_map& operator=(flat_unordered_map&& other) noexcept(is_nothrow_move_assignable_v<base_type>) {
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
    flat_unordered_map(Iterator first, Iterator last) :
    ht_() {
        ht_.insert_unique(first, last);
    }

    /**
     * @brief 范围构造函数，指定初始容量
     * @tparam Iterator 迭代器类型
     * @param first 起始迭代器
     * @param last 结束迭代器
     * @param n 初始容量提示
     */
    template <typename Iterator>
    flat_unordered_map(Iterator first, Iterator last, const size_type n) :
    ht_(n) {
        ht_.insert_unique(first, last);
    }

    /**
     * @brief 范围构造函数，指定初始容量和哈希函数
     * @tparam Iterator 迭代器类型
     * @param first 起始迭代器
     * @param last 结束迭代器
     * @param n 初始容量提示
     * @param hf 哈希函数
     */
    template <typename Iterator>
    flat_unordered_map(Iterator first, Iterator last, const size_type n, const hasher& hf) :
    ht_(n, hf) {
        ht_.insert_unique(first, last);
    }

    /**
     * @brief 范围构造函数，指定初始容量、哈希函数和键相等比较函数
     * @tparam Iterator 迭代器类型
     * @param first 起始迭代器
     * @param last 结束迭代器
     * @param n 初始容量提示
     * @param hf 哈希函数
     * @param eql 键相等比较函数
     */
    template <typename Iterator>
    flat_unordered_map(Iterator first, Iterator last, const size_type n, const hasher& hf, const key_equal& eql) :
    ht_(n, hf, eql) {
        ht_.insert_unique(first, last);
    }

    /**
     * @brief 初始化列表构造函数
     * @param ilist 初始化列表
     */
    flat_unordered_map(std::initializer_list<value_type> ilist) :
    flat_unordered_map(ilist.begin(), ilist.end()) {}

    /**
     * @brief 初始化列表构造函数，指定初始容量
     * @param ilist 初始化列表
     * @param n 初始容量提示
     */
    flat_unordered_map(std::initializer_list<value_type> ilist, const size_type n) :
    flat_unordered_map(ilist.begin(), ilist.end(), n) {}

    /**
     * @brief 初始化列表构造函数，指定初始容量和哈希函数
     * @param ilist 初始化列表
     * @param n 初始容量提示
     * @param hf 哈希函数
     */
    flat_unordered_map(std::initializer_list<value_type> ilist, const size_type n, const hasher& hf) :
    flat_unordered_map(ilist.begin(), ilist.end(), n, hf) {}

    /**
     * @brief 初始化列表构造函数，指定初始容量、哈希函数和键相等比较函数
     * @param ilist 初始化列表
     * @param n 初始容量提示
     * @param hf 哈希函数
     * @param eql 键相等比较函数
     */
    flat_unordered_map(std::initializer_list<value_type> ilist, const size_type n, const hasher& hf,
                       const key_equal& eql) :
    flat_unordered_map(ilist.begin(), ilist.end(), n, hf, eql) {}

    /**
     * @brief 获取起始迭代器
     * @return 指向第一个元素的迭代器
     */
    NEFORCE_NODISCARD iterator begin() noexcept { return ht_.begin(); }

    /**
     * @brief 获取结束迭代器
     * @return 指向末尾的迭代器
     */
    NEFORCE_NODISCARD iterator end() noexcept { return ht_.end(); }

    /**
     * @brief 获取常量起始迭代器
     * @return 指向第一个元素的常量迭代器
     */
    NEFORCE_NODISCARD const_iterator begin() const noexcept { return ht_.begin(); }

    /**
     * @brief 获取常量结束迭代器
     * @return 指向末尾的常量迭代器
     */
    NEFORCE_NODISCARD const_iterator end() const noexcept { return ht_.end(); }

    /**
     * @brief 获取常量起始迭代器
     * @return 指向第一个元素的常量迭代器
     */
    NEFORCE_NODISCARD const_iterator cbegin() const noexcept { return ht_.cbegin(); }

    /**
     * @brief 获取常量结束迭代器
     * @return 指向末尾的常量迭代器
     */
    NEFORCE_NODISCARD const_iterator cend() const noexcept { return ht_.cend(); }

    /**
     * @brief 获取元素数量
     * @return 元素数量
     */
    NEFORCE_NODISCARD size_type size() const noexcept { return ht_.size(); }

    /**
     * @brief 获取最大可能大小
     * @return 最大元素数量
     */
    NEFORCE_NODISCARD size_type max_size() const noexcept { return ht_.max_size(); }

    /**
     * @brief 检查是否为空
     * @return 是否为空
     */
    NEFORCE_NODISCARD bool empty() const noexcept { return ht_.empty(); }

    /**
     * @brief 获取容量
     * @return slot 总数
     */
    NEFORCE_NODISCARD size_type capacity() const noexcept { return ht_.capacity(); }

    /**
     * @brief 统计具有指定键的元素数量
     * @param key 要统计的键
     * @return 匹配的元素数量（0 或 1）
     */
    NEFORCE_NODISCARD size_type count(const key_type& key) const noexcept(noexcept(ht_.count(key))) {
        return ht_.count(key);
    }

    /**
     * @brief 检查是否包含指定键
     * @param key 要检查的键
     * @return 是否包含
     */
    NEFORCE_NODISCARD bool contains(const key_type& key) const noexcept(noexcept(ht_.contains(key))) {
        return ht_.contains(key);
    }

    /**
     * @brief 获取哈希函数对象
     * @return 哈希函数对象的副本
     */
    NEFORCE_NODISCARD hasher hash_function() const noexcept(noexcept(ht_.hash_function())) {
        return ht_.hash_function();
    }

    /**
     * @brief 获取键相等比较函数对象
     * @return 键相等比较函数对象的副本
     */
    NEFORCE_NODISCARD key_equal key_eql() const noexcept(noexcept(ht_.key_eql())) { return ht_.key_eql(); }

    /**
     * @brief 获取当前负载因子
     * @return 负载因子
     */
    NEFORCE_NODISCARD float load_factor() const noexcept { return ht_.load_factor(); }

    /**
     * @brief 获取最大负载因子
     * @return 最大负载因子
     */
    NEFORCE_NODISCARD float max_load_factor() const noexcept { return ht_.max_load_factor(); }

    /**
     * @brief 设置最大负载因子
     * @param lf 新的最大负载因子
     */
    void max_load_factor(const float lf) noexcept { ht_.max_load_factor(lf); }

    /**
     * @brief 重新哈希，调整容量
     * @param n 目标容量
     */
    void rehash(const size_type n) { ht_.rehash(n); }

    /**
     * @brief 预留空间
     * @param n 期望的元素数量
     *
     * 确保容器至少能容纳 n 个元素而不触发 rehash。
     */
    void reserve(const size_type n) { ht_.reserve(n); }

    /**
     * @brief 在容器中就地构造元素
     * @tparam Args 构造参数类型
     * @param args 构造参数
     * @return 插入结果（迭代器和是否成功）
     */
    template <typename... Args>
    pair<iterator, bool> emplace(Args&&... args) {
        return ht_.emplace_unique(_NEFORCE forward<Args>(args)...);
    }

    /**
     * @brief 插入元素（拷贝版本）
     * @param value 要插入的值
     * @return 插入结果（迭代器和是否成功）
     */
    pair<iterator, bool> insert(const value_type& value) { return ht_.insert_unique(value); }

    /**
     * @brief 移动插入元素
     * @param value 要插入的值
     * @return 插入结果（迭代器和是否成功）
     */
    pair<iterator, bool> insert(value_type&& value) { return ht_.insert_unique(_NEFORCE move(value)); }

    /**
     * @brief 范围插入元素
     * @tparam Iterator 迭代器类型
     * @param first 起始迭代器
     * @param last 结束迭代器
     */
    template <typename Iterator>
    void insert(Iterator first, Iterator last) {
        ht_.insert_unique(first, last);
    }

    /**
     * @brief 初始化列表插入元素
     * @param ilist 初始化列表
     */
    void insert(std::initializer_list<value_type> ilist) { ht_.insert_unique(ilist); }

    /**
     * @brief 删除所有具有指定键的元素
     * @param key 要删除的键
     * @return 删除的元素数量
     */
    size_type erase(const key_type& key) noexcept { return ht_.erase(key); }

    /**
     * @brief 删除指定位置的元素
     * @param position 要删除的位置
     * @return 指向被删除元素之后位置的迭代器
     */
    iterator erase(const iterator position) noexcept { return ht_.erase(position); }

    /**
     * @brief 删除指定范围内的元素
     * @param first 起始迭代器
     * @param last 结束迭代器
     * @return 指向最后一个被删除元素之后位置的迭代器
     */
    iterator erase(const iterator first, const iterator last) noexcept { return ht_.erase(first, last); }

    /**
     * @brief 删除指定位置的常量元素
     * @param position 要删除的位置
     * @return 指向被删除元素之后位置的常量迭代器
     */
    const_iterator erase(const const_iterator position) noexcept { return ht_.erase(position); }

    /**
     * @brief 删除指定范围内的常量元素
     * @param first 起始迭代器
     * @param last 结束迭代器
     * @return 指向最后一个被删除元素之后位置的常量迭代器
     */
    const_iterator erase(const const_iterator first, const const_iterator last) noexcept {
        return ht_.erase(first, last);
    }

    /**
     * @brief 清空容器
     */
    void clear() noexcept { ht_.clear(); }

    /**
     * @brief 查找具有指定键的元素
     * @param key 要查找的键
     * @return 指向第一个匹配元素的迭代器，未找到则返回 end()
     */
    NEFORCE_NODISCARD iterator find(const key_type& key) { return ht_.find(key); }

    /**
     * @brief 查找具有指定键的常量元素
     * @param key 要查找的键
     * @return 指向第一个匹配元素的常量迭代器，未找到则返回 cend()
     */
    NEFORCE_NODISCARD const_iterator find(const key_type& key) const { return ht_.find(key); }

    /**
     * @brief 获取等于指定键的元素范围
     * @param key 键值
     * @return 包含范围起始和结束的 pair
     */
    NEFORCE_NODISCARD pair<iterator, iterator> equal_range(const key_type& key) { return ht_.equal_range(key); }

    /**
     * @brief 获取等于指定键的常量元素范围
     * @param key 键值
     * @return 包含范围起始和结束的 pair
     */
    NEFORCE_NODISCARD pair<const_iterator, const_iterator> equal_range(const key_type& key) const {
        return ht_.equal_range(key);
    }

    /**
     * @brief 下标访问操作符
     * @param key 要访问的键
     * @return 对应键的值的引用
     *
     * 如果键不存在，则插入一个默认构造的值并返回其引用。
     */
    NEFORCE_NODISCARD T& operator[](const key_type& key) {
        auto iter = ht_.find(key);
        if (iter == ht_.end()) {
            iter = ht_.emplace_unique(key, T()).first;
        }
        return iter->second;
    }

    /**
     * @brief 带边界检查的常量访问
     * @param key 要访问的键
     * @return 对应键的值的常量引用
     * @throws iterator_exception 如果键不存在
     */
    NEFORCE_NODISCARD const T& at(const key_type& key) const {
        auto iter = ht_.find(key);
        if (iter == end()) {
            NEFORCE_THROW_EXCEPTION(iterator_exception("flat_unordered_map key-value does not exist"));
        }
        return iter->second;
    }

    /**
     * @brief 带边界检查的访问
     * @param key 要访问的键
     * @return 对应键的值的引用
     * @throws iterator_exception 如果键不存在
     */
    NEFORCE_NODISCARD T& at(const key_type& key) {
        auto iter = ht_.find(key);
        if (iter == end()) {
            NEFORCE_THROW_EXCEPTION(iterator_exception("flat_unordered_map key-value does not exist"));
        }
        return iter->second;
    }

    /**
     * @brief 交换两个容器
     * @param other 要交换的容器
     */
    void swap(flat_unordered_map& other) noexcept(is_nothrow_swappable_v<base_type>) { ht_.swap(other.ht_); }

    /**
     * @brief 相等比较操作符
     * @param rhs 右侧容器
     * @return 是否相等
     */
    NEFORCE_NODISCARD bool equal_to(const flat_unordered_map& rhs) const noexcept(noexcept(ht_ == rhs.ht_)) {
        return ht_ == rhs.ht_;
    }

    /**
     * @brief 小于比较操作符
     * @param rhs 右侧容器
     * @return 按字典序比较结果
     */
    NEFORCE_NODISCARD bool less_than(const flat_unordered_map& rhs) const noexcept(noexcept(ht_ < rhs.ht_)) {
        return ht_ < rhs.ht_;
    }
};

#ifdef NEFORCE_STANDARD_17
template <typename Iterator, typename HashFcn = hash<iter_map_key_t<Iterator>>,
          typename Compare = equal_to<iter_map_key_t<Iterator>>, typename Alloc>
flat_unordered_map(Iterator, Iterator, HashFcn = HashFcn(), Compare = Compare(), Alloc = Alloc())
        -> flat_unordered_map<iter_map_key_t<Iterator>, iter_map_value_t<Iterator>, HashFcn, Compare, Alloc>;

template <typename Key, typename T, typename HashFcn = hash<Key>, typename Compare = equal_to<Key>,
          typename Alloc = allocator<pair<Key, T>>>
flat_unordered_map(std::initializer_list<pair<Key, T>>, HashFcn = HashFcn(), Compare = Compare(), Alloc = Alloc())
        -> flat_unordered_map<Key, T, HashFcn, Compare, Alloc>;

template <typename Iterator, typename Alloc>
flat_unordered_map(Iterator, Iterator, Alloc)
        -> flat_unordered_map<iter_map_key_t<Iterator>, iter_map_value_t<Iterator>, hash<iter_map_key_t<Iterator>>,
                              equal_to<iter_map_key_t<Iterator>>, Alloc>;

template <typename Iterator, typename HashFcn, typename Alloc>
flat_unordered_map(Iterator, Iterator, HashFcn, Alloc)
        -> flat_unordered_map<iter_map_key_t<Iterator>, iter_map_value_t<Iterator>, HashFcn,
                              equal_to<iter_map_key_t<Iterator>>, Alloc>;

template <typename Key, typename T, typename Alloc>
flat_unordered_map(std::initializer_list<pair<Key, T>>, Alloc)
        -> flat_unordered_map<Key, T, hash<Key>, equal_to<Key>, Alloc>;

template <typename Key, typename T, typename HashFcn, typename Alloc>
flat_unordered_map(std::initializer_list<pair<Key, T>>, HashFcn, Alloc)
        -> flat_unordered_map<Key, T, HashFcn, equal_to<Key>, Alloc>;
#endif

/** @} */ // Container

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_CORE_CONTAINER_FLAT_UNORDERED_MAP_HPP__
