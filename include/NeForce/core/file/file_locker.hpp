#ifndef NEFORCE_CORE_FILE_FILE_LOCKER_HPP__
#define NEFORCE_CORE_FILE_FILE_LOCKER_HPP__
#include "NeForce/core/file/file_constants.hpp"
#include "NeForce/core/typeinfo/types.hpp"
NEFORCE_BEGIN_NAMESPACE__

/**
 * @class file_locker
 * @brief 文件区域锁管理类
 *
 * 对文件句柄提供区域级的锁定与解锁操作。
 * 不持有文件所有权，文件句柄的生命周期由调用方保证。
 */
class NEFORCE_API file_locker {
public:
    using native_handle_type = _NEFORCE native_handle_type;
    using difference_type    = int64_t;

private:
    native_handle_type handle_;

public:
    explicit file_locker(native_handle_type handle) noexcept;

    file_locker(const file_locker&) = delete;
    file_locker& operator =(const file_locker&) = delete;

    /**
     * @brief 锁定文件区域
     * @param offset 起始偏移
     * @param length 长度（0 表示到文件末尾）
     * @param mode   锁定模式
     */
    bool lock(difference_type offset, difference_type length,
              file_lock mode = file_lock::EXCLUSIVE) const noexcept;

    /**
     * @brief 解锁文件区域
     */
    bool unlock(difference_type offset, difference_type length) const noexcept;

    /**
     * @brief 尝试锁定
     */
    bool try_lock(difference_type offset, difference_type length, file_lock mode) const noexcept;

    /**
     * @brief 查询区域是否被锁定
     * @param lock_out 若不为 nullptr，输出锁定类型
     */
    NEFORCE_NODISCARD bool is_locked(difference_type offset, difference_type length,
                                     file_lock* lock_out = nullptr) const noexcept;

    /**
     * @brief 锁定整个文件
     */
    bool lock_whole(file_lock mode = file_lock::EXCLUSIVE) const noexcept;

    /**
     * @brief 解锁整个文件
     */
    bool unlock_whole() const noexcept;
};


/**
 * @class file_lock_guard
 * @brief RAII 文件区域锁守卫
 */
class NEFORCE_API file_lock_guard {
public:
    using difference_type = file_locker::difference_type;

private:
    file_locker& locker_;
    difference_type offset_;
    difference_type length_;
    bool locked_ = false;

public:
    file_lock_guard(file_locker& locker, difference_type offset, difference_type length, file_lock mode);
    ~file_lock_guard();

    file_lock_guard(const file_lock_guard&) = delete;
    file_lock_guard& operator =(const file_lock_guard&) = delete;

    NEFORCE_NODISCARD bool is_locked() const noexcept { return locked_; }
    bool unlock() noexcept;
};

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_CORE_FILE_FILE_LOCKER_HPP__
