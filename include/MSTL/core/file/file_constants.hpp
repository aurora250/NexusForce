#ifndef MSTL_CORE_FILE_FILE_CONSTANTS_HPP__
#define MSTL_CORE_FILE_FILE_CONSTANTS_HPP__

/**
 * @file file_constants.hpp
 * @brief 文件操作常量定义
 *
 * 此文件定义了文件操作中使用的各种常量和枚举类型，
 * 提供跨平台的文件访问模式、共享模式、创建方式等常量的统一抽象。
 */

#include "MSTL/core/config/c++config.hpp"
#ifdef MSTL_PLATFORM_WINDOWS__
#include <windef.h>
#include <winreg.h>
#include <WinUser.h>
#include <WinBase.h>
#ifdef max
#undef max
#endif
#ifdef min
#undef min
#endif
#endif
#ifdef MSTL_PLATFORM_LINUX__
#include <fcntl.h>
#endif
MSTL_BEGIN_NAMESPACE__

/**
 * @defgroup File 文件
 * @brief 文件操作
 * @{
 */

class MSTL_API file;

/**
 * @brief 文件描述符类型
 *
 * 跨平台的文件描述符类型定义。
 */
using fud_t =
#ifdef MSTL_PLATFORM_WINDOWS__
    ::DWORD;
#elif defined(MSTL_PLATFORM_LINUX__)
    int;
#endif


/**
 * @enum FILE_ACCESS
 * @brief 文件访问模式枚举
 *
 * 定义文件的打开方式，包括读、写、读写和追加模式。
 */
enum class FILE_ACCESS : fud_t {
#ifdef MSTL_PLATFORM_WINDOWS__
    READ = GENERIC_READ,     ///< 只读模式
    WRITE = GENERIC_WRITE,   //< 只写模式
    READ_WRITE = GENERIC_READ | GENERIC_WRITE, ///< 读写模式
    APPEND = FILE_APPEND_DATA | GENERIC_WRITE  ///< 追加模式
#else
    READ = O_RDONLY,     ///< 只读模式
    WRITE = O_WRONLY,    ///< 只写模式
    READ_WRITE = O_RDWR, ///< 读写模式
    APPEND = O_WRONLY | O_APPEND  ///< 追加模式
#endif
};

constexpr FILE_ACCESS operator |(FILE_ACCESS a, FILE_ACCESS b) {
    return static_cast<FILE_ACCESS>(static_cast<fud_t>(a) | static_cast<fud_t>(b));
}

constexpr FILE_ACCESS operator &(FILE_ACCESS a, FILE_ACCESS b) {
    return static_cast<FILE_ACCESS>(static_cast<fud_t>(a) & static_cast<fud_t>(b));
}


/**
 * @enum FILE_SHARED
 * @brief 文件共享模式枚举
 *
 * 定义文件被其他进程访问时的共享权限。
 */
enum class FILE_SHARED : fud_t {
#ifdef MSTL_PLATFORM_WINDOWS__
    SHARE_READ = FILE_SHARE_READ,           ///< 允许其他进程读取
    SHARE_WRITE = FILE_SHARE_WRITE,         ///< 允许其他进程写入
    SHARE_READ_WRITE = FILE_SHARE_READ | FILE_SHARE_WRITE, ///< 允许其他进程读写
    SHARE_DELETE = FILE_SHARE_DELETE,       ///< 允许其他进程删除
    SHARE_ALL = FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, ///< 允许所有操作
    NO_SHARE = 0                            ///< 不允许共享
#else
    SHARE_READ = 1,       ///< 允许其他进程读取
    SHARE_WRITE = 2,      ///< 允许其他进程写入
    SHARE_READ_WRITE = 3, ///< 允许其他进程读写
    SHARE_DELETE = 4,     ///< 允许其他进程删除
    SHARE_ALL = 7,        ///< 允许所有操作
    NO_SHARE = 0          ///< 不允许共享
#endif
};

constexpr FILE_SHARED operator |(FILE_SHARED a, FILE_SHARED b) {
    return static_cast<FILE_SHARED>(static_cast<fud_t>(a) | static_cast<fud_t>(b));
}

constexpr FILE_SHARED operator &(FILE_SHARED a, FILE_SHARED b) {
    return static_cast<FILE_SHARED>(static_cast<fud_t>(a) & static_cast<fud_t>(b));
}


/**
 * @enum FILE_CREATION
 * @brief 文件创建方式枚举
 *
 * 定义文件打开或创建时的行为。
 */
enum class FILE_CREATION : fud_t {
#ifdef MSTL_PLATFORM_WINDOWS__
    CREATE_FORCE = CREATE_ALWAYS,      ///< 强制创建新文件（覆盖已存在）
    CREATE_NO_EXIST = CREATE_NEW,      ///< 仅当文件不存在时创建
    OPEN_FORCE = OPEN_ALWAYS,          ///< 打开文件，不存在则创建
    OPEN_EXIST = OPEN_EXISTING,        ///< 仅打开已存在的文件
    TRUNCATE_EXIST = TRUNCATE_EXISTING ///< 打开已存在文件并清空内容
#else
    CREATE_FORCE = O_CREAT | O_TRUNC,  ///< 强制创建新文件（覆盖已存在）
    CREATE_NO_EXIST = O_CREAT | O_EXCL, ///< 仅当文件不存在时创建
    OPEN_FORCE = O_CREAT,               ///< 打开文件，不存在则创建
    OPEN_EXIST = 0,                     ///< 仅打开已存在的文件
    TRUNCATE_EXIST = O_TRUNC            ///< 打开已存在文件并清空内容
#endif
};

constexpr FILE_CREATION operator |(FILE_CREATION a, FILE_CREATION b) {
    return static_cast<FILE_CREATION>(static_cast<fud_t>(a) | static_cast<fud_t>(b));
}

constexpr FILE_CREATION operator &(FILE_CREATION a, FILE_CREATION b) {
    return static_cast<FILE_CREATION>(static_cast<fud_t>(a) & static_cast<fud_t>(b));
}


/**
 * @enum FILE_ATTRI
 * @brief 文件属性和标志枚举
 *
 * 定义文件的属性和打开时的特殊标志。
 */
enum class FILE_ATTRI : fud_t {
#ifdef MSTL_PLATFORM_WINDOWS__
    NORMAL = FILE_ATTRIBUTE_NORMAL,           ///< 普通文件
    READONLY = FILE_ATTRIBUTE_READONLY,       ///< 只读文件
    HIDDEN = FILE_ATTRIBUTE_HIDDEN,           ///< 隐藏文件
    SYSTEM = FILE_ATTRIBUTE_SYSTEM,           ///< 系统文件
    DIRECTORY = FILE_ATTRIBUTE_DIRECTORY,     ///< 目录
    ARCHIVE = FILE_ATTRIBUTE_ARCHIVE,         ///< 存档文件
    DEVICE = FILE_ATTRIBUTE_DEVICE,           ///< 设备文件
    TEMPORARY = FILE_ATTRIBUTE_TEMPORARY,     ///< 临时文件
    REPARSE_POINT = FILE_ATTRIBUTE_REPARSE_POINT, ///< 重解析点
    COMPRESSED = FILE_ATTRIBUTE_COMPRESSED,   ///< 压缩文件
    OFFLINE = FILE_ATTRIBUTE_OFFLINE,         ///< 离线文件
    ENCRYPTED = FILE_ATTRIBUTE_ENCRYPTED,     ///< 加密文件
    VIRTUAL = FILE_ATTRIBUTE_VIRTUAL,         ///< 虚拟文件
    OVERLAPPED = FILE_FLAG_OVERLAPPED,        ///< 支持异步I/O
    NO_BUFFERING = FILE_FLAG_NO_BUFFERING,    ///< 无缓冲I/O
    WRITE_THROUGH = FILE_FLAG_WRITE_THROUGH,  ///< 写穿缓存
    OTHERS = 0                                 ///< 其他属性
#else
    NORMAL = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH, ///< 普通文件
    READONLY = S_IRUSR | S_IRGRP | S_IROTH,         ///< 只读文件
    DIRECTORY = S_IFDIR,                            ///< 目录
    DEVICE = S_IFBLK | S_IFCHR,                     ///< 设备文件
    REPARSE_POINT = S_IFLNK,                        ///< 符号链接
    OVERLAPPED = 0,                                 ///< Linux不支持异步I/O标志
    NO_BUFFERING = O_DIRECT,                        ///< 直接I/O
    WRITE_THROUGH = O_SYNC,                         ///< 同步写入
    OTHERS = 0                                      ///< 其他属性
#endif
};

constexpr FILE_ATTRI operator |(FILE_ATTRI a, FILE_ATTRI b) {
    return static_cast<FILE_ATTRI>(static_cast<fud_t>(a) | static_cast<fud_t>(b));
}

constexpr FILE_ATTRI operator &(FILE_ATTRI a, FILE_ATTRI b) {
    return static_cast<FILE_ATTRI>(static_cast<fud_t>(a) & static_cast<fud_t>(b));
}


/**
 * @enum FILE_POINTER
 * @brief 文件指针移动方式枚举
 *
 * 定义文件指针移动时的参考位置。
 */
enum class FILE_POINTER : fud_t {
#ifdef MSTL_PLATFORM_WINDOWS__
    BEGIN = FILE_BEGIN,     ///< 从文件开头开始
    CURRENT = FILE_CURRENT, ///< 从当前位置开始
    END = FILE_END          ///< 从文件结尾开始
#else
    BEGIN = SEEK_SET,    ///< 从文件开头开始
    CURRENT = SEEK_CUR,  ///< 从当前位置开始
    END = SEEK_END       ///< 从文件结尾开始
#endif
};


/**
 * @enum FILE_LOCK
 * @brief 文件锁类型枚举
 *
 * 定义文件锁的类型和行为。
 */
enum class FILE_LOCK : fud_t {
#ifdef MSTL_PLATFORM_WINDOWS__
    SHARED = 0,                                   ///< 共享锁
    EXCLUSIVE = LOCKFILE_EXCLUSIVE_LOCK,          ///< 独占锁
    FAIL_IMMEDIATELY = LOCKFILE_FAIL_IMMEDIATELY, ///< 立即失败
    SHARED_NB = LOCKFILE_FAIL_IMMEDIATELY,        ///< 非阻塞共享锁
    EXCLUSIVE_NB = LOCKFILE_EXCLUSIVE_LOCK | LOCKFILE_FAIL_IMMEDIATELY  ///< 非阻塞独占锁
#else
    SHARED = LOCK_SH,                ///< 共享锁
    EXCLUSIVE = LOCK_EX,             ///< 独占锁
    FAIL_IMMEDIATELY = LOCK_NB,      ///< 立即失败
    SHARED_NB = LOCK_SH | LOCK_NB,   ///< 非阻塞共享锁
    EXCLUSIVE_NB = LOCK_EX | LOCK_NB ///< 非阻塞独占锁
#endif
};

constexpr FILE_LOCK operator |(FILE_LOCK a, FILE_LOCK b) {
    return static_cast<FILE_LOCK>(static_cast<fud_t>(a) | static_cast<fud_t>(b));
}

constexpr FILE_LOCK operator &(FILE_LOCK a, FILE_LOCK b) {
    return static_cast<FILE_LOCK>(static_cast<fud_t>(a) & static_cast<fud_t>(b));
}


/**
 * @enum FILE_WATCH_EVENT
 * @brief 文件监视事件枚举
 *
 * 定义文件系统监视器可以捕获的事件类型。
 */
enum class FILE_WATCH_EVENT {
    CREATED = 0x01,   ///< 文件创建事件
    DELETED = 0x02,   ///< 文件删除事件
    MODIFIED = 0x04,  ///< 文件修改事件
    RENAMED = 0x08,   ///< 文件重命名事件
    ACCESSED = 0x10,  ///< 文件访问事件
    ALL = CREATED | DELETED | MODIFIED | RENAMED | ACCESSED  ///< 所有事件
};


/**
 * @enum FILE_MAP_HINT
 * @brief 内存映射文件访问提示枚举
 *
 * 为内存映射文件提供访问模式的提示，用于优化性能。
 */
enum class FILE_MAP_HINT {
    NORMAL = 0,       ///< 常规访问模式
    SEQUENTIAL,       ///< 顺序访问模式
    RANDOM            ///< 随机访问模式
};

/** @} */ // File

MSTL_END_NAMESPACE__
#endif // MSTL_CORE_FILE_FILE_CONSTANTS_HPP__
