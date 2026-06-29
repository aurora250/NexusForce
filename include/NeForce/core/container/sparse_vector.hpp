#ifndef NEFORCE_CORE_CONTAINER_SPARSE_VECTOR_HPP__
#define NEFORCE_CORE_CONTAINER_SPARSE_VECTOR_HPP__

/**
 * @file sparse_vector.hpp
 * @brief 稀疏向量容器
 *
 * 此文件提供了稀疏向量容器的实现。
 * 稀疏向量是一种基于有序平坦数组的关联容器底层实现，
 * 通过二分查找提供对数时间复杂度的查找操作。
 * 作为稀疏关联式容器（sparse_set、sparse_map等）的底层实现。
 */

#include "NeForce/core/algorithm/compare.hpp"
#include "NeForce/core/interface/icollector.hpp"
#include "NeForce/core/interface/iiterator.hpp"
#include "NeForce/core/memory/construct.hpp"
#include "NeForce/core/memory/standard_allocator.hpp"
#include "NeForce/core/utility/pair.hpp"
#include "NeForce/core/container/vector.hpp"
NEFORCE_BEGIN_NAMESPACE__

/**
 * @defgroup SparseVector 稀疏向量
 * @brief 基于有序平坦数组的关联容器底层实现
 *
 * 稀疏向量使用有序 vector 作为底层存储，通过二分查找提供 O(log n) 的查找操作。
 * 插入和删除操作为 O(n)（需要移动元素），但迭代性能优异。
 *
 * @section sparse_vector_complexity 复杂度保证
 *
 * | 操作               | 时间复杂度 | 说明                               |
 * |--------------------|------------|------------------------------------|
 * | 查找               | O(log n)   | 二分查找                           |
 * | 插入               | O(n)       | 二分查找 + 元素移动                 |
 * | 删除               | O(n)       | 二分查找 + 元素移动                 |
 * | 最小/最大          | O(1)       | 有序数组首尾元素                    |
 * | 迭代               | O(1)       | 连续内存遍历                        |
 *
 * @note 本实现提供了两个插入策略：
 *       - insert_unique：键必须唯一，重复键插入失败并返回已存在元素迭代器
 *       - insert_equal：允许重复键，按插入顺序存储
 *
 * @warning 对稀疏向量的修改操作可能使所有迭代器失效（元素移动导致）。
 *          比较函数对象（Compare）必须提供严格的弱序关系。
 * @{
 */

/**
 * @struct sparse_vector_iterator
 * @brief 稀疏向量迭代器
 * @tparam IsConst 是否常量迭代器
 * @tparam SparseVector 稀疏向量类型
 *
 * 提供对稀疏向量元素的随机访问迭代。
 */
template <bool IsConst, typename SparseVector>
struct sparse_vector_iterator : iiterator<sparse_vector_iterator<IsConst, SparseVector>> {
public:
    using container_type = SparseVector;                              ///< 容器类型
    using value_type = typename container_type::value_type;           ///< 值类型
    using size_type = typename container_type::size_type;             ///< 大小类型
    using difference_type = typename container_type::difference_type; ///< 差值类型
    using iterator_category = random_access_iterator_tag;             ///< 随机访问迭代器
    using reference = conditional_t<IsConst, typename container_type::const_reference,
                                    typename container_type::reference>; ///< 引用类型
    using pointer = conditional_t<IsConst, typename container_type::const_pointer,
                                  typename container_type::pointer>; ///< 指针类型

private:
    const container_type* container_ = nullptr; ///< 关联容器指针
    size_type index_ = 0;                       ///< 当前元素索引

    template <typename, typename, typename, typename, typename>
    friend class sparse_vector;

public:
    sparse_vector_iterator() noexcept = default;
    ~sparse_vector_iterator() = default;

    sparse_vector_iterator(const sparse_vector_iterator&) noexcept = default;
    sparse_vector_iterator& operator=(const sparse_vector_iterator&) noexcept = default;
    sparse_vector_iterator(sparse_vector_iterator&&) noexcept = default;
    sparse_vector_iterator& operator=(sparse_vector_iterator&&) noexcept = default;

    /**
     * @brief 构造函数
     * @param index 元素索引
     * @param container 容器指针
     */
    sparse_vector_iterator(size_type index, const container_type* container) noexcept :
    container_(container),
    index_(index) {}

    /**
     * @brief 解引用操作
     * @return 当前元素的引用
     */
    NEFORCE_NODISCARD reference dereference() const noexcept {
        NEFORCE_DEBUG_VERIFY(container_ != nullptr, "Attempting to dereference on null container");
        NEFORCE_DEBUG_VERIFY(index_ < container_->size(), "Attempting to dereference out of boundary");
        return const_cast<container_type*>(container_)->data_[index_];
    }

    /**
     * @brief 递增操作
     */
    NEFORCE_CONSTEXPR20 void increment() noexcept {
        NEFORCE_DEBUG_VERIFY(container_ != nullptr, "Attempting to increment null container");
        NEFORCE_DEBUG_VERIFY(index_ <= container_->size(), "Attempting to increment out of boundary");
        ++index_;
    }

    /**
     * @brief 递减操作
     */
    NEFORCE_CONSTEXPR20 void decrement() noexcept {
        NEFORCE_DEBUG_VERIFY(container_ != nullptr, "Attempting to decrement null container");
        NEFORCE_DEBUG_VERIFY(index_ > 0, "Attempting to decrement before begin");
        --index_;
    }

    /**
     * @brief 随机访问递增
     * @param n 偏移量
     */
    void advance(difference_type n) noexcept {
        NEFORCE_DEBUG_VERIFY(container_ != nullptr, "Attempting to advance null container");
        index_ = static_cast<size_type>(static_cast<difference_type>(index_) + n);
    }

    /**
     * @brief 计算到另一迭代器的距离
     * @param rhs 右侧迭代器
     * @return 距离值
     */
    NEFORCE_NODISCARD difference_type distance_to(const sparse_vector_iterator& rhs) const noexcept {
        NEFORCE_DEBUG_VERIFY(container_ == rhs.container_, "Attempting to distance different container");
        return static_cast<difference_type>(rhs.index_) - static_cast<difference_type>(index_);
    }

    /**
     * @brief 相等比较
     * @param rhs 右侧迭代器
     * @return 是否相等
     */
    NEFORCE_NODISCARD bool equal_to(const sparse_vector_iterator& rhs) const noexcept {
        NEFORCE_DEBUG_VERIFY(container_ == rhs.container_, "Attempting to equal to a different container");
        return index_ == rhs.index_;
    }

    /**
     * @brief 小于比较
     * @param rhs 右侧迭代器
     * @return 是否小于
     */
    NEFORCE_NODISCARD bool less_than(const sparse_vector_iterator& rhs) const noexcept {
        NEFORCE_DEBUG_VERIFY(container_ == rhs.container_, "Attempting to less than a different container");
        return index_ < rhs.index_;
    }

    /**
     * @brief 获取关联容器
     * @return 关联容器指针
     */
    NEFORCE_NODISCARD const container_type* container() const noexcept { return container_; }

    /**
     * @brief 获取当前索引
     * @return 当前索引值
     */
    NEFORCE_NODISCARD size_type index() const noexcept { return index_; }
};


/**
 * @class sparse_vector
 * @brief 稀疏向量容器
 * @tparam Key 键类型
 * @tparam Value 值类型
 * @tparam KeyOfValue 从值中提取键的函数对象
 * @tparam Compare 键比较函数对象
 * @tparam Alloc 分配器类型
 *
 * 稀疏向量是一种基于有序平坦数组的关联容器底层实现。
 * 使用二分查找提供对数时间复杂度的查找操作。
 *
 * @note 稀疏向量在插入/删除时需要移动元素，但迭代性能优异，缓存友好，适用于查找频繁、修改较少且需要有序遍历的场景。
 */
template <typename Key, typename Value, typename KeyOfValue, typename Compare, typename Alloc = allocator<Value>>
class sparse_vector : icollector<sparse_vector<Key, Value, KeyOfValue, Compare, Alloc>> {
    static_assert(is_allocator_v<Alloc>, "Alloc type is not a standard allocator type.");
    static_assert(is_same_v<Value, typename Alloc::value_type>, "allocator type mismatch.");
    static_assert(is_object_v<Value>, "sparse_vector only contains object types.");

public:
    using key_type = Key; ///< 键类型

    using value_type = Value;             ///< 值类型
    using pointer = Value*;               ///< 指针类型
    using reference = Value&;             ///< 引用类型
    using const_pointer = const Value*;   ///< 常量指针类型
    using const_reference = const Value&; ///< 常量引用类型
    using size_type = size_t;             ///< 大小类型
    using difference_type = ptrdiff_t;    ///< 差值类型
    using compare_type = Compare;         ///< 比较器类型

    using iterator = sparse_vector_iterator<false, sparse_vector>;            ///< 迭代器类型
    using const_iterator = sparse_vector_iterator<true, sparse_vector>;       ///< 常量迭代器类型
    using reverse_iterator = _NEFORCE reverse_iterator<iterator>;             ///< 反向迭代器类型
    using const_reverse_iterator = _NEFORCE reverse_iterator<const_iterator>; ///< 常量反向迭代器类型
    using allocator_type = Alloc;                                             ///< 分配器类型

private:
    using container_type = vector<Value, Alloc>; ///< 底层容器类型

    container_type data_;    ///< 底层数据存储
    Compare key_compare_{};  ///< 键比较函数对象
    KeyOfValue extracter_{}; ///< 值提取键函数对象

    template <bool, typename>
    friend struct sparse_vector_iterator;

    /**
     * @brief 从值提取键
     * @param value 值
     * @return 键
     */
    NEFORCE_NODISCARD const Key& key_of(const value_type& value) const noexcept { return extracter_(value); }

    /**
     * @brief 二分查找——不小于指定键的首个位置
     * @param key 要查找的键
     * @return 位置索引
     */
    NEFORCE_NODISCARD size_type lower_bound_index(const key_type& key) const {
        size_type lo = 0;
        size_type hi = data_.size();
        while (lo < hi) {
            size_type mid = lo + (hi - lo) / 2;
            if (key_compare_(key_of(data_[mid]), key)) {
                lo = mid + 1;
            } else {
                hi = mid;
            }
        }
        return lo;
    }

    /**
     * @brief 二分查找——大于指定键的首个位置
     * @param key 要查找的键
     * @return 位置索引
     */
    NEFORCE_NODISCARD size_type upper_bound_index(const key_type& key) const {
        size_type lo = 0;
        size_type hi = data_.size();
        while (lo < hi) {
            size_type mid = lo + (hi - lo) / 2;
            if (key_compare_(key, key_of(data_[mid]))) {
                hi = mid;
            } else {
                lo = mid + 1;
            }
        }
        return lo;
    }

    /**
     * @brief 在指定位置插入元素
     * @param pos 插入位置索引
     * @param value 要插入的值
     * @return 指向插入位置的迭代器
     */
    iterator insert_at(size_type pos, value_type&& value) {
        data_.emplace(data_.begin() + static_cast<difference_type>(pos), _NEFORCE move(value));
        return iterator(pos, this);
    }

    /**
     * @brief 在指定位置插入元素（拷贝版本）
     * @param pos 插入位置索引
     * @param value 要插入的值
     * @return 指向插入位置的迭代器
     */
    iterator insert_at(size_type pos, const value_type& value) {
        data_.emplace(data_.begin() + static_cast<difference_type>(pos), value);
        return iterator(pos, this);
    }

public:
    /**
     * @brief 默认构造函数
     */
    sparse_vector() = default;

    /**
     * @brief 构造函数，指定比较函数
     * @param comp 比较函数对象
     */
    explicit sparse_vector(const Compare& comp) :
    key_compare_(comp) {}

    /**
     * @brief 拷贝构造函数
     * @param other 源稀疏向量
     */
    sparse_vector(const sparse_vector& other) :
    data_(other.data_),
    key_compare_(other.key_compare_),
    extracter_(other.extracter_) {}

    /**
     * @brief 拷贝赋值运算符
     * @param other 源稀疏向量
     * @return 自身引用
     */
    sparse_vector& operator=(const sparse_vector& other) {
        if (_NEFORCE addressof(other) == this) {
            return *this;
        }
        data_ = other.data_;
        key_compare_ = other.key_compare_;
        extracter_ = other.extracter_;
        return *this;
    }

    /**
     * @brief 移动构造函数
     * @param other 源稀疏向量
     */
    sparse_vector(sparse_vector&& other) noexcept(is_nothrow_move_constructible_v<container_type> &&
                                                  is_nothrow_move_constructible_v<Compare> &&
                                                  is_nothrow_move_constructible_v<KeyOfValue>) :
    data_(_NEFORCE move(other.data_)),
    key_compare_(_NEFORCE move(other.key_compare_)),
    extracter_(_NEFORCE move(other.extracter_)) {}

    /**
     * @brief 移动赋值运算符
     * @param other 源稀疏向量
     * @return 自身引用
     */
    sparse_vector& operator=(sparse_vector&& other) noexcept(is_nothrow_move_assignable_v<container_type>) {
        if (_NEFORCE addressof(other) == this) {
            return *this;
        }
        data_ = _NEFORCE move(other.data_);
        key_compare_ = _NEFORCE move(other.key_compare_);
        extracter_ = _NEFORCE move(other.extracter_);
        return *this;
    }

    /**
     * @brief 析构函数
     */
    ~sparse_vector() = default;

    /**
     * @brief 获取起始迭代器
     * @return 指向第一个元素的迭代器
     */
    NEFORCE_NODISCARD iterator begin() noexcept { return iterator(0, this); }

    /**
     * @brief 获取结束迭代器
     * @return 指向最后一个元素之后位置的迭代器
     */
    NEFORCE_NODISCARD iterator end() noexcept { return iterator(data_.size(), this); }

    /**
     * @brief 获取常量起始迭代器
     * @return 指向第一个元素的常量迭代器
     */
    NEFORCE_NODISCARD const_iterator begin() const noexcept { return cbegin(); }

    /**
     * @brief 获取常量结束迭代器
     * @return 指向最后一个元素之后位置的常量迭代器
     */
    NEFORCE_NODISCARD const_iterator end() const noexcept { return cend(); }

    /**
     * @brief 获取常量起始迭代器
     * @return 指向第一个元素的常量迭代器
     */
    NEFORCE_NODISCARD const_iterator cbegin() const noexcept { return const_iterator(0, this); }

    /**
     * @brief 获取常量结束迭代器
     * @return 指向最后一个元素之后位置的常量迭代器
     */
    NEFORCE_NODISCARD const_iterator cend() const noexcept { return const_iterator(data_.size(), this); }

    /**
     * @brief 获取反向起始迭代器
     * @return 指向最后一个元素的反向迭代器
     */
    NEFORCE_NODISCARD reverse_iterator rbegin() noexcept { return reverse_iterator(end()); }

    /**
     * @brief 获取反向结束迭代器
     * @return 指向第一个元素之前位置的反向迭代器
     */
    NEFORCE_NODISCARD reverse_iterator rend() noexcept { return reverse_iterator(begin()); }

    /**
     * @brief 获取常量反向起始迭代器
     * @return 指向最后一个元素的常量反向迭代器
     */
    NEFORCE_NODISCARD const_reverse_iterator rbegin() const noexcept { return crbegin(); }

    /**
     * @brief 获取常量反向结束迭代器
     * @return 指向第一个元素之前位置的常量反向迭代器
     */
    NEFORCE_NODISCARD const_reverse_iterator rend() const noexcept { return crend(); }

    /**
     * @brief 获取常量反向起始迭代器
     * @return 指向最后一个元素的常量反向迭代器
     */
    NEFORCE_NODISCARD const_reverse_iterator crbegin() const noexcept { return const_reverse_iterator(cend()); }

    /**
     * @brief 获取常量反向结束迭代器
     * @return 指向第一个元素之前位置的常量反向迭代器
     */
    NEFORCE_NODISCARD const_reverse_iterator crend() const noexcept { return const_reverse_iterator(cbegin()); }

    /**
     * @brief 获取元素数量
     * @return 元素数量
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
    NEFORCE_NODISCARD Compare key_compare() const noexcept(is_nothrow_copy_constructible_v<Compare>) {
        return key_compare_;
    }

    /**
     * @brief 插入唯一键元素
     * @tparam Args 构造参数类型
     * @param args 构造参数
     * @return 插入结果（迭代器和是否成功）
     */
    template <typename... Args>
    pair<iterator, bool> emplace_unique(Args&&... args) {
        value_type tmp(_NEFORCE forward<Args>(args)...);
        size_type pos = lower_bound_index(key_of(tmp));
        if (pos < data_.size() && !key_compare_(key_of(tmp), key_of(data_[pos]))) {
            return pair<iterator, bool>(iterator(pos, this), false);
        }
        return pair<iterator, bool>(insert_at(pos, _NEFORCE move(tmp)), true);
    }

    /**
     * @brief 拷贝插入唯一键元素
     * @param value 要插入的值
     * @return 插入结果（迭代器和是否成功）
     */
    pair<iterator, bool> insert_unique(const value_type& value) { return emplace_unique(value); }

    /**
     * @brief 移动插入唯一键元素
     * @param value 要插入的值
     * @return 插入结果（迭代器和是否成功）
     */
    pair<iterator, bool> insert_unique(value_type&& value) { return emplace_unique(_NEFORCE move(value)); }

    /**
     * @brief 在提示位置附近构造唯一键元素
     * @tparam Args 构造参数类型
     * @param position 插入位置提示
     * @param args 构造参数
     * @return 指向插入元素的迭代器
     */
    template <typename... Args>
    iterator emplace_unique_hint(iterator position, Args&&... args) {
        value_type tmp(_NEFORCE forward<Args>(args)...);
        const key_type& k = key_of(tmp);

        if (position != end() && position != begin()) {
            iterator before = position;
            --before;
            if (key_compare_(key_of(*before), k) && key_compare_(k, key_of(*position))) {
                return insert_at(position.index(), _NEFORCE move(tmp));
            }
        }
        if (position == begin() && !data_.empty()) {
            if (key_compare_(k, key_of(data_.front()))) {
                return insert_at(0, _NEFORCE move(tmp));
            }
            if (!key_compare_(key_of(data_.front()), k)) {
                return iterator(0, this);
            }
        }
        if (position == end() && !data_.empty()) {
            if (key_compare_(key_of(data_.back()), k)) {
                return insert_at(data_.size(), _NEFORCE move(tmp));
            }
            if (!key_compare_(k, key_of(data_.back()))) {
                return iterator(data_.size() - 1, this);
            }
        }

        return emplace_unique(_NEFORCE move(tmp)).first;
    }

    /**
     * @brief 在提示位置附近拷贝插入唯一键元素
     * @param position 插入位置提示
     * @param value 要插入的值
     * @return 指向插入元素的迭代器
     */
    iterator insert_unique(iterator position, const value_type& value) { return emplace_unique_hint(position, value); }

    /**
     * @brief 在提示位置附近移动插入唯一键元素
     * @param position 插入位置提示
     * @param value 要插入的值
     * @return 指向插入元素的迭代器
     */
    iterator insert_unique(iterator position, value_type&& value) {
        return emplace_unique_hint(position, _NEFORCE move(value));
    }

    /**
     * @brief 范围插入唯一键元素
     * @tparam Iterator 迭代器类型
     * @param first 起始迭代器
     * @param last 结束迭代器
     */
    template <typename Iterator, enable_if_t<is_iter_v<Iterator>, int> = 0>
    void insert_unique(Iterator first, Iterator last) {
        for (; first != last; ++first) {
            insert_unique(*first);
        }
    }

    /**
     * @brief 插入允许重复键元素
     * @tparam Args 构造参数类型
     * @param args 构造参数
     * @return 指向插入元素的迭代器
     */
    template <typename... Args>
    iterator emplace_equal(Args&&... args) {
        value_type tmp(_NEFORCE forward<Args>(args)...);
        size_type pos = upper_bound_index(key_of(tmp));
        return insert_at(pos, _NEFORCE move(tmp));
    }

    /**
     * @brief 拷贝插入允许重复键元素
     * @param value 要插入的值
     * @return 指向插入元素的迭代器
     */
    iterator insert_equal(const value_type& value) { return emplace_equal(value); }

    /**
     * @brief 移动插入允许重复键元素
     * @param value 要插入的值
     * @return 指向插入元素的迭代器
     */
    iterator insert_equal(value_type&& value) { return emplace_equal(_NEFORCE move(value)); }

    /**
     * @brief 在提示位置附近构造允许重复键元素
     * @tparam Args 构造参数类型
     * @param position 插入位置提示
     * @param args 构造参数
     * @return 指向插入元素的迭代器
     */
    template <typename... Args>
    iterator emplace_equal_hint(iterator position, Args&&... args) {
        value_type tmp(_NEFORCE forward<Args>(args)...);
        const key_type& k = key_of(tmp);

        if (position != end() && position != begin()) {
            iterator before = position;
            --before;
            if (key_compare_(key_of(*before), k) && key_compare_(k, key_of(*position))) {
                return insert_at(position.index(), _NEFORCE move(tmp));
            }
        }
        if (position == begin() && !data_.empty()) {
            if (key_compare_(k, key_of(data_.front()))) {
                return insert_at(0, _NEFORCE move(tmp));
            }
        }
        if (position == end() && !data_.empty()) {
            if (!key_compare_(k, key_of(data_.back()))) {
                return insert_at(data_.size(), _NEFORCE move(tmp));
            }
        }

        return emplace_equal(_NEFORCE move(tmp));
    }

    /**
     * @brief 在提示位置附近拷贝插入允许重复键元素
     * @param position 插入位置提示
     * @param value 要插入的值
     * @return 指向插入元素的迭代器
     */
    iterator insert_equal(iterator position, const value_type& value) { return emplace_equal_hint(position, value); }

    /**
     * @brief 在提示位置附近移动插入允许重复键元素
     * @param position 插入位置提示
     * @param value 要插入的值
     * @return 指向插入元素的迭代器
     */
    iterator insert_equal(iterator position, value_type&& value) {
        return emplace_equal_hint(position, _NEFORCE move(value));
    }

    /**
     * @brief 范围插入允许重复键元素
     * @tparam Iterator 迭代器类型
     * @param first 起始迭代器
     * @param last 结束迭代器
     */
    template <typename Iterator, enable_if_t<is_iter_v<Iterator>, int> = 0>
    void insert_equal(Iterator first, Iterator last) {
        for (; first != last; ++first) {
            insert_equal(*first);
        }
    }

    /**
     * @brief 删除所有具有指定键的元素
     * @param key 要删除的键
     * @return 删除的元素数量
     */
    size_type erase(const key_type& key) {
        size_type lo = lower_bound_index(key);
        size_type hi = upper_bound_index(key);
        size_type n = hi - lo;
        if (n > 0) {
            data_.erase(data_.begin() + static_cast<difference_type>(lo),
                        data_.begin() + static_cast<difference_type>(hi));
        }
        return n;
    }

    /**
     * @brief 删除指定位置的元素
     * @param position 要删除的位置
     */
    void erase(iterator position) {
        NEFORCE_DEBUG_VERIFY(position.container() == this, "Attempting to erase from different container");
        NEFORCE_DEBUG_VERIFY(position.index() < data_.size(), "Attempting to erase out of boundary");
        data_.erase(data_.begin() + static_cast<difference_type>(position.index()));
    }

    /**
     * @brief 删除指定范围内的元素
     * @param first 起始迭代器
     * @param last 结束迭代器
     */
    void erase(iterator first, iterator last) {
        NEFORCE_DEBUG_VERIFY(first.container() == this && last.container() == this,
                             "Attempting to erase from different container");
        if (first == begin() && last == end()) {
            clear();
        } else {
            data_.erase(data_.begin() + static_cast<difference_type>(first.index()),
                        data_.begin() + static_cast<difference_type>(last.index()));
        }
    }

    /**
     * @brief 清空
     */
    void clear() { data_.clear(); }

    /**
     * @brief 查找具有指定键的元素
     * @param key 要查找的键
     * @return 指向第一个匹配元素的迭代器，未找到则返回end()
     */
    NEFORCE_NODISCARD iterator find(const key_type& key) {
        size_type pos = lower_bound_index(key);
        if (pos < data_.size() && !key_compare_(key, key_of(data_[pos]))) {
            return iterator(pos, this);
        }
        return end();
    }

    /**
     * @brief 查找具有指定键的元素（常量版本）
     * @param key 要查找的键
     * @return 指向第一个匹配元素的常量迭代器，未找到则返回cend()
     */
    NEFORCE_NODISCARD const_iterator find(const key_type& key) const {
        size_type pos = lower_bound_index(key);
        if (pos < data_.size() && !key_compare_(key, key_of(data_[pos]))) {
            return const_iterator(pos, this);
        }
        return cend();
    }

    /**
     * @brief 统计具有指定键的元素数量
     * @param key 要统计的键
     * @return 匹配的元素数量
     */
    NEFORCE_NODISCARD size_type count(const key_type& key) const {
        return upper_bound_index(key) - lower_bound_index(key);
    }

    /**
     * @brief 获取第一个不小于指定键的元素位置
     * @param key 键值
     * @return 指向第一个不小于key的元素的迭代器
     */
    NEFORCE_NODISCARD iterator lower_bound(const key_type& key) { return iterator(lower_bound_index(key), this); }

    /**
     * @brief 获取第一个不小于指定键的元素位置（常量版本）
     * @param key 键值
     * @return 指向第一个不小于key的元素的常量迭代器
     */
    NEFORCE_NODISCARD const_iterator lower_bound(const key_type& key) const {
        return const_iterator(lower_bound_index(key), this);
    }

    /**
     * @brief 获取第一个大于指定键的元素位置
     * @param key 键值
     * @return 指向第一个大于key的元素的迭代器
     */
    NEFORCE_NODISCARD iterator upper_bound(const key_type& key) { return iterator(upper_bound_index(key), this); }

    /**
     * @brief 获取第一个大于指定键的元素位置（常量版本）
     * @param key 键值
     * @return 指向第一个大于key的元素的常量迭代器
     */
    NEFORCE_NODISCARD const_iterator upper_bound(const key_type& key) const {
        return const_iterator(upper_bound_index(key), this);
    }

    /**
     * @brief 获取等于指定键的元素范围
     * @param key 键值
     * @return 包含lower_bound和upper_bound的pair
     */
    NEFORCE_NODISCARD pair<iterator, iterator> equal_range(const key_type& key) {
        return pair<iterator, iterator>(lower_bound(key), upper_bound(key));
    }

    /**
     * @brief 获取等于指定键的元素范围（常量版本）
     * @param key 键值
     * @return 包含lower_bound和upper_bound的pair
     */
    NEFORCE_NODISCARD pair<const_iterator, const_iterator> equal_range(const key_type& key) const {
        return pair<const_iterator, const_iterator>(lower_bound(key), upper_bound(key));
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
     * @brief 交换两个稀疏向量的内容
     * @param other 要交换的另一个稀疏向量
     */
    void swap(sparse_vector& other) noexcept(is_nothrow_swappable_v<container_type> &&
                                             is_nothrow_swappable_v<Compare> && is_nothrow_swappable_v<KeyOfValue>) {
        data_.swap(other.data_);
        _NEFORCE swap(key_compare_, other.key_compare_);
        _NEFORCE swap(extracter_, other.extracter_);
    }

    /**
     * @brief 相等比较
     * @param rhs 右侧稀疏向量
     * @return 大小相等且对应元素相等返回true
     */
    NEFORCE_NODISCARD bool equal_to(const sparse_vector& rhs) const
            noexcept(noexcept(_NEFORCE equal(cbegin(), cend(), rhs.cbegin()))) {
        return size() == rhs.size() && _NEFORCE equal(cbegin(), cend(), rhs.cbegin());
    }

    /**
     * @brief 小于比较
     * @param rhs 右侧稀疏向量
     * @return 按字典序比较结果
     */
    NEFORCE_NODISCARD bool less_than(const sparse_vector& rhs) const
            noexcept(noexcept(_NEFORCE lexicographical_compare(cbegin(), cend(), rhs.cbegin(), rhs.cend()))) {
        return _NEFORCE lexicographical_compare(cbegin(), cend(), rhs.cbegin(), rhs.cend());
    }
};

/** @} */ // SparseVector

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_CORE_CONTAINER_SPARSE_VECTOR_HPP__
