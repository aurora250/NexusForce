#ifndef NEFORCE_CORE_ASYNC_FUTURE_HPP__
#define NEFORCE_CORE_ASYNC_FUTURE_HPP__

/**
 * @file future.hpp
 * @brief 异步结果消费者
 *
 * 此文件提供了future的实现作为异步结果的消费者。
 */

#include "NeForce/core/async/at_thread_exit.hpp"
#include "NeForce/core/async/atomic_futex.hpp"
#include "NeForce/core/async/call_once.hpp"
#include "NeForce/core/exception/exception_ptr.hpp"
#include "NeForce/core/functional/function.hpp"
#include "NeForce/core/memory/aligned_buffer.hpp"
#include "NeForce/core/memory/allocated_ptr.hpp"
#include "NeForce/core/memory/standard_allocator.hpp"
#include "NeForce/core/memory/weak_ptr.hpp"
#include "NeForce/core/utility/none.hpp"
NEFORCE_BEGIN_NAMESPACE__

/**
 * @defgroup Exceptions 异常类集
 * @brief 异常类集
 * @{
 */

/**
 * @class future_exception
 * @brief 期望值操作异常
 */
struct future_exception final : exception {
    explicit future_exception(const char* info = "Future Operation Failed.", const char* type = static_type,
                              const int code = 0) noexcept :
    exception(info, type, code) {}

    explicit future_exception(const exception& e) :
    exception(e) {}

    ~future_exception() override = default;
    static constexpr auto static_type = "future_exception";
};


/** @} */ // Exceptions

/**
 * @defgroup AsyncComponents 异步组件
 * @brief 异步编程相关组件
 * @{
 */

/**
 * @defgroup Async 异步调用
 * @brief 异步调用组件
 * @{
 */

template <typename Res>
class future;

template <typename Res>
class shared_future;

template <typename Sign>
class packaged_task;

template <typename Res>
class promise;


/**
 * @enum future_errc
 * @brief 期值错误码枚举
 *
 * 定义future/promise操作可能发生的错误类型。
 */
enum class future_errc {
    future_already_retrieved = 1, ///< future已经被获取
    promise_already_satisfied,    ///< promise已经被满足
    no_state,                     ///< 没有关联的状态对象
    broken_promise                ///< 承诺被破坏
};

/**
 * @enum future_status
 * @brief 期值状态枚举
 *
 * 表示future的等待状态。
 */
enum class future_status {
    ready,   ///< 结果已就绪
    timeout, ///< 等待超时
    deferred ///< 延迟执行
};


/**
 * @enum launch
 * @brief 异步启动策略枚举
 *
 * 控制async函数的执行策略。
 */
enum class launch {
    async = 1,   ///< 异步执行，在新线程中运行
    deferred = 2 ///< 延迟执行，在获取结果时运行
};

constexpr launch operator&(const launch left, const launch right) noexcept {
    return static_cast<launch>(static_cast<int>(left) & static_cast<int>(right));
}
constexpr launch operator|(const launch left, const launch right) noexcept {
    return static_cast<launch>(static_cast<int>(left) | static_cast<int>(right));
}
constexpr launch operator^(const launch left, const launch right) noexcept {
    return static_cast<launch>(static_cast<int>(left) ^ static_cast<int>(right));
}
constexpr launch operator~(const launch value) noexcept { return static_cast<launch>(~static_cast<int>(value)); }

constexpr launch& operator&=(launch& left, const launch right) noexcept { return left = left & right; }
constexpr launch& operator|=(launch& left, const launch right) noexcept { return left = left | right; }
constexpr launch& operator^=(launch& left, const launch right) noexcept { return left = left ^ right; }


/**
 * @brief 异步调用结果类型推导
 * @tparam Func 可调用对象类型
 * @tparam Args 参数类型
 * @note 用于async函数返回类型的推导
 */
template <typename Func, typename... Args>
using async_result_t = invoke_result_t<decay_t<Func>, decay_t<Args>...>;

template <typename Func, typename... Args>
future<async_result_t<Func, Args...>> async(launch policy, Func&& function, Args&&... args);

template <typename Func, typename... Args>
future<async_result_t<Func, Args...>> async(Func&& function, Args&&... args);


/// @cond
NEFORCE_BEGIN_INNER__

constexpr const char* future_errc_cstr(const future_errc code) {
    switch (code) {
        case future_errc::future_already_retrieved: {
            return "future_already_retrieved";
        }
        case future_errc::promise_already_satisfied: {
            return "promise_already_satisfied";
        }
        case future_errc::no_state: {
            return "no_state";
        }
        case future_errc::broken_promise: {
            return "broken_promise";
        }
        default: {
            unreachable();
        }
    }
}

/**
 * @class __future_base
 * @brief 期值系统基础类
 *
 * 提供future/promise共享状态的基础实现。
 */
struct __future_base {
    /**
     * @struct result_base
     * @brief 结果基类
     *
     * 存储异步计算的结果或异常。
     */
    struct result_base {
        exception_ptr error_ptr{nullptr}; ///< 异常指针，为空表示正常结果

    protected:
        result_base() noexcept = default;

    public:
        result_base(const result_base&) = delete;
        result_base& operator=(const result_base&) = delete;

        virtual ~result_base() = default;

        /**
         * @brief 销毁结果对象
         * @note 虚函数，由具体子类实现
         */
        virtual void destroy() = 0;

        /**
         * @struct Deleter
         * @brief 结果对象的删除器
         */
        struct Deleter {
            void operator()(result_base* result) const { result->destroy(); }
        };
    };

    template <typename Res>
    using Ptr = unique_ptr<Res, result_base::Deleter>; ///< 结果指针类型定义

    /**
     * @struct basic_result
     * @brief 基础结果存储类模板
     * @tparam Res 结果类型
     *
     * 使用对齐缓冲区存储结果值，支持异常传递。
     */
    template <typename Res>
    struct basic_result : result_base {
    private:
        aligned_buffer<Res> storage; ///< 对齐存储缓冲区
        bool initialized = false;    ///< 是否已初始化标志

        /// 销毁实现
        void destroy() override { delete this; }

    public:
        using result_type = Res; ///< 结果类型定义

        /**
         * @brief 默认构造函数
         */
        basic_result() noexcept = default;

        /**
         * @brief 析构函数
         * @note 如果已初始化，会调用结果值的析构函数
         */
        ~basic_result() override {
            if (initialized) {
                value().~Res();
            }
        }

        /**
         * @brief 获取存储的结果引用
         * @return 结果引用
         */
        Res& value() noexcept { return *storage.ptr(); }

        /**
         * @brief 设置结果值
         * @param result 结果值
         */
        void set(Res&& result) {
            new (storage.addr()) Res(_NEFORCE forward<Res>(result));
            initialized = true;
        }
    };

    /**
     * @struct allocated_result
     * @brief 带分配器的结果存储类
     * @tparam Res 结果类型
     * @tparam Alloc 分配器类型
     *
     * 支持自定义分配器的结果存储。
     */
    template <typename Res, typename Alloc>
    struct allocated_result final : basic_result<Res>, Alloc {
        using allocator_type = inner::__allocator_traits_base::alloc_rebind_t<Alloc, allocated_result>;

        /**
         * @brief 构造函数
         * @param alloc 分配器实例
         */
        explicit allocated_result(const Alloc& alloc) :
        basic_result<Res>(),
        Alloc(alloc) {}

    private:
        /**
         * @brief 销毁实现
         */
        void destroy() override {
            allocator_type alloc(*this);
            allocated_ptr<allocator_type> guard_ptr{alloc, this};
            this->~AllocatedResult();
        }
    };

    /**
     * @brief 分配带分配器的结果对象
     * @tparam Res 结果类型
     * @tparam Alloc 分配器类型
     * @param alloc 分配器
     * @return 结果对象指针
     */
    template <typename Res, typename Alloc>
    static Ptr<allocated_result<Res, Alloc>> allocate_result(const Alloc& alloc) {
        using result_type = allocated_result<Res, Alloc>;
        typename result_type::allocator_type alloc2(alloc);
        auto guard = _NEFORCE allocate_guarded(alloc2);
        auto* ptr = new (static_cast<void*>(guard.get())) result_type{alloc};
        guard = nullptr;
        return Ptr<result_type>(ptr);
    }

    /**
     * @brief 分配基础结果对象
     * @tparam Res 结果类型
     * @tparam T 分配器值类型
     * @return 结果对象指针
     */
    template <typename Res, typename T>
    static Ptr<basic_result<Res>> allocate_result(const _NEFORCE allocator<T>& /*unused*/) {
        return Ptr<basic_result<Res>>(new basic_result<Res>);
    }

    /**
     * @class state_base
     * @brief 共享状态基类
     *
     * 管理future和promise之间的共享状态，包括结果存储、同步和异常处理。
     */
    class state_base {
    public:
        using PtrType = Ptr<result_base>; ///< 结果指针类型

    private:
        /// 状态枚举
        enum status : uint32_t {
            not_ready, ///< 结果未就绪
            ready      ///< 结果已就绪
        };

        PtrType result_ptr;                       ///< 结果指针
        atomic_futex<> status{status::not_ready}; ///< 原子状态变量
        atomic_flag retrieved;                    ///< 是否已获取标志
        once_flag flag;                           ///< 一次性标志，用于确保只设置一次结果

    public:
        /**
         * @brief 默认构造函数
         */
        state_base() noexcept { retrieved.clear(); }

        state_base(const state_base&) = delete;
        state_base& operator=(const state_base&) = delete;
        virtual ~state_base() = default;

        /**
         * @brief 等待结果就绪
         * @return 结果基类引用
         * @note 阻塞当前线程直到结果可用
         */
        result_base& wait() {
            complete_async();
            status.load_when_equal(status::ready, memory_order_acquire);
            return *result_ptr;
        }

        /**
         * @brief 带超时的等待
         * @tparam Rep 时间表示类型
         * @tparam Period 时间周期类型
         * @param relative_time 相对超时时间
         * @return 等待状态
         */
        template <typename Rep, typename Period>
        future_status wait_for(const duration<Rep, Period>& relative_time) {
            if (status.load(memory_order_acquire) == status::ready) {
                return future_status::ready;
            }

            if (is_deferred_future()) {
                return future_status::deferred;
            }

            if (relative_time > relative_time.zero() &&
                status.load_when_equal_for(status::ready, memory_order_acquire, relative_time)) {
                complete_async();
                return future_status::ready;
            }
            return future_status::timeout;
        }

        /**
         * @brief 带绝对时间的等待
         * @tparam Clock 时钟类型
         * @tparam Dur 时长类型
         * @param absolute_time 绝对超时时间点
         * @return 等待状态
         */
        template <typename Clock, typename Dur>
        future_status wait_until(const time_point<Clock, Dur>& absolute_time) {
            static_assert(is_clock_v<Clock>, "Clock type must be clock_t");
            if (status.load(memory_order_acquire) == status::ready) {
                return future_status::ready;
            }

            if (is_deferred_future()) {
                return future_status::deferred;
            }

            if (status.load_when_equal_until(status::ready, memory_order_acquire, absolute_time)) {
                complete_async();
                return future_status::ready;
            }
            return future_status::timeout;
        }

        /**
         * @brief 设置结果
         * @tparam Callable 可调用类型
         * @param result_func 结果设置函数
         * @param ignore_failure 是否忽略设置失败
         * @throw future_exception 如果设置失败且不忽略
         */
        template <typename Callable>
        void set_result(Callable&& result_func, const bool ignore_failure = false) {
            bool did_set = false;
            function<PtrType()> func = _NEFORCE forward<Callable>(result_func);
            call_once(flag, &state_base::do_set, this, _NEFORCE addressof(func), _NEFORCE addressof(did_set));
            if (did_set) {
                status.store_notify_all(status::ready, memory_order_release);
            } else if (!ignore_failure) {
                NEFORCE_THROW_EXCEPTION(future_exception(future_errc_cstr(future_errc::promise_already_satisfied)));
            }
        }

        /**
         * @brief 设置延迟结果
         * @tparam Callable 可调用类型
         * @param result_func 结果设置函数
         * @param self 自身的弱引用
         * @throw future_exception 如果设置失败
         * @note 用于延迟执行的情况
         */
        template <typename Callable>
        void set_delayed_result(Callable&& result_func, weak_ptr<state_base> self) {
            bool did_set = false;
            unique_ptr<make_ready> mr = make_unique<make_ready>();
            function<PtrType()> func = _NEFORCE forward<Callable>(result_func);
            call_once(flag, &state_base::do_set, this, _NEFORCE addressof(func), _NEFORCE addressof(did_set));
            if (!did_set) {
                NEFORCE_THROW_EXCEPTION(future_exception(future_errc_cstr(future_errc::promise_already_satisfied)));
            }
            mr->shared_state = _NEFORCE move(self);
            mr->set();
            mr.release();
        }

        /**
         * @brief 破坏Promise
         * @param result 结果指针
         * @note 设置broken_promise异常
         */
        void break_promise(PtrType result) {
            if (static_cast<bool>(result)) {
                result->error_ptr = make_exception_ptr(future_exception(future_errc_cstr(future_errc::broken_promise)));
                result_ptr.swap(result);
                status.store_notify_all(status::ready, memory_order_release);
            }
        }

        /**
         * @brief 设置结果获取标志
         * @throw future_exception 如果结果已被获取
         */
        void set_retrieved_flag() {
            if (retrieved.test_and_set(memory_order_acquire)) {
                NEFORCE_THROW_EXCEPTION(future_exception(future_errc_cstr(future_errc::future_already_retrieved)));
            }
        }

        /**
         * @brief 结果设置器模板
         * @tparam Res 结果类型
         * @tparam Arg 参数类型
         *
         * 用于不同类型结果的设置器特化。
         */
        template <typename Res, typename Arg>
        struct setter;

        /**
         * @brief 引用类型结果设置器特化
         */
        template <typename Res, typename Arg>
        struct setter<Res, Arg&> {
            static_assert(is_same_v<Res, Arg&> || is_same_v<const Res, Arg>, "Invalid specialisation");

            typename promise<Res>::ptr_type operator()() const {
                promise_ptr->storage->set(*arg_ptr);
                return _NEFORCE move(promise_ptr->storage);
            }
            promise<Res>* promise_ptr;
            Arg* arg_ptr;
        };

        /**
         * @brief 右值引用类型结果设置器特化
         */
        template <typename Res>
        struct setter<Res, Res&&> {
            typename promise<Res>::ptr_type operator()() const {
                promise_ptr->storage->set(_NEFORCE move(*arg_ptr));
                return _NEFORCE move(promise_ptr->storage);
            }
            promise<Res>* promise_ptr;
            Res* arg_ptr;
        };

        /**
         * @brief void类型结果设置器特化
         */
        template <typename Res>
        struct setter<Res, void> {
            static_assert(is_void_v<Res>, "Only used for promise<void>");

            typename promise<Res>::ptr_type operator()() const { return _NEFORCE move(promise_ptr->storage); }
            promise<Res>* promise_ptr;
        };

        struct exception_ptr_tag {};

        /**
         * @brief 异常类型结果设置器特化
         */
        template <typename Res>
        struct setter<Res, exception_ptr_tag> {
            typename promise<Res>::ptr_type operator()() const {
                promise_ptr->storage->error_ptr = *exp_ptr;
                return _NEFORCE move(promise_ptr->storage);
            }
            promise<Res>* promise_ptr;
            exception_ptr* exp_ptr;
        };

        /**
         * @brief 创建通用设置器
         */
        template <typename Res, typename Arg>
        NEFORCE_ALWAYS_INLINE static setter<Res, Arg&&> create_setter(promise<Res>* promise_ptr, Arg&& arg) noexcept {
            return setter<Res, Arg&&>{promise_ptr, _NEFORCE addressof(arg)};
        }

        /**
         * @brief 创建异常设置器
         */
        template <typename Res>
        NEFORCE_ALWAYS_INLINE static setter<Res, exception_ptr_tag> create_setter(exception_ptr& exception,
                                                                                  promise<Res>* promise_ptr) noexcept {
            return setter<Res, exception_ptr_tag>{promise_ptr, &exception};
        }

        /**
         * @brief 创建void结果设置器
         */
        template <typename Res>
        NEFORCE_ALWAYS_INLINE static setter<Res, void> create_setter(promise<Res>* promise_ptr) noexcept {
            return setter<Res, void>{promise_ptr};
        }

        /**
         * @brief 检查共享指针有效性
         * @tparam T 指针类型
         * @param ptr 要检查的指针
         * @throw future_exception 如果指针为空
         */
        template <typename T>
        static void check(const shared_ptr<T>& ptr) {
            if (!static_cast<bool>(ptr)) {
                NEFORCE_THROW_EXCEPTION(future_exception(future_errc_cstr(future_errc::no_state)));
            }
        }

    private:
        /**
         * @brief 执行结果设置
         * @param func 结果设置函数指针
         * @param did_set 设置成功标志
         */
        void do_set(function<PtrType()>* func, bool* did_set) {
            PtrType result = (*func)();
            *did_set = true;
            result_ptr.swap(result);
        }

        /**
         * @brief 完成异步操作
         * @note 由派生类实现具体逻辑
         */
        virtual void complete_async() {}

        /**
         * @brief 是否为延迟future
         * @return 是否是延迟future
         */
        NEFORCE_NODISCARD virtual bool is_deferred_future() const { return false; }

        /**
         * @brief 就绪标记器
         *
         * 用于在线程退出时标记future就绪。
         */
        struct make_ready final : at_thread_exit_elt {
            weak_ptr<state_base> shared_state; ///< 共享状态弱引用

            /**
             * @brief 就绪回调函数
             * @param ptr 指向make_ready对象的指针
             */
            static void run(void* ptr) noexcept {
                auto* const self = static_cast<make_ready*>(ptr);
                const auto state = self->shared_state.lock();
                if (state) {
                    state->status.store_notify_all(status::ready, memory_order_release);
                }
                delete self;
            }

            /**
             * @brief 设置就绪标记
             */
            void set() { thread_exit_register(this, &make_ready::run); }
        };
    };

private:
    /**
     * @brief 结果类型提取器
     * @tparam Ptr 指针类型
     */
    template <typename Ptr>
    struct get_result_type {
        using type = typename Ptr::element_type::result_type;
    };

    template <typename Ptr>
    using result_res_t = typename get_result_type<Ptr>::type;

public:
    class async_state_common;

    template <typename BoundFunc, typename Res = decltype(_NEFORCE declval<BoundFunc&>()())>
    class deferred_state;

    template <typename BoundFunc, typename Res = decltype(_NEFORCE declval<BoundFunc&>()())>
    class async_state_impl;

    template <typename Sign>
    class task_state_base;

    template <typename Func, typename Alloc, typename Sign>
    class task_state;

    template <typename ResPtrT, typename Func, typename ResT = result_res_t<ResPtrT>>
    struct task_setter;

    /**
     * @brief 创建任务设置器
     */
    template <typename ResPtr, typename BoundFunc>
    static task_setter<ResPtr, BoundFunc> create_task_setter(ResPtr& ptr, BoundFunc& call) {
        return {_NEFORCE addressof(ptr), _NEFORCE addressof(call)};
    }
};

/**
 * @brief 引用类型结果特化
 * @tparam Res 引用类型
 */
template <typename Res>
struct __future_base::basic_result<Res&> : __future_base::result_base {
private:
    Res* value_ptr; ///< 指向引用值的指针

    void destroy() override { delete this; }

public:
    using result_type = Res&; ///< 结果类型定义

    /**
     * @brief 默认构造函数
     */
    basic_result() noexcept :
    value_ptr() {}

    /**
     * @brief 设置引用结果
     * @param result 要引用的对象
     */
    void set(Res& result) noexcept { value_ptr = _NEFORCE addressof(result); }

    /**
     * @brief 获取引用结果
     * @return 引用结果
     */
    NEFORCE_NODISCARD Res& get() noexcept { return *value_ptr; }
};

/**
 * @brief void类型结果特化
 */
template <>
struct __future_base::basic_result<void> : __future_base::result_base {
    using result_type = void; ///< 结果类型定义

private:
    void destroy() override { delete this; }
};


/**
 * @brief future基类模板
 * @tparam Res 结果类型
 *
 * 提供future的基础功能，包括等待、状态查询等。
 */
template <typename Res>
class __basic_future : public __future_base {
protected:
    using state_type = shared_ptr<state_base>;             ///< 状态类型
    using result_type = __future_base::basic_result<Res>&; ///< 结果类型

private:
    state_type state_ptr; ///< 共享状态指针

public:
    __basic_future(const __basic_future&) = delete;
    __basic_future& operator=(const __basic_future&) = delete;

    /**
     * @brief 检查future是否有效
     * @return 是否有效
     */
    NEFORCE_NODISCARD bool valid() const noexcept { return static_cast<bool>(state_ptr); }

    /**
     * @brief 等待结果就绪
     * @throw future_exception 当future无效时抛出
     */
    void wait() const {
        state_base::check(state_ptr);
        state_ptr->wait();
    }

    /**
     * @brief 等待结果（相对超时）
     * @tparam Rep 时长表示类型
     * @tparam Period 时长周期类型
     * @param relative_time 相对超时时长
     * @return 等待状态
     * @throw future_exception 当future无效时抛出
     */
    template <typename Rep, typename Period>
    future_status wait_for(const duration<Rep, Period>& relative_time) const {
        state_base::check(state_ptr);
        return state_ptr->wait_for(relative_time);
    }

    /**
     * @brief 等待结果（绝对超时）
     * @tparam Clock 时钟类型
     * @tparam Dur 时长类型
     * @param absolute_time 绝对超时时间点
     * @return 等待状态
     * @throw future_exception 当future无效时抛出
     */
    template <typename Clock, typename Dur>
    future_status wait_until(const time_point<Clock, Dur>& absolute_time) const {
        state_base::check(state_ptr);
        return state_ptr->wait_until(absolute_time);
    }

protected:
    /**
     * @brief 获取结果
     * @return 结果引用
     */
    NEFORCE_NODISCARD result_type get_result() const {
        state_base::check(state_ptr);
        result_base& result = state_ptr->wait();
        if (result.error_ptr != nullptr) {
            rethrow_exception(result.error_ptr);
        }
        return static_cast<result_type>(result);
    }

    /**
     * @brief 交换两个future
     * @param other 要交换的future
     */
    void swap(__basic_future& other) noexcept { state_ptr.swap(other.state_ptr); }

    /**
     * @brief 构造函数
     * @param state 共享状态
     */
    explicit __basic_future(state_type state) :
    state_ptr(move(state)) {
        state_base::check(state_ptr);
        state_ptr->set_retrieved_flag();
    }

    explicit __basic_future(const shared_future<Res>& other) noexcept;
    explicit __basic_future(shared_future<Res>&& other) noexcept;
    explicit __basic_future(future<Res>&& other) noexcept;

    /**
     * @brief 默认构造函数
     */
    constexpr __basic_future() noexcept = default;

    /**
     * @brief 重置器
     *
     * 用于在作用域结束时重置future状态。
     */
    struct reset {
        __basic_future& future_ref;

        /**
         * @brief 构造函数
         * @param future 要管理的future
         */
        explicit reset(__basic_future& future) noexcept :
        future_ref(future) {}

        /**
         * @brief 析构函数
         */
        ~reset() { future_ref.state_ptr.reset(); }
    };
};

NEFORCE_END_INNER__
/// @endcond


template <typename Res, typename Arg>
struct is_location_invariant<inner::__future_base::state_base::setter<Res, Arg>> : true_type {};

template <typename ResPtr, typename Func, typename Res>
struct is_location_invariant<inner::__future_base::task_setter<ResPtr, Func, Res>> : true_type {};


/**
 * @class future
 * @brief 独占future类模板
 * @tparam Res 结果类型
 *
 * 表示一个异步计算的结果，结果只能被获取一次。
 * @note 不支持拷贝，仅支持移动
 */
template <typename Res>
class future : public inner::__basic_future<Res> {
    static_assert(!is_array_v<Res>, "result type must not be an array");
    static_assert(!is_function_v<Res>, "result type must not be a function");
    static_assert(is_destructible_v<Res>, "result type must be destructible");

    friend class promise<Res>;

    template <typename Sign>
    friend class packaged_task;

    template <typename Function, typename... Args>
    friend future<async_result_t<Function, Args...>> async(launch, Function&&, Args&&...);

    using base_type = inner::__basic_future<Res>;
    using state_type = typename base_type::state_type;

    /**
     * @brief 状态构造函数
     */
    explicit future(const state_type& state) :
    base_type(state) {}

public:
    /**
     * @brief 默认构造函数
     */
    constexpr future() noexcept :
    base_type() {}

    /**
     * @brief 移动构造函数
     * @param other 要移动的future
     */
    future(future&& other) noexcept :
    base_type(_NEFORCE move(other)) {}

    future(const future&) = delete;
    future& operator=(const future&) = delete;

    /**
     * @brief 移动赋值运算符
     * @param other 要移动的future
     * @return 当前对象的引用
     */
    future& operator=(future&& other) noexcept {
        future(_NEFORCE move(other)).swap(*this);
        return *this;
    }

    /**
     * @brief 获取结果
     * @return 结果值
     * @note 调用后future将变为无效状态
     */
    Res get() {
        typename base_type::reset reset_(*this);
        return _NEFORCE move(this->get_result().value());
    }

    /**
     * @brief 转换为共享future
     * @return 共享future对象
     */
    shared_future<Res> share() noexcept;
};

/**
 * @brief 引用类型的future特化
 * @tparam Res 引用类型
 */
template <typename Res>
class future<Res&> : public inner::__basic_future<Res&> {
    friend class promise<Res&>;

    template <typename Sign>
    friend class packaged_task;

    template <typename Function, typename... Args>
    friend future<async_result_t<Function, Args...>> async(launch, Function&&, Args&&...);

    using base_type = inner::__basic_future<Res&>;
    using state_type = typename base_type::state_type;

    explicit future(const state_type& state) :
    base_type(state) {}

public:
    constexpr future() noexcept :
    base_type() {}
    future(future&& other) noexcept :
    base_type(_NEFORCE move(other)) {}

    future(const future&) = delete;
    future& operator=(const future&) = delete;

    future& operator=(future&& other) noexcept {
        future(_NEFORCE move(other)).swap(*this);
        return *this;
    }

    /**
     * @brief 获取结果引用
     * @return 结果引用
     * @note 调用后future将变为无效状态
     */
    Res& get() {
        typename base_type::reset reset_(*this);
        return this->get_result().get();
    }

    shared_future<Res&> share() noexcept;
};

/**
 * @brief void类型的future特化
 */
template <>
class future<void> : public inner::__basic_future<void> {
    friend class promise<void>;

    template <typename>
    friend class packaged_task;

    template <typename Function, typename... Args>
    friend future<async_result_t<Function, Args...>> async(launch, Function&&, Args&&...);

    using base_type = inner::__basic_future<void>;
    // NOLINTNEXTLINE(readability-redundant-qualified-alias)
    using state_type = base_type::state_type;

    explicit future(state_type state) :
    base_type(move(state)) {}

public:
    future() noexcept = default;

    future(future&& other) noexcept :
    base_type(_NEFORCE move(other)) {}

    future(const future&) = delete;
    future& operator=(const future&) = delete;

    future& operator=(future&& other) noexcept {
        future(_NEFORCE move(other)).swap(*this);
        return *this;
    }

    /**
     * @brief 获取结果
     * @note 调用后future将变为无效状态
     */
    void get() {
        base_type::reset reset(*this);
        ignore = this->get_result();
    }

    shared_future<void> share() noexcept;
};

/**
 * @class shared_future
 * @brief 共享future类模板
 * @tparam Res 结果类型
 *
 * 表示一个异步计算的结果，结果可以被多次获取。
 */
template <typename Res>
class shared_future : public inner::__basic_future<Res> {
    static_assert(!is_array_v<Res>, "result type must not be an array");
    static_assert(!is_function_v<Res>, "result type must not be a function");
    static_assert(is_destructible_v<Res>, "result type must be destructible");

    using base_type = inner::__basic_future<Res>;

public:
    constexpr shared_future() noexcept :
    base_type() {}
    shared_future(const shared_future& other) noexcept :
    base_type(other) {}
    shared_future(future<Res>&& other) noexcept :
    base_type(_NEFORCE move(other)) {}
    shared_future(shared_future&& other) noexcept :
    base_type(_NEFORCE move(other)) {}

    shared_future& operator=(const shared_future& other) noexcept {
        shared_future(other).swap(*this);
        return *this;
    }

    shared_future& operator=(shared_future&& other) noexcept {
        shared_future(_NEFORCE move(other)).swap(*this);
        return *this;
    }

    /**
     * @brief 获取常量引用结果
     * @return 结果常量引用
     * @note 可以多次调用
     */
    const Res& get() const { return this->get_result().value(); }
};

/**
 * @brief 引用类型的共享future特化
 * @tparam Res 引用类型
 */
template <typename Res>
class shared_future<Res&> : public inner::__basic_future<Res&> {
    using base_type = inner::__basic_future<Res&>;

public:
    constexpr shared_future() noexcept :
    base_type() {}
    shared_future(const shared_future& other) :
    base_type(other) {}
    shared_future(future<Res&>&& other) noexcept :
    base_type(_NEFORCE move(other)) {}
    shared_future(shared_future&& other) noexcept :
    base_type(_NEFORCE move(other)) {}

    shared_future& operator=(const shared_future& other) {
        shared_future(other).swap(*this);
        return *this;
    }

    shared_future& operator=(shared_future&& other) noexcept {
        shared_future(_NEFORCE move(other)).swap(*this);
        return *this;
    }

    /**
     * @brief 获取结果引用
     * @return 结果引用
     */
    Res& get() const { return this->get_result().get(); }
};

/**
 * @brief void类型的共享future特化
 */
template <>
class shared_future<void> : public inner::__basic_future<void> {
    using base_type = inner::__basic_future<void>;

public:
    shared_future() noexcept = default;

    shared_future(const shared_future& other) :
    base_type(other) {}
    shared_future(future<void>&& other) noexcept :
    base_type(_NEFORCE move(other)) {}
    shared_future(shared_future&& other) noexcept :
    base_type(_NEFORCE move(other)) {}

    shared_future& operator=(const shared_future& other) {
        shared_future(other).swap(*this);
        return *this;
    }

    shared_future& operator=(shared_future&& other) noexcept {
        shared_future(_NEFORCE move(other)).swap(*this);
        return *this;
    }

    /**
     * @brief 获取结果
     */
    void get() const { ignore = this->get_result(); }
};

/// @cond

template <typename Res>
inner::__basic_future<Res>::__basic_future(const shared_future<Res>& other) noexcept :
state_ptr(other.state_ptr) {}

template <typename Res>
inner::__basic_future<Res>::__basic_future(shared_future<Res>&& other) noexcept :
state_ptr(_NEFORCE move(other.state_ptr)) {}

template <typename Res>
inner::__basic_future<Res>::__basic_future(future<Res>&& other) noexcept :
state_ptr(_NEFORCE move(other.state_ptr)) {}


template <typename Result>
shared_future<Result> future<Result>::share() noexcept {
    return shared_future<Result>(_NEFORCE move(*this));
}

template <typename Res>
shared_future<Res&> future<Res&>::share() noexcept {
    return shared_future<Res&>(_NEFORCE move(*this));
}

inline shared_future<void> future<void>::share() noexcept { return {_NEFORCE move(*this)}; }

/// @endcond

/**
 * @brief future结果类型转换
 * @tparam T 原始类型
 *
 * 将void类型转换为none_t，其他类型保持不变。
 */
template <typename T>
struct future_result {
    using type = T;
};

/**
 * @brief void类型的future结果转换特化
 */
template <>
struct future_result<void> {
    using type = none_t;
};

/**
 * @brief future结果类型别名
 * @tparam T 原始类型
 */
template <typename T>
using future_result_t = typename future_result<T>::type;


/**
 * @brief 通用future结果获取函数
 * @tparam T void类型
 * @param f future对象
 * @return none_t
 */
template <typename T>
enable_if_t<is_void_v<T>, future_result_t<T>> get(future<T>& f) {
    f.get();
    return none;
}

/**
 * @brief 通用future结果获取函数
 * @tparam T 非void类型
 * @param f future对象
 * @return 结果值
 */
template <typename T>
enable_if_t<!is_void_v<T>, future_result_t<T>> get(future<T>& f) {
    return f.get();
}

/** @} */ // Async

/** @} */ // AsyncComponents

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_CORE_ASYNC_FUTURE_HPP__
