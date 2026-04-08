#ifndef NEFORCE_CORE_ASYNC_GENERATOR_HPP__
#define NEFORCE_CORE_ASYNC_GENERATOR_HPP__
#include "NeForce/core/async/coroutine.hpp"
#if defined(NEFORCE_STANDARD_20) || defined(NEXUSFORCE_ENABLE_DOXYGEN)
#    include "NeForce/core/async/atomic.hpp"
#    include "NeForce/core/exception/exception_ptr.hpp"
#    include "NeForce/core/utility/optional.hpp"
NEFORCE_BEGIN_NAMESPACE__

/**
 * @defgroup Coroutine 协程
 * @brief 协程支持
 * @{
 */

/**
 * @class cancellation_token
 * @brief 取消令牌
 *
 * 用于协程的取消操作，支持多个副本共享同一个取消状态。
 * 可以传递给协程，在适当的位置检查取消状态并抛出异常。
 */
class cancellation_token {
public:
    /**
     * @struct check_awaiter
     * @brief 取消检查等待器
     *
     * 可在协程中等待此对象，如果已取消则抛出异常。
     */
    struct check_awaiter {
        const cancellation_token* token; ///< 令牌指针

        /**
         * @brief 检查是否可立即恢复
         * @return 如果已取消则返回true，表示可以立即恢复（并抛出异常）
         */
        bool await_ready() const noexcept { return token && token->is_cancelled(); }

        /**
         * @brief 暂停协程（实际上从不暂停）
         * @param h 协程句柄
         */
        void await_suspend(coroutine_handle<> h) const noexcept {}

        /**
         * @brief 恢复时执行的操作
         * @throws exception 如果已取消则抛出异常
         */
        void await_resume() const {
            if (token && token->is_cancelled()) {
                NEFORCE_THROW_EXCEPTION(exception("Operation cancelled"));
            }
        }
    };

private:
    struct state {
        atomic<bool> cancelled{false}; ///< 取消标志
        atomic<size_t> ref_count{1};   ///< 引用计数
    };
    state* state_; ///< 共享状态

    /**
     * @brief 释放状态引用
     */
    void release() {
        if (state_ && state_->ref_count.fetch_sub(1, memory_order_acq_rel) == 1) {
            delete state_;
        }
    }

public:
    /**
     * @brief 默认构造函数，创建新的取消令牌
     */
    cancellation_token() :
    state_(new state()) {}

    /**
     * @brief 拷贝构造函数，共享状态
     * @param other 另一个取消令牌
     */
    cancellation_token(const cancellation_token& other) :
    state_(other.state_) {
        if (state_) {
            state_->ref_count.fetch_add(1, memory_order_relaxed);
        }
    }

    /**
     * @brief 拷贝赋值运算符
     * @param other 另一个取消令牌
     * @return 自身引用
     */
    cancellation_token& operator=(const cancellation_token& other) {
        if (addressof(other) == this) {
            return *this;
        }

        release();
        state_ = other.state_;
        if (state_) {
            state_->ref_count.fetch_add(1, memory_order_relaxed);
        }
        return *this;
    }

    /**
     * @brief 析构函数，释放状态
     */
    ~cancellation_token() { release(); }

    /**
     * @brief 请求取消
     */
    void cancel() noexcept {
        if (state_) {
            state_->cancelled.store(true, memory_order_release);
        }
    }

    /**
     * @brief 检查是否已被取消
     * @return 是否已被取消
     */
    bool is_cancelled() const noexcept { return state_ && state_->cancelled.load(memory_order_acquire); }

    /**
     * @brief 获取取消检查等待器
     * @return 检查等待器
     */
    check_awaiter check() const { return check_awaiter{this}; }
};


/**
 * @class generator
 * @brief 懒序列生成器
 * @tparam T 元素类型
 *
 * 使用协程实现的懒序列生成器，支持范围for循环和组合操作。
 * 每次co_yield产生一个值，协程暂停直到下一次迭代。
 */
template <typename T>
class generator {
public:
    /**
     * @struct promise_type
     * @brief 生成器的promise类型
     */
    struct promise_type {
        optional<T> current_value; ///< 当前产生的值
        exception_ptr exception;   ///< 异常指针

        /**
         * @brief 获取生成器对象
         * @return 生成器对象
         */
        generator get_return_object() { return generator{coroutine_handle<promise_type>::from_promise(*this)}; }

        /**
         * @brief 初始暂停点
         * @return 总是暂停
         */
        suspend_always initial_suspend() noexcept { return {}; }

        /**
         * @brief 最终暂停点
         * @return 总是暂停
         */
        suspend_always final_suspend() noexcept { return {}; }

        /**
         * @brief 产生一个值
         * @param value 要产生的值
         * @return 总是暂停
         */
        suspend_always yield_value(T value) {
            current_value = _NEFORCEmove(value);
            return {};
        }

        /**
         * @brief 返回void（生成器不返回值）
         */
        void return_void() noexcept {}

        /**
         * @brief 处理未捕获的异常
         */
        void unhandled_exception() { exception = _NEFORCE current_exception(); }
    };

    /**
     * @class iterator
     * @brief 生成器的输入迭代器
     */
    struct iterator {
        using iterator_category = input_iterator_tag;
        using value_type = T;
        using difference_type = ptrdiff_t;
        using pointer = T*;
        using reference = T&;

        coroutine_handle<promise_type> handle; ///< 协程句柄

        /**
         * @brief 默认构造函数
         */
        iterator() = default;

        /**
         * @brief 从协程句柄构造
         * @param h 协程句柄
         */
        explicit iterator(coroutine_handle<promise_type> h) :
        handle(h) {}

        /**
         * @brief 前置递增，恢复协程获取下一个值
         * @return 自身引用
         */
        iterator& operator++() {
            handle.resume();
            if (handle.done()) {
                handle = nullptr;
            }
            return *this;
        }

        /**
         * @brief 后置递增
         */
        void operator++(int) { ++(*this); }

        /**
         * @brief 解引用，获取当前值
         * @return 当前值的引用
         */
        T& operator*() const noexcept {
            NEFORCE_CONSTEXPR_ASSERT(handle.promise().current_value.has_value());
            return *handle.promise().current_value;
        }

        /**
         * @brief 箭头操作符
         * @return 指向当前值的指针
         */
        T* operator->() const noexcept { return &(*handle.promise().current_value); }

        /**
         * @brief 相等比较
         * @param other 另一个迭代器
         * @return 是否相等
         */
        bool operator==(const iterator& other) const noexcept { return handle == other.handle; }

        /**
         * @brief 不等比较
         * @param other 另一个迭代器
         * @return 是否不等
         */
        bool operator!=(const iterator& other) const noexcept { return !(*this == other); }
    };

private:
    coroutine_handle<promise_type> handle_; ///< 协程句柄

public:
    /**
     * @brief 从协程句柄构造
     * @param h 协程句柄
     */
    explicit generator(coroutine_handle<promise_type> h) :
    handle_(h) {}

    /**
     * @brief 移动构造函数
     * @param other 被移动的对象
     */
    generator(generator&& other) noexcept :
    handle_(other.handle_) {
        other.handle_ = nullptr;
    }

    /**
     * @brief 移动赋值运算符
     * @param other 被移动的对象
     * @return 自身引用
     */
    generator& operator=(generator&& other) noexcept {
        if (addressof(other) == this) {
            return *this;
        }

        if (handle_) {
            handle_.destroy();
        }
        handle_ = other.handle_;
        other.handle_ = nullptr;

        return *this;
    }

    /**
     * @brief 析构函数，销毁协程
     */
    ~generator() {
        if (handle_) {
            handle_.destroy();
        }
    }

    generator(const generator&) = delete;
    generator& operator=(const generator&) = delete;

    /**
     * @brief 获取起始迭代器
     * @return 起始迭代器
     */
    iterator begin() {
        if (handle_) {
            handle_.resume();
            if (handle_.promise().exception) {
                _NEFORCE rethrow_exception(handle_.promise().exception);
            }
            if (handle_.done()) {
                return iterator{nullptr};
            }
        }
        return iterator{handle_};
    }

    /**
     * @brief 获取结束迭代器
     * @return 结束迭代器
     */
    iterator end() { return iterator{nullptr}; }

    /**
     * @brief 映射变换
     * @tparam F 函数类型
     * @param func 变换函数
     * @return 变换后的生成器
     */
    template <typename F>
    invoke_result_t<F, T> map(F&& func) {
        for (auto&& value: *this) {
            co_yield func(_NEFORCE forward<decltype(value)>(value));
        }
    }

    /**
     * @brief 过滤
     * @tparam Pred 谓词类型
     * @param pred 谓词
     * @return 过滤后的生成器
     */
    template <typename Pred>
    generator filter(Pred&& pred) {
        for (auto&& value: *this) {
            if (pred(value)) {
                co_yield _NEFORCE forward<decltype(value)>(value);
            }
        }
    }

    /**
     * @brief 取前n个元素
     * @param n 元素个数
     * @return 新的生成器
     */
    generator take(const size_t n) {
        size_t count = 0;
        for (auto&& value: *this) {
            if (count >= n) {
                break;
            }
            co_yield _NEFORCE forward<decltype(value)>(value);
            ++count;
        }
    }

    /**
     * @brief 跳过前n个元素
     * @param n 要跳过的个数
     * @return 新的生成器
     */
    generator skip(const size_t n) {
        size_t count = 0;
        for (auto&& value: *this) {
            if (count < n) {
                ++count;
                continue;
            }
            co_yield _NEFORCE forward<decltype(value)>(value);
        }
    }

    /**
     * @brief 连接两个生成器
     * @param other 另一个生成器
     * @return 连接后的生成器
     */
    generator chain(generator&& other) {
        for (auto&& value: *this) {
            co_yield _NEFORCE forward<decltype(value)>(value);
        }
        for (auto&& value: other) {
            co_yield _NEFORCE forward<decltype(value)>(value);
        }
    }

    /**
     * @brief 遍历每个元素
     * @tparam F 函数类型
     * @param func 要对每个元素执行的函数
     */
    template <typename F>
    void for_each(F&& func) {
        for (auto&& value: *this) {
            func(_NEFORCE forward<decltype(value)>(value));
        }
    }

    /**
     * @brief 折叠（归约）
     * @tparam Acc 累加器类型
     * @tparam F 函数类型
     * @param init 初始值
     * @param func 折叠函数
     * @return 折叠结果
     */
    template <typename Acc, typename F>
    Acc fold(Acc init, F&& func) {
        Acc result = _NEFORCE move(init);
        for (auto&& value: *this) {
            result = func(_NEFORCE move(result), _NEFORCE forward<decltype(value)>(value));
        }
        return result;
    }
};


/**
 * @class task
 * @brief 异步任务
 * @tparam T 任务结果类型
 *
 * 表示一个可能产生结果的异步操作。
 * 支持co_await等待、取消、组合等操作。
 */
template <typename T>
class task {
public:
    /**
     * @struct promise_type
     * @brief 任务的promise类型
     */
    struct promise_type {
        /**
         * @struct final_awaiter
         * @brief 最终等待器
         */
        struct final_awaiter {
            /**
             * @brief 检查是否可立即恢复
             * @return 总是返回false
             */
            bool await_ready() noexcept { return false; }

            /**
             * @brief 暂停时执行的操作
             * @param h 协程句柄
             * @return 要继续的协程
             */
            coroutine_handle<> await_suspend(coroutine_handle<promise_type> h) noexcept {
                auto& promise = h.promise();
                if (promise.continuation) {
                    return promise.continuation;
                }
                return noop_coroutine();
            }

            /**
             * @brief 恢复时执行的操作
             */
            void await_resume() noexcept {}
        };

        optional<T> result;                  ///< 结果值
        exception_ptr exception;             ///< 异常指针
        coroutine_handle<> continuation;     ///< 继续执行的协程
        cancellation_token* token = nullptr; ///< 取消令牌

        /**
         * @brief 获取任务对象
         * @return 任务对象
         */
        task get_return_object() { return task{coroutine_handle<promise_type>::from_promise(*this)}; }

        /**
         * @brief 设置取消令牌
         * @param t 取消令牌指针
         */
        void set_cancellation_token(cancellation_token* t) noexcept { token = t; }

        /**
         * @brief 检查是否已取消
         * @return 是否已取消
         */
        bool is_cancelled() const noexcept { return token && token->is_cancelled(); }

        /**
         * @brief 初始暂停点
         * @return 总是暂停
         */
        suspend_always initial_suspend() noexcept { return {}; }

        /**
         * @brief 最终暂停点
         * @return 最终等待器
         */
        final_awaiter final_suspend() noexcept { return {}; }

        /**
         * @brief 设置返回值
         * @param value 返回值
         */
        void return_value(T value) { result = _NEFORCE move(value); }

        /**
         * @brief 处理未捕获的异常
         */
        void unhandled_exception() noexcept { exception = current_exception(); }
    };

    /**
     * @struct awaiter
     * @brief 任务等待器
     */
    struct awaiter {
        coroutine_handle<promise_type> handle; ///< 协程句柄

        /**
         * @brief 检查是否可立即恢复
         * @return 如果任务已完成则返回true
         */
        bool await_ready() const noexcept { return handle.done(); }

        /**
         * @brief 暂停时执行的操作
         * @param continuation 继续执行的协程
         * @return 要恢复的协程
         */
        coroutine_handle<> await_suspend(coroutine_handle<> continuation) noexcept {
            handle.promise().continuation = continuation;
            return handle;
        }

        /**
         * @brief 恢复时获取结果
         * @return 任务结果
         */
        T await_resume() {
            if (handle.promise().exception) {
                _NEFORCE rethrow_exception(handle.promise().exception);
            }
            return _NEFORCE move(*handle.promise().result);
        }
    };

private:
    coroutine_handle<promise_type> handle_; ///< 协程句柄

public:
    /**
     * @brief 从协程句柄构造
     * @param h 协程句柄
     */
    explicit task(coroutine_handle<promise_type> h) :
    handle_(h) {}

    /**
     * @brief 移动构造函数
     * @param other 被移动的对象
     */
    task(task&& other) noexcept :
    handle_(other.handle_) {
        other.handle_ = nullptr;
    }

    /**
     * @brief 移动赋值运算符
     * @param other 被移动的对象
     * @return 自身引用
     */
    task& operator=(task&& other) noexcept {
        if (addressof(other) == this) {
            return *this;
        }

        if (handle_) {
            handle_.destroy();
        }
        handle_ = other.handle_;
        other.handle_ = nullptr;

        return *this;
    }

    /**
     * @brief 析构函数，销毁协程
     */
    ~task() {
        if (handle_) {
            handle_.destroy();
        }
    }

    task(const task&) = delete;
    task& operator=(const task&) = delete;

    /**
     * @brief 获取等待器
     * @return 等待器对象
     */
    awaiter operator co_await() { return awaiter{handle_}; }

    /**
     * @brief 检查任务是否已完成
     * @return 是否已完成
     */
    bool done() const noexcept { return handle_.done(); }

    /**
     * @brief 恢复任务执行
     */
    void resume() {
        if (handle_ && !handle_.done()) {
            handle_.resume();
        }
    }

    /**
     * @brief 同步获取任务结果
     * @return 任务结果
     */
    T get() {
        while (!handle_.done()) {
            handle_.resume();
        }
        if (handle_.promise().exception) {
            _NEFORCE rethrow_exception(handle_.promise().exception);
        }
        return _NEFORCE move(*handle_.promise().result);
    }

    /**
     * @brief 设置取消令牌
     * @param token 取消令牌
     */
    void set_cancellation_token(cancellation_token& token) {
        if (handle_) {
            handle_.promise().set_cancellation_token(&token);
        }
    }

    /**
     * @brief 检查任务是否被取消
     * @return 是否被取消
     */
    bool is_cancelled() const noexcept { return handle_ && handle_.promise().is_cancelled(); }
};


/**
 * @brief void特化的任务类
 */
template <>
class task<void> {
public:
    struct promise_type {
        exception_ptr exception;
        coroutine_handle<> continuation;
        cancellation_token* token = nullptr;

        task get_return_object() { return task{coroutine_handle<promise_type>::from_promise(*this)}; }

        void set_cancellation_token(cancellation_token* t) { token = t; }

        bool is_cancelled() const { return token && token->is_cancelled(); }

        suspend_always initial_suspend() noexcept { return {}; }

        struct final_awaiter {
            bool await_ready() noexcept { return false; }

            coroutine_handle<> await_suspend(coroutine_handle<promise_type> h) noexcept {
                auto& promise = h.promise();
                if (promise.continuation) {
                    return promise.continuation;
                }
                return noop_coroutine();
            }

            void await_resume() noexcept {}
        };

        final_awaiter final_suspend() noexcept { return {}; }

        void return_void() noexcept {}

        void unhandled_exception() { exception = current_exception(); }
    };

    struct awaiter {
        coroutine_handle<promise_type> handle;

        bool await_ready() const noexcept { return handle.done(); }

        coroutine_handle<> await_suspend(coroutine_handle<> continuation) noexcept {
            handle.promise().continuation = continuation;
            return handle;
        }

        void await_resume() {
            if (handle.promise().exception) {
                rethrow_exception(handle.promise().exception);
            }
        }
    };

private:
    coroutine_handle<promise_type> handle_;

public:
    explicit task(coroutine_handle<promise_type> h) :
    handle_(h) {}

    task(task&& other) noexcept :
    handle_(other.handle_) {
        other.handle_ = nullptr;
    }

    task& operator=(task&& other) noexcept {
        if (addressof(other) == this) {
            return *this;
        }

        if (handle_) {
            handle_.destroy();
        }
        handle_ = other.handle_;
        other.handle_ = nullptr;

        return *this;
    }

    ~task() {
        if (handle_) {
            handle_.destroy();
        }
    }

    task(const task&) = delete;
    task& operator=(const task&) = delete;

    awaiter operator co_await() { return awaiter{handle_}; }

    bool done() const { return handle_.done(); }

    void resume() {
        if (handle_ && !handle_.done()) {
            handle_.resume();
        }
    }

    void get() {
        while (!handle_.done()) {
            handle_.resume();
        }
        if (handle_.promise().exception) {
            rethrow_exception(handle_.promise().exception);
        }
    }

    void set_cancellation_token(cancellation_token& token) {
        if (handle_) {
            handle_.promise().set_cancellation_token(&token);
        }
    }

    bool is_cancelled() const { return handle_ && handle_.promise().is_cancelled(); }
};


/// @cond
NEFORCE_BEGIN_INNER__

template <typename... Ts>
struct when_all_result {
    tuple<Ts...> values;
};

template <size_t I, typename Tuple, typename... Tasks, enable_if_t<(I < sizeof...(Tasks)), int> = 0>
task<void> when_all_helper(Tuple& results, Tasks&&... tasks) {
    auto& current_task = _NEFORCE get<I>(_NEFORCE forward_as_tuple(_NEFORCE forward<Tasks>(tasks)...));
    _NEFORCE get<I>(results) = co_await _NEFORCE move(current_task);
    co_await when_all_helper<I + 1>(results, _NEFORCE forward<Tasks>(tasks)...);
    co_return;
}

template <size_t I, typename Tuple, typename... Tasks, enable_if_t<I == sizeof...(Tasks), int> = 0>
task<void> when_all_helper(Tuple&, Tasks&&...) {
    co_return;
}

NEFORCE_END_INNER__
/// @endcond

/**
 * @brief 等待所有任务完成
 * @tparam Tasks 任务类型
 * @param tasks 要等待的任务
 * @return 包含所有任务结果的任务
 *
 * 并行执行多个任务，等待所有任务完成后返回结果元组。
 */
template <typename... Tasks>
auto when_all(Tasks&&... tasks) -> task<tuple<typename Tasks::promise_type::result_type...>> {
    tuple<typename Tasks::promise_type::result_type...> results;
    co_await inner::when_all_helper<0>(results, _NEFORCE forward<Tasks>(tasks)...);
    co_return results;
}


/**
 * @brief 带重试的异步操作
 * @tparam T 结果类型
 * @tparam Factory 工厂函数类型
 * @param factory 返回task<T>的工厂函数
 * @param max_attempts 最大尝试次数
 * @param should_retry 判断是否应该重试的函数
 * @return 任务结果
 *
 * 执行异步操作，失败时根据策略重试。
 */
template <typename T, typename Factory>
task<T> retry(Factory&& factory, const size_t max_attempts,
              function<bool(const exception_ptr&)> should_retry = nullptr) {
    static_assert(_NEFORCE is_invocable_r_v<task<T>, Factory>, "Factory must return task<T>");

    exception_ptr last_exception;

    for (size_t attempt = 0; attempt < max_attempts; ++attempt) {
        try {
            co_return co_await factory();
        } catch (...) {
            last_exception = _NEFORCE current_exception();

            if (should_retry && !should_retry(last_exception)) {
                rethrow_exception(last_exception);
            }
            if (attempt == max_attempts - 1) {
                rethrow_exception(last_exception);
            }
        }
    }

    rethrow_exception(last_exception);
}

/** @} */ // Coroutine

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_STANDARD_20
#endif // NEFORCE_CORE_ASYNC_GENERATOR_HPP__
