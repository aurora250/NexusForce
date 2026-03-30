#ifndef NEFORCE_CORE_FILE_FILE_INFO_HPP__
#define NEFORCE_CORE_FILE_FILE_INFO_HPP__
#include "NeForce/core/time/datetime.hpp"
#include "NeForce/core/file/file_constants.hpp"
NEFORCE_BEGIN_NAMESPACE__

/**
 * @class file_info
 * @brief 文件属性与时间管理类
 *
 * 查询和修改已打开文件的属性、权限及时间戳。
 * 不持有文件所有权。
 */
class NEFORCE_API file_info {
public:
    using native_handle_type = _NEFORCE native_handle_type;

#ifdef NEFORCE_PLATFORM_WINDOWS
    using size_type          = ::DWORD;
#else
    using size_type          = size_t;
#endif

private:
    native_handle_type handle_;

public:
    explicit file_info(native_handle_type handle) noexcept;

    file_info(const file_info&) = delete;
    file_info& operator =(const file_info&) = delete;

    /**
     * @brief 获取文件属性
     */
    NEFORCE_NODISCARD file_attri attributes() const noexcept;

    /**
     * @brief 设置文件属性
     */
    bool set_attributes(file_attri attr) noexcept;

    /**
     * @brief 获取文件大小
     */
    NEFORCE_NODISCARD size_type size() const noexcept;
    NEFORCE_NODISCARD uint64_t size64() const noexcept;
    bool size(size_type& out_size) const noexcept;

    NEFORCE_NODISCARD datetime last_access_time() const noexcept;
    bool set_last_access_time(const datetime& dt) noexcept;

    NEFORCE_NODISCARD datetime last_write_time() const noexcept;
    bool set_last_write_time(const datetime& dt) noexcept;

#ifdef NEFORCE_PLATFORM_WINDOWS
    NEFORCE_NODISCARD datetime creation_time() const noexcept;
    bool set_creation_time(const datetime& dt) noexcept;
#endif

#ifdef NEFORCE_PLATFORM_WINDOWS
    bool set_all_times(const datetime& create, const datetime& access, const datetime& write) noexcept;
#else
    bool set_all_times(const datetime& access, const datetime& write) noexcept;
#endif
};

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_CORE_FILE_FILE_INFO_HPP__
