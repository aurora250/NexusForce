#ifndef NEFORCE_CORE_ASYNC_ATOMIC_WAIT_HPP__
#define NEFORCE_CORE_ASYNC_ATOMIC_WAIT_HPP__

/**
 * @file atomic_wait.hpp
 * @brief 原子等待/通知机制
 *
 * 此文件提供了基于FUTEX的原子等待和通知机制，支持高效的线程同步。
 */

#include "NeForce/core/async/futex.hpp"
#include "NeForce/core/async/this_thread.hpp"
#include "NeForce/core/memory/memory.hpp"
NEFORCE_BEGIN_NAMESPACE__

/**
 * @defgroup AtomicOperations 原子操作
 * @brief 原子变量的操作
 * @{
 */

/**
 * @struct default_spin_policy
 * @brief 默认自旋策略
 *
 * 总是返回false，表示不进行额外的自旋等待。
 */
struct default_spin_policy {
    bool operator()() const noexcept { return false; }
};

/**
 * @brief 原子自旋等待
 * @tparam Pred 谓词类型
 * @tparam Spin 自旋策略类型
 * @param pred 等待条件谓词，当返回true时停止等待
 * @param spin 自旋策略对象
 * @return 是否因为谓词为true而停止等待
 *
 * 实现自旋等待机制：
 * 1. 前16次循环中快速检查谓词
 * 2. 前12次使用relax
 * 3. 后4次使用yield
 * 4. 然后根据自旋策略继续等待
 */
template <typename Pred, typename Spin = default_spin_policy>
bool atomic_spin(Pred& pred, Spin spin = Spin{}) noexcept {
    constexpr auto atomic_spin_count = 16;
    constexpr auto atomic_spin_count_relax = 12;
    for (auto idx = 0; idx < atomic_spin_count; ++idx) {
        if (pred()) {
            return true;
        }
        if (idx < atomic_spin_count_relax) {
            this_thread::relax();
        } else {
            this_thread::yield();
        }
    }

    while (spin()) {
        if (pred()) {
            return true;
        }
    }
    return false;
}

/// @cond
NEFORCE_BEGIN_INNER__

/**
 * @struct waiter_pool_base
 * @brief 等待器池基类
 *
 * 管理等待线程的池，包含等待计数和共享值。
 * 使用哈希策略将不同地址映射到有限的池槽中。
 */
struct waiter_pool_base {
    static constexpr auto align_inner = 64; ///< 缓存行对齐大小，避免伪共享

    alignas(align_inner) platform_wait_t wait = 0;  ///< 等待计数，表示有多少线程在等待
    alignas(align_inner) platform_wait_t value = 0; ///< 共享值，用于比较

    waiter_pool_base() = default;

    /**
     * @brief 进入等待状态
     *
     * 原子地增加等待计数。
     */
    void waiter_enter_wait() noexcept {
#ifdef NEFORCE_PLATFORM_WINDOWS
        ::_InterlockedIncrement(&wait);
#else
        __atomic_fetch_add(&wait, 1, __ATOMIC_SEQ_CST);
#endif
    }

    /**
     * @brief 离开等待状态
     *
     * 原子地减少等待计数。
     */
    void waiter_leave_wait() noexcept {
#ifdef NEFORCE_PLATFORM_WINDOWS
        ::_InterlockedDecrement(&wait);
#else
        __atomic_fetch_sub(&wait, 1, __ATOMIC_RELEASE);
#endif
    }

    /**
     * @brief 检查是否有线程在等待
     * @return 是否有等待的线程
     */
    bool waiter_waiting() const noexcept {
#ifdef NEFORCE_PLATFORM_WINDOWS
        platform_wait_t res = ::_InterlockedExchangeAdd(const_cast<volatile platform_wait_t*>(&wait), 0);
#else
        platform_wait_t res;
        __atomic_load(&wait, &res, __ATOMIC_SEQ_CST);
#endif
        return res != 0;
    }

    /**
     * @brief 通知等待的线程
     * @param addr 要通知的地址
     * @param all 是否通知所有线程
     * @param bare 是否为裸通知，也即不检查等待状态
     */
    void waiter_notify(platform_wait_t* addr, bool all, const bool bare) const noexcept {
        if (addr == &value) {
#ifdef NEFORCE_PLATFORM_WINDOWS
            ::_InterlockedIncrement(addr);
#else
            __atomic_fetch_add(addr, 1, __ATOMIC_SEQ_CST);
#endif
            all = true;
        }
        if (bare || waiter_waiting()) {
            futex_notify(addr, all);
        }
    }

    /**
     * @brief 获取地址对应的等待器
     * @param addr 原子变量地址
     * @return 对应的等待器引用
     *
     * 使用简单的哈希策略将地址映射到有限的池中。
     */
    static waiter_pool_base& waiter_for(const void* addr) noexcept {
        constexpr uintptr_t pool_size = 16;
        static waiter_pool_base waiter[pool_size];
        const auto key = (reinterpret_cast<uintptr_t>(addr) >> 2) % pool_size;
        return waiter[key];
    }
};

/**
 * @struct waiter_pool
 * @brief 等待器池
 *
 * 扩展等待器池基类。
 */
struct waiter_pool : waiter_pool_base {
    /**
     * @brief 执行等待操作
     * @param addr 等待地址
     * @param old 期望的值
     */
    NEFORCE_ALWAYS_INLINE void waiter_do_wait(platform_wait_t* addr, const platform_wait_t old) const noexcept {
        _NEFORCE futex_wait(addr, old);
    }
};


/**
 * @struct waiter_base
 * @brief 等待器基类模板
 * @tparam T 等待器类型
 *
 * 提供等待操作的基本框架，处理地址映射和自旋逻辑。
 */
template <typename T>
struct waiter_base {
private:
    /**
     * @brief 检查类型是否适用于平台等待操作
     * @tparam U 要检查的类型
     *
     * 类型必须满足以下条件：
     * 1. 标量类型
     * 2. 大小等于平台等待类型的大小
     * 3. 对齐要求不低于平台等待类型的对齐要求
     */
    template <typename U>
    static constexpr bool platform_wait_valid_v =
            is_scalar_v<U> && sizeof(U) == sizeof(platform_wait_t) && alignof(U*) >= alignof(platform_wait_t);

    template <typename U, enable_if_t<platform_wait_valid_v<U>, int> = 0>
    NEFORCE_ALWAYS_INLINE static void waiter_do_spin_v_impl(platform_wait_t*, const U& old, platform_wait_t& value) {
        _NEFORCE memory_copy(&value, &old, sizeof(value));
    }
    template <typename U, enable_if_t<!platform_wait_valid_v<U>, int> = 0>
    NEFORCE_ALWAYS_INLINE static void waiter_do_spin_v_impl(platform_wait_t* addr, const U&, platform_wait_t& value) {
#ifdef NEFORCE_PLATFORM_WINDOWS
        value = ::_InterlockedExchangeAdd(addr, 0);
#else
        __atomic_load(addr, &value, __ATOMIC_ACQUIRE);
#endif
    }

public:
    using waiter_type = T; ///< 等待器类型

    waiter_type& waiter_;   ///< 引用的等待器
    platform_wait_t* addr_; ///< 等待地址

    /**
     * @brief 获取平台等待有效的类型的等待地址
     */
    template <typename U>
    static enable_if_t<platform_wait_valid_v<U>, platform_wait_t*> waiter_wait_addr(const U* addr, platform_wait_t*) {
        return reinterpret_cast<platform_wait_t*>(const_cast<U*>(addr));
    }

    /**
     * @brief 获取平台等待无效的类型的等待地址
     */
    template <typename U>
    static enable_if_t<!platform_wait_valid_v<U>, platform_wait_t*> waiter_wait_addr(const U*, platform_wait_t* wait) {
        return wait;
    }

    /**
     * @brief 获取地址对应的等待器
     */
    static waiter_type& waiter_for(const void* addr) noexcept {
        static_assert(sizeof(waiter_type) == sizeof(waiter_pool_base),
                      "waiter_for should be same size with waiter_pool_base");
        auto& res = waiter_pool_base::waiter_for(addr);
        return reinterpret_cast<waiter_type&>(res);
    }

    /**
     * @brief 构造函数
     * @tparam U 地址类型
     * @param addr 原子变量地址
     */
    template <typename U>
    explicit waiter_base(const U* addr) noexcept :
    waiter_(waiter_base::waiter_for(addr)),
    addr_(waiter_base::waiter_wait_addr(addr, &waiter_.value)) {}

    /**
     * @brief 通知等待的线程
     * @param all 是否通知所有线程
     * @param bare 是否为裸通知
     */
    void waiter_notify(bool all, bool bare = false) noexcept { waiter_.waiter_notify(addr_, all, bare); }

    /**
     * @brief 执行带值的自旋等待
     */
    template <typename U, typename Func, typename Spin = default_spin_policy>
    static bool waiter_do_spin_v(platform_wait_t* addr, const U& old, Func f, platform_wait_t& value,
                                 Spin spin = Spin{}) {
        auto const pred = [=] { return _NEFORCE memory_compare<U>(old, f()) != 0; };
        waiter_base::waiter_do_spin_v_impl(addr, old, value);
        return _NEFORCE atomic_spin(pred, spin);
    }

    /**
     * @brief 执行带值的自旋等待
     */
    template <typename U, typename Func, typename Spin = default_spin_policy>
    bool waiter_do_spin_v(const U& old, Func f, platform_wait_t& value, Spin spin = Spin{}) {
        return waiter_base::waiter_do_spin_v(addr_, old, f, value, spin);
    }

    /**
     * @brief 执行自定义谓词的自旋等待
     */
    template <typename Pred, typename Spin = default_spin_policy>
    static bool waiter_do_spin(const platform_wait_t* addr, Pred pred, platform_wait_t& value, Spin spin = Spin{}) {
#ifdef NEFORCE_PLATFORM_WINDOWS
        value = ::_InterlockedExchangeAdd(const_cast<volatile LONG*>(addr), 0);
#else
        __atomic_load(addr, &value, __ATOMIC_ACQUIRE);
#endif
        return _NEFORCE atomic_spin(pred, spin);
    }

    /**
     * @brief 执行自定义谓词的自旋等待
     */
    template <typename Pred, typename Spin = default_spin_policy>
    bool waiter_do_spin(Pred pred, platform_wait_t& value, Spin spin = Spin{}) {
        return waiter_base::waiter_do_spin(addr_, pred, value, spin);
    }
};


/**
 * @struct waiter
 * @brief 等待器模板
 * @tparam EntersWait 是否进入等待状态的标签
 *
 * 完整的等待器实现，根据EntersWait标签决定是否更新等待计数。
 */
template <typename EntersWait>
struct waiter : waiter_base<waiter_pool> {
public:
    using base_type = waiter_base<waiter_pool>;

private:
    template <bool Wait = EntersWait::value, enable_if_t<Wait, int> = 0>
    NEFORCE_ALWAYS_INLINE void enter() const noexcept {
        waiter_.waiter_enter_wait();
    }
    template <bool Wait = EntersWait::value, enable_if_t<!Wait, int> = 0>
    NEFORCE_ALWAYS_INLINE void enter() const noexcept {}

    template <bool Wait = EntersWait::value, enable_if_t<Wait, int> = 0>
    NEFORCE_ALWAYS_INLINE void leave() const noexcept {
        waiter_.waiter_leave_wait();
    }
    template <bool Wait = EntersWait::value, enable_if_t<!Wait, int> = 0>
    NEFORCE_ALWAYS_INLINE void leave() const noexcept {}

public:
    /**
     * @brief 构造函数
     * @tparam T 地址类型
     * @param addr 原子变量地址
     */
    template <typename T>
    explicit waiter(const T* addr) noexcept :
    base_type(addr) {
        enter();
    }

    /**
     * @brief 析构函数
     */
    ~waiter() { leave(); }

    /**
     * @brief 执行带值的等待操作
     * @tparam T 值类型
     * @tparam Func 获取当前值的函数类型
     * @param old 期望的旧值
     * @param f 获取当前值的函数
     *
     * 循环执行：自旋等待 -> 如果条件不满足则进行FUTEX等待
     */
    template <typename T, typename Func>
    void waiter_do_wait_v(T old, Func f) {
        do {
            platform_wait_t value;
            if (base_type::waiter_do_spin_v(old, f, value)) {
                return;
            }
            waiter_.waiter_do_wait(base_type::addr_, value);
        } while (_NEFORCE memory_compare<T>(old, f()) == 0);
    }

    /**
     * @brief 执行自定义谓词的等待操作
     * @tparam Pred 谓词类型
     * @param pred 等待条件谓词
     */
    template <typename Pred>
    void waiter_do_wait(Pred pred) noexcept {
        do {
            platform_wait_t value;
            if (base_type::waiter_do_spin(pred, value)) {
                return;
            }
            waiter_.waiter_do_wait(base_type::addr_, value);
        } while (!pred());
    }
};

/// 进入等待的等待器类型
using enters_wait = waiter<true_type>;

/// 裸等待器类型，不更新等待计数
using bare_wait = waiter<false_type>;

NEFORCE_END_INNER__
/// @endcond

/**
 * @brief 基于值的原子等待
 * @tparam T 值类型
 * @tparam Func 获取当前值的函数类型
 * @param addr 原子变量地址
 * @param old 期望的值
 * @param f 获取当前值的函数
 *
 * 等待直到addr处的值不等于old。
 */
template <typename T, typename Func>
void atomic_wait_address_v(const T* addr, T old, Func f) noexcept {
    inner::enters_wait waiter(addr);
    waiter.waiter_do_wait_v(old, f);
}

/**
 * @brief 基于谓词的原子等待
 * @tparam T 地址类型
 * @tparam Pred 谓词类型
 * @param addr 原子变量地址
 * @param pred 等待条件谓词，当返回true时停止等待
 *
 * 等待直到pred()返回true。
 */
template <typename T, typename Pred>
void atomic_wait_address(const T* addr, Pred pred) noexcept {
    inner::enters_wait waiter(addr);
    waiter.waiter_do_wait(pred);
}

/**
 * @brief 原子通知
 * @tparam T 地址类型
 * @param addr 原子变量地址
 * @param all 是否通知所有等待线程
 *
 * 唤醒等待addr处值变化的线程。
 * 使用裸等待器。
 */
template <typename T>
void atomic_notify_address(const T* addr, const bool all) noexcept {
    inner::bare_wait waiter(addr);
    waiter.waiter_notify(all);
}

/** @} */ // AtomicOperations

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_CORE_ASYNC_ATOMIC_WAIT_HPP__
