#ifndef NEFORCE_CORE_ASYNC_BARRIER_HPP__
#define NEFORCE_CORE_ASYNC_BARRIER_HPP__

/**
 * @file barrier.hpp
 * @brief 屏障实现
 *
 * 此文件提供了屏障的实现，用于协调多个线程的同步点。
 * 支持树形屏障算法，提供高性能的线程同步机制。
 */

#include "NeForce/core/async/atomic_base.hpp"
#include "NeForce/core/async/thread.hpp"
#include "NeForce/core/container/array.hpp"
NEFORCE_BEGIN_NAMESPACE__

/**
 * @defgroup AsyncComponents 异步组件
 * @brief 异步编程相关组件
 * @{
 */

/**
 * @defgroup ThreadSync 线程同步
 * @brief 线程同步组件实现
 * @{
 */

/**
 * @struct empty_completion
 * @brief 空完成函数
 *
 * 默认的屏障完成函数，不执行任何操作。
 */
struct empty_completion {
    NEFORCE_ALWAYS_INLINE void operator()() noexcept {}
};


/**
 * @class tree_barrier
 * @brief 树形屏障
 * @tparam CmplFunc 完成函数类型
 *
 * 基于树形算法的屏障实现，使用分层的票证机制减少竞争。
 * 提供高性能的多线程同步，支持动态调整参与线程数。
 */
template <typename CmplFunc>
class tree_barrier {
    using phase_ref_t = atomic_ref_base<byte_t>;        ///< 阶段引用类型
    using phase_cref_t = atomic_ref_base<const byte_t>; ///< 阶段常量引用类型

    static constexpr auto phase_alignment = phase_ref_t::required_alignment; ///< 阶段对齐要求

    /**
     * @struct state_data
     * @brief 状态数据结构
     *
     * 64字节对齐，减少缓存行伪共享。
     */
    struct alignas(64) state_data {
        alignas(phase_alignment) array<byte_t, 64> tickets; ///< 票证数组
    };

    ptrdiff_t expected_count_;                         ///< 期望的参与线程数
    unique_ptr<state_data[]> state_array_;             ///< 状态数组指针
    atomic_base<ptrdiff_t> expected_adjustment_{0};    ///< 期望调整值
    CmplFunc completion_function_;                     ///< 完成函数
    alignas(phase_alignment) byte_t current_phase_{0}; ///< 当前阶段值

    /**
     * @brief 执行到达操作
     * @param old_phase 旧阶段值
     * @param current_index 当前线程索引
     * @return 是否完成本轮屏障
     *
     * 树形屏障算法的核心实现，使用多轮票证交换机制。
     */
    bool do_arrive(const byte_t old_phase, size_t current_index) {
        const auto old_phase_value = old_phase;
        const auto half_step = static_cast<byte_t>(old_phase_value + 1);
        const auto full_step = static_cast<byte_t>(old_phase_value + 2);

        size_t current_expected = expected_count_;
        current_index %= ((expected_count_ + 1) >> 1);

        for (int round = 0;; ++round) {
            if (current_expected <= 1) {
                return true;
            }

            size_t const end_node = ((current_expected + 1) >> 1), last_node = end_node - 1;

            for (;; ++current_index) {
                if (current_index == end_node) {
                    current_index = 0;
                }

                auto expected_phase = old_phase;
                phase_ref_t phase_ref(state_array_[current_index].tickets[round]);

                // NOLINTNEXTLINE(bugprone-branch-clone)
                if (current_index == last_node && ((current_expected & 1) != 0U)) {
                    if (phase_ref.compare_exchange_strong(expected_phase, full_step, memory_order_acq_rel)) {
                        break;
                    }
                } else if (phase_ref.compare_exchange_strong(expected_phase, half_step, memory_order_acq_rel)) {
                    return false;
                } else if (expected_phase == half_step) {
                    if (phase_ref.compare_exchange_strong(expected_phase, full_step, memory_order_acq_rel)) {
                        break;
                    }
                }
            }

            current_expected = last_node + 1;
            current_index >>= 1;
        }
    }

public:
    using arrival_token = byte_t; ///< 到达令牌类型

    /**
     * @brief 获取最大线程数
     * @return 支持的最大线程数
     */
    static constexpr ptrdiff_t max() noexcept { return numeric_traits<ptrdiff_t>::max(); }

    /**
     * @brief 构造函数
     * @param expected 期望的参与线程数
     * @param completion 完成函数
     */
    tree_barrier(const ptrdiff_t expected, CmplFunc completion) :
    expected_count_(expected),
    completion_function_(_NEFORCE move(completion)) {
        size_t const count = (expected_count_ + 1) >> 1;
        state_array_ = make_unique<state_data[]>(count);
    }

    /**
     * @brief 到达屏障点
     * @param update 到达线程数
     * @return 到达令牌
     *
     * 线程到达屏障点，可能触发完成函数并进入下一阶段。
     * 支持批量到达。
     */
    NEFORCE_NODISCARD arrival_token arrive(ptrdiff_t update) {
        constexpr hash<thread::id> hasher;
        const size_t current_index = hasher(this_thread::id());
        phase_ref_t phase_ref(current_phase_);
        const auto old_phase = phase_ref.load(memory_order_relaxed);
        const auto current_phase_value = static_cast<byte_t>(old_phase);

        for (; update != 0; --update) {
            if (do_arrive(old_phase, current_index)) {
                completion_function_();
                expected_count_ += expected_adjustment_.load(memory_order_relaxed);
                expected_adjustment_.store(0, memory_order_relaxed);
                const auto new_phase = static_cast<byte_t>(current_phase_value + 2);
                phase_ref.store(new_phase, memory_order_release);
                phase_ref.notify_all();
            }
        }
        return old_phase;
    }

    /**
     * @brief 等待屏障
     * @param old_phase 到达令牌
     *
     * 等待屏障进入下一阶段，使用原子等待机制避免忙等待。
     */
    void wait(arrival_token&& old_phase) const {
        phase_cref_t phase_ref(current_phase_);
        auto const test_function = [=] { return phase_ref.load(memory_order_acquire) != old_phase; };
        _NEFORCE atomic_wait_address(&current_phase_, test_function);
    }

    /**
     * @brief 到达并退出
     *
     * 线程到达屏障点后退出参与，减少期望线程数。
     */
    void arrive_and_drop() {
        expected_adjustment_.fetch_sub(1, memory_order_relaxed);
        ignore = arrive(1);
    }
};


/**
 * @class barrier
 * @brief 通用屏障类
 * @tparam CmplFunc 完成函数类型
 *
 * 屏障的用户友好接口，包装树形屏障实现。
 * 提供类型安全的到达令牌和简化API。
 */
template <typename CmplFunc = empty_completion>
class barrier {
    using algorithm_type = tree_barrier<CmplFunc>; ///< 底层算法类型
    algorithm_type barrier_impl_;                  ///< 屏障实现实例

public:
    /**
     * @class arrival_token
     * @brief 到达令牌类
     *
     * 表示线程到达屏障的凭证，用于后续等待操作。
     * 确保类型安全和正确的生命周期管理。
     */
    class arrival_token final {
    public:
        arrival_token(arrival_token&&) = default;            ///< 移动构造函数
        arrival_token& operator=(arrival_token&&) = default; ///< 移动赋值运算符
        ~arrival_token() = default;                          ///< 析构函数

    private:
        friend class barrier;
        using internal_token = typename algorithm_type::arrival_token;
        explicit arrival_token(internal_token token) noexcept :
        token_(token) {}
        internal_token token_;
    };

    /**
     * @brief 获取最大线程数
     * @return 支持的最大线程数
     */
    static constexpr ptrdiff_t max() noexcept { return algorithm_type::max(); }

    /**
     * @brief 构造函数
     * @param count 期望的参与线程数
     * @param completion 完成函数
     */
    explicit barrier(ptrdiff_t count, CmplFunc completion = CmplFunc()) :
    barrier_impl_(count, _NEFORCE move(completion)) {}

    barrier(barrier const&) = delete;
    barrier& operator=(barrier const&) = delete;

    /**
     * @brief 到达屏障点
     * @param update 到达线程数
     * @return 到达令牌
     */
    NEFORCE_NODISCARD arrival_token arrive(ptrdiff_t update = 1) { return arrival_token{barrier_impl_.arrive(update)}; }

    /**
     * @brief 等待屏障
     * @param phase 到达令牌
     */
    void wait(arrival_token&& phase) const { barrier_impl_.wait(_NEFORCE move(phase.token_)); }

    /**
     * @brief 到达并等待
     *
     * 组合操作：到达屏障点并等待所有线程到达。
     */
    void arrive_and_wait() { this->wait(arrive()); }

    /**
     * @brief 到达并退出
     *
     * 线程到达屏障点后退出参与。
     */
    void arrive_and_drop() { barrier_impl_.arrive_and_drop(); }
};

/** @} */ // ThreadSync

/** @} */ // AsyncComponents

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_CORE_ASYNC_BARRIER_HPP__
