#ifndef NEFORCE_CORE_ASYNC_LOCK_FREE_QUEUE_HPP__
#define NEFORCE_CORE_ASYNC_LOCK_FREE_QUEUE_HPP__

// ---------------------------------------------------------------------------
// This file is a port of moodycamel::ConcurrentQueue (https://github.com/cameron314/concurrentqueue, 2020 version),
// adapted to the NexusForce code style and components.
//
// Original work:
//   Copyright (c) 2013-2020, Cameron Desrochers.
//   All rights reserved.
//
// Simplified BSD license (license of the ported code):
//   Redistribution and use in source and binary forms, with or without
//   modification, are permitted provided that the following conditions are
//   met:
//   - Redistributions of source code must retain the above copyright notice,
//     this list of conditions and the following disclaimer.
//   - Redistributions in binary form must reproduce the above copyright
//     notice, this list of conditions and the following disclaimer in the
//     documentation and/or other materials provided with the distribution.
//
//   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
//   IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
//   THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
//   PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
//   CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
//   EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
//   PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
//   PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
//   LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
//   NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
//   SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
// The original work is also dual-licensed under the Boost Software License
// 1.0 (https://www.boost.org/LICENSE_1_0.txt).
// ---------------------------------------------------------------------------

/**
 * @file lock_free_queue.hpp
 * @brief 无锁队列
 *
 * 此文件提供了多生产者多消费者无锁队列的实现。
 *
 * 主要功能：
 * - 基于 per-producer SPSC block 子队列架构，block 通过无锁 free_list 回收
 * - 隐式生产者（按线程 ID 哈希自动注册）与显式生产者（令牌）两种模式
 * - 单元素与批量入队/出队（enqueue_bulk / dequeue_bulk）
 * - 无分配 try_enqueue / try_dequeue 与可分配 enqueue / dequeue
 * - 异常安全的批量操作回滚与元素清理
 * - 队列移动语义与 token 生命周期管理
 */

#include "NeForce/core/async/atomic.hpp"
#include "NeForce/core/async/thread.hpp"
#include "NeForce/core/async/thread_exit_notifier.hpp"
#include "NeForce/core/container/array.hpp"
#include "NeForce/core/exception/debug.hpp"
#include "NeForce/core/memory/construct.hpp"
#include "NeForce/core/memory/unique_ptr.hpp"
#include "NeForce/core/numeric/numeric_traits.hpp"
#include <malloc.h>
NEFORCE_BEGIN_NAMESPACE__

/**
 * @defgroup AsyncComponents 异步组件
 * @brief 异步编程相关组件
 * @{
 */

/**
 * @defgroup LockFreeQueue 无锁队列
 * @brief 线程安全的无锁队列
 * @{
 */

class producer_token;
class consumer_token;

/// @cond
NEFORCE_BEGIN_INNER__

NEFORCE_INLINE17 constexpr thread::id invalid_thread_id_zero{0};
#ifdef NEFORCE_PLATFORM_WINDOWS
NEFORCE_INLINE17 constexpr thread::id invalid_thread_id_max{static_cast<thread::id::native_id_type>(-1)};
#endif

template <typename T>
struct const_numeric_max {
    static_assert(is_integral_v<T>, "const_numeric_max can only be used with integers");
    static const T value =
            numeric_traits<T>::is_signed
                    ? (static_cast<T>(1) << (sizeof(T) * numeric_traits<char>::digits)) - static_cast<T>(1)
                    : static_cast<T>(-1);
};

template <typename T>
constexpr char* align_for(char* ptr) noexcept {
    constexpr size_t alignment = alignment_of_v<T>;
    return ptr + (alignment - (reinterpret_cast<uintptr_t>(ptr) % alignment)) % alignment;
}

template <typename T>
constexpr T ceil_to_pow_2(T x) noexcept {
    static_assert(is_integral_v<T> && !numeric_traits<T>::is_signed, "ceil_to_pow_2 requires an unsigned integer type");

    // Adapted from http://graphics.stanford.edu/~seander/bithacks.html#RoundUpPowerOf2
    --x;
    x |= x >> 1;
    x |= x >> 2;
    x |= x >> 4;
    for (size_t i = 1; i < sizeof(T); i <<= 1) {
        x |= x >> (i << 3);
    }
    ++x;
    return x;
}

template <typename T>
constexpr bool circular_less_than(T a, T b) noexcept {
    static_assert(is_integral_v<T> && !numeric_traits<T>::is_signed,
                  "circular_less_than requires an unsigned integer type");
    return (a - b) > (static_cast<T>(1) << (numeric_traits<T>::digits - 1));
}

template <typename T>
const T& nomove(const T& x) noexcept {
    return x;
}

template <bool Enable>
struct nomove_if {
    template <typename T>
    static const T& eval(const T& x) noexcept {
        return x;
    }
};

template <>
struct nomove_if<false> {
    template <typename U>
    static auto eval(U&& x) noexcept -> decltype(_NEFORCE forward<U>(x)) {
        return _NEFORCE forward<U>(x);
    }
};


/**
 * @struct producer_typeless_base
 * @brief 生产者链表无类型基类
 *
 * 供队列与 producer_token / consumer_token 共享的生产者节点基类，
 * 避免令牌类依赖队列的具体模板参数。
 */
struct producer_typeless_base {
    producer_typeless_base* next_{nullptr};
    atomic<bool> inactive_{false};
    producer_token* token_{nullptr};
};

NEFORCE_END_INNER__
/// @endcond


/**
 * @struct lock_free_queue_traits
 * @brief 无锁队列特征配置
 *
 * 通过继承本结构体并覆写所需字段，可定制队列的块大小、
 * 索引容量、子队列大小上限与内存分配函数等。
 */
struct lock_free_queue_traits {
    /// 通用尺寸类型
    using size_t = _NEFORCE size_t;

    /// 入队/出队索引类型
    using index_t = _NEFORCE size_t;

    /// 每个 block 存储的元素数量，必须为 2 的幂
    static constexpr _NEFORCE size_t BLOCK_SIZE = 32;

    /// 显式生产者 block 空状态由逐元素标志切换为计数器的阈值
    static constexpr _NEFORCE size_t EXPLICIT_BLOCK_EMPTY_COUNTER_THRESHOLD = 32;

    /// 单个显式生产者预期持有的满 block 数量，必须为 2 的幂
    static constexpr _NEFORCE size_t EXPLICIT_INITIAL_INDEX_SIZE = 32;

    /// 单个隐式生产者预期持有的满 block 数量，必须为 2 的幂
    static constexpr _NEFORCE size_t IMPLICIT_INITIAL_INDEX_SIZE = 32;

    /// 线程 ID 到隐式生产者的哈希表初始大小，必须为 2 的幂
    static constexpr _NEFORCE size_t INITIAL_IMPLICIT_PRODUCER_HASH_SIZE = 32;

    /// 显式消费者在触发全体消费者轮转前最多消费的元素数量
    static constexpr _NEFORCE uint32_t EXPLICIT_CONSUMER_CONSUMPTION_QUOTA_BEFORE_ROTATE = 256;

    /// 单个子队列的最大元素数（含），超出则入队失败，按块大小向上取整
    static constexpr _NEFORCE size_t MAX_SUBQUEUE_SIZE = inner::const_numeric_max<_NEFORCE size_t>::value;

    /// 等待信号量时自旋的次数（保留字段，供阻塞变体使用）
    static constexpr int MAX_SEMA_SPINS = 10000;

    /// 是否将动态分配的 block 回收进内部 free list
    static constexpr bool RECYCLE_ALLOCATED_BLOCKS = false;

    /**
     * @brief 自定义内存分配函数
     * @param size 请求的字节数
     * @return 分配的内存指针，失败返回 nullptr
     */
    static void* malloc(_NEFORCE size_t size) { return ::malloc(size); }

    /**
     * @brief 自定义内存释放函数
     * @param ptr 要释放的内存指针
     */
    static void free(void* ptr) { ::free(ptr); }
};


template <typename T, typename Traits>
class lock_free_queue;

/**
 * @class producer_token
 * @brief 显式生产者令牌
 *
 * 通过令牌入队可避免每次入队时按线程 ID 查找隐式生产者，
 * 适用于高频生产场景。一个线程通常最多持有一个生产者令牌。
 */
class producer_token {
public:
    /**
     * @brief 构造函数
     * @tparam T 元素类型
     * @tparam Traits 队列特征
     * @param queue 关联的无锁队列
     */
    template <typename T, typename Traits>
    explicit producer_token(lock_free_queue<T, Traits>& queue);

    /**
     * @brief 移动构造函数
     * @param other 被移动的令牌
     */
    producer_token(producer_token&& other) noexcept :
    producer_(other.producer_) {
        other.producer_ = nullptr;
        if (producer_ != nullptr) {
            producer_->token_ = this;
        }
    }

    /**
     * @brief 移动赋值运算符
     * @param other 被移动的令牌
     * @return 自身引用
     */
    producer_token& operator=(producer_token&& other) noexcept {
        swap(other);
        return *this;
    }

    /**
     * @brief 交换两个令牌
     * @param other 另一个令牌
     */
    void swap(producer_token& other) noexcept {
        _NEFORCE swap(producer_, other.producer_);
        if (producer_ != nullptr) {
            producer_->token_ = this;
        }
        if (other.producer_ != nullptr) {
            other.producer_->token_ = &other;
        }
    }

    /**
     * @brief 检查令牌是否有效
     * @return 有效返回 true
     */
    NEFORCE_NODISCARD bool valid() const noexcept { return producer_ != nullptr; }

    /**
     * @brief 析构函数
     *
     * 将关联的生产者标记为可回收，供其他线程复用。
     */
    ~producer_token() {
        if (producer_ != nullptr) {
            producer_->token_ = nullptr;
            producer_->inactive_.store(true, memory_order_release);
        }
    }

    producer_token(const producer_token&) = delete;
    producer_token& operator=(const producer_token&) = delete;

private:
    template <typename T, typename Traits>
    friend class lock_free_queue;

    inner::producer_typeless_base* producer_{nullptr};
};

/**
 * @class consumer_token
 * @brief 显式消费者令牌
 *
 * 通过令牌出队可让消费者固定在自己的子队列位置
 * 并在全局轮转时同步移动，减少多消费者之间的竞争。
 */
class consumer_token {
public:
    /**
     * @brief 构造函数
     * @tparam T 元素类型
     * @tparam Traits 队列特征
     * @param queue 关联的无锁队列
     */
    template <typename T, typename Traits>
    explicit consumer_token(lock_free_queue<T, Traits>& queue);

    /**
     * @brief 移动构造函数
     * @param other 被移动的令牌
     */
    consumer_token(consumer_token&& other) noexcept :
    initial_offset_(other.initial_offset_),
    last_known_global_offset_(other.last_known_global_offset_),
    items_consumed_from_current_(other.items_consumed_from_current_),
    current_producer_(other.current_producer_),
    desired_producer_(other.desired_producer_) {}

    /**
     * @brief 移动赋值运算符
     * @param other 被移动的令牌
     * @return 自身引用
     */
    consumer_token& operator=(consumer_token&& other) noexcept {
        swap(other);
        return *this;
    }

    /**
     * @brief 交换两个令牌
     * @param other 另一个令牌
     */
    void swap(consumer_token& other) noexcept {
        _NEFORCE swap(initial_offset_, other.initial_offset_);
        _NEFORCE swap(last_known_global_offset_, other.last_known_global_offset_);
        _NEFORCE swap(items_consumed_from_current_, other.items_consumed_from_current_);
        _NEFORCE swap(current_producer_, other.current_producer_);
        _NEFORCE swap(desired_producer_, other.desired_producer_);
    }

    consumer_token(const consumer_token&) = delete;
    consumer_token& operator=(const consumer_token&) = delete;

private:
    template <typename T, typename Traits>
    friend class lock_free_queue;

    uint32_t initial_offset_{0};
    uint32_t last_known_global_offset_{0};
    uint32_t items_consumed_from_current_{0};
    inner::producer_typeless_base* current_producer_{nullptr};
    inner::producer_typeless_base* desired_producer_{nullptr};
};

/**
 * @class lock_free_queue
 * @brief 无锁队列类模板
 * @tparam T 元素类型
 * @tparam Traits 队列特征配置
 *
 * 基于 per-producer SPSC 子队列架构实现的多生产者多消费者无锁队列，
 * 支持隐式生产者与显式生产者两种模式。
 */
template <typename T, typename Traits = lock_free_queue_traits>
class lock_free_queue {
public:
    using producer_token_t = ::neforce::producer_token;
    using consumer_token_t = ::neforce::consumer_token;

    using index_t = typename Traits::index_t;
    using size_t = typename Traits::size_t;

    /// 每个 block 存储的元素数量
    static const size_t BLOCK_SIZE = static_cast<size_t>(Traits::BLOCK_SIZE);
    /// 显式生产者 block 空状态标志/计数器切换阈值
    static const size_t EXPLICIT_BLOCK_EMPTY_COUNTER_THRESHOLD =
            static_cast<size_t>(Traits::EXPLICIT_BLOCK_EMPTY_COUNTER_THRESHOLD);
    /// 显式生产者 block 索引初始容量
    static const size_t EXPLICIT_INITIAL_INDEX_SIZE = static_cast<size_t>(Traits::EXPLICIT_INITIAL_INDEX_SIZE);
    /// 隐式生产者 block 索引初始容量
    static const size_t IMPLICIT_INITIAL_INDEX_SIZE = static_cast<size_t>(Traits::IMPLICIT_INITIAL_INDEX_SIZE);
    /// 隐式生产者哈希表初始大小
    static const size_t INITIAL_IMPLICIT_PRODUCER_HASH_SIZE =
            static_cast<size_t>(Traits::INITIAL_IMPLICIT_PRODUCER_HASH_SIZE);
    /// 显式消费者轮转配额
    static const uint32_t EXPLICIT_CONSUMER_CONSUMPTION_QUOTA_BEFORE_ROTATE =
            static_cast<uint32_t>(Traits::EXPLICIT_CONSUMER_CONSUMPTION_QUOTA_BEFORE_ROTATE);

    /// 单个子队列的最大元素数，按块大小向上取整
    static const size_t MAX_SUBQUEUE_SIZE =
            (inner::const_numeric_max<size_t>::value - static_cast<size_t>(Traits::MAX_SUBQUEUE_SIZE) < BLOCK_SIZE)
                    ? inner::const_numeric_max<size_t>::value
                    : ((static_cast<size_t>(Traits::MAX_SUBQUEUE_SIZE) + (BLOCK_SIZE - 1)) / BLOCK_SIZE * BLOCK_SIZE);

    static_assert(!numeric_traits<size_t>::is_signed && is_integral_v<size_t>,
                  "Traits::size_t must be an unsigned integral type");
    static_assert(!numeric_traits<index_t>::is_signed && is_integral_v<index_t>,
                  "Traits::index_t must be an unsigned integral type");
    static_assert(sizeof(index_t) >= sizeof(size_t), "Traits::index_t must be at least as wide as Traits::size_t");
    static_assert((BLOCK_SIZE > 1) && !(BLOCK_SIZE & (BLOCK_SIZE - 1)),
                  "Traits::BLOCK_SIZE must be a power of 2 (and at least 2)");
    static_assert((EXPLICIT_BLOCK_EMPTY_COUNTER_THRESHOLD > 1) &&
                          !(EXPLICIT_BLOCK_EMPTY_COUNTER_THRESHOLD & (EXPLICIT_BLOCK_EMPTY_COUNTER_THRESHOLD - 1)),
                  "Traits::EXPLICIT_BLOCK_EMPTY_COUNTER_THRESHOLD must be a power of 2 (and greater than 1)");
    static_assert((EXPLICIT_INITIAL_INDEX_SIZE > 1) &&
                          !(EXPLICIT_INITIAL_INDEX_SIZE & (EXPLICIT_INITIAL_INDEX_SIZE - 1)),
                  "Traits::EXPLICIT_INITIAL_INDEX_SIZE must be a power of 2 (and greater than 1)");
    static_assert((IMPLICIT_INITIAL_INDEX_SIZE > 1) &&
                          !(IMPLICIT_INITIAL_INDEX_SIZE & (IMPLICIT_INITIAL_INDEX_SIZE - 1)),
                  "Traits::IMPLICIT_INITIAL_INDEX_SIZE must be a power of 2 (and greater than 1)");
    static_assert((INITIAL_IMPLICIT_PRODUCER_HASH_SIZE == 0) ||
                          !(INITIAL_IMPLICIT_PRODUCER_HASH_SIZE & (INITIAL_IMPLICIT_PRODUCER_HASH_SIZE - 1)),
                  "Traits::INITIAL_IMPLICIT_PRODUCER_HASH_SIZE must be a power of 2");
    static_assert(
            INITIAL_IMPLICIT_PRODUCER_HASH_SIZE == 0 || INITIAL_IMPLICIT_PRODUCER_HASH_SIZE >= 1,
            "Traits::INITIAL_IMPLICIT_PRODUCER_HASH_SIZE must be at least 1 (or 0 to disable implicit enqueueing)");

public:
    /**
     * @brief 构造函数
     * @param capacity 预分配的元素容量
     *
     * 预分配指定容量的 block 池，减少运行时堆分配。实际可无分配
     * 插入的元素数取决于生产者数量与块大小。
     *
     * @note 非线程安全,必须在队列被其他线程使用前完成构造
     */
    explicit lock_free_queue(const size_t capacity = 32 * BLOCK_SIZE) {
        implicit_producer_hash_resize_in_progress_.clear(memory_order_relaxed);
        populate_initial_implicit_producer_hash();
        populate_initial_block_list(capacity / BLOCK_SIZE + ((capacity & (BLOCK_SIZE - 1)) == 0 ? 0 : 1));
    }

    /**
     * @brief 构造函数
     * @param min_capacity 任意时刻期望可用的最小元素数
     * @param max_explicit_producers 最大并发显式生产者数
     * @param max_implicit_producers 最大并发隐式生产者数
     *
     * 根据生产者数量自动计算预分配的 block 数量。
     *
     * @note 非线程安全，必须在队列被其他线程使用前完成构造
     */
    lock_free_queue(const size_t min_capacity, const size_t max_explicit_producers,
                    const size_t max_implicit_producers) {
        implicit_producer_hash_resize_in_progress_.clear(memory_order_relaxed);
        populate_initial_implicit_producer_hash();
        size_t blocks = (((min_capacity + BLOCK_SIZE - 1) / BLOCK_SIZE) - 1) * (max_explicit_producers + 1) +
                        2 * (max_explicit_producers + max_implicit_producers);
        populate_initial_block_list(blocks);
    }

    /**
     * @brief 析构函数
     *
     * 销毁所有生产者、隐式生产者哈希表、free list 与初始 block 池。
     *
     * @warning 非线程安全，析构时不得有并发操作
     */
    ~lock_free_queue() {
        auto ptr = producer_list_tail_.load(memory_order_relaxed);
        while (ptr != nullptr) {
            auto next = ptr->next_producer();
            if (ptr->token_ != nullptr) {
                ptr->token_->producer_ = nullptr;
            }
            destroy(ptr);
            ptr = next;
        }

        NEFORCE_IF_CONSTEXPR(INITIAL_IMPLICIT_PRODUCER_HASH_SIZE != 0) {
            auto hash = implicit_producer_hash_.load(memory_order_relaxed);
            while (hash != nullptr) {
                auto prev = hash->prev_;
                if (prev != nullptr) {
                    for (size_t i = 0; i != hash->capacity_; ++i) {
                        hash->entries_[i].~implicit_producer_kvp();
                    }
                    hash->~implicit_producer_hash();
                    (Traits::free)(hash);
                }
                hash = prev;
            }
        }

        auto blk = free_list_.head_unsafe();
        while (blk != nullptr) {
            auto next = blk->free_list_next_.load(memory_order_relaxed);
            if (blk->dynamically_allocated_) {
                destroy(blk);
            }
            blk = next;
        }

        destroy_array(initial_block_pool_, initial_block_pool_size_);
    }

    lock_free_queue(const lock_free_queue&) = delete;
    lock_free_queue& operator=(const lock_free_queue&) = delete;

    /**
     * @brief 移动构造函数
     * @param other 被移动的队列
     * @note 非线程安全，移动后原队列变为空队列，令牌仍有效但只能用于新队列
     */
    lock_free_queue(lock_free_queue&& other) noexcept :
    producer_list_tail_(other.producer_list_tail_.load(memory_order_relaxed)),
    producer_count_(other.producer_count_.load(memory_order_relaxed)),
    initial_block_pool_index_(other.initial_block_pool_index_.load(memory_order_relaxed)),
    initial_block_pool_(other.initial_block_pool_),
    initial_block_pool_size_(other.initial_block_pool_size_),
    free_list_(_NEFORCE move(other.free_list_)),
    next_explicit_consumer_id_(other.next_explicit_consumer_id_.load(memory_order_relaxed)),
    global_explicit_consumer_offset_(other.global_explicit_consumer_offset_.load(memory_order_relaxed)) {
        implicit_producer_hash_resize_in_progress_.clear(memory_order_relaxed);
        populate_initial_implicit_producer_hash();
        swap_implicit_producer_hashes(other);

        other.producer_list_tail_.store(nullptr, memory_order_relaxed);
        other.producer_count_.store(0, memory_order_relaxed);
        other.next_explicit_consumer_id_.store(0, memory_order_relaxed);
        other.global_explicit_consumer_offset_.store(0, memory_order_relaxed);

        other.initial_block_pool_index_.store(0, memory_order_relaxed);
        other.initial_block_pool_size_ = 0;
        other.initial_block_pool_ = nullptr;

        reown_producers();
    }

    /**
     * @brief 移动赋值运算符
     * @param other 被移动的队列
     * @return 自身引用
     * @note 非线程安全；语义为交换两个队列的内部状态
     */
    lock_free_queue& operator=(lock_free_queue&& other) noexcept { return swap_internal(other); }

    /**
     * @brief 交换两个队列的状态
     * @param other 另一个队列
     * @note 非线程安全，交换不会使令牌失效，但令牌只能与交换后的队列配合使用
     */
    void swap(lock_free_queue& other) noexcept { swap_internal(other); }

private:
    lock_free_queue& swap_internal(lock_free_queue& other) noexcept {
        if (this == &other) {
            return *this;
        }

        _NEFORCE swap_relaxed(producer_list_tail_, other.producer_list_tail_);
        _NEFORCE swap_relaxed(producer_count_, other.producer_count_);
        _NEFORCE swap_relaxed(initial_block_pool_index_, other.initial_block_pool_index_);
        _NEFORCE swap(initial_block_pool_, other.initial_block_pool_);
        _NEFORCE swap(initial_block_pool_size_, other.initial_block_pool_size_);
        free_list_.swap(other.free_list_);
        _NEFORCE swap_relaxed(next_explicit_consumer_id_, other.next_explicit_consumer_id_);
        _NEFORCE swap_relaxed(global_explicit_consumer_offset_, other.global_explicit_consumer_offset_);

        swap_implicit_producer_hashes(other);

        reown_producers();
        other.reown_producers();

        return *this;
    }

public:
    /**
     * @brief 入队操作
     * @param item 要入队的元素
     * @return 成功返回 true，内存分配失败或隐式生产被禁用时返回 false
     */
    bool enqueue(const T& item) {
        NEFORCE_IF_CONSTEXPR(INITIAL_IMPLICIT_PRODUCER_HASH_SIZE == 0) { return false; }
        else {
            return inner_enqueue<can_alloc>(item);
        }
    }

    /**
     * @brief 入队操作
     * @param item 要入队的元素
     * @return 成功返回 true，内存分配失败或隐式生产被禁用时返回 false
     */
    bool enqueue(T&& item) {
        NEFORCE_IF_CONSTEXPR(INITIAL_IMPLICIT_PRODUCER_HASH_SIZE == 0) { return false; }
        else {
            return inner_enqueue<can_alloc>(_NEFORCE move(item));
        }
    }

    /**
     * @brief 入队操作
     * @param token 生产者令牌
     * @param item 要入队的元素
     * @return 成功返回 true，内存分配失败时返回 false
     */
    bool enqueue(const producer_token_t& token, const T& item) { return inner_enqueue<can_alloc>(token, item); }

    /**
     * @brief 入队操作
     * @param token 生产者令牌
     * @param item 要入队的元素
     * @return 成功返回 true，内存分配失败时返回 false
     */
    bool enqueue(const producer_token_t& token, T&& item) {
        return inner_enqueue<can_alloc>(token, _NEFORCE move(item));
    }

    /**
     * @brief 批量入队操作
     * @tparam It 输入迭代器类型
     * @param item_first 第一个元素
     * @param count 元素数量
     * @return 全部成功返回 true
     */
    template <typename It>
    bool enqueue_bulk(It item_first, const size_t count) {
        NEFORCE_IF_CONSTEXPR(INITIAL_IMPLICIT_PRODUCER_HASH_SIZE == 0) { return false; }
        else {
            return inner_enqueue_bulk<can_alloc>(item_first, count);
        }
    }

    /**
     * @brief 批量入队操作
     * @tparam It 输入迭代器类型
     * @param token 生产者令牌
     * @param item_first 第一个元素
     * @param count 元素数量
     * @return 全部成功返回 true
     */
    template <typename It>
    bool enqueue_bulk(const producer_token_t& token, It item_first, const size_t count) {
        return inner_enqueue_bulk<can_alloc>(token, item_first, count);
    }

    /**
     * @brief 无分配入队操作
     * @param item 要入队的元素
     * @return 成功返回 true，空间不足或隐式生产被禁用时返回 false
     */
    bool try_enqueue(const T& item) {
        NEFORCE_IF_CONSTEXPR(INITIAL_IMPLICIT_PRODUCER_HASH_SIZE == 0) { return false; }
        else {
            return inner_enqueue<cannot_alloc>(item);
        }
    }

    /**
     * @brief 无分配入队操作
     * @param item 要入队的元素
     * @return 成功返回 true，空间不足或隐式生产被禁用时返回 false
     */
    bool try_enqueue(T&& item) {
        NEFORCE_IF_CONSTEXPR(INITIAL_IMPLICIT_PRODUCER_HASH_SIZE == 0) { return false; }
        else {
            return inner_enqueue<cannot_alloc>(_NEFORCE move(item));
        }
    }

    /**
     * @brief 无分配入队操作
     * @param token 生产者令牌
     * @param item 要入队的元素
     * @return 成功返回 true
     */
    bool try_enqueue(const producer_token_t& token, const T& item) { return inner_enqueue<cannot_alloc>(token, item); }

    /**
     * @brief 无分配入队操作
     * @param token 生产者令牌
     * @param item 要入队的元素
     * @return 成功返回 true
     */
    bool try_enqueue(const producer_token_t& token, T&& item) {
        return inner_enqueue<cannot_alloc>(token, _NEFORCE move(item));
    }

    /**
     * @brief 无分配批量入队操作
     * @tparam It 输入迭代器类型
     * @param item_first 第一个元素
     * @param count 元素数量
     * @return 全部成功返回 true
     */
    template <typename It>
    bool try_enqueue_bulk(It item_first, const size_t count) {
        NEFORCE_IF_CONSTEXPR(INITIAL_IMPLICIT_PRODUCER_HASH_SIZE == 0) { return false; }
        else {
            return inner_enqueue_bulk<cannot_alloc>(item_first, count);
        }
    }

    /**
     * @brief 无分配批量入队操作
     * @tparam It 输入迭代器类型
     * @param token 生产者令牌
     * @param item_first 第一个元素
     * @param count 元素数量
     * @return 全部成功返回 true
     */
    template <typename It>
    bool try_enqueue_bulk(const producer_token_t& token, It item_first, const size_t count) {
        return inner_enqueue_bulk<cannot_alloc>(token, item_first, count);
    }

    /**
     * @brief 尝试出队
     * @tparam U 输出类型
     * @param item 出队元素写入目标
     * @return 成功返回 true
     */
    template <typename U>
    bool try_dequeue(U& item) {
        size_t non_empty_count = 0;
        producer_base* best = nullptr;
        size_t best_size = 0;
        for (auto ptr = producer_list_tail_.load(memory_order_acquire); non_empty_count < 3 && ptr != nullptr;
             ptr = ptr->next_producer()) {
            auto size = ptr->size_approx();
            if (size > 0) {
                if (size > best_size) {
                    best_size = size;
                    best = ptr;
                }
                ++non_empty_count;
            }
        }

        if (non_empty_count > 0) {
            if (best->dequeue(item)) {
                return true;
            }
            for (auto ptr = producer_list_tail_.load(memory_order_acquire); ptr != nullptr;
                 ptr = ptr->next_producer()) {
                if (ptr != best && ptr->dequeue(item)) {
                    return true;
                }
            }
        }
        return false;
    }

    /**
     * @brief 尝试出队
     * @tparam U 输出类型
     * @param item 出队元素写入目标
     * @return 成功返回 true
     */
    template <typename U>
    bool try_dequeue_non_interleaved(U& item) {
        for (auto ptr = producer_list_tail_.load(memory_order_acquire); ptr != nullptr; ptr = ptr->next_producer()) {
            if (ptr->dequeue(item)) {
                return true;
            }
        }
        return false;
    }

    /**
     * @brief 尝试出队
     * @tparam U 输出类型
     * @param token 消费者令牌
     * @param item 出队元素写入目标
     * @return 成功返回 true
     */
    template <typename U>
    bool try_dequeue(consumer_token_t& token, U& item) {
        if (token.desired_producer_ == nullptr ||
            token.last_known_global_offset_ != global_explicit_consumer_offset_.load(memory_order_relaxed)) {
            if (!update_current_producer_after_rotation(token)) {
                return false;
            }
        }

        if (static_cast<producer_base*>(token.current_producer_)->dequeue(item)) {
            if (++token.items_consumed_from_current_ == EXPLICIT_CONSUMER_CONSUMPTION_QUOTA_BEFORE_ROTATE) {
                global_explicit_consumer_offset_.fetch_add(1, memory_order_relaxed);
            }
            return true;
        }

        auto tail = producer_list_tail_.load(memory_order_acquire);
        auto ptr = static_cast<producer_base*>(token.current_producer_)->next_producer();
        if (ptr == nullptr) {
            ptr = tail;
        }
        while (ptr != static_cast<producer_base*>(token.current_producer_)) {
            if (ptr->dequeue(item)) {
                token.current_producer_ = ptr;
                token.items_consumed_from_current_ = 1;
                return true;
            }
            ptr = ptr->next_producer();
            if (ptr == nullptr) {
                ptr = tail;
            }
        }
        return false;
    }

    /**
     * @brief 批量尝试出队
     * @tparam It 输出迭代器类型
     * @param item_first 输出迭代器
     * @param max 最大出队数量
     * @return 实际出队的元素数量
     */
    template <typename It>
    size_t try_dequeue_bulk(It item_first, const size_t max) {
        size_t count = 0;
        for (auto ptr = producer_list_tail_.load(memory_order_acquire); ptr != nullptr; ptr = ptr->next_producer()) {
            count += ptr->dequeue_bulk(item_first, max - count);
            if (count == max) {
                break;
            }
        }
        return count;
    }

    /**
     * @brief 批量尝试出队
     * @tparam It 输出迭代器类型
     * @param token 消费者令牌
     * @param item_first 输出迭代器
     * @param max 最大出队数量
     * @return 实际出队的元素数量
     */
    template <typename It>
    size_t try_dequeue_bulk(consumer_token_t& token, It item_first, const size_t max) {
        if (token.desired_producer_ == nullptr ||
            token.last_known_global_offset_ != global_explicit_consumer_offset_.load(memory_order_relaxed)) {
            if (!update_current_producer_after_rotation(token)) {
                return 0;
            }
        }

        size_t count = static_cast<producer_base*>(token.current_producer_)->dequeue_bulk(item_first, max);
        token.items_consumed_from_current_ += static_cast<uint32_t>(count);
        if (count == max) {
            if (token.items_consumed_from_current_ >= EXPLICIT_CONSUMER_CONSUMPTION_QUOTA_BEFORE_ROTATE) {
                global_explicit_consumer_offset_.fetch_add(1, memory_order_relaxed);
            }
            return max;
        }
        size_t remaining = max - count;

        auto tail = producer_list_tail_.load(memory_order_acquire);
        auto ptr = static_cast<producer_base*>(token.current_producer_)->next_producer();
        if (ptr == nullptr) {
            ptr = tail;
        }
        while (ptr != static_cast<producer_base*>(token.current_producer_)) {
            auto dequeued = ptr->dequeue_bulk(item_first, remaining);
            count += dequeued;
            if (dequeued != 0) {
                token.current_producer_ = ptr;
                token.items_consumed_from_current_ = static_cast<uint32_t>(dequeued);
            }
            if (dequeued == remaining) {
                break;
            }
            remaining -= dequeued;
            ptr = ptr->next_producer();
            if (ptr == nullptr) {
                ptr = tail;
            }
        }
        return count;
    }

    /**
     * @brief 从指定生产者的子队列尝试出队
     * @tparam U 输出类型
     * @param producer 生产者令牌
     * @param item 出队元素写入目标
     * @return 成功返回 true
     */
    template <typename U>
    bool try_dequeue_from_producer(const producer_token_t& producer, U& item) {
        return static_cast<explicit_producer*>(producer.producer_)->dequeue(item);
    }

    /**
     * @brief 从指定生产者的子队列批量尝试出队
     * @tparam It 输出迭代器类型
     * @param producer 生产者令牌
     * @param item_first 输出迭代器
     * @param max 最大出队数量
     * @return 实际出队的元素数量
     */
    template <typename It>
    size_t try_dequeue_bulk_from_producer(const producer_token_t& producer, It item_first, const size_t max) {
        return static_cast<explicit_producer*>(producer.producer_)->dequeue_bulk(item_first, max);
    }

    /**
     * @brief 获取队列中元素的近似数量
     * @return 元素数量的近似值
     *
     * 仅在队列完全稳定时精确。
     */
    size_t size_approx() const {
        size_t size = 0;
        for (auto ptr = producer_list_tail_.load(memory_order_acquire); ptr != nullptr; ptr = ptr->next_producer()) {
            size += ptr->size_approx();
        }
        return size;
    }

    /**
     * @brief 检查队列底层原子变量是否为无锁
     * @return 无锁返回 true
     */
    static constexpr bool is_lock_free() noexcept {
        return atomic<bool>::is_lock_free() && atomic<size_t>::is_lock_free() && atomic<uint32_t>::is_lock_free() &&
               atomic<index_t>::is_lock_free() && atomic<void*>::is_lock_free() &&
               atomic<thread::id::native_id_type>::is_lock_free();
    }

    /**
     * @brief 入队操作
     * @param new_value 要入队的元素
     *
     * @note 内部调用 enqueue；内存分配失败时静默丢弃该元素
     */
    void push(T new_value) { (void) lock_free_queue::enqueue(_NEFORCE move(new_value)); }

    /**
     * @brief 非阻塞出队操作
     * @return 出队元素的 unique_ptr，队列为空时返回空指针
     */
    unique_ptr<T> try_pop() {
        T element;
        if (try_dequeue(element)) {
            return unique_ptr<T>(new T(_NEFORCE move(element)));
        }
        return unique_ptr<T>();
    }

    /**
     * @brief 阻塞出队操作
     * @return 出队元素的 unique_ptr
     * @note 自旋等待，直至成功出队
     */
    unique_ptr<T> pop() {
        for (;;) {
            auto result = try_pop();
            if (result) {
                return result;
            }
            this_thread::relax();
        }
    }

    /**
     * @brief 检查队列是否为空
     * @return 队列为空返回 true
     * @note 由于并发特性，返回值可能瞬间失效
     */
    NEFORCE_NODISCARD bool empty() const { return size_approx() == 0; }

    /**
     * @brief 获取队列中元素的近似数量
     * @return 元素数量的近似值
     * @note 由于并发特性，返回值可能瞬间失效
     */
    NEFORCE_NODISCARD size_t size() const { return size_approx(); }

    /**
     * @brief 清空队列
     * @warning 此方法不是线程安全的，调用者需保证无并发操作
     */
    void clear() {
        while (try_pop()) {
            this_thread::relax();
        }
    }

private:
    friend class producer_token;
    friend class consumer_token;
    struct explicit_producer;
    friend struct explicit_producer;
    struct implicit_producer;
    friend struct implicit_producer;
    friend struct mem_stats;

    enum allocation_mode {
        can_alloc,
        cannot_alloc
    };

    template <allocation_mode can_alloc_mode, typename U>
    bool inner_enqueue(const producer_token_t& token, U&& element) {
        return static_cast<explicit_producer*>(token.producer_)
                ->template enqueue<can_alloc_mode>(_NEFORCE forward<U>(element));
    }

    template <allocation_mode can_alloc_mode, typename U>
    bool inner_enqueue(U&& element) {
        auto producer = get_or_add_implicit_producer();
        return producer == nullptr ? false : producer->template enqueue<can_alloc_mode>(_NEFORCE forward<U>(element));
    }

    template <allocation_mode can_alloc_mode, typename It>
    bool inner_enqueue_bulk(const producer_token_t& token, It item_first, const size_t count) {
        return static_cast<explicit_producer*>(token.producer_)
                ->template enqueue_bulk<can_alloc_mode>(item_first, count);
    }

    template <allocation_mode can_alloc_mode, typename It>
    bool inner_enqueue_bulk(It item_first, const size_t count) {
        auto producer = get_or_add_implicit_producer();
        return producer == nullptr ? false : producer->template enqueue_bulk<can_alloc_mode>(item_first, count);
    }

    bool update_current_producer_after_rotation(consumer_token_t& token) {
        auto tail = producer_list_tail_.load(memory_order_acquire);
        if (token.desired_producer_ == nullptr && tail == nullptr) {
            return false;
        }
        const auto prod_count = producer_count_.load(memory_order_relaxed);
        const auto global_offset = global_explicit_consumer_offset_.load(memory_order_relaxed);
        if (token.desired_producer_ == nullptr) {
            const uint32_t offset = prod_count - 1 - (token.initial_offset_ % prod_count);
            token.desired_producer_ = tail;
            for (uint32_t i = 0; i != offset; ++i) {
                token.desired_producer_ = static_cast<producer_base*>(token.desired_producer_)->next_producer();
                if (token.desired_producer_ == nullptr) {
                    token.desired_producer_ = tail;
                }
            }
        }

        uint32_t delta = global_offset - token.last_known_global_offset_;
        if (delta >= prod_count) {
            delta = delta % prod_count;
        }
        for (uint32_t i = 0; i != delta; ++i) {
            token.desired_producer_ = static_cast<producer_base*>(token.desired_producer_)->next_producer();
            if (token.desired_producer_ == nullptr) {
                token.desired_producer_ = tail;
            }
        }

        token.last_known_global_offset_ = global_offset;
        token.current_producer_ = token.desired_producer_;
        token.items_consumed_from_current_ = 0;
        return true;
    }

    template <typename N>
    struct free_list_node {
        free_list_node() noexcept = default;

        atomic<uint32_t> free_list_refs_{0};
        atomic<N*> free_list_next_{nullptr};
    };

    template <typename N>
    struct free_list {
        free_list() = default;

        free_list(free_list&& other) noexcept :
        free_list_head_(other.free_list_head_.load(memory_order_relaxed)) {
            other.free_list_head_.store(nullptr, memory_order_relaxed);
        }

        void swap(free_list& other) noexcept { _NEFORCE swap_relaxed(free_list_head_, other.free_list_head_); }

        free_list(const free_list&) = delete;
        free_list& operator=(const free_list&) = delete;

        void add(N* node) {
            if (node->free_list_refs_.fetch_add(SHOULD_BE_ON_FREELIST, memory_order_acq_rel) == 0) {
                add_knowing_refcount_is_zero(node);
            }
        }

        N* try_get() {
            auto head = free_list_head_.load(memory_order_acquire);
            while (head != nullptr) {
                auto prev_head = head;
                auto refs = head->free_list_refs_.load(memory_order_relaxed);
                if ((refs & REFS_MASK) == 0 ||
                    !head->free_list_refs_.compare_exchange_strong(refs, refs + 1, memory_order_acquire)) {
                    head = free_list_head_.load(memory_order_acquire);
                    continue;
                }
                auto next = head->free_list_next_.load(memory_order_relaxed);
                if (free_list_head_.compare_exchange_strong(head, next, memory_order_acquire, memory_order_relaxed)) {
                    NEFORCE_DEBUG_VERIFY((head->free_list_refs_.load(memory_order_relaxed) & SHOULD_BE_ON_FREELIST) ==
                                                 0,
                                         "free_list::try_get: node is still marked as should-be-on-freelist");
                    head->free_list_refs_.fetch_sub(2, memory_order_release);
                    return head;
                }
                refs = prev_head->free_list_refs_.fetch_sub(1, memory_order_acq_rel);
                if (refs == SHOULD_BE_ON_FREELIST + 1) {
                    add_knowing_refcount_is_zero(prev_head);
                }
            }
            return nullptr;
        }

        N* head_unsafe() const noexcept { return free_list_head_.load(memory_order_relaxed); }

    private:
        void add_knowing_refcount_is_zero(N* node) noexcept {
            auto head = free_list_head_.load(memory_order_relaxed);
            while (true) {
                node->free_list_next_.store(head, memory_order_relaxed);
                node->free_list_refs_.store(1, memory_order_release);
                if (!free_list_head_.compare_exchange_strong(head, node, memory_order_release, memory_order_relaxed)) {
                    if (node->free_list_refs_.fetch_add(SHOULD_BE_ON_FREELIST - 1, memory_order_acq_rel) == 1) {
                        continue;
                    }
                }
                return;
            }
        }

    private:
        atomic<N*> free_list_head_{nullptr};

        static constexpr uint32_t REFS_MASK = 0x7FFFFFFF;
        static constexpr uint32_t SHOULD_BE_ON_FREELIST = 0x80000000;
    };

    enum inner_queue_context {
        implicit_context = 0,
        explicit_context = 1
    };

    struct block {
        block() = default;

        template <inner_queue_context context>
        NEFORCE_NODISCARD bool is_empty() const {
            NEFORCE_IF_CONSTEXPR(context == explicit_context && BLOCK_SIZE <= EXPLICIT_BLOCK_EMPTY_COUNTER_THRESHOLD) {
                for (size_t i = 0; i < BLOCK_SIZE; ++i) {
                    if (!empty_flags_[i].load(memory_order_relaxed)) {
                        return false;
                    }
                }
                atomic_thread_fence(memory_order_acquire);
                return true;
            }
            else {
                if (elements_completely_dequeued_.load(memory_order_relaxed) == BLOCK_SIZE) {
                    atomic_thread_fence(memory_order_acquire);
                    return true;
                }
                NEFORCE_DEBUG_VERIFY(elements_completely_dequeued_.load(memory_order_relaxed) <= BLOCK_SIZE,
                                     "block::is_empty: counter exceeds block size");
                return false;
            }
        }

        template <inner_queue_context context>
        bool set_empty(const index_t i) {
            NEFORCE_IF_CONSTEXPR(context == explicit_context && BLOCK_SIZE <= EXPLICIT_BLOCK_EMPTY_COUNTER_THRESHOLD) {
                NEFORCE_DEBUG_VERIFY(
                        !empty_flags_[BLOCK_SIZE - 1 - static_cast<size_t>(i & static_cast<index_t>(BLOCK_SIZE - 1))]
                                 .load(memory_order_relaxed),
                        "block::set_empty: flag already set");
                empty_flags_[BLOCK_SIZE - 1 - static_cast<size_t>(i & static_cast<index_t>(BLOCK_SIZE - 1))].store(
                        true, memory_order_release);
                return false;
            }
            else {
                auto prev_val = elements_completely_dequeued_.fetch_add(1, memory_order_acq_rel);
                NEFORCE_DEBUG_VERIFY(prev_val < BLOCK_SIZE, "block::set_empty: counter overflow");
                return prev_val == BLOCK_SIZE - 1;
            }
        }

        template <inner_queue_context context>
        bool set_many_empty(const index_t i, const size_t count) {
            NEFORCE_IF_CONSTEXPR(context == explicit_context && BLOCK_SIZE <= EXPLICIT_BLOCK_EMPTY_COUNTER_THRESHOLD) {
                atomic_thread_fence(memory_order_release);
                size_t idx = BLOCK_SIZE - 1 - static_cast<size_t>(i & static_cast<index_t>(BLOCK_SIZE - 1)) - count + 1;
                for (size_t j = 0; j != count; ++j) {
                    NEFORCE_DEBUG_VERIFY(!empty_flags_[idx + j].load(memory_order_relaxed),
                                         "block::set_many_empty: flag already set");
                    empty_flags_[idx + j].store(true, memory_order_relaxed);
                }
                return false;
            }
            else {
                auto prev_val = elements_completely_dequeued_.fetch_add(count, memory_order_acq_rel);
                NEFORCE_DEBUG_VERIFY(prev_val + count <= BLOCK_SIZE, "block::set_many_empty: counter overflow");
                return prev_val + count == BLOCK_SIZE;
            }
        }

        template <inner_queue_context context>
        void set_all_empty() {
            NEFORCE_IF_CONSTEXPR(context == explicit_context && BLOCK_SIZE <= EXPLICIT_BLOCK_EMPTY_COUNTER_THRESHOLD) {
                for (size_t i = 0; i != BLOCK_SIZE; ++i) {
                    empty_flags_[i].store(true, memory_order_relaxed);
                }
            }
            else {
                elements_completely_dequeued_.store(BLOCK_SIZE, memory_order_relaxed);
            }
        }

        template <inner_queue_context context>
        void reset_empty() {
            NEFORCE_IF_CONSTEXPR(context == explicit_context && BLOCK_SIZE <= EXPLICIT_BLOCK_EMPTY_COUNTER_THRESHOLD) {
                for (size_t i = 0; i != BLOCK_SIZE; ++i) {
                    empty_flags_[i].store(false, memory_order_relaxed);
                }
            }
            else {
                elements_completely_dequeued_.store(0, memory_order_relaxed);
            }
        }

        T* operator[](const index_t idx) noexcept {
            return reinterpret_cast<T*>(&elements_[static_cast<size_t>(idx & static_cast<index_t>(BLOCK_SIZE - 1))]);
        }

        const T* operator[](const index_t idx) const noexcept {
            return reinterpret_cast<const T*>(
                    &elements_[static_cast<size_t>(idx & static_cast<index_t>(BLOCK_SIZE - 1))]);
        }

    private:
        static_assert(alignment_of_v<T> <= sizeof(T),
                      "The queue does not support types with an alignment greater than their size at this time");

        aligned_storage_t<sizeof(T), alignof(T)> elements_[BLOCK_SIZE];

    public:
        block* next_ = nullptr;
        atomic<size_t> elements_completely_dequeued_{0};
        atomic<bool> empty_flags_[BLOCK_SIZE <= EXPLICIT_BLOCK_EMPTY_COUNTER_THRESHOLD ? BLOCK_SIZE : 1];
        atomic<uint32_t> free_list_refs_{0};
        atomic<block*> free_list_next_{nullptr};
        bool dynamically_allocated_ = true;
    };

    static_assert(alignment_of_v<block> >= alignment_of_v<T>,
                  "Internal error: Blocks must be at least as aligned as the type they are wrapping");

    struct producer_base : inner::producer_typeless_base {
        producer_base(lock_free_queue* parent, const bool is_explicit) :
        is_explicit_(is_explicit),
        parent_(parent) {}

        virtual ~producer_base() = default;

        template <typename U>
        bool dequeue(U& element) {
            if (is_explicit_) {
                return static_cast<explicit_producer*>(this)->dequeue(element);
            } else {
                return static_cast<implicit_producer*>(this)->dequeue(element);
            }
        }

        template <typename It>
        size_t dequeue_bulk(It& item_first, const size_t max) {
            if (is_explicit_) {
                return static_cast<explicit_producer*>(this)->dequeue_bulk(item_first, max);
            } else {
                return static_cast<implicit_producer*>(this)->dequeue_bulk(item_first, max);
            }
        }

        producer_base* next_producer() const { return static_cast<producer_base*>(next_); }

        size_t size_approx() const {
            auto tail = tail_index_.load(memory_order_relaxed);
            auto head = head_index_.load(memory_order_relaxed);
            return inner::circular_less_than(head, tail) ? static_cast<size_t>(tail - head) : 0;
        }

        index_t get_tail() const { return tail_index_.load(memory_order_relaxed); }

    protected:
        atomic<index_t> tail_index_{0};
        atomic<index_t> head_index_{0};
        atomic<index_t> dequeue_optimistic_count_{0};
        atomic<index_t> dequeue_overcommit_{0};
        block* tail_block_ = nullptr;

    public:
        bool is_explicit_;
        lock_free_queue* parent_;
    };

    struct explicit_producer : producer_base {
        explicit explicit_producer(lock_free_queue* parent_) :
        producer_base(parent_, true) {
            size_t pool_based_index_size = inner::ceil_to_pow_2(parent_->initial_block_pool_size_) >> 1;
            if (pool_based_index_size > pr_block_index_size_) {
                pr_block_index_size_ = pool_based_index_size;
            }
            new_block_index(0);
        }

        ~explicit_producer() override {
            if (this->tail_block_ != nullptr) {
                block* half_dequeued_block = nullptr;
                if ((this->head_index_.load(memory_order_relaxed) & static_cast<index_t>(BLOCK_SIZE - 1)) != 0) {
                    size_t i = (pr_block_index_front_ - pr_block_index_slots_used_) & (pr_block_index_size_ - 1);
                    while (inner::circular_less_than<index_t>(pr_block_index_entries_[i].base_ + BLOCK_SIZE,
                                                              this->head_index_.load(memory_order_relaxed))) {
                        i = (i + 1) & (pr_block_index_size_ - 1);
                    }
                    NEFORCE_DEBUG_VERIFY(
                            inner::circular_less_than<index_t>(pr_block_index_entries_[i].base_,
                                                               this->head_index_.load(memory_order_relaxed)),
                            "explicit_producer destructor: no partially-dequeued block found");
                    half_dequeued_block = pr_block_index_entries_[i].block_;
                }

                auto blk = this->tail_block_;
                do {
                    blk = blk->next_;
                    if (blk->template is_empty<explicit_context>()) {
                        continue;
                    }

                    size_t i = 0;
                    if (blk == half_dequeued_block) {
                        i = static_cast<size_t>(this->head_index_.load(memory_order_relaxed) &
                                                static_cast<index_t>(BLOCK_SIZE - 1));
                    }

                    auto last_valid_index =
                            (this->tail_index_.load(memory_order_relaxed) & static_cast<index_t>(BLOCK_SIZE - 1)) == 0
                                    ? BLOCK_SIZE
                                    : static_cast<size_t>(this->tail_index_.load(memory_order_relaxed) &
                                                          static_cast<index_t>(BLOCK_SIZE - 1));
                    while (i != BLOCK_SIZE && (blk != this->tail_block_ || i != last_valid_index)) {
                        (*blk)[i++]->~T();
                    }
                } while (blk != this->tail_block_);
            }

            if (this->tail_block_ != nullptr) {
                auto blk = this->tail_block_;
                do {
                    auto next_block = blk->next_;
                    this->parent_->add_block_to_free_list(blk);
                    blk = next_block;
                } while (blk != this->tail_block_);
            }

            auto header = static_cast<block_index_header*>(pr_block_index_raw_);
            while (header != nullptr) {
                auto prev = static_cast<block_index_header*>(header->prev_);
                header->~block_index_header();
                (Traits::free)(header);
                header = prev;
            }
        }

        template <allocation_mode alloc_mode, typename U>
        bool enqueue(U&& element) {
            index_t current_tail_index = this->tail_index_.load(memory_order_relaxed);
            index_t new_tail_index = 1 + current_tail_index;
            if ((current_tail_index & static_cast<index_t>(BLOCK_SIZE - 1)) == 0) {
                auto start_block = this->tail_block_;
                auto original_block_index_slots_used = pr_block_index_slots_used_;
                if (this->tail_block_ != nullptr && this->tail_block_->next_->template is_empty<explicit_context>()) {
                    this->tail_block_ = this->tail_block_->next_;
                    this->tail_block_->template reset_empty<explicit_context>();
                } else {
                    auto head = this->head_index_.load(memory_order_relaxed);
                    NEFORCE_DEBUG_VERIFY(!inner::circular_less_than<index_t>(current_tail_index, head),
                                         "explicit_producer::enqueue: tail precedes head");
                    if (!inner::circular_less_than<index_t>(head, current_tail_index + BLOCK_SIZE) ||
                        (MAX_SUBQUEUE_SIZE != inner::const_numeric_max<size_t>::value &&
                         (MAX_SUBQUEUE_SIZE == 0 || MAX_SUBQUEUE_SIZE - BLOCK_SIZE < current_tail_index - head))) {
                        return false;
                    }
                    if (pr_block_index_raw_ == nullptr || pr_block_index_slots_used_ == pr_block_index_size_) {
                        NEFORCE_IF_CONSTEXPR(alloc_mode == cannot_alloc) { return false; }
                        else if (!new_block_index(pr_block_index_slots_used_)) {
                            return false;
                        }
                    }

                    auto new_block = this->parent_->template requisition_block<alloc_mode>();
                    if (new_block == nullptr) {
                        return false;
                    }
                    new_block->template reset_empty<explicit_context>();
                    if (this->tail_block_ == nullptr) {
                        new_block->next_ = new_block;
                    } else {
                        new_block->next_ = this->tail_block_->next_;
                        this->tail_block_->next_ = new_block;
                    }
                    this->tail_block_ = new_block;
                    ++pr_block_index_slots_used_;
                }

                NEFORCE_IF_CONSTEXPR(!is_nothrow_constructible_v<T, U>) {
                    try {
                        _NEFORCE construct((*this->tail_block_)[current_tail_index], _NEFORCE forward<U>(element));
                    } catch (...) {
                        pr_block_index_slots_used_ = original_block_index_slots_used;
                        this->tail_block_ = start_block == nullptr ? this->tail_block_ : start_block;
                        throw;
                    }
                }
                else {
                    (void) start_block;
                    (void) original_block_index_slots_used;
                }

                auto& entry = block_index_.load(memory_order_relaxed)->entries_[pr_block_index_front_];
                entry.base_ = current_tail_index;
                entry.block_ = this->tail_block_;
                block_index_.load(memory_order_relaxed)->front_.store(pr_block_index_front_, memory_order_release);
                pr_block_index_front_ = (pr_block_index_front_ + 1) & (pr_block_index_size_ - 1);

                NEFORCE_IF_CONSTEXPR(!is_nothrow_constructible_v<T, U>) {
                    this->tail_index_.store(new_tail_index, memory_order_release);
                    return true;
                }
            }

            _NEFORCE construct((*this->tail_block_)[current_tail_index], _NEFORCE forward<U>(element));

            this->tail_index_.store(new_tail_index, memory_order_release);
            return true;
        }

        template <typename U>
        bool dequeue(U& element) {
            auto tail = this->tail_index_.load(memory_order_relaxed);
            auto overcommit = this->dequeue_overcommit_.load(memory_order_relaxed);
            if (inner::circular_less_than<index_t>(
                        this->dequeue_optimistic_count_.load(memory_order_relaxed) - overcommit, tail)) {
                atomic_thread_fence(memory_order_acquire);
                auto my_dequeue_count = this->dequeue_optimistic_count_.fetch_add(1, memory_order_relaxed);
                tail = this->tail_index_.load(memory_order_acquire);

                if (inner::circular_less_than<index_t>(my_dequeue_count - overcommit, tail)) {
                    auto index = this->head_index_.fetch_add(1, memory_order_acq_rel);
                    auto local_block_index = block_index_.load(memory_order_acquire);
                    auto local_block_index_head = local_block_index->front_.load(memory_order_acquire);

                    auto head_base = local_block_index->entries_[local_block_index_head].base_;
                    auto block_base_index = index & ~static_cast<index_t>(BLOCK_SIZE - 1);
                    auto offset =
                            static_cast<size_t>(static_cast<make_signed_t<index_t>>(block_base_index - head_base) /
                                                static_cast<make_signed_t<index_t>>(BLOCK_SIZE));
                    auto blk = local_block_index
                                       ->entries_[(local_block_index_head + offset) & (local_block_index->size_ - 1)]
                                       .block_;

                    auto& el = *((*blk)[index]);
                    // NOLINTNEXTLINE(bugprone-assignment-in-if-condition)
                    if (!noexcept(element = _NEFORCE move(el))) {
                        struct guard {
                            block* blk;
                            index_t index;

                            ~guard() {
                                (*blk)[index]->~T();
                                blk->template set_empty<explicit_context>(index);
                            }
                        } g = {blk, index};

                        element = _NEFORCE move(el);
                    } else {
                        element = _NEFORCE move(el);
                        el.~T();
                        blk->template set_empty<explicit_context>(index);
                    }

                    return true;
                } else {
                    this->dequeue_overcommit_.fetch_add(1, memory_order_release);
                }
            }

            return false;
        }

        template <allocation_mode alloc_mode, typename It>
        NEFORCE_NO_TSAN bool enqueue_bulk(It item_first, const size_t count) {
            index_t start_tail_index = this->tail_index_.load(memory_order_relaxed);
            auto start_block = this->tail_block_;
            auto original_block_index_front = pr_block_index_front_;
            auto original_block_index_slots_used = pr_block_index_slots_used_;

            block* first_allocated_block = nullptr;

            size_t block_base_diff = ((start_tail_index + count - 1) & ~static_cast<index_t>(BLOCK_SIZE - 1)) -
                                     ((start_tail_index - 1) & ~static_cast<index_t>(BLOCK_SIZE - 1));
            index_t current_tail_index = (start_tail_index - 1) & ~static_cast<index_t>(BLOCK_SIZE - 1);
            if (block_base_diff > 0) {
                while (block_base_diff > 0 && this->tail_block_ != nullptr &&
                       this->tail_block_->next_ != first_allocated_block &&
                       this->tail_block_->next_->template is_empty<explicit_context>()) {
                    block_base_diff -= static_cast<index_t>(BLOCK_SIZE);
                    current_tail_index += static_cast<index_t>(BLOCK_SIZE);

                    this->tail_block_ = this->tail_block_->next_;
                    first_allocated_block =
                            first_allocated_block == nullptr ? this->tail_block_ : first_allocated_block;

                    auto& entry = block_index_.load(memory_order_relaxed)->entries_[pr_block_index_front_];
                    entry.base_ = current_tail_index;
                    entry.block_ = this->tail_block_;
                    pr_block_index_front_ = (pr_block_index_front_ + 1) & (pr_block_index_size_ - 1);
                }

                while (block_base_diff > 0) {
                    block_base_diff -= static_cast<index_t>(BLOCK_SIZE);
                    current_tail_index += static_cast<index_t>(BLOCK_SIZE);

                    auto head = this->head_index_.load(memory_order_relaxed);
                    NEFORCE_DEBUG_VERIFY(!inner::circular_less_than<index_t>(current_tail_index, head),
                                         "explicit_producer::enqueue_bulk: tail precedes head");
                    bool full =
                            !inner::circular_less_than<index_t>(head, current_tail_index + BLOCK_SIZE) ||
                            (MAX_SUBQUEUE_SIZE != inner::const_numeric_max<size_t>::value &&
                             (MAX_SUBQUEUE_SIZE == 0 || MAX_SUBQUEUE_SIZE - BLOCK_SIZE < current_tail_index - head));
                    if (pr_block_index_raw_ == nullptr || pr_block_index_slots_used_ == pr_block_index_size_ || full) {
                        NEFORCE_IF_CONSTEXPR(alloc_mode == cannot_alloc) {
                            pr_block_index_front_ = original_block_index_front;
                            pr_block_index_slots_used_ = original_block_index_slots_used;
                            this->tail_block_ = start_block == nullptr ? first_allocated_block : start_block;
                            return false;
                        }
                        else if (full || !new_block_index(original_block_index_slots_used)) {
                            pr_block_index_front_ = original_block_index_front;
                            pr_block_index_slots_used_ = original_block_index_slots_used;
                            this->tail_block_ = start_block == nullptr ? first_allocated_block : start_block;
                            return false;
                        }
                        else {
                            original_block_index_front = original_block_index_slots_used;
                        }
                    }

                    auto new_block = this->parent_->template requisition_block<alloc_mode>();
                    if (new_block == nullptr) {
                        pr_block_index_front_ = original_block_index_front;
                        pr_block_index_slots_used_ = original_block_index_slots_used;
                        this->tail_block_ = start_block == nullptr ? first_allocated_block : start_block;
                        return false;
                    }

                    new_block->template set_all_empty<explicit_context>();
                    if (this->tail_block_ == nullptr) {
                        new_block->next_ = new_block;
                    } else {
                        new_block->next_ = this->tail_block_->next_;
                        this->tail_block_->next_ = new_block;
                    }
                    this->tail_block_ = new_block;
                    first_allocated_block =
                            first_allocated_block == nullptr ? this->tail_block_ : first_allocated_block;

                    ++pr_block_index_slots_used_;

                    auto& entry = block_index_.load(memory_order_relaxed)->entries_[pr_block_index_front_];
                    entry.base_ = current_tail_index;
                    entry.block_ = this->tail_block_;
                    pr_block_index_front_ = (pr_block_index_front_ + 1) & (pr_block_index_size_ - 1);
                }

                auto blk = first_allocated_block;
                while (true) {
                    blk->template reset_empty<explicit_context>();
                    if (blk == this->tail_block_) {
                        break;
                    }
                    blk = blk->next_;
                }

                NEFORCE_IF_CONSTEXPR(is_nothrow_constructible_v<T, decltype(*item_first)>) {
                    block_index_.load(memory_order_relaxed)
                            ->front_.store((pr_block_index_front_ - 1) & (pr_block_index_size_ - 1),
                                           memory_order_release);
                }
            }

            index_t new_tail_index = start_tail_index + static_cast<index_t>(count);
            current_tail_index = start_tail_index;
            auto end_block = this->tail_block_;
            this->tail_block_ = start_block;
            NEFORCE_DEBUG_VERIFY((start_tail_index & static_cast<index_t>(BLOCK_SIZE - 1)) != 0 ||
                                         first_allocated_block != nullptr || count == 0,
                                 "explicit_producer::enqueue_bulk: invariant violated");
            if ((start_tail_index & static_cast<index_t>(BLOCK_SIZE - 1)) == 0 && first_allocated_block != nullptr) {
                this->tail_block_ = first_allocated_block;
            }
            while (true) {
                index_t stop_index =
                        (current_tail_index & ~static_cast<index_t>(BLOCK_SIZE - 1)) + static_cast<index_t>(BLOCK_SIZE);
                if (inner::circular_less_than<index_t>(new_tail_index, stop_index)) {
                    stop_index = new_tail_index;
                }
                NEFORCE_IF_CONSTEXPR(is_nothrow_constructible_v<T, decltype(*item_first)>) {
                    while (current_tail_index != stop_index) {
                        _NEFORCE construct((*this->tail_block_)[current_tail_index++], *item_first++);
                    }
                }
                else {
                    try {
                        while (current_tail_index != stop_index) {
                            _NEFORCE construct(
                                    (*this->tail_block_)[current_tail_index],
                                    inner::nomove_if<!is_nothrow_constructible_v<T, decltype(*item_first)>>::eval(
                                            *item_first));
                            ++current_tail_index;
                            ++item_first;
                        }
                    } catch (...) {
                        auto constructed_stop_index = current_tail_index;
                        auto last_block_enqueued = this->tail_block_;

                        pr_block_index_front_ = original_block_index_front;
                        pr_block_index_slots_used_ = original_block_index_slots_used;
                        this->tail_block_ = start_block == nullptr ? first_allocated_block : start_block;

                        if (!is_trivially_destructible_v<T>) {
                            auto blk = start_block;
                            if ((start_tail_index & static_cast<index_t>(BLOCK_SIZE - 1)) == 0) {
                                blk = first_allocated_block;
                            }
                            current_tail_index = start_tail_index;
                            while (true) {
                                stop_index = (current_tail_index & ~static_cast<index_t>(BLOCK_SIZE - 1)) +
                                             static_cast<index_t>(BLOCK_SIZE);
                                if (inner::circular_less_than<index_t>(constructed_stop_index, stop_index)) {
                                    stop_index = constructed_stop_index;
                                }
                                while (current_tail_index != stop_index) {
                                    (*blk)[current_tail_index++]->~T();
                                }
                                if (blk == last_block_enqueued) {
                                    break;
                                }
                                blk = blk->next_;
                            }
                        }
                        throw;
                    }
                }

                if (this->tail_block_ == end_block) {
                    NEFORCE_DEBUG_VERIFY(current_tail_index == new_tail_index,
                                         "explicit_producer::enqueue_bulk: tail mismatch");
                    break;
                }
                this->tail_block_ = this->tail_block_->next_;
            }

            NEFORCE_IF_CONSTEXPR(!is_nothrow_constructible_v<T, decltype(*item_first)>) {
                if (first_allocated_block != nullptr) {
                    block_index_.load(memory_order_relaxed)
                            ->front_.store((pr_block_index_front_ - 1) & (pr_block_index_size_ - 1),
                                           memory_order_release);
                }
            }

            this->tail_index_.store(new_tail_index, memory_order_release);
            return true;
        }

        template <typename It>
        size_t dequeue_bulk(It& item_first, const size_t max) {
            auto tail = this->tail_index_.load(memory_order_relaxed);
            auto overcommit = this->dequeue_overcommit_.load(memory_order_relaxed);
            auto desired_count = static_cast<size_t>(
                    tail - (this->dequeue_optimistic_count_.load(memory_order_relaxed) - overcommit));
            if (inner::circular_less_than<size_t>(0, desired_count)) {
                desired_count = desired_count < max ? desired_count : max;
                atomic_thread_fence(memory_order_acquire);

                auto my_dequeue_count = this->dequeue_optimistic_count_.fetch_add(desired_count, memory_order_relaxed);

                tail = this->tail_index_.load(memory_order_acquire);
                auto actual_count = static_cast<size_t>(tail - (my_dequeue_count - overcommit));
                if (inner::circular_less_than<size_t>(0, actual_count)) {
                    actual_count = desired_count < actual_count ? desired_count : actual_count;
                    if (actual_count < desired_count) {
                        this->dequeue_overcommit_.fetch_add(desired_count - actual_count, memory_order_release);
                    }

                    auto first_index = this->head_index_.fetch_add(actual_count, memory_order_acq_rel);

                    auto local_block_index = block_index_.load(memory_order_acquire);
                    auto local_block_index_head = local_block_index->front_.load(memory_order_acquire);

                    auto head_base = local_block_index->entries_[local_block_index_head].base_;
                    auto first_block_base_index = first_index & ~static_cast<index_t>(BLOCK_SIZE - 1);
                    auto offset = static_cast<size_t>(
                            static_cast<make_signed_t<index_t>>(first_block_base_index - head_base) /
                            static_cast<make_signed_t<index_t>>(BLOCK_SIZE));
                    auto index_index = (local_block_index_head + offset) & (local_block_index->size_ - 1);

                    auto index = first_index;
                    do {
                        auto first_index_in_block = index;
                        index_t end_index =
                                (index & ~static_cast<index_t>(BLOCK_SIZE - 1)) + static_cast<index_t>(BLOCK_SIZE);
                        end_index = inner::circular_less_than<index_t>(first_index + static_cast<index_t>(actual_count),
                                                                       end_index)
                                            ? first_index + static_cast<index_t>(actual_count)
                                            : end_index;
                        auto blk = local_block_index->entries_[index_index].block_;
                        // NOLINTNEXTLINE(bugprone-assignment-in-if-condition)
                        if (noexcept(*item_first = _NEFORCE move((*(*blk)[index])))) {
                            while (index != end_index) {
                                auto& el = *((*blk)[index]);
                                *item_first++ = _NEFORCE move(el);
                                el.~T();
                                ++index;
                            }
                        } else {
                            try {
                                while (index != end_index) {
                                    auto& el = *((*blk)[index]);
                                    *item_first = _NEFORCE move(el);
                                    ++item_first;
                                    el.~T();
                                    ++index;
                                }
                            } catch (...) {
                                do {
                                    blk = local_block_index->entries_[index_index].block_;
                                    while (index != end_index) {
                                        (*blk)[index++]->~T();
                                    }
                                    blk->template set_many_empty<explicit_context>(
                                            first_index_in_block,
                                            static_cast<size_t>(end_index - first_index_in_block));
                                    index_index = (index_index + 1) & (local_block_index->size_ - 1);

                                    first_index_in_block = index;
                                    end_index = (index & ~static_cast<index_t>(BLOCK_SIZE - 1)) +
                                                static_cast<index_t>(BLOCK_SIZE);
                                    end_index = inner::circular_less_than<index_t>(
                                                        first_index + static_cast<index_t>(actual_count), end_index)
                                                        ? first_index + static_cast<index_t>(actual_count)
                                                        : end_index;
                                } while (index != first_index + actual_count);

                                throw;
                            }
                        }
                        blk->template set_many_empty<explicit_context>(
                                first_index_in_block, static_cast<size_t>(end_index - first_index_in_block));
                        index_index = (index_index + 1) & (local_block_index->size_ - 1);
                    } while (index != first_index + actual_count);

                    return actual_count;
                } else {
                    this->dequeue_overcommit_.fetch_add(desired_count, memory_order_release);
                }
            }

            return 0;
        }

    private:
        struct block_index_entry {
            index_t base_;
            block* block_;
        };

        struct block_index_header {
            size_t size_;
            atomic<size_t> front_;
            block_index_entry* entries_;
            void* prev_;
        };

        bool new_block_index(const size_t number_of_filled_slots_to_expose) {
            auto prev_block_size_mask = pr_block_index_size_ - 1;

            pr_block_index_size_ <<= 1;
            auto* new_raw_ptr =
                    static_cast<char*>((Traits::malloc)(sizeof(block_index_header) + alignment_of_v<block_index_entry> -
                                                        1 + sizeof(block_index_entry) * pr_block_index_size_));
            if (new_raw_ptr == nullptr) {
                pr_block_index_size_ >>= 1;
                return false;
            }

            auto new_block_index_entries = reinterpret_cast<block_index_entry*>(
                    inner::align_for<block_index_entry>(new_raw_ptr + sizeof(block_index_header)));

            size_t j = 0;
            if (pr_block_index_slots_used_ != 0) {
                auto i = (pr_block_index_front_ - pr_block_index_slots_used_) & prev_block_size_mask;
                do {
                    new_block_index_entries[j++] = pr_block_index_entries_[i];
                    i = (i + 1) & prev_block_size_mask;
                } while (i != pr_block_index_front_);
            }

            auto header = new (new_raw_ptr) block_index_header;
            header->size_ = pr_block_index_size_;
            header->front_.store(number_of_filled_slots_to_expose - 1, memory_order_relaxed);
            header->entries_ = new_block_index_entries;
            header->prev_ = pr_block_index_raw_;

            pr_block_index_front_ = j;
            pr_block_index_entries_ = new_block_index_entries;
            pr_block_index_raw_ = new_raw_ptr;
            block_index_.store(header, memory_order_release);

            return true;
        }

    private:
        atomic<block_index_header*> block_index_{nullptr};
        size_t pr_block_index_slots_used_ = 0;
        size_t pr_block_index_size_ = EXPLICIT_INITIAL_INDEX_SIZE >> 1;
        size_t pr_block_index_front_ = 0;
        block_index_entry* pr_block_index_entries_ = nullptr;
        void* pr_block_index_raw_ = nullptr;
    };

    struct implicit_producer : producer_base {
        implicit_producer(lock_free_queue* parent_) :
        producer_base(parent_, false) {
            new_block_index();
        }

        ~implicit_producer() override {
            if (!this->inactive_.load(memory_order_relaxed)) {
                thread_exit_notifier::unsubscribe(&thread_exit_listener_);
            }

            auto tail = this->tail_index_.load(memory_order_relaxed);
            auto index = this->head_index_.load(memory_order_relaxed);
            block* blk = nullptr;
            NEFORCE_DEBUG_VERIFY(index == tail || inner::circular_less_than(index, tail),
                                 "implicit_producer destructor: head past tail");
            bool force_free_last_block = index != tail;
            while (index != tail) {
                if ((index & static_cast<index_t>(BLOCK_SIZE - 1)) == 0 || blk == nullptr) {
                    if (blk != nullptr) {
                        this->parent_->add_block_to_free_list(blk);
                    }
                    blk = get_block_index_entry_for_index(index)->value_.load(memory_order_relaxed);
                }
                ((*blk)[index])->~T();
                ++index;
            }

            if (this->tail_block_ != nullptr &&
                (force_free_last_block || (tail & static_cast<index_t>(BLOCK_SIZE - 1)) != 0)) {
                this->parent_->add_block_to_free_list(this->tail_block_);
            }

            auto local_block_index = block_index_.load(memory_order_relaxed);
            if (local_block_index != nullptr) {
                for (size_t i = 0; i != local_block_index->capacity_; ++i) {
                    local_block_index->index_[i]->~block_index_entry();
                }
                do {
                    auto prev = local_block_index->prev_;
                    local_block_index->~block_index_header();
                    (Traits::free)(local_block_index);
                    local_block_index = prev;
                } while (local_block_index != nullptr);
            }
        }

        template <allocation_mode alloc_mode, typename U>
        bool enqueue(U&& element) {
            index_t current_tail_index = this->tail_index_.load(memory_order_relaxed);
            index_t new_tail_index = 1 + current_tail_index;
            if ((current_tail_index & static_cast<index_t>(BLOCK_SIZE - 1)) == 0) {
                auto head = this->head_index_.load(memory_order_relaxed);
                NEFORCE_DEBUG_VERIFY(!inner::circular_less_than<index_t>(current_tail_index, head),
                                     "implicit_producer::enqueue: tail precedes head");
                if (!inner::circular_less_than<index_t>(head, current_tail_index + BLOCK_SIZE) ||
                    (MAX_SUBQUEUE_SIZE != inner::const_numeric_max<size_t>::value &&
                     (MAX_SUBQUEUE_SIZE == 0 || MAX_SUBQUEUE_SIZE - BLOCK_SIZE < current_tail_index - head))) {
                    return false;
                }

                block_index_entry* idx_entry = nullptr;
                if (!insert_block_index_entry<alloc_mode>(idx_entry, current_tail_index)) {
                    return false;
                }

                auto new_block = this->parent_->template requisition_block<alloc_mode>();
                if (new_block == nullptr) {
                    rewind_block_index_tail();
                    idx_entry->value_.store(nullptr, memory_order_relaxed);
                    return false;
                }
                new_block->template reset_empty<implicit_context>();

                NEFORCE_IF_CONSTEXPR(!is_nothrow_constructible_v<T, U>) {
                    try {
                        _NEFORCE construct((*new_block)[current_tail_index], _NEFORCE forward<U>(element));
                    } catch (...) {
                        rewind_block_index_tail();
                        idx_entry->value_.store(nullptr, memory_order_relaxed);
                        this->parent_->add_block_to_free_list(new_block);
                        throw;
                    }
                }

                idx_entry->value_.store(new_block, memory_order_relaxed);

                this->tail_block_ = new_block;

                NEFORCE_IF_CONSTEXPR(!is_nothrow_constructible_v<T, U>) {
                    this->tail_index_.store(new_tail_index, memory_order_release);
                    return true;
                }
            }

            _NEFORCE construct((*this->tail_block_)[current_tail_index], _NEFORCE forward<U>(element));

            this->tail_index_.store(new_tail_index, memory_order_release);
            return true;
        }

        template <typename U>
        bool dequeue(U& element) {
            index_t tail = this->tail_index_.load(memory_order_relaxed);
            index_t overcommit = this->dequeue_overcommit_.load(memory_order_relaxed);
            if (inner::circular_less_than<index_t>(
                        this->dequeue_optimistic_count_.load(memory_order_relaxed) - overcommit, tail)) {
                atomic_thread_fence(memory_order_acquire);

                index_t my_dequeue_count = this->dequeue_optimistic_count_.fetch_add(1, memory_order_relaxed);
                tail = this->tail_index_.load(memory_order_acquire);
                if (inner::circular_less_than<index_t>(my_dequeue_count - overcommit, tail)) {
                    index_t index = this->head_index_.fetch_add(1, memory_order_acq_rel);

                    auto entry = get_block_index_entry_for_index(index);

                    auto blk = entry->value_.load(memory_order_relaxed);
                    auto& el = *((*blk)[index]);

                    // NOLINTNEXTLINE(bugprone-assignment-in-if-condition)
                    if (!noexcept(element = _NEFORCE move(el))) {
                        struct guard {
                            block* blk;
                            index_t index;
                            block_index_entry* entry;
                            lock_free_queue* parent;

                            ~guard() {
                                (*blk)[index]->~T();
                                if (blk->template set_empty<implicit_context>(index)) {
                                    entry->value_.store(nullptr, memory_order_relaxed);
                                    parent->add_block_to_free_list(blk);
                                }
                            }
                        } g = {blk, index, entry, this->parent_};

                        element = _NEFORCE move(el);
                    } else {
                        element = _NEFORCE move(el);
                        el.~T();

                        if (blk->template set_empty<implicit_context>(index)) {
                            entry->value_.store(nullptr, memory_order_relaxed);
                            this->parent_->add_block_to_free_list(blk);
                        }
                    }

                    return true;
                } else {
                    this->dequeue_overcommit_.fetch_add(1, memory_order_release);
                }
            }

            return false;
        }

        template <allocation_mode alloc_mode, typename It>
        bool enqueue_bulk(It item_first, const size_t count) {
            index_t start_tail_index = this->tail_index_.load(memory_order_relaxed);
            auto start_block = this->tail_block_;
            block* first_allocated_block = nullptr;
            auto end_block = this->tail_block_;

            size_t block_base_diff = ((start_tail_index + count - 1) & ~static_cast<index_t>(BLOCK_SIZE - 1)) -
                                     ((start_tail_index - 1) & ~static_cast<index_t>(BLOCK_SIZE - 1));
            index_t current_tail_index = (start_tail_index - 1) & ~static_cast<index_t>(BLOCK_SIZE - 1);
            if (block_base_diff > 0) {
                do {
                    block_base_diff -= static_cast<index_t>(BLOCK_SIZE);
                    current_tail_index += static_cast<index_t>(BLOCK_SIZE);

                    block_index_entry* idx_entry = nullptr;
                    block* new_block = nullptr;
                    bool index_inserted = false;
                    auto head = this->head_index_.load(memory_order_relaxed);
                    NEFORCE_DEBUG_VERIFY(!inner::circular_less_than<index_t>(current_tail_index, head),
                                         "implicit_producer::enqueue_bulk: tail precedes head");
                    bool full =
                            !inner::circular_less_than<index_t>(head, current_tail_index + BLOCK_SIZE) ||
                            (MAX_SUBQUEUE_SIZE != inner::const_numeric_max<size_t>::value &&
                             (MAX_SUBQUEUE_SIZE == 0 || MAX_SUBQUEUE_SIZE - BLOCK_SIZE < current_tail_index - head));

                    if (full ||
                        // NOLINTNEXTLINE(bugprone-assignment-in-if-condition)
                        !(index_inserted = insert_block_index_entry<alloc_mode>(idx_entry, current_tail_index)) ||
                        // NOLINTNEXTLINE(bugprone-assignment-in-if-condition)
                        (new_block = this->parent_->template requisition_block<alloc_mode>()) == nullptr) {
                        if (index_inserted) {
                            rewind_block_index_tail();
                            idx_entry->value_.store(nullptr, memory_order_relaxed);
                        }
                        current_tail_index = (start_tail_index - 1) & ~static_cast<index_t>(BLOCK_SIZE - 1);
                        for (auto blk = first_allocated_block; blk != nullptr; blk = blk->next_) {
                            current_tail_index += static_cast<index_t>(BLOCK_SIZE);
                            idx_entry = get_block_index_entry_for_index(current_tail_index);
                            idx_entry->value_.store(nullptr, memory_order_relaxed);
                            rewind_block_index_tail();
                        }
                        this->parent_->add_blocks_to_free_list(first_allocated_block);
                        this->tail_block_ = start_block;

                        return false;
                    }

                    new_block->template reset_empty<implicit_context>();
                    new_block->next_ = nullptr;

                    idx_entry->value_.store(new_block, memory_order_relaxed);

                    if ((start_tail_index & static_cast<index_t>(BLOCK_SIZE - 1)) != 0 ||
                        first_allocated_block != nullptr) {
                        NEFORCE_DEBUG_VERIFY(this->tail_block_ != nullptr,
                                             "implicit_producer::enqueue_bulk: tail block is null");
                        this->tail_block_->next_ = new_block;
                    }
                    this->tail_block_ = new_block;
                    end_block = new_block;
                    first_allocated_block = first_allocated_block == nullptr ? new_block : first_allocated_block;
                } while (block_base_diff > 0);
            }

            index_t new_tail_index = start_tail_index + static_cast<index_t>(count);
            current_tail_index = start_tail_index;
            this->tail_block_ = start_block;
            NEFORCE_DEBUG_VERIFY((start_tail_index & static_cast<index_t>(BLOCK_SIZE - 1)) != 0 ||
                                         first_allocated_block != nullptr || count == 0,
                                 "implicit_producer::enqueue_bulk: invariant violated");
            if ((start_tail_index & static_cast<index_t>(BLOCK_SIZE - 1)) == 0 && first_allocated_block != nullptr) {
                this->tail_block_ = first_allocated_block;
            }
            while (true) {
                index_t stop_index =
                        (current_tail_index & ~static_cast<index_t>(BLOCK_SIZE - 1)) + static_cast<index_t>(BLOCK_SIZE);
                if (inner::circular_less_than<index_t>(new_tail_index, stop_index)) {
                    stop_index = new_tail_index;
                }
                NEFORCE_IF_CONSTEXPR(is_nothrow_constructible_v<T, decltype(*item_first)>) {
                    while (current_tail_index != stop_index) {
                        _NEFORCE construct((*this->tail_block_)[current_tail_index++], *item_first++);
                    }
                }
                else {
                    try {
                        while (current_tail_index != stop_index) {
                            _NEFORCE construct(
                                    (*this->tail_block_)[current_tail_index],
                                    inner::nomove_if<!is_nothrow_constructible_v<T, decltype(*item_first)>>::eval(
                                            *item_first));
                            ++current_tail_index;
                            ++item_first;
                        }
                    } catch (...) {
                        auto constructed_stop_index = current_tail_index;
                        auto last_block_enqueued = this->tail_block_;

                        if (!is_trivially_destructible_v<T>) {
                            auto blk = start_block;
                            if ((start_tail_index & static_cast<index_t>(BLOCK_SIZE - 1)) == 0) {
                                blk = first_allocated_block;
                            }
                            current_tail_index = start_tail_index;
                            while (true) {
                                stop_index = (current_tail_index & ~static_cast<index_t>(BLOCK_SIZE - 1)) +
                                             static_cast<index_t>(BLOCK_SIZE);
                                if (inner::circular_less_than<index_t>(constructed_stop_index, stop_index)) {
                                    stop_index = constructed_stop_index;
                                }
                                while (current_tail_index != stop_index) {
                                    (*blk)[current_tail_index++]->~T();
                                }
                                if (blk == last_block_enqueued) {
                                    break;
                                }
                                blk = blk->next_;
                            }
                        }

                        current_tail_index = (start_tail_index - 1) & ~static_cast<index_t>(BLOCK_SIZE - 1);
                        for (auto blk = first_allocated_block; blk != nullptr; blk = blk->next_) {
                            current_tail_index += static_cast<index_t>(BLOCK_SIZE);
                            auto idx_entry = get_block_index_entry_for_index(current_tail_index);
                            idx_entry->value_.store(nullptr, memory_order_relaxed);
                            rewind_block_index_tail();
                        }
                        this->parent_->add_blocks_to_free_list(first_allocated_block);
                        this->tail_block_ = start_block;
                        throw;
                    }
                }

                if (this->tail_block_ == end_block) {
                    NEFORCE_DEBUG_VERIFY(current_tail_index == new_tail_index,
                                         "implicit_producer::enqueue_bulk: tail mismatch");
                    break;
                }
                this->tail_block_ = this->tail_block_->next_;
            }
            this->tail_index_.store(new_tail_index, memory_order_release);
            return true;
        }

        template <typename It>
        size_t dequeue_bulk(It& item_first, const size_t max) {
            auto tail = this->tail_index_.load(memory_order_relaxed);
            auto overcommit = this->dequeue_overcommit_.load(memory_order_relaxed);
            auto desired_count = static_cast<size_t>(
                    tail - (this->dequeue_optimistic_count_.load(memory_order_relaxed) - overcommit));
            if (inner::circular_less_than<size_t>(0, desired_count)) {
                desired_count = desired_count < max ? desired_count : max;
                atomic_thread_fence(memory_order_acquire);

                auto my_dequeue_count = this->dequeue_optimistic_count_.fetch_add(desired_count, memory_order_relaxed);

                tail = this->tail_index_.load(memory_order_acquire);
                auto actual_count = static_cast<size_t>(tail - (my_dequeue_count - overcommit));
                if (inner::circular_less_than<size_t>(0, actual_count)) {
                    actual_count = desired_count < actual_count ? desired_count : actual_count;
                    if (actual_count < desired_count) {
                        this->dequeue_overcommit_.fetch_add(desired_count - actual_count, memory_order_release);
                    }

                    auto first_index = this->head_index_.fetch_add(actual_count, memory_order_acq_rel);

                    auto index = first_index;
                    block_index_header* local_block_index = nullptr;
                    auto index_index = get_block_index_index_for_index(index, local_block_index);
                    do {
                        auto block_start_index = index;
                        index_t end_index =
                                (index & ~static_cast<index_t>(BLOCK_SIZE - 1)) + static_cast<index_t>(BLOCK_SIZE);
                        end_index = inner::circular_less_than<index_t>(first_index + static_cast<index_t>(actual_count),
                                                                       end_index)
                                            ? first_index + static_cast<index_t>(actual_count)
                                            : end_index;

                        auto entry = local_block_index->index_[index_index];
                        auto blk = entry->value_.load(memory_order_relaxed);
                        // NOLINTNEXTLINE(bugprone-assignment-in-if-condition)
                        if (noexcept(*item_first = _NEFORCE move((*(*blk)[index])))) {
                            while (index != end_index) {
                                auto& el = *((*blk)[index]);
                                *item_first++ = _NEFORCE move(el);
                                el.~T();
                                ++index;
                            }
                        } else {
                            try {
                                while (index != end_index) {
                                    auto& el = *((*blk)[index]);
                                    *item_first = _NEFORCE move(el);
                                    ++item_first;
                                    el.~T();
                                    ++index;
                                }
                            } catch (...) {
                                do {
                                    entry = local_block_index->index_[index_index];
                                    blk = entry->value_.load(memory_order_relaxed);
                                    while (index != end_index) {
                                        (*blk)[index++]->~T();
                                    }

                                    if (blk->template set_many_empty<implicit_context>(
                                                block_start_index,
                                                static_cast<size_t>(end_index - block_start_index))) {
                                        entry->value_.store(nullptr, memory_order_relaxed);
                                        this->parent_->add_block_to_free_list(blk);
                                    }
                                    index_index = (index_index + 1) & (local_block_index->capacity_ - 1);

                                    block_start_index = index;
                                    end_index = (index & ~static_cast<index_t>(BLOCK_SIZE - 1)) +
                                                static_cast<index_t>(BLOCK_SIZE);
                                    end_index = inner::circular_less_than<index_t>(
                                                        first_index + static_cast<index_t>(actual_count), end_index)
                                                        ? first_index + static_cast<index_t>(actual_count)
                                                        : end_index;
                                } while (index != first_index + actual_count);

                                throw;
                            }
                        }
                        if (blk->template set_many_empty<implicit_context>(
                                    block_start_index, static_cast<size_t>(end_index - block_start_index))) {
                            entry->value_.store(nullptr, memory_order_relaxed);
                            this->parent_->add_block_to_free_list(blk);
                        }
                        index_index = (index_index + 1) & (local_block_index->capacity_ - 1);
                    } while (index != first_index + actual_count);

                    return actual_count;
                } else {
                    this->dequeue_overcommit_.fetch_add(desired_count, memory_order_release);
                }
            }

            return 0;
        }

    private:
        static constexpr index_t INVALID_BLOCK_BASE = 1;

        struct block_index_entry {
            atomic<index_t> key_;
            atomic<block*> value_;
        };

        struct block_index_header {
            size_t capacity_;
            atomic<size_t> tail_;
            block_index_entry* entries_;
            block_index_entry** index_;
            block_index_header* prev_;
        };

        template <allocation_mode alloc_mode>
        bool insert_block_index_entry(block_index_entry*& idx_entry, const index_t block_start_index) {
            auto local_block_index = block_index_.load(memory_order_relaxed);
            if (local_block_index == nullptr) {
                return false;
            }
            size_t new_tail =
                    (local_block_index->tail_.load(memory_order_relaxed) + 1) & (local_block_index->capacity_ - 1);
            idx_entry = local_block_index->index_[new_tail];
            if (idx_entry->key_.load(memory_order_relaxed) == INVALID_BLOCK_BASE ||
                idx_entry->value_.load(memory_order_relaxed) == nullptr) {
                idx_entry->key_.store(block_start_index, memory_order_relaxed);
                local_block_index->tail_.store(new_tail, memory_order_release);
                return true;
            }

            NEFORCE_IF_CONSTEXPR(alloc_mode == cannot_alloc) { return false; }
            else if (!new_block_index()) {
                return false;
            }
            else {
                local_block_index = block_index_.load(memory_order_relaxed);
                new_tail =
                        (local_block_index->tail_.load(memory_order_relaxed) + 1) & (local_block_index->capacity_ - 1);
                idx_entry = local_block_index->index_[new_tail];
                NEFORCE_DEBUG_VERIFY(idx_entry->key_.load(memory_order_relaxed) == INVALID_BLOCK_BASE,
                                     "implicit_producer::insert_block_index_entry: entry not empty");
                idx_entry->key_.store(block_start_index, memory_order_relaxed);
                local_block_index->tail_.store(new_tail, memory_order_release);
                return true;
            }
        }

        void rewind_block_index_tail() {
            auto local_block_index = block_index_.load(memory_order_relaxed);
            local_block_index->tail_.store((local_block_index->tail_.load(memory_order_relaxed) - 1) &
                                                   (local_block_index->capacity_ - 1),
                                           memory_order_relaxed);
        }

        block_index_entry* get_block_index_entry_for_index(const index_t index) const {
            block_index_header* local_block_index = nullptr;
            auto idx = get_block_index_index_for_index(index, local_block_index);
            return local_block_index->index_[idx];
        }

        size_t get_block_index_index_for_index(const index_t index, block_index_header*& local_block_index) const {
            index_t block_base = index & ~static_cast<index_t>(BLOCK_SIZE - 1);
            local_block_index = block_index_.load(memory_order_acquire);
            auto tail = local_block_index->tail_.load(memory_order_acquire);
            auto tail_base = local_block_index->index_[tail]->key_.load(memory_order_relaxed);
            NEFORCE_DEBUG_VERIFY(tail_base != INVALID_BLOCK_BASE,
                                 "implicit_producer::get_block_index_index_for_index: tail base invalid");
            auto offset = static_cast<size_t>(static_cast<make_signed_t<index_t>>(block_base - tail_base) /
                                              static_cast<make_signed_t<index_t>>(BLOCK_SIZE));
            size_t idx = (tail + offset) & (local_block_index->capacity_ - 1);
            NEFORCE_DEBUG_VERIFY(local_block_index->index_[idx]->key_.load(memory_order_relaxed) == block_base &&
                                         local_block_index->index_[idx]->value_.load(memory_order_relaxed) != nullptr,
                                 "implicit_producer::get_block_index_index_for_index: entry mismatch");
            return idx;
        }

        bool new_block_index() {
            auto prev = block_index_.load(memory_order_relaxed);
            size_t prev_capacity = (prev == nullptr) ? 0 : prev->capacity_;
            size_t entry_count = (prev == nullptr) ? next_block_index_capacity_ : prev_capacity;
            auto* raw = static_cast<char*>(
                    (Traits::malloc)(sizeof(block_index_header) + alignment_of_v<block_index_entry> - 1 +
                                     sizeof(block_index_entry) * entry_count + alignment_of_v<block_index_entry*> - 1 +
                                     sizeof(block_index_entry*) * next_block_index_capacity_));
            if (raw == nullptr) {
                return false;
            }

            auto* header = new (raw) block_index_header;
            auto* entries = reinterpret_cast<block_index_entry*>(
                    inner::align_for<block_index_entry>(raw + sizeof(block_index_header)));
            auto** index = reinterpret_cast<block_index_entry**>(inner::align_for<block_index_entry*>(
                    reinterpret_cast<char*>(entries) + sizeof(block_index_entry) * entry_count));
            if (prev != nullptr) {
                size_t prev_tail = prev->tail_.load(memory_order_relaxed);
                size_t prev_pos = prev_tail;
                size_t i = 0;
                do {
                    prev_pos = (prev_pos + 1) & (prev->capacity_ - 1);
                    index[i++] = prev->index_[prev_pos];
                } while (prev_pos != prev_tail);
                NEFORCE_DEBUG_VERIFY(i == prev_capacity,
                                     "implicit_producer::new_block_index: index copy count mismatch");
            }
            for (size_t i = 0; i != entry_count; ++i) {
                new (entries + i) block_index_entry;
                entries[i].key_.store(INVALID_BLOCK_BASE, memory_order_relaxed);
                index[prev_capacity + i] = entries + i;
            }

            header->prev_ = prev;
            header->entries_ = entries;
            header->index_ = index;
            header->capacity_ = next_block_index_capacity_;
            header->tail_.store((prev_capacity - 1) & (next_block_index_capacity_ - 1), memory_order_relaxed);

            block_index_.store(header, memory_order_release);
            next_block_index_capacity_ <<= 1;
            return true;
        }

    private:
        size_t next_block_index_capacity_ = IMPLICIT_INITIAL_INDEX_SIZE;
        atomic<block_index_header*> block_index_{nullptr};

    public:
        thread_exit_listener thread_exit_listener_;

    private:
        friend struct mem_stats;
    };

    void populate_initial_block_list(const size_t block_count) {
        initial_block_pool_size_ = block_count;
        if (initial_block_pool_size_ == 0) {
            initial_block_pool_ = nullptr;
            return;
        }

        initial_block_pool_ = create_array<block>(block_count);
        if (initial_block_pool_ == nullptr) {
            initial_block_pool_size_ = 0;
        }
        for (size_t i = 0; i < initial_block_pool_size_; ++i) {
            initial_block_pool_[i].dynamically_allocated_ = false;
        }
    }

    block* try_get_block_from_initial_pool() {
        if (initial_block_pool_index_.load(memory_order_relaxed) >= initial_block_pool_size_) {
            return nullptr;
        }
        auto index = initial_block_pool_index_.fetch_add(1, memory_order_relaxed);
        return index < initial_block_pool_size_ ? initial_block_pool_ + index : nullptr;
    }

    void add_block_to_free_list(block* blk) {
        if (!Traits::RECYCLE_ALLOCATED_BLOCKS && blk->dynamically_allocated_) {
            destroy(blk);
        } else {
            free_list_.add(blk);
        }
    }

    void add_blocks_to_free_list(block* blk) {
        while (blk != nullptr) {
            auto next = blk->next_;
            add_block_to_free_list(blk);
            blk = next;
        }
    }

    block* try_get_block_from_free_list() { return free_list_.try_get(); }

    template <allocation_mode can_alloc_mode>
    block* requisition_block() {
        auto* blk = try_get_block_from_initial_pool();
        if (blk != nullptr) {
            return blk;
        }
        blk = try_get_block_from_free_list();
        if (blk != nullptr) {
            return blk;
        }

        NEFORCE_IF_CONSTEXPR(can_alloc_mode == can_alloc) { return create<block>(); }
        else {
            return nullptr;
        }
    }

    /**
     * @struct mem_stats
     * @brief 队列内存统计信息
     *
     * 用于调试与内存分析，仅在无并发访问时调用 get_mem_stats 才有意义。
     */
    struct mem_stats {
        size_t allocated_blocks;
        size_t used_blocks;
        size_t free_blocks;
        size_t owned_blocks_explicit;
        size_t owned_blocks_implicit;
        size_t implicit_producers;
        size_t explicit_producers;
        size_t elements_enqueued;
        size_t block_class_bytes;
        size_t queue_class_bytes;
        size_t implicit_block_index_bytes;
        size_t explicit_block_index_bytes;

        friend class lock_free_queue;

    private:
        static mem_stats get_for(lock_free_queue* q) {
            mem_stats stats{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

            stats.elements_enqueued = q->size_approx();

            auto blk = q->free_list_.head_unsafe();
            while (blk != nullptr) {
                ++stats.allocated_blocks;
                ++stats.free_blocks;
                blk = blk->free_list_next_.load(memory_order_relaxed);
            }

            for (auto ptr = q->producer_list_tail_.load(memory_order_acquire); ptr != nullptr;
                 ptr = ptr->next_producer()) {
                const bool implicit = dynamic_cast<implicit_producer*>(ptr) != nullptr;
                stats.implicit_producers += implicit ? 1 : 0;
                stats.explicit_producers += implicit ? 0 : 1;

                if (implicit) {
                    auto prod = static_cast<implicit_producer*>(ptr);
                    stats.queue_class_bytes += sizeof(implicit_producer);
                    auto head = prod->head_index_.load(memory_order_relaxed);
                    auto tail = prod->tail_index_.load(memory_order_relaxed);
                    auto hash = prod->block_index_.load(memory_order_relaxed);
                    if (hash != nullptr) {
                        for (size_t i = 0; i != hash->capacity_; ++i) {
                            if (hash->index_[i]->key_.load(memory_order_relaxed) !=
                                        implicit_producer::INVALID_BLOCK_BASE &&
                                hash->index_[i]->value_.load(memory_order_relaxed) != nullptr) {
                                ++stats.allocated_blocks;
                                ++stats.owned_blocks_implicit;
                            }
                        }
                        stats.implicit_block_index_bytes +=
                                hash->capacity_ * sizeof(typename implicit_producer::block_index_entry);
                        for (; hash != nullptr; hash = hash->prev_) {
                            stats.implicit_block_index_bytes +=
                                    sizeof(typename implicit_producer::block_index_header) +
                                    hash->capacity_ * sizeof(typename implicit_producer::block_index_entry*);
                        }
                    }
                    for (; inner::circular_less_than<index_t>(head, tail); head += BLOCK_SIZE) {
                        ++stats.used_blocks;
                    }
                } else {
                    auto prod = static_cast<explicit_producer*>(ptr);
                    stats.queue_class_bytes += sizeof(explicit_producer);
                    auto tail_block = prod->tail_block_;
                    bool was_non_empty = false;
                    if (tail_block != nullptr) {
                        auto _blk = tail_block;
                        do {
                            ++stats.allocated_blocks;
                            if (!_blk->template is_empty<explicit_context>() || was_non_empty) {
                                ++stats.used_blocks;
                                was_non_empty = was_non_empty || _blk != tail_block;
                            }
                            ++stats.owned_blocks_explicit;
                            _blk = _blk->next_;
                        } while (_blk != tail_block);
                    }
                    auto index = prod->block_index_.load(memory_order_relaxed);
                    while (index != nullptr) {
                        stats.explicit_block_index_bytes +=
                                sizeof(typename explicit_producer::block_index_header) +
                                index->size_ * sizeof(typename explicit_producer::block_index_entry);
                        index = static_cast<typename explicit_producer::block_index_header*>(index->prev_);
                    }
                }
            }

            auto free_on_initial_pool =
                    q->initial_block_pool_index_.load(memory_order_relaxed) >= q->initial_block_pool_size_
                            ? 0
                            : q->initial_block_pool_size_ - q->initial_block_pool_index_.load(memory_order_relaxed);
            stats.allocated_blocks += free_on_initial_pool;
            stats.free_blocks += free_on_initial_pool;

            stats.block_class_bytes = sizeof(block) * stats.allocated_blocks;
            stats.queue_class_bytes += sizeof(lock_free_queue);

            return stats;
        }
    };

    mem_stats get_mem_stats() { return mem_stats::get_for(this); }

    producer_base* recycle_or_create_producer(const bool is_explicit) {
        for (auto ptr = producer_list_tail_.load(memory_order_acquire); ptr != nullptr; ptr = ptr->next_producer()) {
            if (ptr->inactive_.load(memory_order_relaxed) && ptr->is_explicit_ == is_explicit) {
                bool expected = true;
                if (ptr->inactive_.compare_exchange_strong(expected, /* desired */ false, memory_order_acquire,
                                                           memory_order_relaxed)) {
                    return ptr;
                }
            }
        }

        return add_producer(is_explicit ? static_cast<producer_base*>(create<explicit_producer>(this))
                                        : create<implicit_producer>(this));
    }

    producer_base* add_producer(producer_base* producer) {
        if (producer == nullptr) {
            return nullptr;
        }
        producer_count_.fetch_add(1, memory_order_relaxed);

        auto prev_tail = producer_list_tail_.load(memory_order_relaxed);
        do {
            producer->next_ = prev_tail;
        } while (!producer_list_tail_.compare_exchange_weak(prev_tail, producer, memory_order_release,
                                                            memory_order_relaxed));
        return producer;
    }

    void reown_producers() {
        for (auto ptr = producer_list_tail_.load(memory_order_relaxed); ptr != nullptr; ptr = ptr->next_producer()) {
            ptr->parent_ = this;
        }
    }

    struct implicit_producer_kvp {
        atomic<thread::id> key_{inner::invalid_thread_id_zero};
        implicit_producer* value_{nullptr};

        implicit_producer_kvp() = default;

        implicit_producer_kvp(implicit_producer_kvp&& other) noexcept {
            key_.store(other.key_.load(memory_order_relaxed), memory_order_relaxed);
            value_ = other.value_;
        }

        implicit_producer_kvp& operator=(implicit_producer_kvp&& other) noexcept {
            swap(other);
            return *this;
        }

        void swap(implicit_producer_kvp& other) noexcept {
            if (this != &other) {
                _NEFORCE swap_relaxed(key_, other.key_);
                _NEFORCE swap(value_, other.value_);
            }
        }
    };

    struct implicit_producer_hash {
        size_t capacity_;
        implicit_producer_kvp* entries_;
        implicit_producer_hash* prev_;
    };

    void populate_initial_implicit_producer_hash() {
        NEFORCE_IF_CONSTEXPR(INITIAL_IMPLICIT_PRODUCER_HASH_SIZE == 0) { return; }
        else {
            implicit_producer_hash_count_.store(0, memory_order_relaxed);
            auto hash = &initial_implicit_producer_hash_;
            hash->capacity_ = INITIAL_IMPLICIT_PRODUCER_HASH_SIZE;
            hash->entries_ = initial_implicit_producer_hash_entries_.data();
            for (size_t i = 0; i != INITIAL_IMPLICIT_PRODUCER_HASH_SIZE; ++i) {
                initial_implicit_producer_hash_entries_[i].key_.store(inner::invalid_thread_id_zero,
                                                                      memory_order_relaxed);
            }
            hash->prev_ = nullptr;
            implicit_producer_hash_.store(hash, memory_order_relaxed);
        }
    }

    void swap_implicit_producer_hashes(lock_free_queue& other) {
        NEFORCE_IF_CONSTEXPR(INITIAL_IMPLICIT_PRODUCER_HASH_SIZE == 0) { return; }
        else {
            for (size_t i = 0; i != INITIAL_IMPLICIT_PRODUCER_HASH_SIZE; ++i) {
                initial_implicit_producer_hash_entries_[i].swap(other.initial_implicit_producer_hash_entries_[i]);
            }
            initial_implicit_producer_hash_.entries_ = initial_implicit_producer_hash_entries_.data();
            other.initial_implicit_producer_hash_.entries_ = other.initial_implicit_producer_hash_entries_.data();
            _NEFORCE swap_relaxed(implicit_producer_hash_count_, other.implicit_producer_hash_count_);
            _NEFORCE swap_relaxed(implicit_producer_hash_, other.implicit_producer_hash_);

            if (implicit_producer_hash_.load(memory_order_relaxed) == &other.initial_implicit_producer_hash_) {
                implicit_producer_hash_.store(&initial_implicit_producer_hash_, memory_order_relaxed);
            } else {
                implicit_producer_hash* hash = nullptr;
                for (hash = implicit_producer_hash_.load(memory_order_relaxed);
                     hash->prev_ != &other.initial_implicit_producer_hash_; hash = hash->prev_) {
                    // NOLINTNEXTLINE(readability-redundant-control-flow)
                    continue;
                }
                hash->prev_ = &initial_implicit_producer_hash_;
            }

            if (other.implicit_producer_hash_.load(memory_order_relaxed) == &initial_implicit_producer_hash_) {
                other.implicit_producer_hash_.store(&other.initial_implicit_producer_hash_, memory_order_relaxed);
            } else {
                implicit_producer_hash* hash = nullptr;
                for (hash = other.implicit_producer_hash_.load(memory_order_relaxed);
                     hash->prev_ != &initial_implicit_producer_hash_; hash = hash->prev_) {
                    // NOLINTNEXTLINE(readability-redundant-control-flow)
                    continue;
                }
                hash->prev_ = &other.initial_implicit_producer_hash_;
            }
        }
    }

    implicit_producer* get_or_add_implicit_producer() {
        auto id = this_thread::id();
        const auto hashed_id = id.to_hash();

        auto* main_hash = implicit_producer_hash_.load(memory_order_acquire);
        NEFORCE_DEBUG_VERIFY(main_hash != nullptr, "implicit producer hash is null");
        for (auto* hash = main_hash; hash != nullptr; hash = hash->prev_) {
            auto index = hashed_id;
            while (true) {
                index &= hash->capacity_ - 1;
                auto probed_key = hash->entries_[index].key_.load(memory_order_relaxed);
                if (probed_key == id) {
                    auto* value = hash->entries_[index].value_;
                    if (hash != main_hash) {
                        index = hashed_id;
                        while (true) {
                            index &= main_hash->capacity_ - 1;
                            auto empty = inner::invalid_thread_id_zero;
#ifdef NEFORCE_PLATFORM_WINDOWS
                            auto reusable = inner::invalid_thread_id_max;
                            if (main_hash->entries_[index].key_.compare_exchange_strong(empty, id, memory_order_seq_cst,
                                                                                        memory_order_relaxed) ||
                                main_hash->entries_[index].key_.compare_exchange_strong(
                                        reusable, id, memory_order_seq_cst, memory_order_relaxed)) {
#else
                            if (main_hash->entries_[index].key_.compare_exchange_strong(empty, id, memory_order_seq_cst,
                                                                                        memory_order_relaxed)) {
#endif
                                main_hash->entries_[index].value_ = value;
                                break;
                            }
                            ++index;
                        }
                    }
                    return value;
                }
                if (probed_key == inner::invalid_thread_id_zero) {
                    break;
                }
                ++index;
            }
        }

        auto new_count = 1 + implicit_producer_hash_count_.fetch_add(1, memory_order_relaxed);
        while (true) {
            // NOLINTNEXTLINE(clang-analyzer-core.NullDereference)
            if (new_count >= (main_hash->capacity_ >> 1) &&
                !implicit_producer_hash_resize_in_progress_.test_and_set(memory_order_acquire)) {
                main_hash = implicit_producer_hash_.load(memory_order_acquire);
                if (new_count >= (main_hash->capacity_ >> 1)) {
                    size_t new_capacity = main_hash->capacity_ << 1;
                    while (new_count >= (new_capacity >> 1)) {
                        new_capacity <<= 1;
                    }
                    auto* raw = static_cast<char*>((Traits::malloc)(sizeof(implicit_producer_hash) +
                                                                    alignment_of_v<implicit_producer_kvp> - 1 +
                                                                    sizeof(implicit_producer_kvp) * new_capacity));
                    if (raw == nullptr) {
                        implicit_producer_hash_count_.fetch_sub(1, memory_order_relaxed);
                        implicit_producer_hash_resize_in_progress_.clear(memory_order_relaxed);
                        return nullptr;
                    }

                    auto* new_hash = new (raw) implicit_producer_hash;
                    new_hash->capacity_ = static_cast<size_t>(new_capacity);
                    new_hash->entries_ = reinterpret_cast<implicit_producer_kvp*>(
                            inner::align_for<implicit_producer_kvp>(raw + sizeof(implicit_producer_hash)));
                    for (size_t i = 0; i != new_capacity; ++i) {
                        new (new_hash->entries_ + i) implicit_producer_kvp;
                        new_hash->entries_[i].key_.store(inner::invalid_thread_id_zero, memory_order_relaxed);
                    }
                    new_hash->prev_ = main_hash;
                    implicit_producer_hash_.store(new_hash, memory_order_release);
                    implicit_producer_hash_resize_in_progress_.clear(memory_order_release);
                    main_hash = new_hash;
                } else {
                    implicit_producer_hash_resize_in_progress_.clear(memory_order_release);
                }
            }

            if (new_count < (main_hash->capacity_ >> 1) + (main_hash->capacity_ >> 2)) {
                auto* producer = static_cast<implicit_producer*>(recycle_or_create_producer(false));
                if (producer == nullptr) {
                    implicit_producer_hash_count_.fetch_sub(1, memory_order_relaxed);
                    return nullptr;
                }

                producer->thread_exit_listener_.callback = &lock_free_queue::implicit_producer_thread_exited_callback;
                producer->thread_exit_listener_.user_data = producer;
                thread_exit_notifier::subscribe(&producer->thread_exit_listener_);

                auto index = hashed_id;
                while (true) {
                    index &= main_hash->capacity_ - 1;
                    auto empty = inner::invalid_thread_id_zero;
#ifdef NEFORCE_PLATFORM_WINDOWS
                    auto reusable = inner::invalid_thread_id_max;
                    if (main_hash->entries_[index].key_.compare_exchange_strong(reusable, id, memory_order_seq_cst,
                                                                                memory_order_relaxed)) {
                        implicit_producer_hash_count_.fetch_sub(1, memory_order_relaxed);
                        main_hash->entries_[index].value_ = producer;
                        break;
                    }
#endif
                    if (main_hash->entries_[index].key_.compare_exchange_strong(empty, id, memory_order_seq_cst,
                                                                                memory_order_relaxed)) {
                        main_hash->entries_[index].value_ = producer;
                        break;
                    }
                    ++index;
                }
                return producer;
            }

            main_hash = implicit_producer_hash_.load(memory_order_acquire);
        }
    }

    void implicit_producer_thread_exited(implicit_producer* producer) {
        auto* hash = implicit_producer_hash_.load(memory_order_acquire);
        NEFORCE_DEBUG_VERIFY(hash != nullptr, "implicit producer hash is null");
        const auto id = this_thread::id();
        const auto hashed_id = id.to_hash();
        thread::id probed_key;

        for (; hash != nullptr; hash = hash->prev_) {
            auto index = hashed_id;
            do {
                index &= hash->capacity_ - 1;
                probed_key = id;
#ifdef NEFORCE_PLATFORM_WINDOWS
                if (hash->entries_[index].key_.compare_exchange_strong(probed_key, inner::invalid_thread_id_max,
                                                                       memory_order_seq_cst, memory_order_relaxed)) {
                    break;
                }
#else
                if (hash->entries_[index].key_.compare_exchange_strong(probed_key, inner::invalid_thread_id_zero,
                                                                       memory_order_seq_cst, memory_order_relaxed)) {
                    break;
                }
#endif
                ++index;
            } while (probed_key != inner::invalid_thread_id_zero);
        }

        producer->inactive_.store(true, memory_order_release);
    }

    static void implicit_producer_thread_exited_callback(void* user_data) {
        auto* producer = static_cast<implicit_producer*>(user_data);
        auto queue = producer->parent_;
        queue->implicit_producer_thread_exited(producer);
    }

    template <typename TAlign>
    static void* aligned_malloc(const size_t size) {
        NEFORCE_IF_CONSTEXPR(alignment_of_v<TAlign> <= alignof(max_align_t)) { return (Traits::malloc)(size); }
        else {
            size_t alignment = alignment_of_v<TAlign>;
            void* raw = (Traits::malloc)(size + alignment - 1 + sizeof(void*));
            if (raw == nullptr) {
                return nullptr;
            }
            char* ptr = inner::align_for<TAlign>(static_cast<char*>(raw) + sizeof(void*));
            *(reinterpret_cast<void**>(ptr) - 1) = raw;
            return ptr;
        }
    }

    template <typename TAlign>
    static void aligned_free(void* ptr) {
        NEFORCE_IF_CONSTEXPR(alignment_of_v<TAlign> <= alignof(max_align_t)) { (Traits::free)(ptr); }
        else {
            (Traits::free)(ptr ? *(static_cast<void**>(ptr) - 1) : nullptr);
        }
    }

    template <typename U>
    static U* create_array(const size_t count) {
        NEFORCE_DEBUG_VERIFY(count > 0, "create_array: count must be positive");
        U* p = static_cast<U*>(aligned_malloc<U>(sizeof(U) * count));
        if (p == nullptr) {
            return nullptr;
        }

        for (size_t i = 0; i != count; ++i) {
            new (p + i) U();
        }
        return p;
    }

    template <typename U>
    static void destroy_array(U* p, const size_t count) {
        if (p != nullptr) {
            NEFORCE_DEBUG_VERIFY(count > 0, "destroy_array: count must be positive");
            for (size_t i = count; i != 0;) {
                (p + --i)->~U();
            }
        }
        aligned_free<U>(p);
    }

    template <typename U>
    static U* create() {
        void* p = aligned_malloc<U>(sizeof(U));
        return p != nullptr ? new (p) U : nullptr;
    }

    template <typename U, typename A1>
    static U* create(A1&& a1) {
        void* p = aligned_malloc<U>(sizeof(U));
        return p != nullptr ? new (p) U(_NEFORCE forward<A1>(a1)) : nullptr;
    }

    template <typename U>
    static void destroy(U* p) {
        if (p != nullptr) {
            p->~U();
        }
        aligned_free<U>(p);
    }

private:
    atomic<producer_base*> producer_list_tail_{nullptr};
    atomic<uint32_t> producer_count_{0};

    atomic<size_t> initial_block_pool_index_{0};
    block* initial_block_pool_;
    size_t initial_block_pool_size_;

    free_list<block> free_list_;

    atomic<implicit_producer_hash*> implicit_producer_hash_;
    atomic<size_t> implicit_producer_hash_count_;
    implicit_producer_hash initial_implicit_producer_hash_;
    array<implicit_producer_kvp, INITIAL_IMPLICIT_PRODUCER_HASH_SIZE> initial_implicit_producer_hash_entries_;
    atomic_flag implicit_producer_hash_resize_in_progress_;

    atomic<uint32_t> next_explicit_consumer_id_{0};
    atomic<uint32_t> global_explicit_consumer_offset_{0};
};

template <typename T, typename Traits>
producer_token::producer_token(lock_free_queue<T, Traits>& queue) :
producer_(queue.recycle_or_create_producer(true)) {
    if (producer_ != nullptr) {
        producer_->token_ = this;
    }
}

template <typename T, typename Traits>
consumer_token::consumer_token(lock_free_queue<T, Traits>& queue) {
    initial_offset_ = queue.next_explicit_consumer_id_.fetch_add(1, memory_order_release);
    last_known_global_offset_ = static_cast<uint32_t>(-1);
}

/** @} */ // LockFreeQueue

/** @} */ // AsyncComponents

NEFORCE_END_NAMESPACE__

#undef NEFORCE_IF_CONSTEXPR

#endif // NEFORCE_CORE_ASYNC_LOCK_FREE_QUEUE_HPP__
