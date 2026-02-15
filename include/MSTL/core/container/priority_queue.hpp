#ifndef MSTL_CORE_CONTAINER_PRIORITY_QUEUE_HPP__
#define MSTL_CORE_CONTAINER_PRIORITY_QUEUE_HPP__

/**
 * @file priority_queue.hpp
 * @brief MSTL优先队列容器适配器
 *
 * 此文件提供了优先队列容器适配器的实现。
 * 优先队列是一种允许快速访问最大/最小元素的数据结构，
 * 元素按照优先级顺序出队。通过堆算法维护元素优先级。
 */

#include "MSTL/core/algorithm/heap.hpp"
#include "MSTL/core/container/vector.hpp"
MSTL_BEGIN_NAMESPACE__

/**
 * @defgroup PriorityQueue 优先队列
 * @brief 基于堆的优先级队列容器适配器
 * @{
 */

/**
 * @class priority_queue
 * @brief 优先队列容器适配器
 * @tparam T 元素类型
 * @tparam Sequence 底层容器类型，默认为vector<T>
 * @tparam Compare 比较函数对象类型，默认为less<T>
 *
 * 优先队列是一种容器适配器，提供常数时间访问最大/最小元素，
 * 对数时间插入和删除。元素按照严格弱序（strict weak ordering）
 * 确定的优先级顺序出队。默认使用最大堆（less）实现，即top()返回最大元素。
 * 通过指定不同的Compare可改变优先级顺序。
 */
template <typename T, typename Sequence = vector<T>,
    typename Compare = less<typename Sequence::value_type>>
class priority_queue : public icollector<priority_queue<T, Sequence, Compare>> {
    static_assert(is_object_v<T>, "priority queue only contains object types.");
    static_assert(is_same_v<T, typename Sequence::value_type>, "priority queue require consistent types.");

public:
    using value_type        = typename Sequence::value_type;  ///< 值类型
    using difference_type   = typename Sequence::difference_type;  ///< 差值类型
    using size_type         = typename Sequence::size_type;  ///< 大小类型
    using reference         = typename Sequence::reference;  ///< 引用类型
    using const_reference   = typename Sequence::const_reference;  ///< 常量引用类型

private:
    compressed_pair<Compare, Sequence> pair_{ default_construct_tag{} };  ///< 压缩存储比较函数和底层容器

    /**
     * @brief 对底层容器建堆
     *
     * 将底层容器中的元素构建成堆结构。
     */
    MSTL_ALWAYS_INLINE void make_heap_inside() {
        _MSTL make_heap(pair_.value.begin(), pair_.value.end(), pair_.get_base());
    }

public:
    /**
     * @brief 默认构造函数
     *
     * 构造一个空的优先队列，使用默认比较函数。
     */
    priority_queue() = default;

    /**
     * @brief 构造函数，指定比较函数
     * @param comp 比较函数对象
     */
    explicit priority_queue(const Compare& comp)
    noexcept(is_nothrow_default_constructible_v<Sequence> &&
             is_nothrow_copy_constructible_v<Compare>)
    : pair_(exact_arg_construct_tag{}, comp) {}

    /**
     * @brief 构造函数，指定比较函数和底层容器副本
     * @param comp 比较函数对象
     * @param seq 底层容器副本
     */
    priority_queue(const Compare& comp, const Sequence& seq)
    : pair_(exact_arg_construct_tag{}, comp, seq) {
        make_heap_inside();
    }

    /**
     * @brief 构造函数，指定比较函数和移动的底层容器
     * @param comp 比较函数对象
     * @param seq 要移动的底层容器
     */
    priority_queue(const Compare& comp, Sequence&& seq)
    noexcept(is_nothrow_move_constructible_v<Sequence> &&
             is_nothrow_copy_constructible_v<Compare>)
    : pair_(exact_arg_construct_tag{}, comp, _MSTL move(seq)) {
        make_heap_inside();
    }

    /**
     * @brief 范围构造函数，指定底层容器副本
     * @tparam Iterator 迭代器类型
     * @param first 起始迭代器
     * @param last 结束迭代器
     * @param seq 底层容器副本
     */
    template <typename Iterator, enable_if_t<is_iter_v<Iterator>, int> = 0>
    priority_queue(Iterator first, Iterator last, const Sequence& seq)
    : pair_(default_construct_tag{}, seq) {
        pair_.value.insert(pair_.value.end(), first, last);
        make_heap_inside();
    }

    /**
     * @brief 范围构造函数
     * @tparam Iterator 迭代器类型
     * @param first 起始迭代器
     * @param last 结束迭代器
     */
    template <typename Iterator, enable_if_t<is_iter_v<Iterator>, int> = 0>
    priority_queue(Iterator first, Iterator last)
    : pair_(default_construct_tag{}, first, last) {
        make_heap_inside();
    }

    /**
     * @brief 范围构造函数，指定比较函数
     * @tparam Iterator 迭代器类型
     * @param first 起始迭代器
     * @param last 结束迭代器
     * @param comp 比较函数对象
     */
    template <typename Iterator, enable_if_t<is_iter_v<Iterator>, int> = 0>
    priority_queue(Iterator first, Iterator last, const Compare& comp)
    : pair_(exact_arg_construct_tag{}, comp, first, last) {
        make_heap_inside();
    }

    /**
     * @brief 范围构造函数，指定比较函数和底层容器副本
     * @tparam Iterator 迭代器类型
     * @param first 起始迭代器
     * @param last 结束迭代器
     * @param comp 比较函数对象
     * @param seq 底层容器副本
     */
    template <typename Iterator, enable_if_t<is_iter_v<Iterator>, int> = 0>
    priority_queue(Iterator first, Iterator last, const Compare& comp, const Sequence& seq)
    : pair_(exact_arg_construct_tag{}, comp, seq) {
        pair_.value.insert(pair_.value.end(), first, last);
        make_heap_inside();
    }

    /**
     * @brief 范围构造函数，指定比较函数和移动的底层容器
     * @tparam Iterator 迭代器类型
     * @param first 起始迭代器
     * @param last 结束迭代器
     * @param comp 比较函数对象
     * @param seq 要移动的底层容器
     */
    template <typename Iterator, enable_if_t<is_iter_v<Iterator>, int> = 0>
    priority_queue(Iterator first, Iterator last, const Compare& comp, Sequence&& seq)
    : pair_(exact_arg_construct_tag{}, comp, _MSTL move(seq)) {
        pair_.value.insert(pair_.value.end(), first, last);
        make_heap_inside();
    }

    /**
     * @brief 检查优先队列是否为空
     * @return 队列为空返回true，否则返回false
     */
    MSTL_NODISCARD bool empty() const
    noexcept(noexcept(_MSTL declval<Sequence>().empty())) {
        return pair_.value.empty();
    }

    /**
     * @brief 获取优先队列大小
     * @return 队列中的元素数量
     */
    MSTL_NODISCARD size_type size() const
    noexcept(noexcept(_MSTL declval<Sequence>().size())) {
        return pair_.value.size();
    }

    /**
     * @brief 访问优先级最高的元素
     * @return 优先级最高元素的常量引用
     *
     * 返回堆顶元素，即根据Compare确定的优先级最高的元素。
     */
    MSTL_NODISCARD const_reference top() const
    noexcept(noexcept(_MSTL declval<Sequence>().front())) {
        return pair_.value.front();
    }

    /**
     * @brief 插入元素（拷贝版本）
     * @param value 要插入的值
     *
     * 将元素添加到优先队列中，并维护堆性质。
     */
    void push(const value_type& value) {
        pair_.value.push_back(value);
        _MSTL push_heap(pair_.value.begin(), pair_.value.end(), pair_.get_base());
    }

    /**
     * @brief 插入元素（移动版本）
     * @param value 要插入的值
     *
     * 将元素移动到优先队列中，并维护堆性质。
     */
    void push(value_type&& value) {
        pair_.value.push_back(_MSTL move(value));
        _MSTL push_heap(pair_.value.begin(), pair_.value.end(), pair_.get_base());
    }

    /**
     * @brief 移除优先级最高的元素
     *
     * 删除堆顶元素，并维护堆性质。
     */
    void pop() {
        _MSTL pop_heap(pair_.value.begin(), pair_.value.end(), pair_.get_base());
        pair_.value.pop_back();
    }

    /**
     * @brief 在优先队列中就地构造元素
     * @tparam Args 构造参数类型
     * @param args 构造参数
     *
     * 在底层容器末尾就地构造元素，并维护堆性质。
     */
    template <typename... Args>
    void emplace(Args&&... args) {
        pair_.value.emplace_back(_MSTL forward<Args>(args)...);
        _MSTL push_heap(pair_.value.begin(), pair_.value.end(), pair_.get_base());
    }

    /**
     * @brief 交换两个优先队列的内容
     * @param other 要交换的另一个优先队列
     */
    void swap(priority_queue& other)
    noexcept(is_nothrow_swappable_v<Sequence> && is_nothrow_swappable_v<Compare>) {
        pair_.swap(other.pair_);
    }

    /**
     * @brief 相等比较操作符
     * @param rhs 右侧优先队列
     * @return 如果两个优先队列的底层容器相等返回true
     * @note 此比较基于底层容器的元素顺序，而非优先级顺序。
     */
    MSTL_NODISCARD bool operator ==(const priority_queue& rhs) const
    noexcept(noexcept(pair_.value == rhs.pair_.value)) {
        return pair_.value == rhs.pair_.value;
    }

    /**
     * @brief 小于比较操作符
     * @param rhs 右侧优先队列
     * @return 按字典序比较底层容器的结果
     * @note 此比较基于底层容器的元素顺序，而非优先级顺序。
     */
    MSTL_NODISCARD bool operator <(const priority_queue& rhs) const
    noexcept(noexcept(pair_.value < rhs.pair_.value)) {
        return pair_.value < rhs.pair_.value;
    }
};

#ifdef MSTL_SUPPORT_DEDUCTION_GUIDES__
template <typename Compare, typename Sequence>
priority_queue(Compare, Sequence) -> priority_queue<typename Sequence::value_type, Sequence, Compare>;

template <typename Iterator, typename Compare = less<iter_value_t<Iterator>>,
    typename Sequence = vector<iter_value_t<Iterator>>>
priority_queue(Iterator, Iterator, Compare = Compare(), Sequence = Sequence())
-> priority_queue<iter_value_t<Iterator>, Sequence, Compare>;
#endif

/** @} */ // PriorityQueue

MSTL_END_NAMESPACE__
#endif // MSTL_CORE_CONTAINER_PRIORITY_QUEUE_HPP__
