#ifndef NEFORCE_CORE_CONTAINER_DEQUE_HPP__
#define NEFORCE_CORE_CONTAINER_DEQUE_HPP__

/**
 * @file deque.hpp
 * @brief 双端队列容器
 *
 * 此文件提供了双端队列容器的实现。
 * 支持在两端高效插入/删除元素。
 */

#include "NeForce/core/algorithm/compare.hpp"
#include "NeForce/core/algorithm/shift.hpp"
#include "NeForce/core/interface/icollector.hpp"
#include "NeForce/core/interface/iiterator.hpp"
#include "NeForce/core/memory/standard_allocator.hpp"
#include "NeForce/core/memory/uninitialized.hpp"
#include "NeForce/core/utility/compressed_pair.hpp"
NEFORCE_BEGIN_NAMESPACE__

/**
 * @defgroup Container 标准容器
 * @brief 支持标准算法的容器的实现
 * @{
 */

/**
 * @struct deque_iterator
 * @brief 双端队列迭代器
 * @tparam IsConst 是否常量迭代器
 * @tparam Deque 双端队列类型
 * @tparam BufSize 缓冲区大小
 *
 * 为deque提供随机访问迭代器支持，处理跨缓冲区的遍历操作。
 * 维护当前元素指针、当前缓冲区边界和节点映射表指针。
 */
template <bool IsConst, typename Deque, size_t BufSize = 0>
struct deque_iterator : iiterator<deque_iterator<IsConst, Deque, BufSize>> {
public:
    using container_type = Deque;                                     ///< 容器类型
    using value_type = typename container_type::value_type;           ///< 值类型
    using size_type = typename container_type::size_type;             ///< 大小类型
    using difference_type = typename container_type::difference_type; ///< 差值类型
    using iterator_category = random_access_iterator_tag;             ///< 迭代器类别（随机访问）
    using reference = conditional_t<IsConst, typename container_type::const_reference,
                                    typename container_type::reference>; ///< 引用类型
    using pointer = conditional_t<IsConst, typename container_type::const_pointer,
                                  typename container_type::pointer>; ///< 指针类型

    /**
     * @brief 计算双端队列缓冲区大小
     * @param n 用户指定的缓冲区大小
     * @param sz 元素类型大小
     * @return 计算得到的缓冲区大小
     *
     * 如果n不为0，返回n；否则根据元素大小计算合适的缓冲区大小：
     * 元素大小小于256字节时返回4096/sz，否则返回16。
     */
    static constexpr size_t deque_buf_size(const size_t n, const size_t sz) noexcept {
        constexpr size_t buffer_threshhold = 256;
        constexpr size_t buffer_max_size = MEMORY_BIG_ALLOC_THRESHHOLD;
        return n != 0 ? n : sz < buffer_threshhold ? buffer_max_size / sz : 16;
    }

    /// 缓冲区大小
    static constexpr difference_type buffer_size = deque_buf_size(BufSize, sizeof(value_type));

private:
    pointer current_ = nullptr;                 ///< 指向当前元素
    pointer first_ = nullptr;                   ///< 指向当前缓冲区的起始位置
    pointer last_ = nullptr;                    ///< 指向当前缓冲区的结束位置
    pointer* node_ = nullptr;                   ///< 指向当前节点
    const container_type* container_ = nullptr; ///< 关联容器指针

    template <typename, typename, size_t>
    friend class deque;
    template <bool, typename, size_t>
    friend struct deque_iterator;

private:
    /**
     * @brief 切换到新的缓冲区
     * @param new_map 新的节点指针
     *
     * 更新node_指向新的节点，并更新first_和last_为新缓冲区的边界。
     */
    void change_buff(pointer* new_map) noexcept {
        node_ = new_map;
        first_ = *new_map;
        last_ = first_ + buffer_size;
    }

public:
    deque_iterator() noexcept = default;
    ~deque_iterator() = default;

    deque_iterator(const deque_iterator&) noexcept = default;
    deque_iterator& operator=(const deque_iterator&) noexcept = default;
    deque_iterator(deque_iterator&&) noexcept = default;
    deque_iterator& operator=(deque_iterator&&) noexcept = default;

    /**
     * @brief 构造函数
     * @param cur 当前元素指针
     * @param map 节点指针
     * @param deq 容器指针
     */
    deque_iterator(pointer cur, pointer* map, const container_type* deq) :
    current_(cur),
    first_(*map),
    last_(*map + buffer_size),
    node_(map),
    container_(deq) {}

    /**
     * @brief 构造函数
     * @param cur 当前元素指针
     * @param first 缓冲区起始指针
     * @param last 缓冲区结束指针
     * @param node 节点指针
     * @param deq 容器指针
     */
    deque_iterator(pointer cur, pointer first, pointer last, pointer* node, const container_type* deq) noexcept :
    current_(cur),
    first_(first),
    last_(last),
    node_(node),
    container_(deq) {}

    /**
     * @brief 从另一个迭代器转换构造（常量/非常量转换）
     * @tparam IsConst2 源迭代器的常量性
     * @param other 源迭代器
     */
    template <bool IsConst2>
    explicit deque_iterator(const deque_iterator<IsConst2, Deque, BufSize>& other) noexcept :
    current_(const_cast<pointer>(other.current_)),
    first_(const_cast<pointer>(other.first_)),
    last_(const_cast<pointer>(other.last_)),
    node_(const_cast<pointer*>(other.node_)),
    container_(other.container_) {}

    /**
     * @brief 解引用操作
     * @return 当前元素的引用
     */
    NEFORCE_NODISCARD reference dereference() const noexcept {
        NEFORCE_DEBUG_VERIFY(current_ && container_, "Attempting to dereference on a null pointer");
        NEFORCE_DEBUG_VERIFY(first_ <= current_ && current_ < last_, "Attempting to dereference out of boundary");
        return *current_;
    }

    /**
     * @brief 递增操作
     *
     * 如果当前指针到达缓冲区末尾，切换到下一个缓冲区。
     */
    void increment() noexcept {
        NEFORCE_DEBUG_VERIFY(current_ && container_, "Attempting to increment a null pointer");
        ++current_;
        if (current_ == last_) {
            deque_iterator::change_buff(node_ + 1);
            current_ = first_;
        }
    }

    /**
     * @brief 递减操作
     *
     * 如果当前指针到达缓冲区起始，切换到上一个缓冲区。
     */
    void decrement() noexcept {
        NEFORCE_DEBUG_VERIFY(current_ && container_, "Attempting to decrement a null pointer");
        if (current_ == first_) {
            deque_iterator::change_buff(node_ - 1);
            current_ = last_;
        }
        --current_;
    }

    /**
     * @brief 前进操作
     * @param off 前进距离
     *
     * 处理跨缓冲区的前进/后退操作。
     */
    void advance(difference_type off) noexcept {
        NEFORCE_DEBUG_VERIFY((current_ && container_) || off == 0, "Attempting to advance a null pointer");
        const difference_type offset = off + (current_ - first_);
        if (offset >= 0 && offset < buffer_size) {
            current_ += off;
        } else {
            difference_type node_offset =
                    offset > 0 ? offset / buffer_size : -static_cast<difference_type>((-offset - 1) / buffer_size) - 1;
            deque_iterator::change_buff(node_ + node_offset);
            current_ = first_ + (offset - node_offset * buffer_size);
        }
    }

    /**
     * @brief 计算距离操作
     * @param other 另一个迭代器
     * @return 两个迭代器之间的距离
     *
     * 考虑缓冲区间隔计算准确距离。
     */
    NEFORCE_NODISCARD NEFORCE_CONSTEXPR20 difference_type distance_to(const deque_iterator& other) const noexcept {
        NEFORCE_DEBUG_VERIFY(container_ == other.container_, "Attempting to distance to a different container");
        return (node_ - other.node_) * buffer_size + (current_ - first_) - (other.current_ - other.first_);
    }

    /**
     * @brief 下标访问操作符
     * @param n 偏移量
     * @return 偏移位置元素的引用
     */
    NEFORCE_NODISCARD reference operator[](const difference_type n) noexcept { return *(*this + n); }

    /**
     * @brief 相等比较
     * @param rhs 右侧迭代器
     * @return 是否相等
     */
    NEFORCE_NODISCARD bool equal(const deque_iterator& rhs) const noexcept {
        NEFORCE_DEBUG_VERIFY(container_ == rhs.container_, "Attempting to equal to a different container");
        return current_ == rhs.current_;
    }

    /**
     * @brief 小于比较
     * @param rhs 右侧迭代器
     * @return 当前迭代器是否在rhs之前
     */
    NEFORCE_NODISCARD bool less_than(const deque_iterator& rhs) const noexcept {
        NEFORCE_DEBUG_VERIFY(container_ == rhs.container_, "Attempting to less than a different container");
        return node_ == rhs.node_ ? current_ < rhs.current_ : node_ < rhs.node_;
    }

    /**
     * @brief 获取底层指针
     * @return 当前元素指针
     */
    NEFORCE_NODISCARD pointer base() const noexcept { return current_; }

    /**
     * @brief 获取关联容器
     * @return 关联容器指针
     */
    NEFORCE_NODISCARD const container_type* container() const noexcept { return container_; }
};


/**
 * @class deque
 * @brief 双端队列容器
 * @tparam T 元素类型
 * @tparam Alloc 分配器类型
 * @tparam BufSize 缓冲区大小（0表示自动计算）
 *
 * 双端队列是一种支持在两端高效插入和删除的序列容器。
 * 采用分块存储结构：元素被分割成多个固定大小的缓冲区，
 * 这些缓冲区的指针存储在中控器中，从而实现动态增长和两端操作。
 * 提供随机访问迭代器，但迭代器可能因重新分配而失效。
 */
template <typename T, typename Alloc = allocator<T>, size_t BufSize = 0>
class deque : public icollector<deque<T, Alloc>> {
    static_assert(is_allocator_v<Alloc>, "Alloc type is not a standard allocator type.");
    static_assert(is_same_v<T, typename Alloc::value_type>, "allocator type mismatch.");
    static_assert(is_object_v<T>, "deque only contains object types.");

public:
    using pointer = T*;                                                       ///< 指针类型
    using reference = T&;                                                     ///< 引用类型
    using const_pointer = const T*;                                           ///< 常量指针类型
    using const_reference = const T&;                                         ///< 常量引用类型
    using value_type = T;                                                     ///< 值类型
    using size_type = size_t;                                                 ///< 大小类型
    using difference_type = ptrdiff_t;                                        ///< 差值类型
    using iterator = deque_iterator<false, deque, BufSize>;                   ///< 迭代器类型
    using const_iterator = deque_iterator<true, deque, BufSize>;              ///< 常量迭代器类型
    using reverse_iterator = _NEFORCE reverse_iterator<iterator>;             ///< 反向迭代器类型
    using const_reverse_iterator = _NEFORCE reverse_iterator<const_iterator>; ///< 常量反向迭代器类型
    using allocator_type = Alloc;                                             ///< 分配器类型

    /// 缓冲区大小
    static constexpr difference_type buffer_size = iterator::buffer_size;

private:
    using map_pointer = pointer*;                                          ///< 中控器指针类型
    using map_allocator = typename Alloc::template rebind<pointer>::other; ///< 中控器分配器类型

    iterator start_{};  ///< 起始迭代器
    iterator finish_{}; ///< 结束迭代器
    // 压缩存储：分配器 + 中控器大小
    compressed_pair<allocator_type, size_type> map_size_pair_{default_construct_tag{}, 0};
    // 压缩存储：中控器分配器 + 中控器指针
    compressed_pair<map_allocator, map_pointer> map_pair_{default_construct_tag{}, nullptr};

private:
    /// 初始中控器大小
    static constexpr size_t init_map_size = 8;

    /**
     * @brief 创建中控器
     * @param n 中控器大小
     * @return 创建的中控器指针
     */
    map_pointer create_map(const size_t n) {
        map_pointer map = map_pair_.get_base().allocate(n);
        for (size_t i = 0; i < n; ++i) {
            *(map + i) = nullptr;
        }
        return map;
    }

    /**
     * @brief 创建节点（缓冲区）
     * @param start 起始节点指针
     * @param finish 结束节点指针
     *
     * 为指定范围内的每个节点分配缓冲区内存。
     */
    void create_nodes(map_pointer start, map_pointer finish) {
        map_pointer cur;
        try {
            for (cur = start; cur <= finish; ++cur) {
                *cur = map_size_pair_.get_base().allocate(buffer_size);
            }
        } catch (...) {
            while (cur != start) {
                --cur;
                map_size_pair_.get_base().deallocate(*cur, buffer_size);
                *cur = nullptr;
            }
            throw;
        }
    }

    /**
     * @brief 销毁节点（缓冲区）
     * @param start 起始节点指针
     * @param finish 结束节点指针
     *
     * 释放指定范围内节点的缓冲区内存。
     */
    void destroy_nodes(map_pointer start, map_pointer finish) noexcept(is_nothrow_destructible_v<value_type>) {
        for (map_pointer cur = start; cur <= finish; ++cur) {
            if (*cur == nullptr) {
                continue;
            }
            map_size_pair_.get_base().deallocate(*cur, buffer_size);
            *cur = nullptr;
        }
    }

    /**
     * @brief 创建中控器和节点
     * @param n 元素数量
     *
     * 根据元素数量分配中控器和必要的缓冲区。
     */
    void create_map_and_nodes(const size_type n) {
        size_type node_nums = n / buffer_size + (n % buffer_size ? 1 : 0);
        map_size_pair_.value = _NEFORCE max(init_map_size, node_nums + 2);

        try {
            map_pair_.value = deque::create_map(map_size_pair_.value);
        } catch (...) {
            map_pair_.value = nullptr;
            map_size_pair_.value = 0;
            throw;
        }

        map_pointer nstart = map_pair_.value + (map_size_pair_.value - node_nums) / 2;
        map_pointer nfinish = node_nums == 0 ? nstart : nstart + node_nums - 1;

        try {
            deque::create_nodes(nstart, nfinish);
        } catch (...) {
            deque::destroy_nodes(map_pair_.value, map_pair_.value + map_size_pair_.value - 1);
            map_pair_.get_base().deallocate(map_pair_.value, map_size_pair_.value);
            map_pair_.value = nullptr;
            map_size_pair_.value = 0;
            throw;
        }

        start_.change_buff(nstart);
        finish_.change_buff(nfinish);
        start_.current_ = start_.first_;
        finish_.current_ = n == 0 ? start_.first_ : finish_.first_ + (n % buffer_size ? n % buffer_size : buffer_size);
        start_.container_ = this;
        finish_.container_ = this;
    }

    /**
     * @brief 填充初始化
     * @param n 元素数量
     * @param value 填充值
     */
    void fill_initialize(const size_type n, const T& value) {
        deque::create_map_and_nodes(n);
        if (n == 0) {
            return;
        }

        for (map_pointer cur = start_.node_; cur < finish_.node_; ++cur) {
            _NEFORCE uninitialized_fill(*cur, *cur + buffer_size, value);
        }
        _NEFORCE uninitialized_fill(finish_.first_, finish_.current_, value);
    }

    /**
     * @brief 范围拷贝初始化（非前向迭代器版本）
     * @tparam Iterator 迭代器类型
     * @param first 起始迭代器
     * @param last 结束迭代器
     */
    template <typename Iterator>
    enable_if_t<!is_ranges_fwd_iter_v<Iterator>> copy_initialize(Iterator first, Iterator last) {
        deque::create_map_and_nodes(0);
        deque::insert(end(), first, last);
        return;
    }

    /**
     * @brief 范围拷贝初始化（前向迭代器版本）
     * @tparam Iterator 迭代器类型
     * @param first 起始迭代器
     * @param last 结束迭代器
     */
    template <typename Iterator>
    enable_if_t<is_ranges_fwd_iter_v<Iterator>> copy_initialize(Iterator first, Iterator last) {
        deque::create_map_and_nodes(_NEFORCE distance(first, last));

        for (map_pointer cur = start_.node_; cur < finish_.node_; ++cur) {
            Iterator next = _NEFORCE next(first, buffer_size);
            _NEFORCE uninitialized_copy(first, next, *cur);
            first = next;
        }
        _NEFORCE uninitialized_copy(first, last, finish_.first_);
        return;
    }

    /**
     * @brief 辅助赋值函数
     * @param n 元素数量
     * @param value 要赋的值
     */
    void assign_aux_n(size_type n, const T& value) {
        if (n > size()) {
            _NEFORCE fill(begin(), end(), value);
            deque::insert(end(), n - size(), value);
        } else {
            deque::erase(begin() + n, end());
            _NEFORCE fill(begin(), end(), value);
        }
    }

    /**
     * @brief 范围赋值（非前向迭代器版本）
     * @tparam Iterator 迭代器类型
     * @param first 起始迭代器
     * @param last 结束迭代器
     */
    template <typename Iterator>
    enable_if_t<!is_ranges_fwd_iter_v<Iterator>> assign_ranges(Iterator first, Iterator last) {
        clear();
        deque::insert(end(), first, last);
        return;
    }

    /**
     * @brief 范围赋值（前向迭代器版本）
     * @tparam Iterator 迭代器类型
     * @param first 起始迭代器
     * @param last 结束迭代器
     */
    template <typename Iterator>
    enable_if_t<is_ranges_fwd_iter_v<Iterator>> assign_ranges(Iterator first, Iterator last) {
        auto first1 = begin();
        auto last1 = end();

        for (; first != last && first1 != last1; ++first, ++first1) {
            *first1 = *first;
        }

        if (first1 != last1) {
            deque::erase(first1, last1);
        } else {
            deque::insert(first1, last1);
        }
        return;
    }

    /**
     * @brief 重新分配中控器
     * @param n 需要新增的缓冲区数量
     * @param add_at_front 是否在前端添加
     *
     * 当需要在前端或后端添加新缓冲区而空间不足时，
     * 重新分配中控器以容纳更多缓冲区指针。
     */
    void reallocate_map(const size_type n, const bool add_at_front) {
        const size_type begin_left = start_.current_ - start_.first_;

        if (add_at_front && begin_left < n) {
            const size_t needed = (n - begin_left) / buffer_size + 1;

            if (needed > static_cast<size_type>(start_.node_ - map_pair_.value)) {
                const size_type new_size =
                        _NEFORCE max(map_size_pair_.value << 1, map_size_pair_.value + needed + init_map_size);
                map_pointer map = deque::create_map(new_size);
                const size_type old_buf = finish_.node_ - start_.node_ + 1;
                const size_type new_buf = needed + old_buf;

                auto begin = map + (new_size - new_buf) / 2;
                auto mid = begin + needed;
                auto end = mid + old_buf;

                deque::create_nodes(begin, mid - 1);
                for (auto begin1 = mid, begin2 = start_.node_; begin1 != end; ++begin1, ++begin2) {
                    *begin1 = *begin2;
                }

                map_pair_.get_base().deallocate(map_pair_.value, map_size_pair_.value);
                map_pair_.value = map;
                map_size_pair_.value = new_size;
                start_ = iterator(*mid + (start_.current_ - start_.first_), mid, this);
                finish_ = iterator(*(end - 1) + (finish_.current_ - finish_.first_), end - 1, this);
                return;
            }

            deque::create_nodes(start_.node_ - needed, start_.node_ - 1);
            return;
        }

        const size_type end_left = finish_.last_ - finish_.current_ - 1;

        if (!add_at_front && end_left < n) {

            const size_type needed = (n - end_left) / buffer_size + 1;

            if (needed > static_cast<size_type>((map_pair_.value + map_size_pair_.value) - finish_.node_ - 1)) {
                const size_type new_size =
                        _NEFORCE max(map_size_pair_.value << 1, map_size_pair_.value + needed + init_map_size);
                map_pointer map = deque::create_map(new_size);
                const size_type old_buf = finish_.node_ - start_.node_ + 1;
                const size_type new_buf = needed + old_buf;

                auto begin = map + (new_size - new_buf) / 2;
                auto mid = begin + old_buf;
                auto end = mid + needed;

                for (auto begin1 = begin, begin2 = start_.node_; begin1 != mid; ++begin1, ++begin2) {
                    *begin1 = *begin2;
                }
                deque::create_nodes(mid, end - 1);

                map_pair_.get_base().deallocate(map_pair_.value, map_size_pair_.value);
                map_pair_.value = map;
                map_size_pair_.value = new_size;
                start_ = iterator(*begin + (start_.current_ - start_.first_), begin, this);
                finish_ = iterator(*(mid - 1) + (finish_.current_ - finish_.first_), mid - 1, this);
                return;
            }

            deque::create_nodes(finish_.node_ + 1, finish_.node_ + needed);
        }
    }

    /**
     * @brief 范围插入辅助函数
     * @tparam Iterator 迭代器类型
     * @param position 插入位置
     * @param first 起始迭代器
     * @param last 结束迭代器
     * @param n 元素数量
     */
    template <typename Iterator>
    void insert_ranges_n(iterator position, Iterator first, Iterator last, size_type n) {
        difference_type dist_before = position - start_;

        if (dist_before < static_cast<difference_type>(size() / 2)) {
            deque::reallocate_map(n, true);
            auto old_start = start_;
            auto new_start = start_ - n;
            position = start_ + dist_before;

            try {
                if (dist_before >= n) {
                    iterator start_n = start_ + n;
                    _NEFORCE uninitialized_copy(start_, start_n, new_start);
                    start_ = new_start;
                    _NEFORCE copy(start_n, position, old_start);
                    _NEFORCE copy(first, last, position - n);
                } else {
                    auto mid = _NEFORCE next(first, n - dist_before);
                    _NEFORCE uninitialized_copy(first, mid, _NEFORCE uninitialized_copy(start_, position, new_start));
                    start_ = new_start;
                    _NEFORCE copy(mid, last, old_start);
                }
            } catch (...) {
                if (new_start.node_ != start_.node_) {
                    deque::destroy_nodes(new_start.node_, start_.node_ - 1);
                }
                throw;
            }
        } else {
            deque::reallocate_map(n, false);
            auto old_finish = finish_;
            auto new_finish = finish_ + n;
            const size_type dist_after = size() - dist_before;
            position = finish_ - dist_after;

            try {
                if (dist_after > n) {
                    auto finish_n = finish_ - n;
                    _NEFORCE uninitialized_copy(finish_n, finish_, finish_);
                    finish_ = new_finish;
                    _NEFORCE copy_backward(position, finish_n, old_finish);
                    _NEFORCE copy(first, last, position);
                } else {
                    auto mid = _NEFORCE next(first, dist_after);
                    _NEFORCE uninitialized_copy(position, finish_, _NEFORCE uninitialized_copy(mid, last, finish_));
                    finish_ = new_finish;
                    _NEFORCE copy(first, mid, position);
                }
            } catch (...) {
                if (new_finish.node_ != finish_.node_) {
                    deque::destroy_nodes(finish_.node_ + 1, new_finish.node_);
                }
                throw;
            }
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
    enable_if_t<!is_ranges_fwd_iter_v<Iterator>> insert_ranges(iterator position, Iterator first, Iterator last) {
        if (last <= first) {
            return;
        }

        const size_type n = _NEFORCE distance(first, last);
        const size_type dist_before = position - start_;

        if (dist_before < size() / 2) {
            deque::reallocate_map(n, true);
        } else {
            deque::reallocate_map(n, false);
        }

        position = start_ + dist_before;
        auto cur = --last;

        for (size_type i = 0; i < n; ++i, --cur) {
            deque::insert(position, *cur);
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
    enable_if_t<is_ranges_fwd_iter_v<Iterator>> insert_ranges(iterator position, Iterator first, Iterator last) {
        if (last <= first) {
            return;
        }
        const size_type n = _NEFORCE distance(first, last);

        if (position.current_ == start_.current_) {
            deque::reallocate_map(n, true);
            auto new_start = start_ - n;
            try {
                _NEFORCE uninitialized_copy(first, last, new_start);
                start_ = new_start;
            } catch (...) {
                if (new_start.node_ != start_.node_) {
                    deque::destroy_nodes(new_start.node_, start_.node_ - 1);
                }
                throw;
            }
        } else if (position.current_ == finish_.current_) {
            deque::reallocate_map(n, false);
            auto new_finish = finish_ + n;
            try {
                _NEFORCE uninitialized_copy(first, last, finish_);
                finish_ = new_finish;
            } catch (...) {
                if (new_finish.node_ != finish_.node_) {
                    deque::destroy_nodes(finish_.node_ + 1, new_finish.node_);
                }
                throw;
            }
        } else {
            deque::insert_ranges_n(position, first, last, n);
        }
        return;
    }

    /**
     * @brief 插入n个指定值的元素辅助函数
     * @param position 插入位置
     * @param n 元素数量
     * @param value 要插入的值
     */
    void insert_n_aux(iterator position, size_type n, const T& value) {
        difference_type dist_before = position - start_;
        if (dist_before < static_cast<difference_type>(size() / 2)) {
            deque::reallocate_map(n, true);
            auto old_start = start_;
            auto new_start = start_ - n;
            position = start_ + dist_before;

            try {
                if (dist_before >= n) {
                    iterator start_n = start_ + n;
                    _NEFORCE uninitialized_copy(start_, start_n, new_start);
                    start_ = new_start;
                    _NEFORCE copy(start_n, position, old_start);
                    _NEFORCE fill(position - n, position, value);
                } else {
                    _NEFORCE uninitialized_fill(_NEFORCE uninitialized_copy(start_, position, new_start), start_,
                                                value);
                    start_ = new_start;
                    _NEFORCE fill(old_start, position, value);
                }
            } catch (...) {
                if (new_start.node_ != start_.node_) {
                    deque::destroy_nodes(new_start.node_, start_.node_ - 1);
                }
                throw;
            }
        } else {
            deque::reallocate_map(n, false);
            auto old_finish = finish_;
            auto new_finish = finish_ + n;
            const size_type dist_after = size() - dist_before;
            position = finish_ - dist_after;

            try {
                if (dist_after > n) {
                    auto finish_n = finish_ - n;
                    _NEFORCE uninitialized_copy(finish_n, finish_, finish_);
                    finish_ = new_finish;
                    _NEFORCE copy_backward(position, finish_n, old_finish);
                    _NEFORCE fill(position, position + n, value);
                } else {
                    _NEFORCE uninitialized_fill(finish_, position + n, value);
                    _NEFORCE uninitialized_copy(position, finish_, position + n);
                    finish_ = new_finish;
                    _NEFORCE fill(position, old_finish, value);
                }
            } catch (...) {
                if (new_finish.node_ != finish_.node_) {
                    deque::destroy_nodes(finish_.node_ + 1, new_finish.node_);
                }
                throw;
            }
        }
    }

    /**
     * @brief 插入单个元素辅助函数
     * @tparam Args 构造参数类型
     * @param position 插入位置
     * @param args 构造参数
     * @return 指向插入元素的迭代器
     */
    template <typename... Args>
    iterator insert_aux(iterator position, Args&&... args) noexcept(is_nothrow_move_assignable_v<value_type>) {
        size_type index = position - start_;

        if (index < size() / 2) {
            deque::emplace_front(front());

            iterator new_pos = start_ + index;
            if (new_pos != finish_ - 1) {
                _NEFORCE copy_backward(new_pos, finish_ - 1, finish_);
            }

            _NEFORCE destroy(new_pos.current_);
            _NEFORCE construct(new_pos.current_, _NEFORCE forward<Args>(args)...);
            return new_pos;
        } else {
            deque::emplace_back(back());

            iterator new_pos = start_ + index;
            if (new_pos != start_) {
                _NEFORCE copy_backward(start_, new_pos, new_pos + 1);
            }

            _NEFORCE destroy(new_pos.current_);
            _NEFORCE construct(new_pos.current_, _NEFORCE forward<Args>(args)...);
            return new_pos;
        }
    }

public:
    /**
     * @brief 默认构造函数
     *
     * 构造一个空双端队列。
     */
    deque() { deque::fill_initialize(0, _NEFORCE initialize<T>()); }

    /**
     * @brief 构造包含n个默认构造元素的双端队列
     * @param n 元素数量
     */
    explicit deque(const size_type n) { deque::fill_initialize(n, _NEFORCE initialize<T>()); }

    /**
     * @brief 构造包含n个指定值元素的双端队列
     * @param n 元素数量
     * @param value 初始值
     */
    deque(const size_type n, const T& value) { deque::fill_initialize(n, value); }

    /**
     * @brief 范围构造函数
     * @tparam Iterator 迭代器类型
     * @param first 起始迭代器
     * @param last 结束迭代器
     */
    template <typename Iterator, enable_if_t<is_iter_v<Iterator>, int> = 0>
    deque(Iterator first, Iterator last) {
        deque::copy_initialize(first, last);
    }

    /**
     * @brief 初始化列表构造函数
     * @param ilist 初始化列表
     */
    deque(std::initializer_list<T> ilist) { deque::copy_initialize(ilist.begin(), ilist.end()); }

    /**
     * @brief 初始化列表赋值运算符
     * @param ilist 初始化列表
     * @return 自身引用
     */
    deque& operator=(std::initializer_list<T> ilist) {
        deque tmp(ilist);
        deque::swap(tmp);
        return *this;
    }

    /**
     * @brief 拷贝构造函数
     * @param other 源双端队列
     */
    deque(const deque& other) { deque::copy_initialize(other.cbegin(), other.cend()); }

    /**
     * @brief 拷贝赋值运算符
     * @param other 源双端队列
     * @return 自身引用
     */
    deque& operator=(const deque& other) {
        if (_NEFORCE addressof(other) == this) {
            return *this;
        }

        const size_t len = size();
        if (len >= other.size()) {
            deque::erase(_NEFORCE copy(other.start_, other.finish_, start_), finish_);
        } else {
            auto mid = other.begin() + len;
            _NEFORCE copy(other.begin(), mid, start_);
            deque::insert(end(), mid, other.end());
        }
        return *this;
    }

    /**
     * @brief 移动构造函数
     * @param other 源双端队列
     */
    deque(deque&& other) noexcept { deque::swap(other); }

    /**
     * @brief 移动赋值运算符
     * @param other 源双端队列
     * @return 自身引用
     */
    deque& operator=(deque&& other) noexcept {
        if (_NEFORCE addressof(other) == this) {
            return *this;
        }
        clear();
        deque::swap(other);
        return *this;
    }

    /**
     * @brief 析构函数
     *
     * 销毁所有元素并释放所有内存。
     */
    ~deque() {
        if (map_pair_.value == nullptr) {
            return;
        }
        clear();

        if (map_pair_.value != nullptr) {
            for (size_type i = 0; i < map_size_pair_.value; ++i) {
                if (map_pair_.value[i] != nullptr) {
                    map_size_pair_.get_base().deallocate(map_pair_.value[i], buffer_size);
                    map_pair_.value[i] = nullptr;
                }
            }

            map_pair_.get_base().deallocate(map_pair_.value, map_size_pair_.value);
            map_pair_.value = nullptr;
        }
    }

    /**
     * @brief 获取起始迭代器
     * @return 指向第一个元素的迭代器
     */
    NEFORCE_NODISCARD iterator begin() noexcept { return iterator(start_); }

    /**
     * @brief 获取结束迭代器
     * @return 指向最后一个元素之后位置的迭代器
     */
    NEFORCE_NODISCARD iterator end() noexcept { return iterator(finish_); }

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
    NEFORCE_NODISCARD const_iterator cbegin() const noexcept { return const_iterator(start_); }

    /**
     * @brief 获取常量结束迭代器
     * @return 指向最后一个元素之后位置的常量迭代器
     */
    NEFORCE_NODISCARD const_iterator cend() const noexcept { return const_iterator(finish_); }

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
     * @brief 获取当前元素数量
     * @return 元素数量
     */
    NEFORCE_NODISCARD size_type size() const noexcept { return finish_ - start_; }

    /**
     * @brief 获取最大可能大小
     * @return 最大元素数量
     */
    NEFORCE_NODISCARD size_type max_size() const noexcept { return static_cast<size_type>(-1); }

    /**
     * @brief 检查是否为空
     * @return 是否为空
     */
    NEFORCE_NODISCARD bool empty() const noexcept { return finish_ == start_; }

    /**
     * @brief 访问第一个元素
     * @return 第一个元素的引用
     */
    NEFORCE_NODISCARD reference front() noexcept {
        NEFORCE_DEBUG_VERIFY(!empty(), "front called on empty deque");
        return *start_;
    }

    /**
     * @brief 访问第一个常量元素
     * @return 第一个元素的常量引用
     */
    NEFORCE_NODISCARD const_reference front() const noexcept {
        NEFORCE_DEBUG_VERIFY(!empty(), "front called on empty deque");
        return *start_;
    }

    /**
     * @brief 访问最后一个元素
     * @return 最后一个元素的引用
     */
    NEFORCE_NODISCARD reference back() noexcept {
        NEFORCE_DEBUG_VERIFY(!empty(), "back called on empty deque");
        return *(finish_ - 1);
    }

    /**
     * @brief 访问最后一个常量元素
     * @return 最后一个元素的常量引用
     */
    NEFORCE_NODISCARD const_reference back() const noexcept {
        NEFORCE_DEBUG_VERIFY(!empty(), "back called on empty deque");
        return *(finish_ - 1);
    }

    /**
     * @brief 调整大小
     * @param n 新的大小
     * @param value 用于填充新元素的默认值
     *
     * 如果新大小小于当前大小，删除多余元素；
     * 如果大于当前大小，在末尾插入指定值的副本。
     */
    void resize(size_type n, const T& value) {
        const auto old_size = size();
        if (n < old_size) {
            deque::erase(start_ + n, finish_);
        } else {
            deque::insert(finish_, n - old_size, value);
        }
    }

    /**
     * @brief 使用默认构造的元素调整大小
     * @param n 新的大小
     */
    void resize(const size_type n) { deque::resize(n, _NEFORCE initialize<T>()); }

    /**
     * @brief 在指定位置构造元素
     * @tparam Args 构造参数类型
     * @param position 插入位置
     * @param args 构造参数
     * @return 指向插入元素的迭代器
     */
    template <typename... Args>
    iterator emplace(iterator position, Args&&... args) {
        if (position.current_ == start_.current_) {
            deque::emplace_front(_NEFORCE forward<Args>(args)...);
            return start_;
        }
        if (position.current_ == finish_.current_) {
            deque::emplace_back(_NEFORCE forward<Args>(args)...);
            return finish_ - 1;
        }
        return deque::insert_aux(position, _NEFORCE forward<Args>(args)...);
    }

    /**
     * @brief 在末尾构造元素
     * @tparam Args 构造参数类型
     * @param args 构造参数
     */
    template <typename... Args>
    void emplace_back(Args&&... args) {
        if (finish_.current_ != finish_.last_ - 1) {
            _NEFORCE construct(finish_.current_, _NEFORCE forward<Args>(args)...);
            ++finish_.current_;
        } else {
            deque::reallocate_map(1, false);
            _NEFORCE construct(finish_.current_, _NEFORCE forward<Args>(args)...);
            ++finish_;
        }
    }

    /**
     * @brief 在开头构造元素
     * @tparam Args 构造参数类型
     * @param args 构造参数
     */
    template <typename... Args>
    void emplace_front(Args&&... args) {
        if (start_.current_ != start_.first_) {
            _NEFORCE construct(start_.current_ - 1, _NEFORCE forward<Args>(args)...);
            --start_.current_;
        } else {
            deque::reallocate_map(1, true);
            --start_;
            _NEFORCE construct(start_.current_, _NEFORCE forward<Args>(args)...);
        }
    }

    /**
     * @brief 在末尾拷贝插入元素
     * @param value 要插入的值
     */
    void push_back(const T& value) { deque::emplace_back(value); }

    /**
     * @brief 在开头拷贝插入元素
     * @param value 要插入的值
     */
    void push_front(const T& value) { deque::emplace_front(value); }

    /**
     * @brief 在末尾移动插入元素
     * @param value 要插入的值
     */
    void push_back(T&& value) { deque::emplace_back(_NEFORCE move(value)); }

    /**
     * @brief 在开头移动插入元素
     * @param value 要插入的值
     */
    void push_front(T&& value) { deque::emplace_front(_NEFORCE move(value)); }

    /**
     * @brief 移除末尾元素
     */
    void pop_back() noexcept(is_nothrow_destructible_v<value_type>) {
        NEFORCE_DEBUG_VERIFY(!empty(), "pop_back called on empty deque");

        if (finish_.current_ != finish_.first_) {
            --finish_.current_;
            _NEFORCE destroy(finish_.current_);
        } else {
            _NEFORCE destroy(finish_.current_);
            auto cur = finish_.node_;
            map_size_pair_.get_base().deallocate(*cur, buffer_size);
            *cur = nullptr;
            --finish_;
        }
    }

    /**
     * @brief 移除开头元素
     */
    void pop_front() noexcept(is_nothrow_destructible_v<value_type>) {
        NEFORCE_DEBUG_VERIFY(!empty(), "pop_front called on empty deque");

        if (start_.current_ != start_.last_ - 1) {
            _NEFORCE destroy(start_.current_);
            ++start_.current_;
        } else {
            _NEFORCE destroy(start_.current_);
            auto cur = start_.node_;
            map_size_pair_.get_base().deallocate(start_.first_, buffer_size);
            *cur = nullptr;
            ++start_;
        }
    }

    /**
     * @brief 赋值n个指定值的元素
     * @param count 元素数量
     * @param value 要赋的值
     */
    void assign(const size_type count, const T& value) { deque::assign_aux_n(count, value); }

    /**
     * @brief 范围赋值
     * @tparam Iterator 迭代器类型
     * @param first 起始迭代器
     * @param last 结束迭代器
     */
    template <typename Iterator, enable_if_t<is_iter_v<Iterator>, int> = 0>
    void assign(Iterator first, Iterator last) {
        deque::assign_ranges(first, last);
    }

    /**
     * @brief 初始化列表赋值
     * @param ilist 初始化列表
     */
    void assign(std::initializer_list<T> ilist) { deque::assign_ranges(ilist.begin(), ilist.end()); }

    /**
     * @brief 在指定位置拷贝插入元素
     * @param position 插入位置
     * @param value 要插入的值
     * @return 指向插入元素的迭代器
     */
    iterator insert(iterator position, const T& value) {
        if (position.current_ == start_.current_) {
            deque::push_front(value);
            return start_;
        }
        if (position.current_ == finish_.current_) {
            deque::push_back(value);
            return _NEFORCE prev(finish_);
        }
        return deque::insert_aux(position, value);
    }

    /**
     * @brief 在指定位置移动插入元素
     * @param position 插入位置
     * @param value 要插入的值
     * @return 指向插入元素的迭代器
     */
    iterator insert(iterator position, T&& value) {
        if (position.current_ == start_.current_) {
            deque::emplace_front(_NEFORCE move(value));
            return start_;
        }
        if (position.current_ == finish_.current_) {
            deque::emplace_back(_NEFORCE move(value));
            return _NEFORCE prev(finish_);
        }
        return deque::insert_aux(position, _NEFORCE move(value));
    }

    /**
     * @brief 范围插入
     * @tparam Iterator 迭代器类型
     * @param position 插入位置
     * @param first 起始迭代器
     * @param last 结束迭代器
     */
    template <typename Iterator, enable_if_t<is_iter_v<Iterator>, int> = 0>
    void insert(iterator position, Iterator first, Iterator last) {
        deque::insert_ranges(position, first, last);
    }

    /**
     * @brief 初始化列表插入
     * @param position 插入位置
     * @param list 初始化列表
     */
    iterator insert(iterator position, std::initializer_list<T> list) {
        return deque::insert(position, list.begin(), list.end());
    }

    /**
     * @brief 插入n个指定值的元素
     * @param position 插入位置
     * @param n 元素数量
     * @param value 要插入的值
     */
    void insert(iterator position, const size_t n, const T& value) {
        if (position.current_ == start_.current_) {
            deque::reallocate_map(n, true);
            auto new_start = start_ - n;
            _NEFORCE uninitialized_fill_n(new_start, n, value);
            start_ = new_start;
        } else if (position.current_ == finish_.current_) {
            deque::reallocate_map(n, false);
            auto new_finish = finish_ + n;
            _NEFORCE uninitialized_fill_n(finish_, n, value);
            finish_ = new_finish;
        } else {
            return deque::insert_n_aux(position, n, value);
        }
    }

    /**
     * @brief 删除指定位置的元素
     * @param position 要删除的位置
     * @return 指向被删除元素之后位置的迭代器
     */
    iterator erase(iterator position) {
        iterator next = _NEFORCE next(position);
        const size_type dest_before = position - start_;
        if (dest_before < size() / 2) {
            _NEFORCE copy_backward(start_, position, next);
            deque::pop_front();
        } else {
            _NEFORCE copy(next, finish_, position);
            deque::pop_back();
        }
        return start_ + dest_before;
    }

    /**
     * @brief 删除指定范围内的元素
     * @param first 起始迭代器
     * @param last 结束迭代器
     * @return 指向最后一个被删除元素之后位置的迭代器
     */
    iterator erase(iterator first, iterator last) {
        if (first == start_ && last == finish_) {
            clear();
            return finish_;
        }

        const size_type len = last - first;
        const size_type dist_before = first - start_;
        if (dist_before < (size() - len) / 2) {
            _NEFORCE copy_backward(start_, first, last);
            iterator new_start = start_ + len;
            _NEFORCE destroy(start_.current_, new_start.current_);
            start_ = new_start;
        } else {
            _NEFORCE copy(last, finish_, first);
            iterator new_finish = finish_ - len;
            _NEFORCE destroy(new_finish.current_, finish_.current_);
            finish_ = new_finish;
        }
        return start_ + dist_before;
    }

    /**
     * @brief 收缩容量以适应当前大小
     *
     * 释放两端未使用的缓冲区，但保留中控器。
     */
    void shrink_to_fit() noexcept(is_nothrow_destructible_v<value_type>) {
        for (map_pointer cur = map_pair_.value; cur < start_.node_; ++cur) {
            if (*cur == nullptr) {
                continue;
            }
            map_size_pair_.get_base().deallocate(*cur, buffer_size);
            *cur = nullptr;
        }
        for (map_pointer cur = finish_.node_ + 1; cur < map_pair_.value + map_size_pair_.value; ++cur) {
            if (*cur == nullptr) {
                continue;
            }
            map_size_pair_.get_base().deallocate(*cur, buffer_size);
            *cur = nullptr;
        }
    }

    /**
     * @brief 清空双端队列
     *
     * 销毁所有元素，释放所有缓冲区，重置迭代器。
     */
    void clear() noexcept(is_nothrow_destructible_v<value_type>) {
        for (map_pointer cur = start_.node_ + 1; cur < finish_.node_; ++cur) {
            _NEFORCE destroy(*cur, *cur + buffer_size);
        }

        if (start_.node_ == finish_.node_) {
            _NEFORCE destroy(start_.current_, finish_.current_);
        } else {
            _NEFORCE destroy(start_.current_, start_.last_);
            _NEFORCE destroy(finish_.first_, finish_.current_);
        }

        shrink_to_fit();
        finish_ = start_;
    }

    /**
     * @brief 带边界检查的常量索引访问
     * @param position 索引位置
     * @return 指定位置元素的常量引用
     */
    NEFORCE_NODISCARD const_reference at(size_type position) const noexcept { return start_[position]; }

    /**
     * @brief 带边界检查的索引访问
     * @param position 索引位置
     * @return 指定位置元素的引用
     */
    NEFORCE_NODISCARD reference at(const size_type position) noexcept { return start_[position]; }

    /**
     * @brief 常量下标访问操作符
     * @param position 索引位置
     * @return 指定位置元素的常量引用
     */
    NEFORCE_NODISCARD const_reference operator[](const size_type position) const noexcept { return at(position); }

    /**
     * @brief 下标访问操作符
     * @param position 索引位置
     * @return 指定位置元素的引用
     */
    NEFORCE_NODISCARD reference operator[](const size_type position) noexcept { return at(position); }

    /**
     * @brief 交换两个双端队列的内容
     * @param other 要交换的另一个双端队列
     */
    void swap(deque& other) noexcept(is_nothrow_swappable_v<map_allocator> && is_nothrow_swappable_v<allocator_type>) {
        if (_NEFORCE addressof(other) == this) {
            return;
        }

        _NEFORCE swap(start_, other.start_);
        _NEFORCE swap(finish_, other.finish_);
        _NEFORCE swap(map_pair_, other.map_pair_);
        _NEFORCE swap(map_size_pair_, other.map_size_pair_);
    }

    /**
     * @brief 相等比较操作符
     * @param rhs 右侧双端队列
     * @return 如果两个双端队列大小相等且对应元素相等返回true
     */
    NEFORCE_NODISCARD bool equal_to(const deque& rhs) const
            noexcept(noexcept(_NEFORCE equal(cbegin(), cend(), rhs.cbegin()))) {
        return size() == rhs.size() && _NEFORCE equal(cbegin(), cend(), rhs.cbegin());
    }

    /**
     * @brief 小于比较操作符
     * @param rhs 右侧双端队列
     * @return 按字典序比较结果
     */
    NEFORCE_NODISCARD bool less_than(const deque& rhs) const
            noexcept(noexcept(_NEFORCE lexicographical_compare(cbegin(), cend(), rhs.cbegin(), rhs.cend()))) {
        return _NEFORCE lexicographical_compare(cbegin(), cend(), rhs.cbegin(), rhs.cend());
    }
};

#ifdef NEFORCE_STANDARD_17
template <typename T, typename Alloc>
deque(T, Alloc = Alloc()) -> deque<T, Alloc>;

template <typename Iterator, typename Alloc>
deque(Iterator, Iterator, Alloc = Alloc()) -> deque<iter_value_t<Iterator>, Alloc>;
#endif

/** @} */ // Container

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_CORE_CONTAINER_DEQUE_HPP__
