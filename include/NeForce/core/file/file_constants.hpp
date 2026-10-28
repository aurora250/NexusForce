#ifndef NEFORCE_CORE_FILE_FILE_CONSTANTS_HPP__
#define NEFORCE_CORE_FILE_FILE_CONSTANTS_HPP__

/**
 * @file file_constants.hpp
 * @brief 文件操作常量定义
 *
 * 此文件定义了文件操作中使用的各种常量和枚举类型，
 * 提供跨平台的文件访问模式、共享模式、创建方式等常量的统一抽象。
 */

#include "NeForce/core/config/windef.hpp"
#include "NeForce/core/typeinfo/types.hpp"
#ifdef NEFORCE_PLATFORM_WINDOWS
#    include <windef.h>
#    include <WinBase.h>
#    ifdef max
#        undef max
#    endif
#    ifdef min
#        undef min
#    endif
#endif
#ifdef NEFORCE_PLATFORM_POSIX
#    include <cstdio>
#    include <fcntl.h>
#    include <sys/stat.h>
#endif
NEFORCE_BEGIN_NAMESPACE__

/**
 * @defgroup File 文件
 * @brief 文件操作
 * @{
 */

/**
 * @enum file_access
 * @brief 文件访问模式枚举
 *
 * 定义文件的打开方式，包括读、写、读写和追加模式。
 */
enum class file_access : native_id_type {
#ifdef NEFORCE_PLATFORM_WINDOWS
    read = GENERIC_READ,                       ///< 只读模式
    write = GENERIC_WRITE,                     ///< 只写模式
    read_write = GENERIC_READ | GENERIC_WRITE, ///< 读写模式
    append = FILE_APPEND_DATA | GENERIC_WRITE  ///< 追加模式
#else
    read = O_RDONLY,             ///< 只读模式
    write = O_WRONLY,            ///< 只写模式
    read_write = O_RDWR,         ///< 读写模式
    append = O_WRONLY | O_APPEND ///< 追加模式
#endif
};

constexpr file_access operator|(file_access a, file_access b) noexcept {
    return static_cast<file_access>(static_cast<native_id_type>(a) | static_cast<native_id_type>(b));
}

constexpr file_access operator&(file_access a, file_access b) noexcept {
    return static_cast<file_access>(static_cast<native_id_type>(a) & static_cast<native_id_type>(b));
}


/**
 * @enum file_shared
 * @brief 文件共享模式枚举
 *
 * 定义文件被其他进程访问时的共享权限。
 */
enum class file_shared : native_id_type {
#ifdef NEFORCE_PLATFORM_WINDOWS
    share_read = FILE_SHARE_READ,                                       ///< 允许其他进程读取
    share_write = FILE_SHARE_WRITE,                                     ///< 允许其他进程写入
    share_read_write = FILE_SHARE_READ | FILE_SHARE_WRITE,              ///< 允许其他进程读写
    share_delete = FILE_SHARE_DELETE,                                   ///< 允许其他进程删除
    share_all = FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, ///< 允许所有操作
    no_share = 0                                                        ///< 不允许共享
#else
    share_read = 1,       ///< 允许其他进程读取
    share_write = 2,      ///< 允许其他进程写入
    share_read_write = 3, ///< 允许其他进程读写
    share_delete = 4,     ///< 允许其他进程删除
    share_all = 7,        ///< 允许所有操作
    no_share = 0          ///< 不允许共享
#endif
};

constexpr file_shared operator|(file_shared a, file_shared b) noexcept {
    return static_cast<file_shared>(static_cast<native_id_type>(a) | static_cast<native_id_type>(b));
}

constexpr file_shared operator&(file_shared a, file_shared b) noexcept {
    return static_cast<file_shared>(static_cast<native_id_type>(a) & static_cast<native_id_type>(b));
}


/**
 * @enum file_creation
 * @brief 文件创建方式枚举
 *
 * 定义文件打开或创建时的行为。
 */
enum class file_creation : native_id_type {
#ifdef NEFORCE_PLATFORM_WINDOWS
    create_force = CREATE_ALWAYS,      ///< 强制创建新文件（覆盖已存在）
    create_no_exist = CREATE_NEW,      ///< 仅当文件不存在时创建
    open_force = OPEN_ALWAYS,          ///< 打开文件，不存在则创建
    open_exist = OPEN_EXISTING,        ///< 仅打开已存在的文件
    truncate_exist = TRUNCATE_EXISTING ///< 打开已存在文件并清空内容
#else
    create_force = O_CREAT | O_TRUNC,   ///< 强制创建新文件（覆盖已存在）
    create_no_exist = O_CREAT | O_EXCL, ///< 仅当文件不存在时创建
    open_force = O_CREAT,               ///< 打开文件，不存在则创建
    open_exist = 0,                     ///< 仅打开已存在的文件
    truncate_exist = O_TRUNC            ///< 打开已存在文件并清空内容
#endif
};

constexpr file_creation operator|(file_creation a, file_creation b) noexcept {
    return static_cast<file_creation>(static_cast<native_id_type>(a) | static_cast<native_id_type>(b));
}

constexpr file_creation operator&(file_creation a, file_creation b) noexcept {
    return static_cast<file_creation>(static_cast<native_id_type>(a) & static_cast<native_id_type>(b));
}


/**
 * @enum file_attri
 * @brief 文件属性和标志枚举
 *
 * 定义文件的属性和打开时的特殊标志。
 */
enum class file_attri : native_id_type {
#ifdef NEFORCE_PLATFORM_WINDOWS
    normal = FILE_ATTRIBUTE_NORMAL,               ///< 普通文件
    readonly = FILE_ATTRIBUTE_READONLY,           ///< 只读文件
    hidden = FILE_ATTRIBUTE_HIDDEN,               ///< 隐藏文件
    system = FILE_ATTRIBUTE_SYSTEM,               ///< 系统文件
    directory = FILE_ATTRIBUTE_DIRECTORY,         ///< 目录
    archive = FILE_ATTRIBUTE_ARCHIVE,             ///< 存档文件
    device = FILE_ATTRIBUTE_DEVICE,               ///< 设备文件
    temporary = FILE_ATTRIBUTE_TEMPORARY,         ///< 临时文件
    reparse_point = FILE_ATTRIBUTE_REPARSE_POINT, ///< 重解析点
    compressed = FILE_ATTRIBUTE_COMPRESSED,       ///< 压缩文件
    offline = FILE_ATTRIBUTE_OFFLINE,             ///< 离线文件
    encrypted = FILE_ATTRIBUTE_ENCRYPTED,         ///< 加密文件
    virtual_ = FILE_ATTRIBUTE_VIRTUAL,            ///< 虚拟文件
    overlapped = FILE_FLAG_OVERLAPPED,            ///< 支持异步I/O
    no_buffering = FILE_FLAG_NO_BUFFERING,        ///< 无缓冲I/O
    write_through = FILE_FLAG_WRITE_THROUGH,      ///< 写穿缓存
    others = 0                                    ///< 其他属性
#else
    normal = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH, ///< 普通文件
    readonly = S_IRUSR | S_IRGRP | S_IROTH,         ///< 只读文件
    directory = S_IFDIR,                            ///< 目录
    device = S_IFBLK | S_IFCHR,                     ///< 设备文件
    reparse_point = S_IFLNK,                        ///< 符号链接
    overlapped = 0,                                 ///< Linux不支持异步I/O标志
    no_buffering = O_DIRECT,                        ///< 直接I/O
    write_through = O_SYNC,                         ///< 同步写入
    others = 0                                      ///< 其他属性
#endif
};

constexpr file_attri operator|(file_attri a, file_attri b) noexcept {
    return static_cast<file_attri>(static_cast<native_id_type>(a) | static_cast<native_id_type>(b));
}

constexpr file_attri operator&(file_attri a, file_attri b) noexcept {
    return static_cast<file_attri>(static_cast<native_id_type>(a) & static_cast<native_id_type>(b));
}


/**
 * @enum file_pointer
 * @brief 文件指针移动方式枚举
 *
 * 定义文件指针移动时的参考位置。
 */
enum class file_pointer : native_id_type {
#ifdef NEFORCE_PLATFORM_WINDOWS
    begin = FILE_BEGIN,     ///< 从文件开头开始
    current = FILE_CURRENT, ///< 从当前位置开始
    end = FILE_END          ///< 从文件结尾开始
#else
    begin = SEEK_SET,   ///< 从文件开头开始
    current = SEEK_CUR, ///< 从当前位置开始
    end = SEEK_END      ///< 从文件结尾开始
#endif
};


/**
 * @enum file_lock
 * @brief 文件锁类型枚举
 *
 * 定义文件锁的类型和行为。
 */
enum class file_lock : native_id_type {
#ifdef NEFORCE_PLATFORM_WINDOWS
    shared = 0,                                                        ///< 共享锁
    exclusive = LOCKFILE_EXCLUSIVE_LOCK,                               ///< 独占锁
    fail_immediately = LOCKFILE_FAIL_IMMEDIATELY,                      ///< 立即失败
    shared_nb = LOCKFILE_FAIL_IMMEDIATELY,                             ///< 非阻塞共享锁
    exclusive_nb = LOCKFILE_EXCLUSIVE_LOCK | LOCKFILE_FAIL_IMMEDIATELY ///< 非阻塞独占锁
#else
    shared = LOCK_SH,                ///< 共享锁
    exclusive = LOCK_EX,             ///< 独占锁
    fail_immediately = LOCK_NB,      ///< 立即失败
    shared_nb = LOCK_SH | LOCK_NB,   ///< 非阻塞共享锁
    exclusive_nb = LOCK_EX | LOCK_NB ///< 非阻塞独占锁
#endif
};

constexpr file_lock operator|(file_lock a, file_lock b) noexcept {
    return static_cast<file_lock>(static_cast<native_id_type>(a) | static_cast<native_id_type>(b));
}

constexpr file_lock operator&(file_lock a, file_lock b) noexcept {
    return static_cast<file_lock>(static_cast<native_id_type>(a) & static_cast<native_id_type>(b));
}


/**
 * @enum file_watch_event
 * @brief 文件监视事件枚举
 *
 * 定义文件系统监视器可以捕获的事件类型。
 */
enum class file_watch_event {
    created = 0x01,  ///< 文件创建事件
    deleted = 0x02,  ///< 文件删除事件
    modified = 0x04, ///< 文件修改事件
    renamed = 0x08,  ///< 文件重命名事件
    /**
     * @brief 文件访问事件
     * @warning Windows 上受 NTFS 最后访问时间策略与延迟聚合影响，可能长时间不触发
     */
    accessed = 0x10,
    all = created | deleted | modified | renamed | accessed ///< 所有事件
};


/**
 * @enum file_map_hint
 * @brief 内存映射文件访问提示枚举
 *
 * 为内存映射文件提供访问模式的提示，用于优化性能。
 */
enum class file_map_hint {
    normal = 0, ///< 常规访问模式
    sequential, ///< 顺序访问模式
    random      ///< 随机访问模式
};

/** @} */ // File

NEFORCE_END_NAMESPACE__
#endif // NEFORCE_CORE_FILE_FILE_CONSTANTS_HPP__
