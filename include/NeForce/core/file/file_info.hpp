#ifndef NEFORCE_CORE_FILE_FILE_INFO_HPP__
#define NEFORCE_CORE_FILE_FILE_INFO_HPP__

/**
 * @file file_info.hpp
 * @brief 文件属性与时间信息管理
 *
 * 此文件提供了查询和修改已打开文件属性的功能，包括文件大小、
 * 权限属性、访问时间、修改时间等信息。支持跨平台的属性操作。
 */

#include "NeForce/core/time/datetime.hpp"
#include "NeForce/core/file/file_constants.hpp"
NEFORCE_BEGIN_NAMESPACE__

/**
 * @defgroup File 文件
 * @brief 文件操作
 * @{
 */

/**
 * @class file_info
 * @brief 文件属性与时间管理类
 *
 * 查询和修改已打开文件的属性、权限及时间戳。
 *
 * @note 不持有文件句柄所有权，句柄生命周期由调用方保证。
 */
class NEFORCE_API file_info {
public:
    using native_handle_type = _NEFORCE native_handle_type; ///< 原生文件句柄类型

#ifdef NEFORCE_PLATFORM_WINDOWS
    using size_type = ::DWORD;  ///< 大小类型
#else
    using size_type = size_t;   ///< 大小类型
#endif

private:
    native_handle_type handle_; ///< 文件句柄

public:
    /**
     * @brief 构造函数
     * @param handle 已打开的文件句柄
     *
     * 关联指定的文件句柄。
     */
    explicit file_info(native_handle_type handle) noexcept;

    file_info(const file_info&) = delete;
    file_info& operator =(const file_info&) = delete;

    /**
     * @brief 获取文件属性
     * @return 文件属性标志
     *
     * 返回文件的属性信息。
     * 如果获取失败，返回file_attri::OTHERS。
     */
    NEFORCE_NODISCARD file_attri attributes() const noexcept;

    /**
     * @brief 设置文件属性
     * @param attr 要设置的属性标志
     * @return 设置成功返回true，失败返回false
     * @note 可能需要权限。
     */
    bool set_attributes(file_attri attr) noexcept;

    /**
     * @brief 获取文件大小
     * @return 文件大小（字节），如果文件超过4GB则返回0
     *
     * 获取文件大小，适用于小于4GB的文件。
     * 对于大文件，建议使用size64()方法。
     */
    NEFORCE_NODISCARD size_type size() const noexcept;

    /**
     * @brief 获取文件大小
     * @return 文件大小（字节），支持超过4GB的大文件
     *
     * 获取文件大小的64位版本，支持大文件处理。
     */
    NEFORCE_NODISCARD uint64_t size64() const noexcept;

    /**
     * @brief 获取文件大小
     * @param out_size 输出文件大小
     * @return 成功返回true，失败返回false
     *
     * 获取文件大小并返回操作状态，可检测文件是否超过32位范围。
     * 对于超过32位范围的文件，返回false。
     */
    bool size(size_type& out_size) const noexcept;

    /**
     * @brief 获取最后访问时间
     * @return 最后访问时间
     *
     * 返回文件最后一次被读取或执行的时间。
     * 如果获取失败，返回epoch时间。
     */
    NEFORCE_NODISCARD datetime last_access_time() const noexcept;

    /**
     * @brief 设置最后访问时间
     * @param dt 要设置的时间
     * @return 设置成功返回true，失败返回false
     *
     * 修改文件的最后访问时间。
     *
     * @note 可能需要权限。
     */
    bool set_last_access_time(const datetime& dt) noexcept;

    /**
     * @brief 获取最后修改时间
     * @return 最后修改时间
     *
     * 返回文件内容最后一次被修改的时间。
     * 如果获取失败，返回epoch时间。
     */
    NEFORCE_NODISCARD datetime last_write_time() const noexcept;

    /**
     * @brief 设置最后修改时间
     * @param dt 要设置的时间
     * @return 设置成功返回true，失败返回false
     *
     * 修改文件的最后修改时间。
     *
     * @note 可能需要权限。
     */
    bool set_last_write_time(const datetime& dt) noexcept;

#ifdef NEFORCE_PLATFORM_WINDOWS
    /**
     * @brief 获取创建时间
     * @return 文件创建时间
     *
     * 返回文件的创建时间。
     * 如果获取失败，返回epoch时间。
     */
    NEFORCE_NODISCARD datetime creation_time() const noexcept;

    /**
     * @brief 设置创建时间
     * @param dt 要设置的时间
     * @return 设置成功返回true，失败返回false
     *
     * 修改文件的创建时间。
     *
     * @note 可能需要权限。
     */
    bool set_creation_time(const datetime& dt) noexcept;
#endif

#ifdef NEFORCE_PLATFORM_WINDOWS
    /**
     * @brief 同时设置所有三个时间戳
     * @param create 创建时间
     * @param access 最后访问时间
     * @param write 最后修改时间
     * @return 设置成功返回true，失败返回false
     *
     * 一次性设置文件的创建时间、访问时间和修改时间。
     */
    bool set_all_times(const datetime& create, const datetime& access, const datetime& write) noexcept;
#else
    /**
     * @brief 同时设置访问和修改时间
     * @param access 最后访问时间
     * @param write 最后修改时间
     * @return 设置成功返回true，失败返回false
     *
     * 一次性设置文件的访问时间和修改时间。
     */
    bool set_all_times(const datetime& access, const datetime& write) noexcept;
#endif
};

/** @} */ // File

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_CORE_FILE_FILE_INFO_HPP__
