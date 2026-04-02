#ifndef NEFORCE_CORE_FILE_FILE_LOCKER_HPP__
#define NEFORCE_CORE_FILE_FILE_LOCKER_HPP__

/**
 * @file file_locker.hpp
 * @brief 文件区域锁管理
 *
 * 此文件提供了文件区域锁定功能，
 * 支持对文件的部分区域进行独占或共享锁定。
 *
 * 主要功能：
 * - 区域锁定：锁定文件的指定区域
 * - 区域解锁：解锁已锁定的区域
 * - 尝试锁定：非阻塞的锁定操作
 * - 锁状态查询：检查指定区域是否被锁定
 * - RAII锁守卫：自动管理锁的生命周期
 */

#include "NeForce/core/file/file_constants.hpp"
#include "NeForce/core/typeinfo/types.hpp"
NEFORCE_BEGIN_NAMESPACE__

/**
 * @defgroup File 文件
 * @brief 文件操作
 * @{
 */

/**
 * @class file_locker
 * @brief 文件区域锁管理类
 *
 * 对文件句柄提供区域级的锁定与解锁操作：
 * - 多进程间同步文件访问
 * - 保护文件的关键区域
 * - 实现进程间互斥
 *
 * @note 文件锁是劝告性的（advisory），不强制遵守。进程可以选择忽略锁状态继续访问文件。
 * @note 不持有文件句柄所有权，句柄生命周期由调用方保证。
 */
class NEFORCE_API file_locker {
public:
    using native_handle_type = _NEFORCE native_handle_type; ///< 原生文件句柄类型
    using difference_type = int64_t;                        ///< 偏移量类型

private:
    native_handle_type handle_; ///< 文件句柄

public:
    /**
     * @brief 构造函数
     * @param handle 已打开的文件句柄
     *
     * 关联指定的文件句柄。
     */
    explicit file_locker(native_handle_type handle) noexcept;

    file_locker(const file_locker&) = delete;
    file_locker& operator=(const file_locker&) = delete;

    /**
     * @brief 锁定文件区域
     * @param offset 起始偏移量（字节）
     * @param length 锁定长度（字节），0表示到文件末尾
     * @param mode 锁定模式
     * @return 锁定成功返回true，失败返回false
     *
     * 阻塞等待直到获得锁。
     * 如果length为0，锁定从offset到文件末尾的所有区域。
     */
    bool lock(difference_type offset, difference_type length, file_lock mode = file_lock::EXCLUSIVE) const noexcept;

    /**
     * @brief 解锁文件区域
     * @param offset 起始偏移量
     * @param length 解锁长度，0表示到文件末尾
     * @return 解锁成功返回true，失败返回false
     *
     * 释放指定区域的锁。解锁的区域必须与锁定的区域完全匹配。
     */
    bool unlock(difference_type offset, difference_type length) const noexcept;

    /**
     * @brief 尝试锁定
     * @param offset 起始偏移量
     * @param length 锁定长度
     * @param mode 锁定模式
     * @return 立即获得锁返回true，锁被占用返回false
     *
     * 尝试获取锁，如果锁已被占用则立即返回false，不会阻塞等待。
     */
    bool try_lock(difference_type offset, difference_type length, file_lock mode) const noexcept;

    /**
     * @brief 查询区域是否被锁定
     * @param offset 起始偏移量
     * @param length 查询长度
     * @param lock_out 如果非空，输出当前持有的锁类型
     * @return 区域被锁定返回true，未被锁定返回false
     *
     * 查询指定区域是否被其他进程锁定。
     *
     * @note 由于锁状态随时变化，查询结果仅作参考。
     */
    NEFORCE_NODISCARD bool is_locked(difference_type offset, difference_type length,
                                     file_lock* lock_out = nullptr) const noexcept;

    /**
     * @brief 锁定整个文件
     * @param mode 锁定模式
     * @return 锁定成功返回true
     *
     * 锁定从文件开头到末尾的整个文件。
     */
    bool lock_whole(file_lock mode = file_lock::EXCLUSIVE) const noexcept;

    /**
     * @brief 解锁整个文件
     * @return 解锁成功返回true
     *
     * 解锁整个文件。
     */
    bool unlock_whole() const noexcept;
};


/**
 * @class file_lock_guard
 * @brief 文件区域锁守卫
 *
 * 自动管理文件锁的生命周期，
 * 在构造时获取锁，在析构时自动释放锁。
 */
class NEFORCE_API file_lock_guard {
public:
    using difference_type = file_locker::difference_type; ///< 偏移量类型

private:
    file_locker& locker_;    ///< 文件锁管理器引用
    difference_type offset_; ///< 锁定的起始偏移
    difference_type length_; ///< 锁定的长度
    bool locked_ = false;    ///< 是否持有锁

public:
    /**
     * @brief 构造函数，立即获取锁
     * @param locker 文件锁管理器
     * @param offset 锁定起始偏移
     * @param length 锁定长度
     * @param mode 锁定模式
     *
     * 在构造时尝试获取锁。如果获取失败，locked_标志为false。
     */
    file_lock_guard(file_locker& locker, difference_type offset, difference_type length, file_lock mode);

    /**
     * @brief 析构函数，自动释放锁
     *
     * 如果当前持有锁，自动释放。
     */
    ~file_lock_guard();

    file_lock_guard(const file_lock_guard&) = delete;
    file_lock_guard& operator=(const file_lock_guard&) = delete;

    /**
     * @brief 检查是否持有锁
     * @return 持有锁返回true
     */
    NEFORCE_NODISCARD bool is_locked() const noexcept { return locked_; }

    /**
     * @brief 手动释放锁
     * @return 释放成功返回true
     *
     * 提前释放锁，之后析构函数不会再尝试释放。
     * 释放后is_locked()返回false。
     */
    bool unlock() noexcept;
};

/** @} */ // File

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_CORE_FILE_FILE_LOCKER_HPP__
