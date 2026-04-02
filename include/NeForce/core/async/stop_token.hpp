#ifndef NEFORCE_CORE_ASYNC_STOP_TOKEN_HPP__
#define NEFORCE_CORE_ASYNC_STOP_TOKEN_HPP__

/**
 * @file stop_token.hpp
 * @brief 停止令牌实现
 *
 * 此文件提供了停止令牌和停止源的实现，用于跨线程请求和响应停止操作。
 */

#include "NeForce/core/async/atomic.hpp"
#include "NeForce/core/async/semaphore.hpp"
#include "NeForce/core/async/thread.hpp"
#include "NeForce/core/utility/none.hpp"
NEFORCE_BEGIN_NAMESPACE__

/**
 * @defgroup StopTokens 停止令牌
 * @brief 线程间停止请求和响应机制
 * @{
 */

class stop_source;


/**
 * @class stop_token
 * @brief 停止令牌类
 *
 * 停止令牌用于查询是否收到停止请求，但不具备发起停止请求的能力。
 */
class stop_token {
private:
    /**
     * @struct stop_callback_node
     * @brief 停止回调节点
     *
     * 表示一个注册的回调函数，当停止请求发生时被调用。
     */
    struct stop_callback_node {
        using callback_type = void(stop_callback_node*)
#ifdef NEFORCE_STANDARD_17
                noexcept
#endif
                ; ///< 回调函数类型

        callback_type* callback;            ///< 回调函数指针
        stop_callback_node* prev = nullptr; ///< 前驱节点
        stop_callback_node* next = nullptr; ///< 后继节点
        bool* destroyed = nullptr;          ///< 销毁标志指针
        binary_semaphore done_semaphore{0}; ///< 完成信号量

        /**
         * @brief 构造函数
         * @param cb 回调函数指针
         */
        explicit stop_callback_node(callback_type* cb) :
        callback(cb) {}

        /**
         * @brief 执行回调函数
         */
        void run() noexcept { callback(this); }
    };

    /**
     * @struct stop_state
     * @brief 停止状态
     *
     * 管理停止请求的状态和已注册的回调函数。
     */
    struct stop_state {
        using value_type = uint32_t;
        static constexpr value_type stop_requested_bit = 1; ///< 停止请求位
        static constexpr value_type locked_bit = 2;         ///< 锁定位
        static constexpr value_type ssrc_counter_inc = 4;   ///< stop_source计数器增量

        atomic<value_type> owners{1};               ///< 所有者计数（stop_token数量）
        atomic<value_type> value{ssrc_counter_inc}; ///< 状态值
        stop_callback_node* head = nullptr;         ///< 回调链表头节点
        thread::id requester_thread_id;             ///< 请求停止的线程ID

        stop_state() = default; ///< 默认构造函数

        /**
         * @brief 检查是否可能收到停止请求
         * @return 是否可能收到停止请求
         */
        bool stop_possible() noexcept { return value.load(memory_order_acquire) & ~locked_bit; }

        /**
         * @brief 检查是否已收到停止请求
         * @return 是否已收到停止请求
         */
        bool stop_requested() noexcept { return value.load(memory_order_acquire) & stop_requested_bit; }

        /**
         * @brief 增加所有者计数
         */
        void add_owner() noexcept { owners.fetch_add(1, memory_order_relaxed); }

        /**
         * @brief 减少所有者计数
         * @note 当所有者计数为0时删除状态对象
         */
        void release_ownership() noexcept {
            if (owners.fetch_sub(1, memory_order_acq_rel) == 1) {
                delete this;
            }
        }

        /**
         * @brief 增加stop_source计数
         */
        void add_stop_source() noexcept { value.fetch_add(ssrc_counter_inc, memory_order_relaxed); }

        /**
         * @brief 减少stop_source计数
         */
        void remove_stop_source() noexcept { value.fetch_sub(ssrc_counter_inc, memory_order_release); }

        /**
         * @brief 锁定状态
         */
        void lock() noexcept {
            auto old_value = value.load(memory_order_relaxed);
            while (!try_lock(old_value, memory_order_relaxed)) {
            }
        }

        /**
         * @brief 解锁状态
         */
        void unlock() noexcept { value.fetch_sub(locked_bit, memory_order_release); }

        /**
         * @brief 请求停止
         * @return 是否成功请求停止（首次请求返回true）
         *
         * 设置停止请求标志，并执行所有已注册的回调函数。
         */
        bool request_stop() noexcept {
            auto old_value = value.load(memory_order_acquire);
            do {
                if (old_value & stop_requested_bit) {
                    return false;
                }
            } while (!try_lock_and_stop(old_value));

            requester_thread_id = this_thread::id();

            while (head) {
                bool last_callback;
                stop_callback_node* callback_node = head;
                head = head->next;
                if (head) {
                    head->prev = nullptr;
                    last_callback = false;
                } else {
                    last_callback = true;
                }
                unlock();

                bool destroyed = false;
                callback_node->destroyed = &destroyed;
                callback_node->run();
                if (!destroyed) {
                    callback_node->destroyed = nullptr;
                    callback_node->done_semaphore.release();
                }

                if (last_callback) {
                    return true;
                }
                lock();
            }
            unlock();
            return true;
        }

        /**
         * @brief 注册回调函数
         * @param callback_node 回调节点
         * @return 是否注册成功
         *
         * 如果已经请求停止，立即执行回调并返回false。
         */
        bool register_callback(stop_callback_node* callback_node) noexcept {
            auto old_value = value.load(memory_order_acquire);
            do {
                if (old_value & stop_requested_bit) {
                    callback_node->run();
                    return false;
                }
                if (old_value < ssrc_counter_inc) {
                    return false;
                }
            } while (!try_lock(old_value));

            callback_node->next = head;
            if (head) {
                head->prev = callback_node;
            }
            head = callback_node;
            unlock();
            return true;
        }

        /**
         * @brief 移除回调函数
         * @param callback_node 回调节点
         *
         * 从链表中移除回调节点。如果正在执行回调，则等待完成。
         */
        void remove_callback(stop_callback_node* callback_node) {
            lock();

            if (callback_node == head) {
                head = head->next;
                if (head) {
                    head->prev = nullptr;
                }
                unlock();
                return;
            }
            if (callback_node->prev) {
                callback_node->prev->next = callback_node->next;
                if (callback_node->next) {
                    callback_node->next->prev = callback_node->prev;
                }
                unlock();
                return;
            }

            unlock();

            if (requester_thread_id != this_thread::id()) {
                callback_node->done_semaphore.acquire();
                return;
            }

            if (callback_node->destroyed) {
                *callback_node->destroyed = true;
            }
        }

        /**
         * @brief 尝试锁定状态
         * @param current_value 当前状态值
         * @param failure_order 失败时的内存顺序
         * @return 是否锁定成功
         */
        bool try_lock(value_type& current_value, const memory_order failure_order = memory_order_acquire) noexcept {
            return do_try_lock(current_value, 0, memory_order_acquire, failure_order);
        }

        /**
         * @brief 尝试锁定状态并设置停止请求标志
         * @param current_value 当前状态值
         * @return 是否操作成功
         */
        bool try_lock_and_stop(value_type& current_value) noexcept {
            return do_try_lock(current_value, stop_requested_bit, memory_order_acq_rel, memory_order_acquire);
        }

        /**
         * @brief 尝试锁定状态的核心实现
         */
        bool do_try_lock(value_type& current_value, value_type new_bits, const memory_order success_order,
                         const memory_order failure_order) noexcept {
            if (current_value & locked_bit) {
                this_thread::relax();
                current_value = value.load(failure_order);
                return false;
            }
            new_bits |= locked_bit;
            return value.compare_exchange_weak(current_value, current_value | new_bits, success_order, failure_order);
        }
    };

    /**
     * @struct stop_state_reference
     * @brief 停止状态的引用计数包装
     *
     * 管理stop_state的生命周期，实现引用计数。
     */
    struct stop_state_reference {
    private:
        stop_state* ptr_ = nullptr; ///< 指向停止状态的指针

    public:
        stop_state_reference() = default; ///< 默认构造函数

        /**
         * @brief 构造函数
         * @param ref stop_source引用
         */
        explicit stop_state_reference(const stop_source& ref) :
        ptr_(new stop_state()) {}

        /**
         * @brief 拷贝构造函数
         * @param other 要拷贝的引用
         */
        stop_state_reference(const stop_state_reference& other) noexcept :
        ptr_(other.ptr_) {
            if (ptr_) {
                ptr_->add_owner();
            }
        }

        /**
         * @brief 移动构造函数
         * @param other 要移动的引用
         */
        stop_state_reference(stop_state_reference&& other) noexcept :
        ptr_(other.ptr_) {
            other.ptr_ = nullptr;
        }

        /**
         * @brief 拷贝赋值运算符
         */
        stop_state_reference& operator=(const stop_state_reference& other) noexcept {
            const auto new_ptr = other.ptr_;
            if (new_ptr != ptr_) {
                if (new_ptr) {
                    new_ptr->add_owner();
                }
                if (ptr_) {
                    ptr_->release_ownership();
                }
                ptr_ = new_ptr;
            }
            return *this;
        }

        /**
         * @brief 移动赋值运算符
         */
        stop_state_reference& operator=(stop_state_reference&& other) noexcept {
            stop_state_reference(move(other)).swap(*this);
            return *this;
        }

        /**
         * @brief 析构函数
         */
        ~stop_state_reference() {
            if (ptr_) {
                ptr_->release_ownership();
            }
        }

        /**
         * @brief 交换两个引用
         */
        void swap(stop_state_reference& other) noexcept { _NEFORCE swap(ptr_, other.ptr_); }

        /**
         * @brief 布尔转换运算符
         */
        explicit operator bool() const noexcept { return ptr_ != nullptr; }

        /**
         * @brief 箭头运算符
         */
        stop_state* operator->() const noexcept { return ptr_; }

        /**
         * @brief 相等比较运算符
         */
        bool operator==(const stop_state_reference& rhs) const noexcept { return ptr_ == rhs.ptr_; }

        /**
         * @brief 不等比较运算符
         */
        bool operator!=(const stop_state_reference& rhs) const noexcept { return ptr_ != rhs.ptr_; }
    };

private:
    stop_state_reference state_ref_; ///< 停止状态引用

    friend class stop_source;

    template <typename Callback> friend class stop_callback;

    /**
     * @brief 私有构造函数
     * @param state_ref 停止状态引用
     */
    explicit stop_token(stop_state_reference state_ref) noexcept :
    state_ref_{move(state_ref)} {}

public:
    stop_token() noexcept = default;                             ///< 默认构造函数
    ~stop_token() = default;                                     ///< 析构函数
    stop_token(const stop_token&) noexcept = default;            ///< 拷贝构造函数
    stop_token& operator=(const stop_token&) noexcept = default; ///< 拷贝赋值运算符
    stop_token(stop_token&&) noexcept = default;                 ///< 移动构造函数
    stop_token& operator=(stop_token&&) noexcept = default;      ///< 移动赋值运算符

    /**
     * @brief 检查是否可能收到停止请求
     * @return 是否可能收到停止请求
     */
    NEFORCE_NODISCARD bool stop_possible() const noexcept {
        return static_cast<bool>(state_ref_) && state_ref_->stop_possible();
    }

    /**
     * @brief 检查是否已收到停止请求
     * @return 是否已收到停止请求
     */
    NEFORCE_NODISCARD bool stop_requested() const noexcept {
        return static_cast<bool>(state_ref_) && state_ref_->stop_requested();
    }

    /**
     * @brief 交换两个停止令牌
     * @param other 要交换的停止令牌
     */
    void swap(stop_token& other) noexcept { state_ref_.swap(other.state_ref_); }

    /**
     * @brief 相等比较运算符
     * @param rhs 要比较的停止令牌
     * @return 是否相等
     *
     * 检查两个停止令牌是否共享同一个停止状态。
     */
    NEFORCE_NODISCARD bool operator==(const stop_token& rhs) const { return state_ref_ == rhs.state_ref_; }
};


/**
 * @class stop_source
 * @brief 停止源类
 *
 * 停止源用于发起停止请求，可以创建stop_token供其他线程查询。
 * 一个stop_source对应一个停止状态，多个stop_token可以共享这个状态。
 */
class stop_source {
private:
    stop_token::stop_state_reference state_ref_;

public:
    /**
     * @brief 默认构造函数
     *
     * 创建具有停止能力的stop_source。
     */
    stop_source() :
    state_ref_(*this) {}

    /**
     * @brief 构造函数
     * @param none 空标记
     *
     * 创建不具有停止能力的stop_source。
     */
    explicit stop_source(none_t none) noexcept {}

    /**
     * @brief 拷贝构造函数
     * @param other 要拷贝的stop_source
     */
    stop_source(const stop_source& other) noexcept :
    state_ref_(other.state_ref_) {
        if (state_ref_) {
            state_ref_->add_stop_source();
        }
    }

    stop_source(stop_source&&) noexcept = default; ///< 移动构造函数

    /**
     * @brief 拷贝赋值运算符
     */
    stop_source& operator=(const stop_source& other) noexcept {
        if (state_ref_ != other.state_ref_) {
            stop_source sink(move(*this));
            state_ref_ = other.state_ref_;
            if (state_ref_) {
                state_ref_->add_stop_source();
            }
        }
        return *this;
    }

    stop_source& operator=(stop_source&&) noexcept = default; ///< 移动赋值运算符

    /**
     * @brief 析构函数
     *
     * 减少stop_source计数，当计数为0时清除停止能力。
     */
    ~stop_source() {
        if (state_ref_) {
            state_ref_->remove_stop_source();
        }
    }

    /**
     * @brief 检查是否具有停止能力
     * @return 是否具有停止能力
     */
    NEFORCE_NODISCARD bool stop_possible() const noexcept { return static_cast<bool>(state_ref_); }

    /**
     * @brief 检查是否已请求停止
     * @return 是否已请求停止
     */
    NEFORCE_NODISCARD bool stop_requested() noexcept { return stop_possible() && state_ref_->stop_requested(); }

    /**
     * @brief 请求停止
     * @return 是否成功请求停止（首次请求返回true）
     *
     * 设置停止标志并执行所有注册的回调函数。
     */
    NEFORCE_NODISCARD bool request_stop() noexcept {
        if (stop_possible()) {
            return state_ref_->request_stop();
        }
        return false;
    }

    /**
     * @brief 获取停止令牌
     * @return 关联的stop_token
     */
    NEFORCE_NODISCARD stop_token get_token() const noexcept { return stop_token{state_ref_}; }

    /**
     * @brief 交换两个停止源
     * @param other 要交换的停止源
     */
    void swap(stop_source& other) noexcept { state_ref_.swap(other.state_ref_); }

    /**
     * @brief 相等比较运算符
     */
    NEFORCE_NODISCARD bool operator==(const stop_source& rhs) const noexcept { return state_ref_ == rhs.state_ref_; }

    /**
     * @brief 不等比较运算符
     */
    NEFORCE_NODISCARD bool operator!=(const stop_source& rhs) const noexcept { return state_ref_ != rhs.state_ref_; }
};


/**
 * @class stop_callback
 * @brief 停止回调类模板
 * @tparam Callback 回调函数类型
 *
 * 在停止令牌上注册回调函数，当停止请求发生时自动执行。
 * 回调函数在析构时自动注销。
 *
 * @note 回调函数应该不抛异常
 * @note 回调函数应该快速执行，避免阻塞
 */
template <typename Callback> class NEFORCE_NODISCARD stop_callback {
    static_assert(is_nothrow_destructible_v<Callback>, "Callback should be nothrow destructible.");
    static_assert(is_invocable_v<Callback>, "Callback should be invocable.");

public:
    using callback_type = Callback; ///< 回调函数类型

private:
    struct callback_impl : stop_token::stop_callback_node {
        template <typename Cb>
        explicit callback_impl(Cb&& callback) :
        stop_callback_node(&execute_callback),
        callback(forward<Cb>(callback)) {}

        Callback callback;

        static void execute_callback(stop_callback_node* node) noexcept {
            Callback& cb = static_cast<callback_impl*>(node)->callback;
            forward<Callback>(cb)();
        }
    };

    callback_impl callback_impl;
    stop_token::stop_state_reference state_ref_;

public:
    /**
     * @brief 左值构造函数
     * @tparam Cb 回调函数类型
     * @param token 停止令牌
     * @param callback 回调函数
     */
    template <typename Cb, enable_if_t<is_constructible_v<Callback, Cb>, int> = 0>
    explicit stop_callback(const stop_token& token, Cb&& callback) noexcept(is_nothrow_constructible_v<Callback, Cb>) :
    callback_impl(forward<Cb>(callback)) {
        if (auto state_ref = token.state_ref_) {
            if (state_ref->register_callback(&callback_impl)) {
                state_ref_.swap(state_ref);
            }
        }
    }

    /**
     * @brief 右值构造函数
     * @tparam Cb 回调函数类型
     * @param token 停止令牌
     * @param callback 回调函数
     */
    template <typename Cb, enable_if_t<is_constructible_v<Callback, Cb>, int> = 0>
    explicit stop_callback(stop_token&& token, Cb&& callback) noexcept(is_nothrow_constructible_v<Callback, Cb>) :
    callback_impl(forward<Cb>(callback)) {
        if (auto& state_ref = token.state_ref_) {
            if (state_ref->register_callback(&callback_impl)) {
                state_ref_.swap(state_ref);
            }
        }
    }

    stop_callback(const stop_callback&) = delete;            ///< 禁止拷贝构造
    stop_callback& operator=(const stop_callback&) = delete; ///< 禁止拷贝赋值
    stop_callback(stop_callback&&) = delete;                 ///< 禁止移动构造
    stop_callback& operator=(stop_callback&&) = delete;      ///< 禁止移动赋值

    /**
     * @brief 析构函数
     *
     * 自动从停止令牌注销回调。
     */
    ~stop_callback() {
        if (state_ref_) {
            state_ref_->remove_callback(&callback_impl);
        }
    }
};

#ifdef NEFORCE_STANDARD_17
template <typename Callback> stop_callback(stop_token, Callback) -> stop_callback<Callback>;
#endif

/** @} */ // StopTokens

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_CORE_ASYNC_STOP_TOKEN_HPP__
