#ifndef MSTL_CORE_CONTAINER_QUEUE_HPP__
#define MSTL_CORE_CONTAINER_QUEUE_HPP__

/**
 * @file queue.hpp
 * @brief MSTL队列容器适配器
 *
 * 此文件提供了队列容器适配器的实现。
 * 队列是一种先进先出（FIFO）的数据结构，
 * 元素从一端（队尾）插入，从另一端（队首）删除。
 */

#include "MSTL/core/container/deque.hpp"
MSTL_BEGIN_NAMESPACE__

/**
 * @defgroup Queue 队列
 * @brief 先进先出（FIFO）容器适配器
 * @{
 */

/**
 * @class queue
 * @brief 队列容器适配器
 * @tparam T 元素类型
 * @tparam Sequence 底层容器类型，默认为deque<T>
 *
 * 队列是一种容器适配器，提供先进先出的数据结构特性。
 * 元素只能在队尾插入，在队首删除。支持基本的队列操作：
 * 入队（push）、出队（pop）、访问队首（front）和队尾（back）元素。
 *
 * 默认使用deque作为底层容器，也可指定其他支持front、back、
 * push_back和pop_front操作的容器（如list）。
 */
template <typename T, typename Sequence = deque<T>>
class queue : public icollector<queue<T, Sequence>> {
    static_assert(is_object_v<T>, "queue only contains object types.");
    static_assert(is_same_v<T, typename Sequence::value_type>, "queue require consistent types.");

public:
    using value_type        = typename Sequence::value_type;  ///< 值类型
    using difference_type   = typename Sequence::difference_type;  ///< 差值类型
    using size_type         = typename Sequence::size_type;  ///< 大小类型
    using reference         = typename Sequence::reference;  ///< 引用类型
    using const_reference   = typename Sequence::const_reference;  ///< 常量引用类型
    using iterator          = typename Sequence::iterator;  ///< 迭代器类型
    using const_iterator    = typename Sequence::const_iterator;  ///< 常量迭代器类型

private:
    Sequence seq_{};  ///< 底层容器实例

public:
    /**
     * @brief 默认构造函数
     *
     * 构造一个空队列。
     */
    queue() = default;

    /**
     * @brief 构造函数，使用指定的底层容器副本
     * @param other 底层容器副本
     */
    explicit queue(const Sequence& other)
    : seq_(other) {}

    /**
     * @brief 移动构造函数，使用指定的底层容器
     * @param other 要移动的底层容器
     */
    explicit queue(Sequence&& other)
    noexcept(is_nothrow_move_constructible_v<Sequence>)
    : seq_(_MSTL move(other)) {}

    /**
     * @brief 析构函数
     */
    ~queue() = default;

    /**
     * @brief 获取队列大小
     * @return 队列中的元素数量
     */
    MSTL_NODISCARD size_type size() const
    noexcept(noexcept(seq_.size())) {
        return seq_.size();
    }

    /**
     * @brief 检查队列是否为空
     * @return 队列为空返回true，否则返回false
     */
    MSTL_NODISCARD bool empty() const
    noexcept(noexcept(seq_.empty())) {
        return seq_.empty();
    }

    /**
     * @brief 访问队首元素
     * @return 队首元素的引用
     */
    MSTL_NODISCARD reference front()
    noexcept(noexcept(seq_.front())) {
        return seq_.front();
    }

    /**
     * @brief 常量版本，访问队首元素
     * @return 队首元素的常量引用
     */
    MSTL_NODISCARD const_reference front() const
    noexcept(noexcept(seq_.front())) {
        return seq_.front();
    }

    /**
     * @brief 访问队尾元素
     * @return 队尾元素的引用
     */
    MSTL_NODISCARD reference back()
    noexcept(noexcept(seq_.back())) {
        return seq_.back();
    }

    /**
     * @brief 常量版本，访问队尾元素
     * @return 队尾元素的常量引用
     */
    MSTL_NODISCARD const_reference back() const
    noexcept(noexcept(seq_.back())) {
        return seq_.back();
    }

    /**
     * @brief 获取起始迭代器
     * @return 指向队首的迭代器
     */
    MSTL_NODISCARD iterator begin()
    noexcept(noexcept(seq_.begin())) {
        return seq_.begin();
    }

    /**
     * @brief 获取结束迭代器
     * @return 指向队尾之后位置的迭代器
     */
    MSTL_NODISCARD iterator end()
    noexcept(noexcept(seq_.end())) {
        return seq_.end();
    }

    /**
     * @brief 获取常量起始迭代器
     * @return 指向队首的常量迭代器
     */
    MSTL_NODISCARD const_iterator begin() const
    noexcept(noexcept(seq_.begin())) {
        return seq_.begin();
    }

    /**
     * @brief 获取常量结束迭代器
     * @return 指向队尾之后位置的常量迭代器
     */
    MSTL_NODISCARD const_iterator end() const
    noexcept(noexcept(seq_.end())) {
        return seq_.end();
    }

    /**
     * @brief 获取常量起始迭代器
     * @return 指向队首的常量迭代器
     */
    MSTL_NODISCARD const_iterator cbegin() const
    noexcept(noexcept(seq_.cbegin())) {
        return seq_.cbegin();
    }

    /**
     * @brief 获取常量结束迭代器
     * @return 指向队尾之后位置的常量迭代器
     */
    MSTL_NODISCARD const_iterator cend() const
    noexcept(noexcept(seq_.cend())) {
        return seq_.cend();
    }

    /**
     * @brief 入队操作（拷贝版本）
     * @param value 要插入的值
     *
     * 将元素添加到队尾。
     */
    void push(const T& value) {
        seq_.push_back(value);
    }

    /**
     * @brief 入队操作（移动版本）
     * @param value 要插入的值
     *
     * 将元素移动到队尾。
     */
    void push(T&& value) {
        seq_.push_back(_MSTL move(value));
    }

    /**
     * @brief 出队操作
     *
     * 移除队首元素。
     */
    void pop()
    noexcept(noexcept(seq_.pop_front())) {
        seq_.pop_front();
    }

    /**
     * @brief 在队尾就地构造元素
     * @tparam Args 构造参数类型
     * @param args 构造参数
     * @return 新构造元素的引用
     */
    template <typename... Args>
    decltype(auto) emplace(Args&&... args) {
        return seq_.emplace_back(_MSTL forward<Args>(args)...);
    }

    /**
     * @brief 交换两个队列的内容
     * @param other 要交换的另一个队列
     */
    void swap(queue& other)
    noexcept(is_nothrow_swappable_v<Sequence>) {
        _MSTL swap(seq_, other.seq_);
    }

    /**
     * @brief 相等比较操作符
     * @param rhs 右侧队列
     * @return 如果两个队列大小相等且对应元素相等返回true
     */
    MSTL_NODISCARD bool operator ==(const queue& rhs) const
    noexcept(noexcept(seq_ == rhs.seq_)) {
        return seq_ == rhs.seq_;
    }

    /**
     * @brief 小于比较操作符
     * @param rhs 右侧队列
     * @return 按字典序比较结果
     */
    MSTL_NODISCARD bool operator <(const queue& rhs) const
    noexcept(noexcept(seq_ < rhs.seq_)) {
        return seq_ < rhs.seq_;
    }
};

#if MSTL_SUPPORT_DEDUCTION_GUIDES__
template <typename Sequence>
queue(Sequence) -> queue<typename Sequence::value_type, Sequence>;
#endif

/** @} */ // Queue

MSTL_END_NAMESPACE__
#endif // MSTL_CORE_CONTAINER_QUEUE_HPP__
