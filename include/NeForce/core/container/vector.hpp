#ifndef NEFORCE_CORE_CONTAINER_VECTOR_HPP__
#define NEFORCE_CORE_CONTAINER_VECTOR_HPP__

/**
 * @file vector.hpp
 * @brief 动态大小数组容器
 *
 * 此文件提供了动态大小数组容器（向量）的实现。
 * 支持动态内存管理、随机访问迭代器和插入/删除操作。
 */

#include "NeForce/core/memory/memory_view.hpp"
#include "NeForce/core/memory/standard_allocator.hpp"
#include "NeForce/core/memory/uninitialized.hpp"
NEFORCE_BEGIN_NAMESPACE__

/**
 * @defgroup Container 标准容器
 * @brief 支持标准算法的容器的实现
 * @{
 */

/**
 * @struct vector_iterator
 * @brief 向量迭代器
 * @tparam IsConst 是否常量迭代器
 * @tparam Vector 向量类型
 *
 * 为vector提供迭代器支持，包含边界检查和调试验证。
 */
template <bool IsConst, typename Vector>
struct vector_iterator : iiterator<vector_iterator<IsConst, Vector>> {
public:
    using container_type = Vector;                                    ///< 容器类型
    using value_type = typename container_type::value_type;           ///< 值类型
    using size_type = typename container_type::size_type;             ///< 大小类型
    using difference_type = typename container_type::difference_type; ///< 差值类型
    using iterator_category = contiguous_iterator_tag;                ///< 迭代器类别
    using reference = conditional_t<IsConst, typename container_type::const_reference,
                                    typename container_type::reference>; ///< 引用类型
    using pointer = conditional_t<IsConst, typename container_type::const_pointer,
                                  typename container_type::pointer>; ///< 指针类型

private:
    pointer current_ = nullptr;                 ///< 当前指针位置
    const container_type* container_ = nullptr; ///< 关联容器指针

public:
    NEFORCE_CONSTEXPR20 vector_iterator() noexcept = default;
    NEFORCE_CONSTEXPR20 ~vector_iterator() = default;

    NEFORCE_CONSTEXPR20 vector_iterator(const vector_iterator&) noexcept = default;
    NEFORCE_CONSTEXPR20 vector_iterator& operator=(const vector_iterator&) noexcept = default;
    NEFORCE_CONSTEXPR20 vector_iterator(vector_iterator&&) noexcept = default;
    NEFORCE_CONSTEXPR20 vector_iterator& operator=(vector_iterator&&) noexcept = default;

    /**
     * @brief 构造函数
     * @param ptr 初始指针位置
     * @param vec 关联容器指针
     */
    NEFORCE_CONSTEXPR20 vector_iterator(pointer ptr, const container_type* vec) noexcept :
    current_(ptr),
    container_(vec) {}

    /**
     * @brief 解引用操作
     * @return 当前元素的引用
     */
    NEFORCE_NODISCARD NEFORCE_CONSTEXPR20 reference dereference() const noexcept {
        NEFORCE_DEBUG_VERIFY(current_ && container_, "Attempting to dereference on a null pointer");
        NEFORCE_DEBUG_VERIFY(container_->start_ <= current_ && current_ < container_->finish_,
                             "Attempting to dereference out of boundary");
        return *current_;
    }

    /**
     * @brief 递增操作
     */
    NEFORCE_CONSTEXPR20 void increment() noexcept {
        NEFORCE_DEBUG_VERIFY(current_ && container_, "Attempting to increment a null pointer");
        NEFORCE_DEBUG_VERIFY(current_ < container_->finish_, "Attempting to increment out of boundary");
        ++current_;
    }

    /**
     * @brief 递减操作
     */
    NEFORCE_CONSTEXPR20 void decrement() noexcept {
        NEFORCE_DEBUG_VERIFY(current_ && container_, "Attempting to decrement a null pointer");
        NEFORCE_DEBUG_VERIFY(container_->start_ < current_, "Attempting to decrement out of boundary");
        --current_;
    }

    /**
     * @brief 前进操作
     * @param off 前进距离
     */
    NEFORCE_CONSTEXPR20 void advance(difference_type off) noexcept {
        NEFORCE_DEBUG_VERIFY((current_ && container_) || off == 0, "Attempting to advance a null pointer");
        NEFORCE_DEBUG_VERIFY((off < 0 ? off >= container_->start_ - current_ : off <= container_->finish_ - current_),
                             "Attempting to advance out of boundary");
        current_ += off;
    }

    /**
     * @brief 计算距离操作
     * @param other 另一个迭代器
     * @return 两个迭代器之间的距离
     */
    NEFORCE_NODISCARD NEFORCE_CONSTEXPR20 difference_type distance_to(const vector_iterator& other) const noexcept {
        NEFORCE_DEBUG_VERIFY(container_ == other.container_, "Attempting to distance to a different container");
        return static_cast<difference_type>(current_ - other.current_);
    }

    /**
     * @brief 下标访问操作符
     * @param off 偏移量
     * @return 偏移位置元素的引用
     */
    NEFORCE_NODISCARD NEFORCE_CONSTEXPR20 reference operator[](difference_type off) noexcept { return *(*this + off); }

    /**
     * @brief 相等比较
     * @param rhs 右侧迭代器
     * @return 是否相等
     */
    NEFORCE_NODISCARD NEFORCE_CONSTEXPR20 bool equal(const vector_iterator& rhs) const noexcept {
        NEFORCE_DEBUG_VERIFY(container_ == rhs.container_, "Attempting to equal to a different container");
        return current_ == rhs.current_;
    }

    /**
     * @brief 小于比较
     * @param rhs 右侧迭代器
     * @return 当前迭代器是否在rhs之前
     */
    NEFORCE_NODISCARD NEFORCE_CONSTEXPR20 bool less_than(const vector_iterator& rhs) const noexcept {
        NEFORCE_DEBUG_VERIFY(container_ == rhs.container_, "Attempting to less than a different container");
        return current_ < rhs.current_;
    }

    /**
     * @brief 获取底层指针
     * @return 当前指针
     */
    NEFORCE_NODISCARD NEFORCE_CONSTEXPR20 pointer base() const noexcept { return current_; }

    /**
     * @brief 获取关联容器
     * @return 关联容器指针
     */
    NEFORCE_NODISCARD NEFORCE_CONSTEXPR20 const container_type* container() const noexcept { return container_; }
};


/**
 * @class vector
 * @brief 动态大小数组容器
 * @tparam T 元素类型
 * @tparam Alloc 分配器类型
 *
 * 动态增长的数组容器，提供连续的存储空间和随机访问能力。
 * 支持在末尾高效插入/删除元素，以及在任意位置插入/删除。
 */
template <typename T, typename Alloc = allocator<T>>
class vector : public icollector<vector<T, Alloc>> {
    static_assert(is_object_v<T>, "vector only contains object types.");
    static_assert(is_allocator_v<Alloc>, "Alloc type is not a standard allocator type.");
    static_assert(is_same_v<T, typename Alloc::value_type>, "allocator type mismatch.");

public:
    using pointer = T*;                                                       ///< 指针类型
    using reference = T&;                                                     ///< 引用类型
    using const_pointer = const T*;                                           ///< 常量指针类型
    using const_reference = const T&;                                         ///< 常量引用类型
    using value_type = T;                                                     ///< 值类型
    using size_type = size_t;                                                 ///< 大小类型
    using difference_type = ptrdiff_t;                                        ///< 差值类型
    using iterator = vector_iterator<false, vector<T, Alloc>>;                ///< 迭代器类型
    using const_iterator = vector_iterator<true, vector<T, Alloc>>;           ///< 常量迭代器类型
    using reverse_iterator = _NEFORCE reverse_iterator<iterator>;             ///< 反向迭代器类型
    using const_reverse_iterator = _NEFORCE reverse_iterator<const_iterator>; ///< 常量反向迭代器类型
    using allocator_type = Alloc;                                             ///< 分配器类型

private:
    pointer start_ = nullptr;  ///< 指向分配内存的起始位置
    pointer finish_ = nullptr; ///< 指向已构造元素的末尾
    compressed_pair<allocator_type, pointer> pair_{default_construct_tag{},
                                                   nullptr}; ///< 压缩存储的分配器和容量末尾指针

    template <bool, typename>
    friend struct vector_iterator;

public:
    /// 特殊值，表示未找到或"直到末尾"
    static constexpr size_type npos = static_cast<size_type>(-1);

private:
    /**
     * @brief 填充初始化
     * @param n 元素数量
     * @param value 填充值
     */
    NEFORCE_CONSTEXPR20 void fill_initialize(size_type n, const T& value) {
        start_ = pair_.get_base().allocate(n);
        _NEFORCE uninitialized_fill_n(start_, n, value);
        finish_ = start_ + n;
        pair_.value = finish_;
    }

    /**
     * @brief 分配内存并复制元素
     * @tparam Iterator 迭代器类型
     * @param n 元素数量
     * @param first 起始迭代器
     * @param last 结束迭代器
     * @return 分配的内存起始指针
     */
    template <typename Iterator>
    NEFORCE_CONSTEXPR20 pointer allocate_and_copy(size_type n, Iterator first, Iterator last) {
        NEFORCE_DEBUG_VERIFY(n < max_size(), "vector allocate out of allocate bounds.");
        pointer result = pair_.get_base().allocate(n);
        pointer finish = result;
        try {
            finish = _NEFORCE uninitialized_copy(first, last, result);
        } catch (...) {
            _NEFORCE destroy(result, finish);
            pair_.get_base().deallocate(result, n);
            throw;
        }
        return result;
    }

    /**
     * @brief 分配内存并移动元素
     * @tparam Iterator 迭代器类型
     * @param n 元素数量
     * @param first 起始迭代器
     * @param last 结束迭代器
     * @return 分配的内存起始指针
     */
    template <typename Iterator>
    NEFORCE_CONSTEXPR20 pointer allocate_and_move(size_type n, Iterator first, Iterator last) {
        pointer result = pair_.get_base().allocate(n);
        pointer finish = result;
        try {
            finish = _NEFORCE uninitialized_move(first, last, result);
        } catch (...) {
            _NEFORCE destroy(result, finish);
            pair_.get_base().deallocate(result, n);
            throw;
        }
        return result;
    }

    /**
     * @brief 范围初始化（非前向迭代器版本）
     * @tparam Iterator 迭代器类型
     * @param first 起始迭代器
     * @param last 结束迭代器
     */
    template <typename Iterator>
    NEFORCE_CONSTEXPR20 enable_if_t<!is_ranges_fwd_iter_v<Iterator>> range_initialize(Iterator first, Iterator last) {
        for (; first != last; ++first) {
            vector::push_back(*first);
        }
        return;
    }

    /**
     * @brief 范围初始化（前向迭代器版本）
     * @tparam Iterator 迭代器类型
     * @param first 起始迭代器
     * @param last 结束迭代器
     */
    template <typename Iterator>
    NEFORCE_CONSTEXPR20 enable_if_t<is_ranges_fwd_iter_v<Iterator>> range_initialize(Iterator first, Iterator last) {
        size_type n = _NEFORCE distance(first, last);
        start_ = vector::allocate_and_copy(n, first, last);
        finish_ = start_ + n;
        pair_.value = finish_;
        return;
    }

    /**
     * @brief 释放内存
     */
    NEFORCE_CONSTEXPR20 void deallocate() noexcept {
        if (start_) {
            pair_.get_base().deallocate(start_, pair_.value - start_);
        }
    }

    /**
     * @brief 范围插入（非前向迭代器版本）
     * @tparam Iterator 迭代器类型
     * @param position 插入位置
     * @param first 起始迭代器
     * @param last 结束迭代器
     */
    template <typename Iterator>
    NEFORCE_CONSTEXPR20 enable_if_t<!is_ranges_fwd_iter_v<Iterator>> range_insert(iterator position, Iterator first,
                                                                                  Iterator last) {
        while (first != last) {
            position = vector::insert(position, *first);
            ++position;
            ++first;
        }
        return;
    }

    /**
     * @brief 范围插入（前向迭代器版本）
     * @tparam Iterator 迭代器类型
     * @param position 插入位置
     * @param first 起始迭代器
     * @param last 结束迭代器
     */
    template <typename Iterator>
    NEFORCE_CONSTEXPR20 enable_if_t<is_ranges_fwd_iter_v<Iterator>> range_insert(iterator position, Iterator first,
                                                                                 Iterator last) {
        if (first == last) {
            return;
        }

        const size_t n = _NEFORCE distance(first, last);

        if (static_cast<size_t>(pair_.value - finish_) >= n) {
            const size_t elems_after = end() - position;
            iterator old_finish = end();

            if (elems_after > n) {
                _NEFORCE uninitialized_copy(finish_ - n, finish_, finish_);
                finish_ += n;
                _NEFORCE copy_backward(position, old_finish - n, old_finish);
                _NEFORCE copy(first, last, position);
            } else {
                Iterator mid = first;
                _NEFORCE advance(mid, elems_after);
                _NEFORCE uninitialized_copy(mid, last, finish_);
                finish_ += (n - elems_after);
                _NEFORCE uninitialized_move(position, old_finish, finish_);
                finish_ += elems_after;
                _NEFORCE copy(first, mid, position);
            }
        } else {
            const size_type old_size = size();
            const size_type len = old_size + _NEFORCE max(old_size, n);
            pointer new_start = pair_.get_base().allocate(len);
            pointer new_finish = new_start;

            new_finish = _NEFORCE uninitialized_copy(begin(), position, new_start);
            new_finish = _NEFORCE uninitialized_copy(first, last, new_finish);
            new_finish = _NEFORCE uninitialized_copy(position, end(), new_finish);

            _NEFORCE destroy(start_, finish_);
            deallocate();

            start_ = new_start;
            finish_ = new_finish;
            pair_.value = new_start + len;
        }
        return;
    }

    /**
     * @brief 辅助赋值函数（非前向迭代器版本）
     * @tparam Iterator 迭代器类型
     * @param first 起始迭代器
     * @param last 结束迭代器
     */
    template <typename Iterator>
    NEFORCE_CONSTEXPR20 enable_if_t<!is_ranges_fwd_iter_v<Iterator>> assign_aux(Iterator first, Iterator last) {
        clear();
        while (first != last) {
            vector::push_back(*first);
            ++first;
        }
        return;
    }

    /**
     * @brief 辅助赋值函数（前向迭代器版本）
     * @tparam Iterator 迭代器类型
     * @param first 起始迭代器
     * @param last 结束迭代器
     */
    template <typename Iterator>
    NEFORCE_CONSTEXPR20 enable_if_t<is_ranges_fwd_iter_v<Iterator>> assign_aux(Iterator first, Iterator last) {
        const size_t n = _NEFORCE distance(first, last);
        if (n > capacity()) {
            pointer new_start = vector::allocate_and_copy(n, first, last);
            _NEFORCE destroy(start_, finish_);
            deallocate();
            start_ = new_start;
            finish_ = start_ + n;
            pair_.value = start_ + n;
        } else if (n > size()) {
            Iterator mid = first;
            _NEFORCE advance(mid, size());
            _NEFORCE copy(first, mid, start_);
            finish_ = _NEFORCE uninitialized_copy(mid, last, finish_);
        } else {
            _NEFORCE copy(first, last, start_);
            vector::erase(begin() + n, end());
        }
        return;
    }

public:
    /**
     * @brief 默认构造函数
     *
     * 构造一个空向量，初始容量为1。
     */
    NEFORCE_CONSTEXPR20 vector() {
        constexpr size_type init_cap = 1;
        pointer result = pair_.get_base().allocate(init_cap);
        finish_ = start_ = result;
        pair_.value = finish_ + init_cap;
    }

    /**
     * @brief 构造包含n个默认构造元素的向量
     * @param n 元素数量
     */
    NEFORCE_CONSTEXPR20 explicit vector(const size_type n) {
        start_ = pair_.get_base().allocate(n);
        finish_ = start_;
        try {
            for (size_type i = 0; i < n; ++i) {
                _NEFORCE construct(finish_);
                ++finish_;
            }
        } catch (...) {
            _NEFORCE destroy(start_, finish_);
            pair_.get_base().deallocate(start_, n);
            throw;
        }
        pair_.value = start_ + n;
    }

    /**
     * @brief 构造包含n个指定值元素的向量
     * @param n 64位无符号元素数量
     * @param value 初始值
     */
    NEFORCE_CONSTEXPR20 explicit vector(const size_type n, const T& value) { vector::fill_initialize(n, value); }

    /**
     * @brief 构造包含n个指定值元素的向量
     * @param n 32位元素数量
     * @param value 初始值
     */
    NEFORCE_CONSTEXPR20 explicit vector(const int32_t n, const T& value) { vector::fill_initialize(n, value); }

    /**
     * @brief 构造包含n个指定值元素的向量
     * @param n 64位元素数量
     * @param value 初始值
     */
    NEFORCE_CONSTEXPR20 explicit vector(const int64_t n, const T& value) { vector::fill_initialize(n, value); }

    /**
     * @brief 拷贝构造函数
     * @param other 源向量
     */
    NEFORCE_CONSTEXPR20 vector(const vector& other) {
        const size_type n = other.size();
        start_ = vector::allocate_and_copy(n, other.begin(), other.end());
        finish_ = start_ + n;
        pair_.value = finish_;
    }

    /**
     * @brief 拷贝赋值运算符
     * @param other 源向量
     * @return 自身引用
     */
    NEFORCE_CONSTEXPR20 vector& operator=(const vector& other) {
        if (_NEFORCE addressof(other) == this) {
            return *this;
        }

        vector::clear();
        vector::insert(end(), other.cbegin(), other.cend());
        return *this;
    }

    /**
     * @brief 移动构造函数
     * @param other 源向量
     */
    NEFORCE_CONSTEXPR20
    vector(vector&& other) noexcept(is_nothrow_swappable_v<compressed_pair<allocator_type, pointer>>) {
        vector::swap(other);
    }

    /**
     * @brief 移动赋值运算符
     * @param other 源向量
     * @return 自身引用
     */
    NEFORCE_CONSTEXPR20 vector&
    operator=(vector&& other) noexcept(is_nothrow_destructible_v<value_type> &&
                                       is_nothrow_swappable_v<compressed_pair<allocator_type, pointer>>) {
        if (_NEFORCE addressof(other) == this) {
            return *this;
        }

        vector::clear();
        vector::swap(other);
        return *this;
    }

    /**
     * @brief 范围构造函数
     * @tparam Iterator 迭代器类型
     * @param first 起始迭代器
     * @param last 结束迭代器
     */
    template <typename Iterator, enable_if_t<is_ranges_iter_v<Iterator>, int> = 0>
    NEFORCE_CONSTEXPR20 vector(Iterator first, Iterator last) {
        vector::range_initialize(first, last);
    }

    /**
     * @brief 范围构造函数
     * @tparam Iterator 迭代器类型
     * @param first 起始迭代器
     * @param n 迭代长度
     */
    template <typename Iterator>
    NEFORCE_CONSTEXPR20 vector(Iterator first, const size_type n) :
    vector(first, _NEFORCE next(first, n)) {}

    /**
     * @brief 初始化列表构造函数
     * @param ilist 初始化列表
     */
    NEFORCE_CONSTEXPR20 vector(std::initializer_list<T> ilist) :
    vector(ilist.begin(), ilist.end()) {}

    /**
     * @brief 内存视图构造函数
     * @param view 内存视图
     */
    NEFORCE_CONSTEXPR20 vector(memory_view<T> view) :
    vector(view.begin(), view.end()) {}

    /**
     * @brief 初始化列表赋值运算符
     * @param ilist 初始化列表
     * @return 自身引用
     */
    NEFORCE_CONSTEXPR20 vector& operator=(std::initializer_list<T> ilist) {
        if (ilist.size() > capacity()) {
            pointer new_ = vector::allocate_and_move(ilist.end() - ilist.begin(), ilist.begin(), ilist.end());
            _NEFORCE destroy(start_, finish_);
            deallocate();
            start_ = new_;
            finish_ = start_ + ilist.size();
            pair_.value = start_ + ilist.size();
        } else if (size() >= ilist.size()) {
            iterator i = _NEFORCE copy(ilist.begin(), ilist.end(), begin());
            _NEFORCE destroy(i.base(), finish_);
        } else {
            _NEFORCE copy(ilist.begin(), ilist.begin() + size(), start_);
            _NEFORCE uninitialized_copy(ilist.begin() + size(), ilist.end(), finish_);
        }
        finish_ = start_ + ilist.size();
        return *this;
    }

    /**
     * @brief 析构函数
     */
    NEFORCE_CONSTEXPR20 ~vector() {
        clear();
        deallocate();
    }

    /**
     * @brief 获取起始迭代器
     * @return 指向第一个元素的迭代器
     */
    NEFORCE_NODISCARD NEFORCE_CONSTEXPR20 iterator begin() noexcept { return iterator(start_, this); }

    /**
     * @brief 获取结束迭代器
     * @return 指向无效元素的迭代器
     */
    NEFORCE_NODISCARD NEFORCE_CONSTEXPR20 iterator end() noexcept { return iterator(finish_, this); }

    /**
     * @brief 获取常量起始迭代器
     * @return 指向第一个元素的常量迭代器
     */
    NEFORCE_NODISCARD NEFORCE_CONSTEXPR20 const_iterator begin() const noexcept { return cbegin(); }

    /**
     * @brief 获取常量结束迭代器
     * @return 指向无效元素的常量迭代器
     */
    NEFORCE_NODISCARD NEFORCE_CONSTEXPR20 const_iterator end() const noexcept { return cend(); }

    /**
     * @brief 获取反向起始迭代器
     * @return 指向无效元素的反向迭代器
     */
    NEFORCE_NODISCARD NEFORCE_CONSTEXPR20 reverse_iterator rbegin() noexcept { return reverse_iterator(end()); }

    /**
     * @brief 获取反向结束迭代器
     * @return 指向第一个元素的反向迭代器
     */
    NEFORCE_NODISCARD NEFORCE_CONSTEXPR20 reverse_iterator rend() noexcept { return reverse_iterator(begin()); }

    /**
     * @brief 获取常量反向起始迭代器
     * @return 指向无效元素的常量反向迭代器
     */
    NEFORCE_NODISCARD NEFORCE_CONSTEXPR20 const_reverse_iterator rbegin() const noexcept { return crbegin(); }

    /**
     * @brief 获取常量反向结束迭代器
     * @return 指向第一个元素的常量反向迭代器
     */
    NEFORCE_NODISCARD NEFORCE_CONSTEXPR20 const_reverse_iterator rend() const noexcept { return crend(); }

    /**
     * @brief 获取常量起始迭代器
     * @return 指向第一个元素的常量迭代器
     */
    NEFORCE_NODISCARD NEFORCE_CONSTEXPR20 const_iterator cbegin() const noexcept {
        return const_iterator(start_, this);
    }

    /**
     * @brief 获取常量结束迭代器
     * @return 指向无效元素的常量迭代器
     */
    NEFORCE_NODISCARD NEFORCE_CONSTEXPR20 const_iterator cend() const noexcept { return const_iterator(finish_, this); }

    /**
     * @brief 获取常量反向起始迭代器
     * @return 指向无效元素的常量反向迭代器
     */
    NEFORCE_NODISCARD NEFORCE_CONSTEXPR20 const_reverse_iterator crbegin() const noexcept {
        return const_reverse_iterator(cend());
    }

    /**
     * @brief 获取常量反向结束迭代器
     * @return 指向第一个元素的常量反向迭代器
     */
    NEFORCE_NODISCARD NEFORCE_CONSTEXPR20 const_reverse_iterator crend() const noexcept {
        return const_reverse_iterator(cbegin());
    }

    /**
     * @brief 获取当前元素数量
     * @return 元素数量
     */
    NEFORCE_NODISCARD NEFORCE_CONSTEXPR20 size_type size() const noexcept {
        return static_cast<size_type>(finish_ - start_);
    }

    /**
     * @brief 获取最大可能大小
     * @return 最大元素数量
     */
    NEFORCE_NODISCARD static constexpr size_type max_size() noexcept { return static_cast<size_type>(-1) / sizeof(T); }

    /**
     * @brief 获取当前容量
     * @return 已分配内存可容纳的元素数量
     */
    NEFORCE_NODISCARD NEFORCE_CONSTEXPR20 size_type capacity() const noexcept {
        return static_cast<size_type>(pair_.value - start_);
    }

    /**
     * @brief 检查是否为空
     * @return 是否为空
     */
    NEFORCE_NODISCARD NEFORCE_CONSTEXPR20 bool empty() const noexcept { return start_ == finish_; }

    /**
     * @brief 获取底层数据指针
     * @return 指向第一个元素的指针
     */
    NEFORCE_NODISCARD NEFORCE_CONSTEXPR20 pointer data() noexcept {
        NEFORCE_DEBUG_VERIFY(!empty(), "data called on empty vector");
        return start_;
    }

    /**
     * @brief 获取底层数据常量指针
     * @return 指向第一个元素的常量指针
     */
    NEFORCE_NODISCARD NEFORCE_CONSTEXPR20 const_pointer data() const noexcept {
        NEFORCE_DEBUG_VERIFY(!empty(), "data called on empty vector");
        return start_;
    }

    /**
     * @brief 获取底层数据的视图
     * @return 指向元素的视图
     */
    NEFORCE_NODISCARD NEFORCE_CONSTEXPR20 memory_view<T> view() noexcept {
        NEFORCE_DEBUG_VERIFY(!empty(), "data called on empty vector");
        return {start_, size()};
    }

    /**
     * @brief 获取底层数据的常量视图
     * @return 指向元素的常量视图
     */
    NEFORCE_NODISCARD NEFORCE_CONSTEXPR20 memory_view<const T> view() const noexcept {
        NEFORCE_DEBUG_VERIFY(!empty(), "data called on empty vector");
        return {start_, size()};
    }

    /**
     * @brief 获取底层数据的视图
     * @return 指向元素的视图
     */
    NEFORCE_NODISCARD NEFORCE_CONSTEXPR20 memory_view<T> view(const size_type off, size_type count = npos) noexcept {
        NEFORCE_DEBUG_VERIFY(!empty(), "data called on empty vector");
        count = _NEFORCE min(count, size() - off);
        return {start_ + off, count};
    }

    /**
     * @brief 获取底层数据的常量视图
     * @return 指向元素的常量视图
     */
    NEFORCE_NODISCARD NEFORCE_CONSTEXPR20 memory_view<const T> view(const size_type off,
                                                                    size_type count = npos) const noexcept {
        NEFORCE_DEBUG_VERIFY(!empty(), "data called on empty vector");
        count = _NEFORCE min(count, size() - off);
        return {start_ + off, count};
    }

    /**
     * @brief 访问第一个元素
     * @return 第一个元素的引用
     */
    NEFORCE_NODISCARD NEFORCE_CONSTEXPR20 reference front() noexcept {
        NEFORCE_DEBUG_VERIFY(!empty(), "front called on empty vector");
        return *start_;
    }

    /**
     * @brief 访问第一个常量元素
     * @return 第一个元素的常量引用
     */
    NEFORCE_NODISCARD NEFORCE_CONSTEXPR20 const_reference front() const noexcept {
        NEFORCE_DEBUG_VERIFY(!empty(), "front called on empty vector");
        return *start_;
    }

    /**
     * @brief 访问最后一个元素
     * @return 最后一个元素的引用
     */
    NEFORCE_NODISCARD NEFORCE_CONSTEXPR20 reference back() noexcept {
        NEFORCE_DEBUG_VERIFY(!empty(), "back called on empty vector");
        return *(finish_ - 1);
    }

    /**
     * @brief 访问最后一个常量元素
     * @return 最后一个元素的常量引用
     */
    NEFORCE_NODISCARD NEFORCE_CONSTEXPR20 const_reference back() const noexcept {
        NEFORCE_DEBUG_VERIFY(!empty(), "back called on empty vector");
        return *(finish_ - 1);
    }

    /**
     * @brief 预留容量
     * @param n 要预留的容量
     *
     * 确保向量有至少n的容量，如果当前容量不足则重新分配。
     */
    NEFORCE_CONSTEXPR20 void reserve(const size_type n) {
        NEFORCE_DEBUG_VERIFY(n < max_size(), "vector reserve out of allocate bounds.");
        if (capacity() >= n) {
            return;
        }

        size_type new_capacity = _NEFORCE max(capacity() * 2, n);
        pointer new_start = pair_.get_base().allocate(new_capacity);
        pointer new_finish = new_start;

        try {
            new_finish = _NEFORCE uninitialized_move(start_, finish_, new_start);
        } catch (...) {
            _NEFORCE destroy(new_start, new_finish);
            pair_.get_base().deallocate(new_start, new_capacity);
            throw;
        }

        _NEFORCE destroy(start_, finish_);
        deallocate();
        start_ = new_start;
        finish_ = new_finish;
        pair_.value = start_ + new_capacity;
    }

    /**
     * @brief 调整大小
     * @param new_size 新的大小
     * @param value 用于填充新元素的默认值
     *
     * 如果新大小小于当前大小，删除多余元素；
     * 如果大于当前大小，在末尾插入指定值的副本。
     */
    NEFORCE_CONSTEXPR20 void resize(size_type new_size, const T& value) {
        if (new_size < size()) {
            vector::erase(begin() + new_size, end());
        } else {
            vector::insert(end(), new_size - size(), value);
        }
    }

    /**
     * @brief 使用默认构造的元素调整大小
     * @param new_size 新的大小
     */
    NEFORCE_CONSTEXPR20 void resize(const size_type new_size) { vector::resize(new_size, T()); }

    /**
     * @brief 在指定位置构造元素
     * @tparam Args 构造参数类型
     * @param position 插入位置
     * @param args 构造参数
     */
    template <typename... Args>
    NEFORCE_CONSTEXPR20 void emplace(iterator position, Args&&... args) {
        if (finish_ != pair_.value) {
            if (position == end()) {
                _NEFORCE construct(finish_, _NEFORCE forward<Args>(args)...);
                ++finish_;
            } else {
                _NEFORCE construct(finish_, _NEFORCE move(*(finish_ - 1)));
                ++finish_;
                pointer pos_ptr = position.base();
                _NEFORCE move_backward(pos_ptr, finish_ - 2, finish_ - 1);
                _NEFORCE construct(pos_ptr, _NEFORCE forward<Args>(args)...);
            }
            return;
        }

        const size_type old_size = size();
        const size_type new_cap = old_size != 0 ? 2 * old_size : 1;
        pointer new_start = pair_.get_base().allocate(new_cap);
        pointer new_finish = new_start;

        try {
            new_finish = _NEFORCE uninitialized_move(begin(), position, new_start);
            _NEFORCE construct(new_finish, _NEFORCE forward<Args>(args)...);
            ++new_finish;
            new_finish = _NEFORCE uninitialized_move(position, end(), new_finish);
        } catch (...) {
            _NEFORCE destroy(new_start, new_finish);
            pair_.get_base().deallocate(new_start, new_cap);
            throw;
        }

        _NEFORCE destroy(begin(), end());
        deallocate();

        start_ = new_start;
        finish_ = new_finish;
        pair_.value = new_start + new_cap;
    }

    /**
     * @brief 在末尾构造元素
     * @tparam Args 构造参数类型
     * @param args 构造参数
     */
    template <typename... Args>
    NEFORCE_CONSTEXPR20 void emplace_back(Args&&... args) {
        if (finish_ != pair_.value) {
            _NEFORCE construct(finish_, _NEFORCE forward<Args>(args)...);
            ++finish_;
        } else {
            vector::emplace(end(), _NEFORCE forward<Args>(args)...);
        }
    }

    /**
     * @brief 在末尾拷贝插入元素
     * @param value 要插入的值
     */
    NEFORCE_CONSTEXPR20 void push_back(const T& value) { vector::emplace_back(value); }

    /**
     * @brief 在末尾移动插入元素
     * @param value 要插入的值
     */
    NEFORCE_CONSTEXPR20 void push_back(T&& value) { vector::emplace_back(_NEFORCE move(value)); }

    /**
     * @brief 移除末尾元素
     */
    NEFORCE_CONSTEXPR20 void pop_back() noexcept(is_nothrow_destructible_v<T>) {
        NEFORCE_DEBUG_VERIFY(!empty(), "pop called in an empty vector")
        _NEFORCE destroy(finish_ - 1);
        --finish_;
    }

    /**
     * @brief 移除并返回末尾元素
     * @return 被移除的元素
     */
    NEFORCE_CONSTEXPR20 T pop_back_v() noexcept(is_nothrow_destructible_v<T> && is_nothrow_move_constructible_v<T>) {
        NEFORCE_DEBUG_VERIFY(!empty(), "pop called in an empty vector")
        T value{_NEFORCE move(finish_ - 1)};
        _NEFORCE destroy(finish_ - 1);
        --finish_;
        return _NEFORCE move(value);
    }

    /**
     * @brief 赋值n个指定值的元素
     * @param n 元素数量
     * @param value 要赋的值
     */
    NEFORCE_CONSTEXPR20 void assign(size_type n, const value_type& value) {
        if (n > capacity()) {
            pointer new_start = pair_.get_base().allocate(n);
            pointer new_finish = new_start;

            try {
                new_finish = _NEFORCE uninitialized_fill_n(new_start, n, value);
            } catch (...) {
                _NEFORCE destroy(new_start, new_finish);
                pair_.get_base().deallocate(new_start, n);
                throw;
            }

            _NEFORCE destroy(start_, finish_);
            deallocate();

            start_ = new_start;
            finish_ = new_finish;
            pair_.value = start_ + n;
        } else if (n > size()) {
            _NEFORCE fill(begin(), end(), value);
            finish_ = _NEFORCE uninitialized_fill_n(finish_, n - size(), value);
        } else {
            _NEFORCE fill_n(begin(), n, value);
            vector::erase(begin() + n, end());
        }
    }

    /**
     * @brief 范围赋值
     * @tparam Iterator 迭代器类型
     * @param first 起始迭代器
     * @param last 结束迭代器
     */
    template <typename Iterator, enable_if_t<is_iter_v<Iterator>, int> = 0>
    NEFORCE_CONSTEXPR20 void assign(Iterator first, Iterator last) {
        vector::assign_aux(first, last);
    }

    /**
     * @brief 初始化列表赋值
     * @param ilist 初始化列表
     */
    NEFORCE_CONSTEXPR20 void assign(std::initializer_list<value_type> ilist) {
        vector::assign(ilist.begin(), ilist.end());
    }

    /**
     * @brief 在指定位置拷贝插入元素
     * @param position 插入位置
     * @param value 要插入的值
     * @return 指向插入元素的迭代器
     */
    NEFORCE_CONSTEXPR20 iterator insert(iterator position, const value_type& value) {
        size_type n = position - begin();
        vector::emplace(position, value);
        return begin() + n;
    }

    /**
     * @brief 在指定位置移动插入元素
     * @param position 插入位置
     * @param value 要插入的值
     * @return 指向插入元素的迭代器
     */
    NEFORCE_CONSTEXPR20 iterator insert(iterator position, value_type&& value) {
        size_type n = position - begin();
        vector::emplace(position, _NEFORCE move(value));
        return begin() + n;
    }

    /**
     * @brief 在指定位置插入默认构造的元素
     * @param position 插入位置
     * @return 指向插入元素的迭代器
     */
    NEFORCE_CONSTEXPR20 iterator insert(iterator position) { return vector::insert(position, T()); }

    /**
     * @brief 范围插入
     * @tparam Iterator 迭代器类型
     * @param position 插入位置
     * @param first 起始迭代器
     * @param last 结束迭代器
     */
    template <typename Iterator, enable_if_t<is_iter_v<Iterator>, int> = 0>
    NEFORCE_CONSTEXPR20 void insert(iterator position, Iterator first, Iterator last) {
        vector::range_insert(position, first, last);
    }

    /**
     * @brief 初始化列表插入
     * @param position 插入位置
     * @param ilist 初始化列表
     */
    NEFORCE_CONSTEXPR20 void insert(iterator position, std::initializer_list<value_type> ilist) {
        vector::insert(position, ilist.begin(), ilist.end());
    }

    /**
     * @brief 插入n个指定值的元素
     * @param position 插入位置
     * @param n 元素数量
     * @param value 要插入的值
     */
    NEFORCE_CONSTEXPR20 void insert(iterator position, size_type n, const value_type& value) {
        if (n == 0) {
            return;
        }

        if (static_cast<size_type>(pair_.value - finish_) >= n) {
            const size_type elems_after = _NEFORCE distance(begin(), position);
            iterator old_finish = end();

            if (elems_after > n) {
                _NEFORCE uninitialized_copy(finish_ - n, finish_, finish_);
                finish_ += n;
                _NEFORCE copy_backward(position, old_finish - n, old_finish);
                _NEFORCE fill(position, position + n, value);
            } else {
                _NEFORCE uninitialized_fill_n(finish_, n - elems_after, value);
                finish_ += n - elems_after;
                _NEFORCE uninitialized_move(position, old_finish, end());
                finish_ += elems_after;
                _NEFORCE destroy(position, old_finish);
                _NEFORCE uninitialized_fill(position, old_finish, value);
            }
        } else {
            const size_type old_size = size();
            const size_type len = old_size + _NEFORCE max(old_size, n);

            pointer new_start = pair_.get_base().allocate(len);
            pointer new_finish = _NEFORCE uninitialized_copy(begin(), position, new_start);
            new_finish = _NEFORCE uninitialized_fill_n(new_finish, n, value);
            new_finish = _NEFORCE uninitialized_copy(position, end(), new_finish);

            _NEFORCE destroy(start_, finish_);
            deallocate();

            start_ = new_start;
            finish_ = new_finish;
            pair_.value = new_start + len;
        }
    }

    /**
     * @brief 删除指定范围内的元素
     * @param first 起始迭代器
     * @param last 结束迭代器
     * @return 指向最后一个被删除元素之后位置的迭代器
     */
    NEFORCE_CONSTEXPR20 iterator erase(iterator first,
                                       iterator last) noexcept(is_nothrow_move_assignable_v<value_type> &&
                                                               is_nothrow_destructible_v<value_type>) {
        NEFORCE_DEBUG_VERIFY(_NEFORCE distance(first, last) >= 0, "vector erase out of ranges.");

        const auto elems_after = end() - last;
        if (elems_after > 0) {
            _NEFORCE move_backward(last, end(), first + elems_after);
        }

        pointer new_finish = finish_ - (last - first);
        _NEFORCE destroy(new_finish, finish_);
        finish_ = new_finish;
        return first;
    }

    /**
     * @brief 删除指定位置的元素
     * @param position 要删除的位置
     * @return 指向被删除元素之后位置的迭代器
     */
    NEFORCE_CONSTEXPR20 iterator erase(iterator position) noexcept(is_nothrow_move_assignable_v<value_type>) {
        if (position + 1 != end()) {
            _NEFORCE move(position + 1, end(), position);
        }

        --finish_;
        _NEFORCE destroy(finish_);
        return position;
    }

    /**
     * @brief 收缩容量以适应当前大小
     *
     * 释放多余的容量，使capacity()等于size()。
     */
    NEFORCE_CONSTEXPR20 void shrink_to_fit() {
        if (capacity() == size()) {
            return;
        }

        if (size() == 0) {
            deallocate();
            start_ = finish_ = pair_.value = nullptr;
            return;
        }

        pointer new_start = pair_.get_base().allocate(size());
        pointer new_finish = new_start;

        try {
            new_finish = _NEFORCE uninitialized_move(start_, finish_, new_start);
        } catch (...) {
            _NEFORCE destroy(new_start, new_finish);
            pair_.get_base().deallocate(new_start, size());
            throw;
        }

        _NEFORCE destroy(start_, finish_);
        deallocate();

        start_ = new_start;
        finish_ = new_finish;
        pair_.value = new_start + size();
    }

    /**
     * @brief 清空向量
     *
     * 销毁所有元素，但保留容量。
     */
    NEFORCE_CONSTEXPR20 void clear() noexcept(is_nothrow_destructible_v<value_type>) {
        if (empty()) {
            return;
        }

        _NEFORCE destroy(start_, finish_);
        finish_ = start_;
    }

    /**
     * @brief 带边界检查的常量索引访问
     * @param position 索引位置
     * @return 指定位置元素的常量引用
     */
    NEFORCE_NODISCARD NEFORCE_CONSTEXPR20 const_reference at(const size_type position) const noexcept {
        NEFORCE_DEBUG_VERIFY(position < size(), "vector access out of range");
        return *(start_ + position);
    }

    /**
     * @brief 带边界检查的索引访问
     * @param position 索引位置
     * @return 指定位置元素的引用
     */
    NEFORCE_NODISCARD NEFORCE_CONSTEXPR20 reference at(const size_type position) noexcept {
        NEFORCE_DEBUG_VERIFY(position < size(), "vector access out of range");
        return *(start_ + position);
    }

    /**
     * @brief 常量下标访问操作符
     * @param position 索引位置
     * @return 指定位置元素的常量引用
     */
    NEFORCE_NODISCARD NEFORCE_CONSTEXPR20 const_reference operator[](const size_type position) const noexcept {
        return at(position);
    }

    /**
     * @brief 下标访问操作符
     * @param position 索引位置
     * @return 指定位置元素的引用
     */
    NEFORCE_NODISCARD NEFORCE_CONSTEXPR20 reference operator[](const size_type position) noexcept {
        return at(position);
    }

    /**
     * @brief 交换两个向量的内容
     * @param other 要交换的另一个向量
     */
    NEFORCE_CONSTEXPR20 void swap(vector& other) noexcept(is_nothrow_swappable_v<allocator_type>) {
        if (_NEFORCE addressof(other) == this) {
            return;
        }

        _NEFORCE swap(start_, other.start_);
        _NEFORCE swap(finish_, other.finish_);
        _NEFORCE swap(pair_, other.pair_);
    }

    /**
     * @brief 相等比较操作符
     * @param rhs 右侧向量
     * @return 如果两个向量大小相等且对应元素相等返回true
     */
    NEFORCE_NODISCARD NEFORCE_CONSTEXPR20 bool equal_to(const vector& rhs) const
            noexcept(noexcept(_NEFORCE equal(cbegin(), cend(), rhs.cbegin()))) {
        return size() == rhs.size() && _NEFORCE equal(cbegin(), cend(), rhs.cbegin());
    }

    /**
     * @brief 小于比较操作符
     * @param rhs 右侧向量
     * @return 按字典序比较结果
     */
    NEFORCE_NODISCARD NEFORCE_CONSTEXPR20 bool less_than(const vector& rhs) const
            noexcept(noexcept(_NEFORCE lexicographical_compare(cbegin(), cend(), rhs.cbegin(), rhs.cend()))) {
        return _NEFORCE lexicographical_compare(cbegin(), cend(), rhs.cbegin(), rhs.cend());
    }
};

#ifdef NEFORCE_STANDARD_17
template <typename T, typename Alloc>
vector(T, Alloc = Alloc()) -> vector<T, Alloc>;

template <typename Iterator, typename Alloc>
vector(Iterator, Iterator, Alloc = Alloc()) -> vector<iter_value_t<Iterator>, Alloc>;
#endif

/**
 * @brief 字节向量类型别名
 */
using byte_vector = vector<byte_t>;

/** @} */ // Container

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_CORE_CONTAINER_VECTOR_HPP__
