#ifndef NEFORCE_CORE_ASYNC_CHANNEL_HPP__
#define NEFORCE_CORE_ASYNC_CHANNEL_HPP__

/**
 * @file channel.hpp
 * @brief CSP 有缓冲消息通道
 *
 * 提供线程安全的有限容量消息通道，支持阻塞和非阻塞发送/接收。
 * 适用于协程间和线程间消息传递。
 */

#include "NeForce/core/async/atomic.hpp"
#include "NeForce/core/async/condition_variable.hpp"
#include "NeForce/core/async/lock_free_queue.hpp"
#include "NeForce/core/async/mutex.hpp"
#include "NeForce/core/utility/optional.hpp"
NEFORCE_BEGIN_NAMESPACE__

/**
 * @defgroup AsyncComponents 异步组件
 * @brief 异步编程相关组件
 * @{
 */

/**
 * @defgroup Channel 消息通道
 * @brief CSP 有缓冲消息通道
 * @{
 */

/**
 * @class channel
 * @brief 有限容量的 CSP 消息通道
 * @tparam T 消息类型
 *
 * 支持多生产者/多消费者的线程安全消息通道。
 * 容量为 0 表示无缓冲（同步握手），capacity_max 表示无上限。
 */
template <typename T>
class channel {
private:
    size_t capacity_;
    lock_free_queue<T> queue_;
    atomic<size_t> size_{0};
    atomic<bool> closed_{false};
    optional<T> handoff_;
    mutable mutex control_mutex_;
    condition_variable not_empty_;
    condition_variable not_full_;

    NEFORCE_NODISCARD bool push_locked(T value) noexcept(is_nothrow_move_constructible_v<T>) {
        if (!queue_.enqueue(_NEFORCE move(value))) {
            return false;
        }
        size_.fetch_add(1, memory_order_relaxed);
        not_empty_.notify_one();
        return true;
    }

    NEFORCE_NODISCARD bool pop_locked(T& out) {
        if (size_.load(memory_order_relaxed) == 0) {
            return false;
        }
        if (!queue_.try_dequeue(out)) {
            return false;
        }
        size_.fetch_sub(1, memory_order_relaxed);
        not_full_.notify_one();
        return true;
    }

public:
    /**
     * @brief 构造函数
     * @param capacity 通道容量，0 表示无缓冲，capacity_max 表示无上限
     */
    explicit channel(size_t capacity = capacity_max) :
    capacity_(capacity),
    queue_(capacity == capacity_max ? 1024 : (capacity == 0 ? 32 : capacity)) {}

    channel(const channel&) = delete;
    channel& operator=(const channel&) = delete;

    /**
     * @brief 最大容量常量
     */
    static constexpr size_t capacity_max = numeric_traits<size_t>::max();

    /**
     * @brief 尝试发送消息（非阻塞）
     * @param value 要发送的消息
     * @return 成功返回 true；通道已满、已关闭或无缓冲时返回 false
     * @note 不会等待缓冲区空间，但会与并发的写/读操作互斥
     */
    bool try_write(T value) noexcept(is_nothrow_move_constructible_v<T>) {
        if (capacity_ == 0) {
            return false;
        }
        unique_lock<mutex> lock{control_mutex_};
        if (closed_.load(memory_order_acquire) || size_.load(memory_order_acquire) >= capacity_) {
            return false;
        }
        return push_locked(_NEFORCE move(value));
    }

    /**
     * @brief 发送消息（阻塞直到空间可用或通道关闭）
     * @param value 要发送的消息
     * @return 成功返回 true；通道关闭或消息入队失败返回 false
     * @note 无缓冲通道会等待读者取走消息后才返回
     * @note 等待期间不持有内部锁
     */
    bool write(T value) {
        unique_lock<mutex> lock{control_mutex_};
        if (capacity_ == 0) {
            not_full_.wait(lock, [this] { return closed_.load(memory_order_acquire) || !handoff_.has_value(); });
            if (closed_.load(memory_order_acquire)) {
                return false;
            }
            handoff_.emplace(_NEFORCE move(value));
            size_.store(1, memory_order_release);
            not_empty_.notify_all();
            not_full_.wait(lock, [this] { return closed_.load(memory_order_acquire) || !handoff_.has_value(); });
            return true;
        }
        not_full_.wait(lock, [this] {
            return closed_.load(memory_order_acquire) || size_.load(memory_order_acquire) < capacity_;
        });
        if (closed_.load(memory_order_acquire)) {
            return false;
        }
        return push_locked(_NEFORCE move(value));
    }

    /**
     * @brief 尝试读取消息（非阻塞）
     * @param out 输出消息
     * @return 成功返回 true
     * @note 不会等待新消息，但会与并发的写/读操作互斥
     */
    bool try_read(T& out) {
        unique_lock<mutex> lock{control_mutex_};
        if (capacity_ == 0) {
            if (!handoff_.has_value()) {
                return false;
            }
            out = _NEFORCE move(*handoff_);
            handoff_.reset();
            size_.store(0, memory_order_release);
            not_full_.notify_all();
            return true;
        }
        return pop_locked(out);
    }

    /**
     * @brief 读取消息（阻塞直到有数据或通道关闭）
     * @param out 输出消息
     * @return 成功返回 true；关闭且无数据返回 false
     * @note 已关闭但仍有缓冲消息时会先取完缓冲消息
     * @note 等待期间不持有内部锁
     */
    bool read(T& out) {
        unique_lock<mutex> lock{control_mutex_};
        if (capacity_ == 0) {
            not_empty_.wait(lock, [this] { return closed_.load(memory_order_acquire) || handoff_.has_value(); });
            if (!handoff_.has_value()) {
                return false;
            }
            out = _NEFORCE move(*handoff_);
            handoff_.reset();
            size_.store(0, memory_order_release);
            not_full_.notify_all();
            return true;
        }
        not_empty_.wait(lock,
                        [this] { return closed_.load(memory_order_acquire) || size_.load(memory_order_acquire) > 0; });
        return pop_locked(out);
    }

    /**
     * @brief 关闭通道
     */
    void close() noexcept {
        unique_lock<mutex> lock{control_mutex_};
        closed_.store(true, memory_order_release);
        not_empty_.notify_all();
        not_full_.notify_all();
    }

    /**
     * @brief 检查通道是否已关闭
     */
    NEFORCE_NODISCARD bool is_closed() const noexcept { return closed_.load(memory_order_acquire); }

    /**
     * @brief 当前缓冲消息数量
     */
    NEFORCE_NODISCARD size_t size() const noexcept { return size_.load(memory_order_acquire); }

    /**
     * @brief 检查缓冲区是否为空
     */
    NEFORCE_NODISCARD bool empty() const noexcept { return size() == 0; }
};

/** @} */ // Channel

/** @} */ // AsyncComponents

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_CORE_ASYNC_CHANNEL_HPP__
