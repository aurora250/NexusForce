#ifndef NEFORCE_CORE_ASYNC_LOCK_FREE_QUEUE_HPP__
#define NEFORCE_CORE_ASYNC_LOCK_FREE_QUEUE_HPP__

/**
 * @file lock_free_queue.hpp
 * @brief 无锁队列
 *
 * 此文件提供了无锁队列的实现，支持多线程并发访问，
 * 无需使用互斥锁即可实现线程安全的队列操作。
 */

#include "NeForce/core/async/atomic.hpp"
#include "NeForce/core/memory/unique_ptr.hpp"
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

/**
 * @class lock_free_queue
 * @brief 无锁队列类模板
 * @tparam T 元素类型
 *
 * 基于原子操作和引用计数实现的多生产者多消费者无锁队列。
 * 使用Michael-Scott算法实现，支持并发push和pop操作。
 *
 * @note 采用ABA问题预防机制
 */
template <typename T>
class lock_free_queue {
private:
    /**
     * @struct node_counter
     * @brief 节点计数器结构
     *
     * 使用位域优化存储，包含内部引用计数和外部计数器数量。
     */
    struct node_counter {
        uint32_t internal_count : 30;
        uint32_t external_counters : 2;
    };

    struct node;

    /**
     * @struct counted_node_ptr
     * @brief 带计数的节点指针
     *
     * 包含节点指针和外部引用计数，用于原子操作。
     */
    struct counted_node_ptr {
        int external_count = 0; ///< 外部引用计数
        node* ptr = nullptr;    ///< 节点指针

        /**
         * @brief 默认构造函数
         */
        counted_node_ptr() noexcept = default;
    };

    /**
     * @struct node
     * @brief 队列节点结构
     *
     * 存储数据和维护引用计数，通过原子操作保证线程安全。
     */
    struct node {
        atomic<T*> data;               ///< 数据指针原子变量
        atomic<node_counter> count;    ///< 节点计数器原子变量
        atomic<counted_node_ptr> next; ///< 下一个节点原子变量

        /**
         * @brief 构造函数
         * @param external_count 初始外部计数
         */
        explicit node(int external_count = 2) {
            data.store(nullptr, memory_order_relaxed);

            node_counter new_count;
            new_count.internal_count = 0;
            new_count.external_counters = external_count;
            count.store(new_count);

            counted_node_ptr node_ptr;
            node_ptr.ptr = nullptr;
            node_ptr.external_count = 0;

            next.store(node_ptr);
        }

        /**
         * @brief 释放内部引用
         *
         * 减少内部引用计数，如果引用计数归零且无外部引用，则删除节点。
         */
        void release_ref() {
            node_counter old_counter = count.load(memory_order_relaxed);
            node_counter new_counter;
            do {
                new_counter = old_counter;
                --new_counter.internal_count;
            } while (!count.compare_exchange_strong(old_counter, new_counter, memory_order_acquire,
                                                    memory_order_relaxed));
            if (!new_counter.internal_count && !new_counter.external_counters) {
                delete this;
                destruct_count.fetch_add(1);
            }
        }
    };

private:
    static atomic<int> destruct_count;
    static atomic<int> construct_count;

    atomic<counted_node_ptr> head; ///< 队列头指针原子变量
    atomic<counted_node_ptr> tail; ///< 队列尾指针原子变量
    atomic<size_t> push_count_{0}; ///< 入队计数器
    atomic<size_t> pop_count_{0};  ///< 出队计数器

private:
    /**
     * @brief 设置新的尾节点
     * @param old_tail 原尾节点引用
     * @param new_tail 新尾节点
     *
     * 原子更新尾指针，并适当释放原节点的引用。
     */
    void set_new_tail(counted_node_ptr& old_tail, counted_node_ptr const& new_tail) {
        node* const current_tail_ptr = old_tail.ptr;
        while (!tail.compare_exchange_weak(old_tail, new_tail) && old_tail.ptr == current_tail_ptr) {
            this_thread::relax();
        }
        if (old_tail.ptr == current_tail_ptr) {
            lock_free_queue::free_external_counter(old_tail);
        } else {
            current_tail_ptr->release_ref();
        }
    }

    /**
     * @brief 释放外部计数器
     * @param old_node_ptr 待释放的节点指针
     * @note 静态成员函数
     *
     * 减少外部计数器数量，如果引用完全归零则删除节点。
     */
    static void free_external_counter(counted_node_ptr& old_node_ptr) {
        node* const ptr = old_node_ptr.ptr;
        int const count_increase = old_node_ptr.external_count - 2;
        node_counter old_counter = ptr->count.load(_NEFORCE memory_order_relaxed);
        node_counter new_counter;
        do {
            new_counter = old_counter;
            --new_counter.external_counters;
            new_counter.internal_count += count_increase;
        } while (!ptr->count.compare_exchange_strong(old_counter, new_counter, memory_order_acquire,
                                                     memory_order_relaxed));
        if (!new_counter.internal_count && !new_counter.external_counters) {
            destruct_count.fetch_add(1);
            delete ptr;
        }
    }

    /**
     * @brief 增加外部计数
     * @param counter 计数器原子变量
     * @param old_counter 原计数器值
     * @note 静态成员函数
     *
     * 原子增加外部引用计数，防止ABA问题。
     */
    static void increase_external_count(atomic<counted_node_ptr>& counter, counted_node_ptr& old_counter) {
        counted_node_ptr new_counter;
        do {
            new_counter = old_counter;
            ++new_counter.external_count;
        } while (
                !counter.compare_exchange_strong(old_counter, new_counter, memory_order_acquire, memory_order_relaxed));
        old_counter.external_count = new_counter.external_count;
    }

public:
    /**
     * @brief 默认构造函数
     *
     * 初始化队列，创建哨兵节点作为初始的头尾节点。
     */
    lock_free_queue() {
        counted_node_ptr new_next;
        new_next.ptr = new node();
        new_next.external_count = 1;
        tail.store(new_next);
        head.store(new_next);
    }

    /**
     * @brief 析构函数
     *
     * 清空队列并删除哨兵节点。
     */
    ~lock_free_queue() {
        while (pop()) {
            this_thread::relax();
        }
        auto head_counted_node = head.load();
        delete head_counted_node.ptr;
    }

    /**
     * @brief 入队操作
     * @param new_value 要入队的新值
     *
     * 将新值压入队列尾部，支持多线程并发入队。
     * 使用CAS操作保证线程安全。
     */
    void push(T new_value) {
        unique_ptr<T> new_data(new T(new_value));
        counted_node_ptr new_next;
        new_next.ptr = new node;
        new_next.external_count = 1;
        counted_node_ptr old_tail = tail.load();
        for (;;) {
            lock_free_queue::increase_external_count(tail, old_tail);
            T* old_data = nullptr;
            if (old_tail.ptr->data.compare_exchange_strong(old_data, new_data.get())) {
                counted_node_ptr old_next;
                if (!old_tail.ptr->next.compare_exchange_strong(old_next, new_next)) {
                    delete new_next.ptr;
                    new_next = old_next;
                }
                this->set_new_tail(old_tail, new_next);
                new_data.release();
                break;
            }
            counted_node_ptr old_next;
            if (old_tail.ptr->next.compare_exchange_strong(old_next, new_next)) {
                old_next = new_next;
                new_next.ptr = new node;
            }
            this->set_new_tail(old_tail, old_next);
        }
        ++construct_count;
        push_count_.fetch_add(1, memory_order_relaxed);
    }

    /**
     * @brief 出队操作
     * @return 出队元素的unique_ptr，如果队列为空则返回空指针
     *
     * 从队列头部弹出元素，支持多线程并发出队。
     * 使用CAS操作保证线程安全。
     */
    unique_ptr<T> pop() {
        counted_node_ptr old_head = head.load(memory_order_relaxed);
        for (;;) {
            lock_free_queue::increase_external_count(head, old_head);
            node* const ptr = old_head.ptr;
            if (ptr == tail.load().ptr) {
                ptr->release_ref();
                return unique_ptr<T>();
            }
            counted_node_ptr next = ptr->next.load();
            if (head.compare_exchange_strong(old_head, next)) {
                T* res = ptr->data.exchange(nullptr);
                lock_free_queue::free_external_counter(old_head);
                pop_count_.fetch_add(1, memory_order_relaxed);
                return unique_ptr<T>(res);
            }
            ptr->release_ref();
        }
    }

    /**
     * @brief 尝试出队操作
     * @return 出队元素的unique_ptr，如果队列为空或竞争失败则返回空指针
     *
     * 尝试从队列头部弹出元素，最多重试3次。
     * 如果队列为空或多次CAS失败，则返回空指针。
     *
     * @note 此方法不会自旋等待，适合在高竞争环境下使用
     */
    unique_ptr<T> try_pop() {
        counted_node_ptr old_head{};

        for (int retry = 0; retry < 3; ++retry) {
            old_head = head.load(memory_order_relaxed);
            lock_free_queue::increase_external_count(head, old_head);
            node* ptr = old_head.ptr;

            if (ptr == tail.load().ptr) {
                ptr->release_ref();
                return unique_ptr<T>();
            }

            counted_node_ptr next = ptr->next.load();
            if (head.compare_exchange_strong(old_head, next)) {
                T* res = ptr->data.exchange(nullptr);
                lock_free_queue::free_external_counter(old_head);
                pop_count_.fetch_add(1, memory_order_relaxed);
                return unique_ptr<T>(res);
            }

            ptr->release_ref();
        }

        return unique_ptr<T>();
    }

    /**
     * @brief 检查队列是否为空
     * @return 如果队列为空返回true，否则返回false
     *
     * @note 由于无锁队列的并发特性，此方法返回的结果可能瞬间失效。
     *       仅用于监控和统计，不应用于同步控制。
     */
    NEFORCE_NODISCARD bool empty() const noexcept {
        const counted_node_ptr head_ptr = head.load(memory_order_acquire);
        const counted_node_ptr tail_ptr = tail.load(memory_order_acquire);
        return head_ptr.ptr == tail_ptr.ptr;
    }

    /**
     * @brief 获取队列中元素的近似数量
     * @return 队列中元素的近似数量
     *
     * @note 由于无锁队列的并发特性，返回值可能不精确。
     *       此方法通过维护入队和出队计数器来计算队列大小。
     *       仅用于监控和统计，不应用于同步控制。
     */
    NEFORCE_NODISCARD size_t size() const noexcept {
        const size_t push_cnt = push_count_.load(memory_order_relaxed);
        const size_t pop_cnt = pop_count_.load(memory_order_relaxed);
        return push_cnt - pop_cnt;
    }

    /**
     * @brief 清空队列
     *
     * 循环调用pop()直到队列为空，删除所有元素。
     *
     * @warning 此方法不是线程安全的！
     *          调用clear()时，应确保没有其他线程同时进行push或pop操作。
     *          如果有并发的push操作，clear可能永远无法完成。
     *
     * @note 建议在单线程环境或确保独占访问时调用此方法。
     */
    void clear() {
        while (unique_ptr<T> ptr = this->pop()) {
            this_thread::relax();
        }
    }
};

template <typename T>
atomic<int> lock_free_queue<T>::destruct_count{0};

template <typename T>
atomic<int> lock_free_queue<T>::construct_count{0};

/** @} */ // LockFreeQueue

/** @} */ // AsyncComponents

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_CORE_ASYNC_LOCK_FREE_QUEUE_HPP__
