#ifndef NEFORCE_CORE_ASYNC_CHANNEL_HPP__
#define NEFORCE_CORE_ASYNC_CHANNEL_HPP__

/**
 * @file channel.hpp
 * @brief CSP 有缓冲消息通道
 *
 * 提供线程安全的有限容量消息通道，支持阻塞和非阻塞发送/接收。
 * 适用于协程间和线程间消息传递。
 */

#include "NeForce/core/async/condition_variable.hpp"
#include "NeForce/core/async/mutex.hpp"
#include "NeForce/core/container/deque.hpp"
NEFORCE_BEGIN_NAMESPACE__

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
 *
 * @note 容量耗尽时写入阻塞，无数据时读取阻塞。
 */
template <typename T>
class channel {
private:
    size_t capacity_;
    bool closed_{false};
    mutable mutex mutex_;
    condition_variable cv_;
    deque<T> buffer_;

public:
    /**
     * @brief 构造函数
     * @param capacity 通道容量，0 表示无缓冲，capacity_max 表示无上限
     * @throws value_exception capacity_max 容量不合法时抛出
     */
    explicit channel(size_t capacity = capacity_max) :
    capacity_(capacity) {}

    channel(const channel&) = delete;
    channel& operator=(const channel&) = delete;

    /**
     * @brief 最大容量常量
     */
    static constexpr size_t capacity_max = numeric_traits<size_t>::max();

    /**
     * @brief 尝试发送消息（非阻塞）
     * @param value 要发送的消息
     * @return 发送成功返回 true，通道已满或已关闭返回 false
     */
    bool try_write(T value) {
        unique_lock<mutex> lock{mutex_};
        if (closed_) {
            return false;
        }
        if (capacity_ > 0 && buffer_.size() >= capacity_) {
            return false;
        }
        buffer_.push_back(move(value));
        cv_.notify_one();
        return true;
    }

    /**
     * @brief 发送消息（阻塞直到空间可用或通道关闭）
     * @param value 要发送的消息
     * @return 发送成功返回 true，通道关闭返回 false
     */
    bool write(T value) {
        unique_lock<mutex> lock{mutex_};
        cv_.wait(lock, [this] {
            return closed_ || (capacity_ == 0 && buffer_.empty()) || (capacity_ > 0 && buffer_.size() < capacity_);
        });
        if (closed_) {
            return false;
        }
        buffer_.push_back(move(value));
        cv_.notify_one();
        return true;
    }

    /**
     * @brief 尝试读取消息（非阻塞）
     * @param out 输出消息
     * @return 读取成功返回 true，通道空或关闭且无数据返回 false
     */
    bool try_read(T& out) {
        unique_lock<mutex> lock{mutex_};
        if (buffer_.empty()) {
            return false;
        }
        out = move(buffer_.front());
        buffer_.pop_front();
        cv_.notify_one();
        return true;
    }

    /**
     * @brief 读取消息（阻塞直到有数据或通道关闭）
     * @param out 输出消息
     * @return 读取成功返回 true，通道关闭且无数据返回 false
     */
    bool read(T& out) {
        unique_lock<mutex> lock{mutex_};
        cv_.wait(lock, [this] { return closed_ || !buffer_.empty(); });
        if (buffer_.empty()) {
            return false;
        }
        out = move(buffer_.front());
        buffer_.pop_front();
        cv_.notify_one();
        return true;
    }

    /**
     * @brief 关闭通道
     *
     * 关闭后不再接受新消息，唤醒所有等待的读写线程。
     * 已缓冲的数据仍可读取。
     */
    void close() {
        lock<mutex> lock{mutex_};
        closed_ = true;
        cv_.notify_all();
    }

    /**
     * @brief 检查通道是否已关闭
     * @return 已关闭返回 true
     */
    NEFORCE_NODISCARD bool is_closed() const noexcept {
        lock<mutex> lock{mutex_};
        return closed_;
    }

    /**
     * @brief 当前缓冲消息数量
     * @return 消息数量
     */
    NEFORCE_NODISCARD size_t size() const noexcept {
        lock<mutex> lock{mutex_};
        return buffer_.size();
    }

    /**
     * @brief 检查缓冲区是否为空
     * @return 为空返回 true
     */
    NEFORCE_NODISCARD bool empty() const noexcept {
        lock<mutex> lock{mutex_};
        return buffer_.empty();
    }
};

/** @} */ // Channel

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_CORE_ASYNC_CHANNEL_HPP__
