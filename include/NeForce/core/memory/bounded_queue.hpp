#ifndef NEFORCE_CORE_MEMORY_BOUNDED_QUEUE_HPP__
#define NEFORCE_CORE_MEMORY_BOUNDED_QUEUE_HPP__
#include "NeForce/core/container/vector.hpp"
NEFORCE_BEGIN_NAMESPACE__

/**
 * @class bounded_queue
 * @brief 线程安全的有界环形队列（非阻塞）
 * @tparam T 元素类型
 *
 * 基于环形缓冲区实现，push/pop 为 O(1) 操作。
 * 调用者需自行处理满队列时的溢出策略。
 */
template <typename T>
class bounded_queue {
private:
    vector<T> buffer_; ///< 环形缓冲区
    size_t head_{0};   ///< 队首索引
    size_t tail_{0};   ///< 队尾索引
    size_t count_{0};  ///< 当前元素数量
    size_t capacity_;  ///< 最大容量

public:
    /**
     * @brief 构造指定容量的队列
     * @param cap 最大容量
     */
    explicit bounded_queue(const size_t cap) :
    buffer_(cap),
    capacity_(cap) {}

    bounded_queue(const bounded_queue&) = default;
    bounded_queue& operator=(const bounded_queue&) = default;
    bounded_queue(bounded_queue&&) noexcept = default;
    bounded_queue& operator=(bounded_queue&&) noexcept = default;

    /** @return 队列是否已满 */
    NEFORCE_NODISCARD bool full() const noexcept { return count_ == capacity_; }

    /** @return 队列是否为空 */
    NEFORCE_NODISCARD bool empty() const noexcept { return count_ == 0; }

    /** @return 当前元素数量 */
    NEFORCE_NODISCARD size_t size() const noexcept { return count_; }

    /** @return 最大容量 */
    NEFORCE_NODISCARD size_t capacity() const noexcept { return capacity_; }

    /**
     * @brief 向队尾压入元素（调用者需确保队列未满）
     * @param item 要压入的元素
     */
    void push(T&& item) noexcept {
        buffer_[tail_] = _NEFORCE move(item);
        tail_ = (tail_ + 1) % capacity_;
        ++count_;
    }

    /**
     * @brief 从队首弹出元素（调用者需确保队列非空）
     * @return 弹出的元素
     */
    NEFORCE_NODISCARD T pop() noexcept {
        T item = _NEFORCE move(buffer_[head_]);
        head_ = (head_ + 1) % capacity_;
        --count_;
        return item;
    }

    /**
     * @brief 访问队首元素
     * @return 队首元素的引用
     */
    NEFORCE_NODISCARD T& front() noexcept { return buffer_[head_]; }
};

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_CORE_MEMORY_BOUNDED_QUEUE_HPP__
