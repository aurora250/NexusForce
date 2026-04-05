#ifndef NEFORCE_CORE_ASYNC_SIGNALS_HPP__
#define NEFORCE_CORE_ASYNC_SIGNALS_HPP__

/**
 * @file signals.hpp
 * @brief 信号槽机制
 *
 * 此文件提供了信号槽观察者模式。
 * 支持线程安全的多播委托、连接管理、自动断开、优先级等功能。
 */

#include "NeForce/core/async/mutex.hpp"
#include "NeForce/core/container/vector.hpp"
#include "NeForce/core/functional/apply.hpp"
#include "NeForce/core/memory/weak_ptr.hpp"
#include "NeForce/core/utility/optional.hpp"
NEFORCE_BEGIN_NAMESPACE__

/**
 * @defgroup Signals 信号槽
 * @brief 观察者模式的信号槽实现
 * @{
 */

/**
 * @enum callback_result
 * @brief 回调结果枚举
 *
 * 用于控制回调是否在调用后自动断开。
 */
enum class callback_result {
    keep,  ///< 保留回调
    erase, ///< 调用后自动删除
};


/**
 * @struct oneshot_t
 * @brief 一次性连接标签
 *
 * 用于标记连接只触发一次后自动断开。
 */
struct oneshot_t {
    constexpr oneshot_t() noexcept = default;
};

/**
 * @brief 一次性连接标签实例
 */
NEFORCE_INLINE17 constexpr oneshot_t oneshot{};

/**
 * @enum nshot_t
 * @brief 多次连接标签
 *
 * 用于标记连接触发指定次数后自动断开。
 */
enum class nshot_t : size_t {
};


/**
 * @class connection
 * @brief 连接句柄
 *
 * 用于管理信号槽连接，可以手动断开连接或检查连接状态。
 * 可实现连接状态的线程安全共享。
 */
class connection {
private:
    shared_ptr<bool> connected_; ///< 连接状态标志

public:
    /**
     * @brief 默认构造函数，创建处于已连接状态的对象
     */
    connection() :
    connected_(_NEFORCE make_shared<bool>(true)) {}

    /**
     * @brief 断开连接
     *
     * 设置连接状态为false，后续信号触发将忽略此连接。
     * 如果连接已被断开，此操作无效果。
     */
    void disconnect() noexcept {
        if (connected_) {
            *connected_ = false;
        }
    }

    /**
     * @brief 检查连接是否有效
     * @return 连接是否仍然有效
     */
    bool connected() const noexcept { return connected_ && *connected_; }

    /**
     * @brief 获取连接标志的内部指针
     * @return 共享指针指向bool标志
     */
    shared_ptr<bool> flag() const noexcept { return connected_; }
};


/**
 * @class scoped_connection
 * @brief 作用域连接
 *
 * 析构时自动断开连接。
 */
class scoped_connection {
private:
    connection conn_{}; ///< 被管理的连接

public:
    /**
     * @brief 默认构造函数
     */
    scoped_connection() = default;

    /**
     * @brief 从连接构造
     * @param conn 要管理的连接
     */
    explicit scoped_connection(connection conn) noexcept :
    conn_(_NEFORCE move(conn)) {}

    /**
     * @brief 析构函数，自动断开连接
     */
    ~scoped_connection() { conn_.disconnect(); }

    scoped_connection(const scoped_connection&) = delete;
    scoped_connection& operator=(const scoped_connection&) = delete;

    /**
     * @brief 移动构造函数
     * @param other 被移动的对象
     */
    scoped_connection(scoped_connection&& other) noexcept :
    conn_(_NEFORCE move(other.conn_)) {}

    /**
     * @brief 移动赋值运算符
     * @param other 被移动的对象
     * @return 自身引用
     */
    scoped_connection& operator=(scoped_connection&& other) noexcept {
        if (this != &other) {
            conn_.disconnect();
            conn_ = _NEFORCE move(other.conn_);
        }
        return *this;
    }

    /**
     * @brief 手动断开连接
     */
    void disconnect() noexcept { conn_.disconnect(); }

    /**
     * @brief 检查连接是否有效
     * @return 连接是否仍然有效
     */
    bool connected() const noexcept { return conn_.connected(); }

    /**
     * @brief 释放连接所有权
     * @return 被释放的连接
     *
     * 将管理的连接返回给调用者，之后scoped_connection不再管理它。
     */
    connection release() noexcept { return _NEFORCE move(conn_); }
};


template <typename... Types>
struct signal;

/**
 * @class signal_blocker
 * @brief 信号阻塞器
 *
 * 信号阻塞管理器，在作用域内临时阻塞信号触发。
 */
template <typename... Types>
class signal_blocker {
private:
    shared_ptr<bool> blocked_flag_; ///< 阻塞标志
    bool old_value_;                ///< 原值
    bool released_ = false;         ///< 是否已手动解除

public:
    /**
     * @brief 构造函数，阻塞指定信号
     * @param sig 要阻塞的信号
     */
    explicit signal_blocker(signal<Types...>& sig) noexcept :
    blocked_flag_(sig.block_flag()) {
        old_value_ = *blocked_flag_;
        *blocked_flag_ = true;
    }

    /**
     * @brief 析构函数，恢复原状态
     */
    ~signal_blocker() {
        if (blocked_flag_ && !released_) {
            *blocked_flag_ = old_value_;
        }
    }

    signal_blocker(const signal_blocker&) = delete;
    signal_blocker& operator=(const signal_blocker&) = delete;

    /**
     * @brief 手动解除阻塞
     *
     * 提前解除阻塞，不会等待析构时恢复。
     */
    void unblock() noexcept {
        if (blocked_flag_ && !released_) {
            *blocked_flag_ = old_value_;
            released_ = true;
        }
    }
};


/**
 * @struct signal
 * @brief 信号类
 * @tparam Types 信号参数类型列表
 *
 * 多播委托实现。
 */
template <typename... Types>
struct signal {
private:
    /**
     * @struct slot_entry
     * @brief 槽条目
     */
    struct slot_entry {
        using callback_type = function<callback_result(Types...)>; ///< 回调函数类型

        callback_type callback;          ///< 回调函数
        shared_ptr<bool> connected_flag; ///< 连接状态标志
        int priority;                    ///< 优先级（值越大优先级越高）

        slot_entry(callback_type cb, shared_ptr<bool> flag, const int pri = 0) :
        callback(_NEFORCE move(cb)),
        connected_flag(_NEFORCE move(flag)),
        priority(pri) {}
    };

    vector<slot_entry> slots_{};                                       ///< 槽列表
    shared_ptr<bool> blocked_flag_{_NEFORCE make_shared<bool>(false)}; ///< 阻塞标志
    mutable mutex mutex_;                                              ///< 互斥锁

    template <typename T>
    static shared_ptr<T> lock_if_weak(const weak_ptr<T>& self) {
        return self.lock();
    }
    template <typename T>
    static const shared_ptr<T>& lock_if_weak(const shared_ptr<T>& self) noexcept {
        return self;
    }
    template <typename T>
    static T* lock_if_weak(T* self) noexcept {
        return self;
    }

    template <typename Func>
    auto with_lock(Func&& func) const {
        lock<mutex> lock(mutex_);
        return _NEFORCE forward<Func>(func)();
    }

    template <typename Func>
    auto with_lock(Func&& func) {
        lock<mutex> lock(mutex_);
        return _NEFORCE forward<Func>(func)();
    }

    /**
     * @brief 绑定成员函数（普通版本）
     * @tparam Self 对象类型
     * @tparam MemFn 成员函数类型
     * @param self 对象指针/智能指针
     * @param memfn 成员函数指针
     * @return 包装后的可调用对象
     */
    template <typename Self, typename MemFn>
    auto bind(Self self, MemFn memfn) {
        return [self = _NEFORCE move(self), memfn](Types... args) {
            auto ptr = signal::lock_if_weak(self);
            if (ptr == nullptr) {
                return callback_result::erase;
            }
            ((*ptr).*memfn)(_NEFORCE forward<Types>(args)...);
            return callback_result::keep;
        };
    }

    /**
     * @brief 绑定成员函数（一次性版本）
     * @tparam Self 对象类型
     * @tparam MemFn 成员函数类型
     * @param self 对象指针/智能指针
     * @param memfn 成员函数指针
     * @param one 一次性标签
     * @return 包装后的可调用对象
     */
    template <typename Self, typename MemFn>
    auto bind(Self self, MemFn memfn, oneshot_t one) {
        return [self = _NEFORCE move(self), memfn](Types... args) {
            auto ptr = signal::lock_if_weak(self);
            if (ptr == nullptr) {
                return callback_result::erase;
            }
            ((*ptr).*memfn)(_NEFORCE forward<Types>(args)...);
            return callback_result::erase;
        };
    }

    /**
     * @brief 绑定成员函数（多次版本）
     * @tparam Self 对象类型
     * @tparam MemFn 成员函数类型
     * @param self 对象指针/智能指针
     * @param memfn 成员函数指针
     * @param n nshot_t标签
     * @return 包装后的可调用对象
     */
    template <typename Self, typename MemFn>
    auto bind(Self self, MemFn memfn, nshot_t n) {
        return [self = _NEFORCE move(self), memfn, n = static_cast<size_t>(n)](Types... args) mutable {
            if (n == 0) {
                return callback_result::erase;
            }
            auto ptr = signal::lock_if_weak(self);
            if (ptr == nullptr) {
                return callback_result::erase;
            }
            ((*ptr).*memfn)(_NEFORCE forward<Types>(args)...);
            --n;
            if (n == 0) {
                return callback_result::erase;
            }
            return callback_result::keep;
        };
    }

    /**
     * @brief 连接实现（返回callback_result的版本）
     * @tparam Func 函数类型
     * @param callback 回调函数
     * @param priority 优先级
     * @return 连接句柄
     */
    template <typename Func>
    enable_if_t<is_invocable_r_v<callback_result, Func, Types...>, connection> connect_impl(Func callback,
                                                                                            int priority) {
        connection conn{};

        auto it = slots_.begin();
        while (it != slots_.end() && it->priority >= priority) {
            ++it;
        }
        slots_.emplace(it, _NEFORCE move(callback), conn.flag(), priority);

        return conn;
    }

    /**
     * @brief 连接实现（返回void的版本）
     * @tparam Func 函数类型
     * @param callback 回调函数
     * @param priority 优先级
     * @return 连接句柄
     */
    template <typename Func>
    enable_if_t<!is_invocable_r_v<callback_result, Func, Types...>, connection> connect_impl(Func callback,
                                                                                             int priority) {
        connection conn{};

        auto wrapped = [callback = _NEFORCE move(callback)](Types... args) mutable {
            callback(_NEFORCE forward<Types>(args)...);
            return callback_result::keep;
        };

        auto it = slots_.begin();
        while (it != slots_.end() && it->priority >= priority) {
            ++it;
        }
        slots_.emplace(it, _NEFORCE move(wrapped), conn.flag(), priority);

        return conn;
    }

    size_t slot_count_unlocked() const noexcept {
        size_t count = 0;
        for (const auto& slot: slots_) {
            if (slot.connected_flag && *slot.connected_flag) {
                ++count;
            }
        }
        return count;
    }

public:
    signal() = default;
    signal(const signal&) = delete;
    signal& operator=(const signal&) = delete;
    signal(signal&&) = default;
    signal& operator=(signal&&) = default;

    /**
     * @brief 连接成员函数（默认优先级0）
     * @tparam Self 对象类型
     * @tparam MemFn 成员函数类型
     * @tparam Tag 连接标签类型
     * @param self 对象指针/智能指针
     * @param memfn 成员函数指针
     * @param tag 连接标签（可选）
     * @return 连接句柄
     */
    template <typename Self, typename MemFn, typename... Tag>
    connection connect(Self self, MemFn memfn, Tag... tag) {
        return this->connect(_NEFORCE move(self), memfn, 0, tag...);
    }

    /**
     * @brief 连接普通函数/lambda
     * @tparam Func 函数类型
     * @param callback 回调函数
     * @param priority 优先级（值越大优先级越高）
     * @return 连接句柄
     */
    template <typename Func>
    connection connect(Func callback, int priority = 0) {
        return this->with_lock([this, callback = _NEFORCE move(callback), priority] {
            return this->connect_impl(_NEFORCE move(callback), priority);
        });
    }

    /**
     * @brief 连接成员函数
     * @tparam Self 对象类型
     * @tparam MemFn 成员函数类型
     * @tparam Tag 连接标签类型
     * @param self 对象指针/智能指针
     * @param memfn 成员函数指针
     * @param priority 优先级
     * @param tag 连接标签（可选）
     * @return 连接句柄
     */
    template <typename Self, typename MemFn, typename... Tag>
    connection connect(Self self, MemFn memfn, int priority, Tag... tag) {
        static_assert(sizeof...(Tag) <= 1, "only zero or one tag is allowed");

        return this->with_lock([this, self = _NEFORCE move(self), memfn = _NEFORCE move(memfn), priority, tag...] {
            connection conn{};

            auto it = slots_.begin();
            while (it != slots_.end() && it->priority >= priority) {
                ++it;
            }
            slots_.emplace(it, this->bind(_NEFORCE move(self), memfn, tag...), conn.flag(), priority);

            return conn;
        });
    }

    /**
     * @brief 触发信号
     * @param args 信号参数
     *
     * 同步调用所有已连接的槽函数。会自动清理已断开或请求擦除的槽。
     * 如果信号被阻塞，则什么也不做。
     */
    void emit(Types... args) {
        vector<slot_entry> snapshot;
        {
            lock<mutex> lk(mutex_);
            if (is_blocked()) {
                return;
            }
            snapshot = slots_;
        }

        for (auto& slot: snapshot) {
            if (!slot.connected_flag || !(*slot.connected_flag)) {
                continue;
            }

            const callback_result res = slot.callback(args...);
            if (res == callback_result::erase) {
                *slot.connected_flag = false;
            }
        }

        {
            lock<mutex> lk(mutex_);
            slots_.erase(
                    _NEFORCE remove_if(slots_.begin(), slots_.end(),
                                       [](const slot_entry& s) { return !s.connected_flag || !(*s.connected_flag); }),
                    slots_.end());
        }
    }

    /**
     * @brief 执行器触发信号
     * @tparam Executor 执行器类型
     * @param executor 执行器对象
     * @param args 信号参数
     *
     * 在指定的执行器上异步调用emit。
     */
    template <typename Executor>
    void emit_executor(Executor& executor, Types... args) {
        auto weak_flag = weak_ptr<bool>(blocked_flag_);
        auto args_tuple = _NEFORCE make_tuple(_NEFORCE forward<Types>(args)...);

        executor.post([this, weak_flag = _NEFORCE move(weak_flag), args_tuple = _NEFORCE move(args_tuple)]() mutable {
            if (weak_flag.expired()) {
                return;
            }

            _NEFORCE apply([this](auto&&... a) { this->emit(_NEFORCE forward<decltype(a)>(a)...); },
                           _NEFORCE move(args_tuple));
        });
    }

    /**
     * @brief 函数调用操作符
     * @param args 信号参数
     */
    template <typename... Args>
    void operator()(Args&&... args) {
        this->emit(_NEFORCE forward<Args>(args)...);
    }

    /**
     * @brief 断开所有连接
     */
    void disconnect_all() {
        this->with_lock([this] { slots_.clear(); });
    }

    /**
     * @brief 连接另一个信号
     * @param other 另一个信号
     * @param priority 优先级
     * @return 连接句柄
     *
     * 当此信号触发时，会转发给other信号。
     */
    connection connect_signal(signal& other, int priority = 0) {
        return this->connect(
                [other_ptr = &other](Types... args) {
                    other_ptr->emit(args...);
                    return callback_result::keep;
                },
                priority);
    }

    /**
     * @brief 连接另一个信号（指针版本）
     * @param other 另一个信号指针
     * @param priority 优先级
     * @return 连接句柄
     *
     * 当此信号触发时，如果指针非空，则转发给other信号。
     */
    connection connect_signal(signal* other, int priority = 0) {
        return this->connect(
                [other](Types... args) {
                    if (other) {
                        other->emit(args...);
                    }
                    return callback_result::keep;
                },
                priority);
    }

    /**
     * @brief 条件连接
     * @tparam Func 回调函数类型
     * @tparam Predicate 谓词类型
     * @param callback 回调函数
     * @param pred 谓词
     * @param priority 优先级
     * @return 连接句柄
     *
     * 仅当谓词返回true时才调用回调。
     */
    template <typename Func, typename Predicate>
    connection connect_if(Func callback, Predicate pred, int priority = 0) {
        using result_type = invoke_result_t<Predicate, Types...>;
        static_assert(is_boolean_v<result_type>, "only boolean results are allowed");

        return this->connect(
                [callback = _NEFORCE move(callback), pred = _NEFORCE move(pred)](Types... args) mutable {
                    if (pred(args...)) {
                        callback(_NEFORCE forward<Types>(args)...);
                    }
                    return callback_result::keep;
                },
                priority);
    }

    /**
     * @brief 条件连接（成员函数版本）
     * @tparam Self 对象类型
     * @tparam MemFn 成员函数类型
     * @tparam Predicate 谓词类型
     * @tparam Tag 连接标签类型
     * @param self 对象指针/智能指针
     * @param memfn 成员函数指针
     * @param pred 谓词
     * @param priority 优先级
     * @param tag 连接标签
     * @return 连接句柄
     */
    template <typename Self, typename MemFn, typename Predicate, typename... Tag>
    connection connect_if(Self self, MemFn memfn, Predicate pred, int priority, Tag... tag) {
        using result_type = invoke_result_t<Predicate, Types...>;
        static_assert(is_boolean_v<result_type>, "only boolean results are allowed");
        static_assert(sizeof...(Tag) <= 1, "only zero or one tag is allowed");

        auto bound = this->bind(_NEFORCE move(self), memfn, tag...);

        return this->connect(
                [bound = _NEFORCE move(bound), pred = _NEFORCE move(pred)](Types... args) mutable {
                    if (pred(args...)) {
                        return bound(_NEFORCE forward<Types>(args)...);
                    }
                    return callback_result::keep;
                },
                priority);
    }

    /**
     * @brief 过滤连接
     * @tparam Func 回调函数类型
     * @tparam Filter 过滤器类型
     * @param callback 回调函数
     * @param filter 过滤器
     * @param priority 优先级
     * @return 连接句柄
     *
     * 过滤器可以修改或抑制参数。
     */
    template <typename Func, typename Filter>
    connection connect_filtered(Func callback, Filter filter, int priority = 0) {
        using result_type = invoke_result_t<Filter, Types...>;
        static_assert(is_optional_v<result_type>, "only optional results are allowed");

        return this->connect(
                [callback = _NEFORCE move(callback), filter = _NEFORCE move(filter)](Types... args) mutable {
                    auto filtered = filter(args...);
                    if (filtered) {
                        _NEFORCE apply(
                                [&callback](auto&&... filtered_args) {
                                    callback(_NEFORCE forward<decltype(filtered_args)>(filtered_args)...);
                                },
                                _NEFORCE move(*filtered));
                    }
                    return callback_result::keep;
                },
                priority);
    }

    /**
     * @brief 变换连接
     * @tparam Func 回调函数类型
     * @tparam Transform 变换函数类型
     * @param callback 回调函数
     * @param transform 变换函数
     * @param priority 优先级
     * @return 连接句柄
     *
     * 变换函数将参数转换为新类型后传递给回调。
     */
    template <typename Func, typename Transform>
    connection connect_transformed(Func callback, Transform transform, int priority = 0) {
        static_assert(is_invocable_v<Transform, Types...>, "only function inputs are allowed");

        return this->connect(
                [callback = _NEFORCE move(callback), transform = _NEFORCE move(transform)](Types... args) mutable {
                    callback(transform(_NEFORCE forward<Types>(args)...));
                    return callback_result::keep;
                },
                priority);
    }

    /**
     * @brief 获取阻塞标志
     * @return 阻塞标志的共享指针
     */
    shared_ptr<bool> block_flag() const noexcept { return blocked_flag_; }

    /**
     * @brief 检查信号是否被阻塞
     * @return 是否被阻塞
     */
    bool is_blocked() const noexcept { return blocked_flag_ && *blocked_flag_; }

    /**
     * @brief 获取活跃槽的数量
     * @return 槽数量
     */
    size_t slot_count() const noexcept {
        return this->with_lock([this] { return slot_count_unlocked(); });
    }

    /**
     * @brief 检查信号是否为空（无活跃槽）
     * @return 是否为空
     */
    NEFORCE_NODISCARD bool empty() const noexcept {
        return this->with_lock([this] { return slot_count_unlocked() == 0; });
    }
};

/** @} */ // Signals

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_CORE_ASYNC_SIGNALS_HPP__
