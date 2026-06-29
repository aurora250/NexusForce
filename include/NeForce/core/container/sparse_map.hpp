#ifndef NEFORCE_CORE_CONTAINER_SPARSE_MAP_HPP__
#define NEFORCE_CORE_CONTAINER_SPARSE_MAP_HPP__

/**
 * @file sparse_map.hpp
 * @brief 稀疏映射容器
 *
 * 此文件提供了稀疏映射容器的实现。
 * sparse_map是一种有序关联容器，包含具有唯一键的键值对。
 * 键按严格弱序排序，支持对数时间复杂度的查找操作。
 * 底层使用有序平坦数组实现（sparse_vector），迭代性能优异。
 *
 * @note 与红黑树map不同，sparse_map底层使用平坦数组存储，
 *       因此value_type使用pair<Key, T>（非const Key）以保证元素可移动。
 * @note 适用于查找频繁、修改较少且需要有序遍历的场景。
 * @warning 插入和删除操作为O(n)，可能使所有迭代器失效。
 */

#include "NeForce/core/container/sparse_vector.hpp"
NEFORCE_BEGIN_NAMESPACE__

/**
 * @defgroup Container 标准容器
 * @brief 支持标准算法的容器的实现
 * @{
 */

/**
 * @class sparse_map
 * @brief 稀疏映射容器
 * @tparam Key 键类型
 * @tparam T 值类型
 * @tparam Compare 键比较函数类型，默认为less<Key>
 * @tparam Alloc 分配器类型
 *
 * sparse_map是一种关联容器，存储键值对（key-value pairs），每个键在容器中唯一。
 * 元素按照键的顺序自动排序，排序标准由Compare函数对象指定。
 * 底层使用有序平坦数组实现，查找O(log n)，迭代缓存友好。
 */
template <typename Key, typename T, typename Compare = less<Key>, typename Alloc = allocator<pair<Key, T>>>
class sparse_map : public icollector<sparse_map<Key, T, Compare, Alloc>> {
    static_assert(is_allocator_v<Alloc>, "Alloc type is not a standard allocator type.");
    static_assert(is_same_v<pair<Key, T>, typename Alloc::value_type>, "allocator type mismatch.");
    static_assert(is_object_v<T>, "sparse_map only contains object types.");

public:
    using key_type = Key;            ///< 键类型
    using data_type = T;             ///< 数据类型
    using mapped_type = T;           ///< 映射值类型
    using value_type = pair<Key, T>; ///< 值类型
    using key_compare = Compare;     ///< 键比较函数类型

    /**
     * @struct value_compare
     * @brief 值比较函数对象
     *
     * 用于比较两个键值对，实际是比较它们的键。
     */
    struct value_compare {
    private:
        Compare comp_; ///< 键比较函数
        friend class sparse_map;

    public:
        explicit value_compare(Compare comp) :
        comp_(comp) {}

        /**
         * @brief 比较两个键值对
         * @param lhs 左侧值
         * @param rhs 右侧值
         * @return 如果lhs的键小于rhs的键返回true
         */
        bool operator()(const value_type& lhs, const value_type& rhs) const noexcept {
            return comp_(lhs.first, rhs.first);
        }
    };

private:
    using base_type = sparse_vector<Key, pair<Key, T>, select1st<pair<Key, T>>, Compare, Alloc>; ///< 底层稀疏向量类型

public:
    using size_type = typename base_type::size_type;                           ///< 大小类型
    using difference_type = typename base_type::difference_type;               ///< 差值类型
    using pointer = typename base_type::pointer;                               ///< 指针类型
    using const_pointer = typename base_type::const_pointer;                   ///< 常量指针类型
    using reference = typename base_type::reference;                           ///< 引用类型
    using const_reference = typename base_type::const_reference;               ///< 常量引用类型
    using iterator = typename base_type::iterator;                             ///< 迭代器类型
    using const_iterator = typename base_type::const_iterator;                 ///< 常量迭代器类型
    using reverse_iterator = typename base_type::reverse_iterator;             ///< 反向迭代器类型
    using const_reverse_iterator = typename base_type::const_reverse_iterator; ///< 常量反向迭代器类型
    using allocator_type = typename base_type::allocator_type;                 ///< 分配器类型

private:
    base_type data_; ///< 底层稀疏向量实例

public:
    /**
     * @brief 默认构造函数
     *
     * 构造一个空sparse_map，使用默认的比较函数。
     */
    sparse_map() :
    data_(Compare()) {}

    /**
     * @brief 构造函数，指定比较函数
     * @param comp 比较函数对象
     */
    explicit sparse_map(const key_compare& comp) :
    data_(comp) {}

    /**
     * @brief 拷贝构造函数
     * @param other 源sparse_map
     */
    sparse_map(const sparse_map& other) :
    data_(other.data_) {}

    /**
     * @brief 拷贝赋值运算符
     * @param other 源sparse_map
     * @return 自身引用
     */
    sparse_map& operator=(const sparse_map& other) {
        if (_NEFORCE addressof(other) == this) {
            return *this;
        }
        data_ = other.data_;
        return *this;
    }

    /**
     * @brief 移动构造函数
     * @param other 源sparse_map
     */
    sparse_map(sparse_map&& other) noexcept(is_nothrow_move_constructible_v<base_type>) :
    data_(_NEFORCE move(other.data_)) {}

    /**
     * @brief 移动赋值运算符
     * @param other 源sparse_map
     * @return 自身引用
     */
    sparse_map& operator=(sparse_map&& other) noexcept(is_nothrow_move_assignable_v<base_type>) {
        if (_NEFORCE addressof(other) == this) {
            return *this;
        }
        data_ = _NEFORCE move(other.data_);
        return *this;
    }

    /**
     * @brief 范围构造函数
     * @tparam Iterator 迭代器类型
     * @param first 起始迭代器
     * @param last 结束迭代器
     */
    template <typename Iterator>
    sparse_map(Iterator first, Iterator last) :
    data_(Compare()) {
        data_.insert_unique(first, last);
    }

    /**
     * @brief 范围构造函数，指定比较函数
     * @tparam Iterator 迭代器类型
     * @param first 起始迭代器
     * @param last 结束迭代器
     * @param comp 比较函数对象
     */
    template <typename Iterator>
    sparse_map(Iterator first, Iterator last, const key_compare& comp) :
    data_(comp) {
        data_.insert_unique(first, last);
    }

    /**
     * @brief 初始化列表构造函数
     * @param ilist 初始化列表
     */
    sparse_map(std::initializer_list<value_type> ilist) :
    sparse_map(ilist.begin(), ilist.end()) {}

    /**
     * @brief 初始化列表构造函数，指定比较函数
     * @param ilist 初始化列表
     * @param comp 比较函数对象
     */
    sparse_map(std::initializer_list<value_type> ilist, const key_compare& comp) :
    sparse_map(ilist.begin(), ilist.end(), comp) {}

    /**
     * @brief 初始化列表赋值运算符
     * @param ilist 初始化列表
     * @return 自身引用
     */
    sparse_map& operator=(std::initializer_list<value_type> ilist) {
        clear();
        insert(ilist.begin(), ilist.end());
        return *this;
    }

    /**
     * @brief 析构函数
     */
    ~sparse_map() = default;

    /**
     * @brief 获取起始迭代器
     * @return 指向第一个元素的迭代器
     */
    NEFORCE_NODISCARD iterator begin() noexcept { return data_.begin(); }

    /**
     * @brief 获取结束迭代器
     * @return 指向最后一个元素之后位置的迭代器
     */
    NEFORCE_NODISCARD iterator end() noexcept { return data_.end(); }

    /**
     * @brief 获取常量起始迭代器
     * @return 指向第一个元素的常量迭代器
     */
    NEFORCE_NODISCARD const_iterator begin() const noexcept { return data_.cbegin(); }

    /**
     * @brief 获取常量结束迭代器
     * @return 指向最后一个元素之后位置的常量迭代器
     */
    NEFORCE_NODISCARD const_iterator end() const noexcept { return data_.cend(); }

    /**
     * @brief 获取常量起始迭代器
     * @return 指向第一个元素的常量迭代器
     */
    NEFORCE_NODISCARD const_iterator cbegin() const noexcept { return data_.cbegin(); }

    /**
     * @brief 获取常量结束迭代器
     * @return 指向最后一个元素之后位置的常量迭代器
     */
    NEFORCE_NODISCARD const_iterator cend() const noexcept { return data_.cend(); }

    /**
     * @brief 获取反向起始迭代器
     * @return 指向最后一个元素的反向迭代器
     */
    NEFORCE_NODISCARD reverse_iterator rbegin() noexcept { return data_.rbegin(); }

    /**
     * @brief 获取反向结束迭代器
     * @return 指向第一个元素之前位置的反向迭代器
     */
    NEFORCE_NODISCARD reverse_iterator rend() noexcept { return data_.rend(); }

    /**
     * @brief 获取常量反向起始迭代器
     * @return 指向最后一个元素的常量反向迭代器
     */
    NEFORCE_NODISCARD const_reverse_iterator rbegin() const noexcept { return data_.rbegin(); }

    /**
     * @brief 获取常量反向结束迭代器
     * @return 指向第一个元素之前位置的常量反向迭代器
     */
    NEFORCE_NODISCARD const_reverse_iterator rend() const noexcept { return data_.rend(); }

    /**
     * @brief 获取常量反向起始迭代器
     * @return 指向最后一个元素的常量反向迭代器
     */
    NEFORCE_NODISCARD const_reverse_iterator crbegin() const noexcept { return data_.crbegin(); }

    /**
     * @brief 获取常量反向结束迭代器
     * @return 指向第一个元素之前位置的常量反向迭代器
     */
    NEFORCE_NODISCARD const_reverse_iterator crend() const noexcept { return data_.crend(); }

    /**
     * @brief 获取元素数量
     * @return sparse_map中的元素数量
     */
    NEFORCE_NODISCARD size_type size() const noexcept { return data_.size(); }

    /**
     * @brief 获取最大可能大小
     * @return 最大元素数量
     */
    NEFORCE_NODISCARD size_type max_size() const noexcept { return data_.max_size(); }

    /**
     * @brief 检查是否为空
     * @return 是否为空
     */
    NEFORCE_NODISCARD bool empty() const noexcept { return data_.empty(); }

    /**
     * @brief 获取键比较函数对象
     * @return 键比较函数对象的副本
     */
    NEFORCE_NODISCARD key_compare key_comp() const noexcept { return data_.key_compare(); }

    /**
     * @brief 获取值比较函数对象
     * @return 值比较函数对象的副本
     */
    NEFORCE_NODISCARD value_compare value_comp() const noexcept { return value_compare(data_.key_compare()); }

    /**
     * @brief 构造元素
     * @tparam Args 构造参数类型
     * @param args 构造参数
     * @return 插入结果（迭代器和是否成功）
     */
    template <typename... Args>
    pair<iterator, bool> emplace(Args&&... args) {
        return data_.emplace_unique(_NEFORCE forward<Args>(args)...);
    }

    /**
     * @brief 拷贝插入元素
     * @param value 要插入的值
     * @return 插入结果（迭代器和是否成功）
     */
    pair<iterator, bool> insert(const value_type& value) { return data_.insert_unique(value); }

    /**
     * @brief 移动插入元素
     * @param value 要插入的值
     * @return 插入结果（迭代器和是否成功）
     */
    pair<iterator, bool> insert(value_type&& value) { return data_.emplace_unique(_NEFORCE move(value)); }

    /**
     * @brief 在提示位置附近就地构造元素
     * @tparam Args 构造参数类型
     * @param position 插入位置提示
     * @param args 构造参数
     * @return 指向插入元素的迭代器
     */
    template <typename... Args>
    iterator emplace_hint(iterator position, Args&&... args) {
        return data_.emplace_unique_hint(position, _NEFORCE forward<Args>(args)...);
    }

    /**
     * @brief 在提示位置附近拷贝插入元素
     * @param position 插入位置提示
     * @param value 要插入的值
     * @return 指向插入元素的迭代器
     */
    iterator insert(iterator position, const value_type& value) { return data_.insert_unique(position, value); }

    /**
     * @brief 在提示位置附近移动插入元素
     * @param position 插入位置提示
     * @param value 要插入的值
     * @return 指向插入元素的迭代器
     */
    iterator insert(iterator position, value_type&& value) {
        return data_.insert_unique(position, _NEFORCE move(value));
    }

    /**
     * @brief 范围插入元素
     * @tparam Iterator 迭代器类型
     * @param first 起始迭代器
     * @param last 结束迭代器
     */
    template <typename Iterator>
    void insert(Iterator first, Iterator last) {
        data_.insert_unique(first, last);
    }

    /**
     * @brief 删除指定位置的元素
     * @param position 要删除的位置
     */
    void erase(iterator position) noexcept(noexcept(data_.erase(position))) { data_.erase(position); }

    /**
     * @brief 删除所有具有指定键的元素
     * @param key 要删除的键
     * @return 删除的元素数量
     */
    size_type erase(const key_type& key) noexcept(noexcept(data_.erase(key))) { return data_.erase(key); }

    /**
     * @brief 删除指定范围内的元素
     * @param first 起始迭代器
     * @param last 结束迭代器
     */
    void erase(iterator first, iterator last) noexcept(noexcept(data_.erase(first, last))) { data_.erase(first, last); }

    /**
     * @brief 清空sparse_map
     */
    void clear() noexcept(noexcept(data_.clear())) { data_.clear(); }

    /**
     * @brief 查找具有指定键的元素
     * @param key 要查找的键
     * @return 指向第一个匹配元素的迭代器，未找到则返回end()
     */
    NEFORCE_NODISCARD iterator find(const key_type& key) { return data_.find(key); }

    /**
     * @brief 常量查找具有指定键的元素
     * @param key 要查找的键
     * @return 指向第一个匹配元素的常量迭代器，未找到则返回cend()
     */
    NEFORCE_NODISCARD const_iterator find(const key_type& key) const { return data_.find(key); }

    /**
     * @brief 统计具有指定键的元素数量
     * @param key 要统计的键
     * @return 匹配的元素数量
     */
    NEFORCE_NODISCARD size_type count(const key_type& key) const { return data_.count(key); }

    /**
     * @brief 获取第一个不小于指定键的元素位置
     * @param key 键值
     * @return 指向第一个不小于key的元素的迭代器
     */
    NEFORCE_NODISCARD iterator lower_bound(const key_type& key) { return data_.lower_bound(key); }

    /**
     * @brief 获取第一个不小于指定键的常量元素位置
     * @param key 键值
     * @return 指向第一个不小于key的元素的常量迭代器
     */
    NEFORCE_NODISCARD const_iterator lower_bound(const key_type& key) const { return data_.lower_bound(key); }

    /**
     * @brief 获取第一个大于指定键的元素位置
     * @param key 键值
     * @return 指向第一个大于key的元素的迭代器
     */
    NEFORCE_NODISCARD iterator upper_bound(const key_type& key) { return data_.upper_bound(key); }

    /**
     * @brief 获取第一个大于指定键的常量元素位置
     * @param key 键值
     * @return 指向第一个大于key的元素的常量迭代器
     */
    NEFORCE_NODISCARD const_iterator upper_bound(const key_type& key) const { return data_.upper_bound(key); }

    /**
     * @brief 获取等于指定键的元素范围
     * @param key 键值
     * @return 包含lower_bound和upper_bound的pair
     */
    NEFORCE_NODISCARD pair<iterator, iterator> equal_range(const key_type& key) { return data_.equal_range(key); }

    /**
     * @brief 获取等于指定键的常量元素范围
     * @param key 键值
     * @return 包含lower_bound和upper_bound的pair
     */
    NEFORCE_NODISCARD pair<const_iterator, const_iterator> equal_range(const key_type& key) const {
        return data_.equal_range(key);
    }

    /**
     * @brief 下标访问操作符
     * @param key 要访问的键
     * @return 对应键的值的引用
     *
     * 如果键不存在，则插入一个默认构造的值并返回其引用。
     */
    NEFORCE_NODISCARD mapped_type& operator[](const key_type& key) {
        iterator iter = data_.lower_bound(key);
        if (iter == end() || key_comp()(key, iter->first)) {
            iter = data_.emplace_unique_hint(iter, key, initialize<T>());
        }
        return iter->second;
    }

    /**
     * @brief 右值键下标访问操作符
     * @param key 要访问的键
     * @return 对应键的值的引用
     *
     * 如果键不存在，则插入一个默认构造的值并返回其引用。
     */
    NEFORCE_NODISCARD mapped_type& operator[](key_type&& key) {
        iterator iter = data_.lower_bound(key);
        if (iter == end() || key_comp()(key, iter->first)) {
            iter = data_.emplace_unique_hint(iter, _NEFORCE move(key), initialize<T>());
        }
        return iter->second;
    }

    /**
     * @brief 带边界检查的常量访问
     * @param key 要访问的键
     * @return 对应键的值的常量引用
     * @throw value_exception 如果键不存在
     */
    NEFORCE_NODISCARD const mapped_type& at(const key_type& key) const {
        const_iterator iter = data_.lower_bound(key);
        if (iter == end() || key_comp()(key, iter->first)) {
            NEFORCE_THROW_EXCEPTION(value_exception("the value of this key does not exists."));
        }
        return iter->second;
    }

    /**
     * @brief 带边界检查的访问
     * @param key 要访问的键
     * @return 对应键的值的引用
     * @throw value_exception 如果键不存在
     */
    NEFORCE_NODISCARD mapped_type& at(const key_type& key) {
        iterator iter = data_.lower_bound(key);
        if (iter == end() || key_comp()(key, iter->first)) {
            NEFORCE_THROW_EXCEPTION(value_exception("the value of this key does not exists."));
        }
        return iter->second;
    }

    /**
     * @brief 预留容量
     * @param n 预留的元素数量
     */
    void reserve(size_type n) { data_.reserve(n); }

    /**
     * @brief 获取当前容量
     * @return 当前分配的容量
     */
    NEFORCE_NODISCARD size_type capacity() const noexcept { return data_.capacity(); }

    /**
     * @brief 收缩容量以适应实际大小
     */
    void shrink_to_fit() { data_.shrink_to_fit(); }

    /**
     * @brief 交换两个sparse_map的内容
     * @param other 要交换的另一个sparse_map
     */
    void swap(sparse_map& other) noexcept(noexcept(data_.swap(other.data_))) { data_.swap(other.data_); }

    /**
     * @brief 相等比较操作符
     * @param rhs 右侧sparse_map
     * @return 如果两个sparse_map大小相等且对应元素相等返回true
     */
    NEFORCE_NODISCARD bool equal_to(const sparse_map& rhs) const noexcept(noexcept(data_ == rhs.data_)) {
        return data_ == rhs.data_;
    }

    /**
     * @brief 小于比较操作符
     * @param rhs 右侧sparse_map
     * @return 按字典序比较结果
     */
    NEFORCE_NODISCARD bool less_than(const sparse_map& rhs) const noexcept(noexcept(data_ < rhs.data_)) {
        return data_ < rhs.data_;
    }
};

#ifdef NEFORCE_STANDARD_17
template <typename Iterator, typename Compare,
          typename Alloc = allocator<pair<iter_map_key_t<Iterator>, iter_map_value_t<Iterator>>>>
sparse_map(Iterator, Iterator, Compare = Compare(), Alloc = Alloc())
        -> sparse_map<iter_map_key_t<Iterator>, iter_map_value_t<Iterator>, Compare, Alloc>;

template <typename Key, typename T, typename Compare = less<Key>, typename Alloc = allocator<pair<Key, T>>>
sparse_map(std::initializer_list<pair<Key, T>>, Compare = Compare(), Alloc = Alloc())
        -> sparse_map<Key, T, Compare, Alloc>;

template <typename Iterator, typename Alloc>
sparse_map(Iterator, Iterator, Alloc)
        -> sparse_map<iter_map_key_t<Iterator>, iter_map_value_t<Iterator>, less<iter_map_key_t<Iterator>>, Alloc>;

template <typename Key, typename T, typename Alloc>
sparse_map(std::initializer_list<pair<Key, T>>, Alloc) -> sparse_map<Key, T, less<Key>, Alloc>;
#endif

/** @} */ // Container

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_CORE_CONTAINER_SPARSE_MAP_HPP__
