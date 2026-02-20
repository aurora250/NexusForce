#ifndef MSTL_CORE_FILE_FILE_CONSTANTS_HPP__
#define MSTL_CORE_FILE_FILE_CONSTANTS_HPP__
#include "MSTL/core/config/c++config.hpp"
#ifdef MSTL_PLATFORM_WINDOWS__
#include <Windows.h>
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

MSTL_INLINE17 constexpr auto FILE_SPLITER =
#ifdef MSTL_PLATFORM_WINDOWS__
    "\\/";
#elif defined(MSTL_PLATFORM_LINUX__)
    "/";
#endif

MSTL_INLINE17 constexpr char PREFERRED_SEPARATOR =
#ifdef MSTL_PLATFORM_WINDOWS__
    '\\';
#elif defined(MSTL_PLATFORM_LINUX__)
    '/';
#endif

MSTL_INLINE17 constexpr size_t FILE_BUFFER_SIZE = 8192; // 8KB Buffer


using fud_t =
#ifdef MSTL_PLATFORM_WINDOWS__
    ::DWORD;
#elif defined(MSTL_PLATFORM_LINUX__)
    int;
#endif

enum class FILE_ACCESS : fud_t {
#ifdef MSTL_PLATFORM_WINDOWS__
    READ = GENERIC_READ,
    WRITE = GENERIC_WRITE,
    READ_WRITE = GENERIC_READ | GENERIC_WRITE,
    APPEND = FILE_APPEND_DATA | GENERIC_WRITE
#elif defined(MSTL_PLATFORM_LINUX__)
    READ = O_RDONLY,
    WRITE = O_WRONLY,
    READ_WRITE = O_RDWR,
    APPEND = O_WRONLY | O_APPEND
#endif
};

constexpr FILE_ACCESS operator |(FILE_ACCESS a, FILE_ACCESS b) {
    return static_cast<FILE_ACCESS>(static_cast<fud_t>(a) | static_cast<fud_t>(b));
}
constexpr FILE_ACCESS operator &(FILE_ACCESS a, FILE_ACCESS b) {
    return static_cast<FILE_ACCESS>(static_cast<fud_t>(a) & static_cast<fud_t>(b));
}


enum class FILE_SHARED : fud_t {
#ifdef MSTL_PLATFORM_WINDOWS__
    SHARE_READ = FILE_SHARE_READ,
    SHARE_WRITE = FILE_SHARE_WRITE,
    SHARE_READ_WRITE = FILE_SHARE_READ | FILE_SHARE_WRITE,
    SHARE_DELETE = FILE_SHARE_DELETE,
    SHARE_ALL = FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
    NO_SHARE = 0
#elif defined(MSTL_PLATFORM_LINUX__)
    SHARE_READ = 1,
    SHARE_WRITE = 2,
    SHARE_READ_WRITE = 3,
    SHARE_DELETE = 4,
    SHARE_ALL = 7,
    NO_SHARE = 0
#endif
};

constexpr FILE_SHARED operator |(FILE_SHARED a, FILE_SHARED b) {
    return static_cast<FILE_SHARED>(static_cast<fud_t>(a) | static_cast<fud_t>(b));
}
constexpr FILE_SHARED operator &(FILE_SHARED a, FILE_SHARED b) {
    return static_cast<FILE_SHARED>(static_cast<fud_t>(a) & static_cast<fud_t>(b));
}


enum class FILE_CREATION : fud_t {
#ifdef MSTL_PLATFORM_WINDOWS__
    CREATE_FORCE = CREATE_ALWAYS,
    CREATE_NO_EXIST = CREATE_NEW,
    OPEN_FORCE = OPEN_ALWAYS,
    OPEN_EXIST = OPEN_EXISTING,
    TRUNCATE_EXIST = TRUNCATE_EXISTING
#elif defined(MSTL_PLATFORM_LINUX__)
    CREATE_FORCE = O_CREAT | O_TRUNC,
    CREATE_NO_EXIST = O_CREAT | O_EXCL,
    OPEN_FORCE = O_CREAT,
    OPEN_EXIST = 0,
    TRUNCATE_EXIST = O_TRUNC
#endif
};

constexpr FILE_CREATION operator |(FILE_CREATION a, FILE_CREATION b) {
    return static_cast<FILE_CREATION>(static_cast<fud_t>(a) | static_cast<fud_t>(b));
}
constexpr FILE_CREATION operator &(FILE_CREATION a, FILE_CREATION b) {
    return static_cast<FILE_CREATION>(static_cast<fud_t>(a) & static_cast<fud_t>(b));
}


enum class FILE_ATTRI : fud_t {
#ifdef MSTL_PLATFORM_WINDOWS__
    NORMAL = FILE_ATTRIBUTE_NORMAL,
    READONLY = FILE_ATTRIBUTE_READONLY,
    HIDDEN = FILE_ATTRIBUTE_HIDDEN,
    SYSTEM = FILE_ATTRIBUTE_SYSTEM,
    DIRECTORY = FILE_ATTRIBUTE_DIRECTORY,
    ARCHIVE = FILE_ATTRIBUTE_ARCHIVE,
    DEVICE = FILE_ATTRIBUTE_DEVICE,
    TEMPORARY = FILE_ATTRIBUTE_TEMPORARY,
    REPARSE_POINT = FILE_ATTRIBUTE_REPARSE_POINT,
    COMPRESSED = FILE_ATTRIBUTE_COMPRESSED,
    OFFLINE = FILE_ATTRIBUTE_OFFLINE,
    ENCRYPTED = FILE_ATTRIBUTE_ENCRYPTED,
    VIRTUAL = FILE_ATTRIBUTE_VIRTUAL,
    OVERLAPPED = FILE_FLAG_OVERLAPPED,
    NO_BUFFERING = FILE_FLAG_NO_BUFFERING,
    WRITE_THROUGH = FILE_FLAG_WRITE_THROUGH,
    OTHERS = 0
#elif defined(MSTL_PLATFORM_LINUX__)
    NORMAL = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH,
    READONLY = S_IRUSR | S_IRGRP | S_IROTH,
    DIRECTORY = S_IFDIR,
    DEVICE = S_IFBLK | S_IFCHR,
    REPARSE_POINT = S_IFLNK,
    OVERLAPPED = 0,
    NO_BUFFERING = O_DIRECT,
    WRITE_THROUGH = O_SYNC,
    OTHERS = 0
#endif
};

constexpr FILE_ATTRI operator |(FILE_ATTRI a, FILE_ATTRI b) {
    return static_cast<FILE_ATTRI>(static_cast<fud_t>(a) | static_cast<fud_t>(b));
}
constexpr FILE_ATTRI operator &(FILE_ATTRI a, FILE_ATTRI b) {
    return static_cast<FILE_ATTRI>(static_cast<fud_t>(a) & static_cast<fud_t>(b));
}


enum class FILE_POINTER : fud_t {
#ifdef MSTL_PLATFORM_WINDOWS__
    BEGIN = FILE_BEGIN,
    CURRENT = FILE_CURRENT,
    END = FILE_END
#elif defined(MSTL_PLATFORM_LINUX__)
    BEGIN = SEEK_SET,
    CURRENT = SEEK_CUR,
    END = SEEK_END
#endif
};

enum class FILE_LOCK : fud_t {
#ifdef MSTL_PLATFORM_WINDOWS__
    SHARED = 0,
    EXCLUSIVE = LOCKFILE_EXCLUSIVE_LOCK,
    FAIL_IMMEDIATELY = LOCKFILE_FAIL_IMMEDIATELY,
    SHARED_NB = LOCKFILE_FAIL_IMMEDIATELY,
    EXCLUSIVE_NB = LOCKFILE_EXCLUSIVE_LOCK | LOCKFILE_FAIL_IMMEDIATELY
#elif defined(MSTL_PLATFORM_LINUX__)
    SHARED = LOCK_SH,
    EXCLUSIVE = LOCK_EX,
    FAIL_IMMEDIATELY = LOCK_NB,
    SHARED_NB = LOCK_SH | LOCK_NB,
    EXCLUSIVE_NB = LOCK_EX | LOCK_NB
#endif
};

constexpr FILE_LOCK operator |(FILE_LOCK a, FILE_LOCK b) {
    return static_cast<FILE_LOCK>(static_cast<fud_t>(a) | static_cast<fud_t>(b));
}
constexpr FILE_LOCK operator &(FILE_LOCK a, FILE_LOCK b) {
    return static_cast<FILE_LOCK>(static_cast<fud_t>(a) & static_cast<fud_t>(b));
}


enum class FILE_WATCH_EVENT {
    CREATED = 0x01,
    DELETED = 0x02,
    MODIFIED = 0x04,
    RENAMED = 0x08,
    ACCESSED = 0x10,
    ALL = CREATED | DELETED | MODIFIED | RENAMED | ACCESSED
};

enum class FILE_MAP_HINT {
    NORMAL = 0,
    SEQUENTIAL,
    RANDOM
};

MSTL_END_NAMESPACE__
#endif // MSTL_CORE_FILE_FILE_CONSTANTS_HPP__
