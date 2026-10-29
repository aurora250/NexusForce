#ifndef NEFORCE_CORE_ASYNC_HEAP_TIMER_HPP__
#define NEFORCE_CORE_ASYNC_HEAP_TIMER_HPP__

/**
 * @file heap_timer.hpp
 * @brief 最小堆定时器
 *
 * 按到期时间排序的定时器容器：
 * 插入与惰性取消均为 O(log n)，取最早到期时间与排空到期项在锁内完成。
 */

#include "NeForce/core/async/atomic.hpp"
#include "NeForce/core/async/mutex.hpp"
#include "NeForce/core/container/flat_unordered_map.hpp"
#include "NeForce/core/container/vector.hpp"
#include "NeForce/core/time/clocks.hpp"
NEFORCE_BEGIN_NAMESPACE__

/**
 * @defgroup AsyncComponents 异步组件
 * @brief 异步编程相关组件
 * @{
 */

/**
 * @defgroup Timer 定时器
 * @brief 定时器容器
 * @{
 */

/**
 * @class heap_timer
 * @brief 最小堆定时器
 * @note 到期时间的比较只精确到时钟的 tick；同一 tick 内的多项按插入顺序排队
 * @warning payload 由调用方解释，容器不持有其生命周期；取消或排空后调用方需自行释放资源
 */
class NEFORCE_API heap_timer {
public:
    /**
     * @brief 定时器标识符类型
     */
    using token = uint64_t;

    /**
     * @struct expired_entry
     * @brief 已到期条目
     */
    struct expired_entry {
        token id;          ///< 插入时返回的标识符
        uintptr_t payload; ///< 插入时携带的不透明载荷
    };

    heap_timer() = default;
    ~heap_timer() = default;

    heap_timer(const heap_timer&) = delete;
    heap_timer& operator=(const heap_timer&) = delete;

    /**
     * @brief 插入一个定时项
     * @param deadline 绝对到期时间
     * @param payload 调用方载荷，容器原样返回
     * @return 定时项标识符
     */
    token insert(const steady_clock::time_point& deadline, uintptr_t payload = 0);

    /**
     * @brief 取消定时项（惰性删除）
     * @param id 定时项标识符
     * @return 定时项仍在等待中并成功标记取消返回 true；已触发或已取消返回 false
     */
    bool cancel(token id);

    /**
     * @brief 获取最早到期时间（精确值）
     * @param out 输出最早到期时间
     * @return 存在待触发的定时项返回 true
     */
    NEFORCE_NODISCARD bool next_deadline(steady_clock::time_point& out) const;

    /**
     * @brief 获取最早到期时间（无锁提示值）
     * @return 最早到期时间距 steady_clock 纪元的纳秒数，0 表示当前没有待触发项
     */
    NEFORCE_NODISCARD int64_t next_deadline_ns() const noexcept;

    /**
     * @brief 取出已到期的定时项
     * @param now 当前时间
     * @param out 输出数组，容量至少为 max_count
     * @param max_count 本次最多取出的条目数
     * @return 实际取出并写入 out 的条目数
     */
    size_t drain_expired(const steady_clock::time_point& now, expired_entry* out, size_t max_count);

    /**
     * @brief 待触发定时项数量
     */
    NEFORCE_NODISCARD size_t size() const;

    /**
     * @brief 是否没有待触发定时项
     */
    NEFORCE_NODISCARD bool empty() const;

    /**
     * @brief 清空全部定时项
     * @note 被清空的项不会回调，调用方需自行回收 payload 关联的资源
     */
    void clear();

private:
    /**
     * @struct entry
     * @brief 堆内条目
     */
    struct entry {
        steady_clock::time_point deadline; ///< 绝对到期时间
        token id;                          ///< 标识符
        uintptr_t payload;                 ///< 调用方载荷
    };

    static bool later(const entry& lhs, const entry& rhs) noexcept;
    void drop_cancelled_locked();
    void refresh_hint_locked();

    mutable mutex mutex_;                       ///< 保护堆与状态表
    vector<entry> heap_;                        ///< 最小堆
    flat_unordered_map<token, uint8_t> states_; ///< 标识符到标记映射，0 待触发 / 1 已取消
    atomic<int64_t> next_deadline_ns_{0};       ///< 最早到期时间提示
    token next_id_{1};                          ///< 标识符计数器
    size_t live_{0};                            ///< 未取消且未触发的条目数
};

/** @} */ // Timer

/** @} */ // AsyncComponents

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_CORE_ASYNC_HEAP_TIMER_HPP__
