#ifndef MSTL_FILE_HPP__
#define MSTL_FILE_HPP__
#include "vector.hpp"
#include "datetime.hpp"
#ifdef MSTL_PLATFORM_LINUX__
#include <fcntl.h>
#include <sys/stat.h>
#endif
MSTL_BEGIN_NAMESPACE__

using file_flag_type =
#ifdef MSTL_PLATFORM_WINDOWS__
    unsigned long;
#elif defined(MSTL_PLATFORM_LINUX__)
    int;
#endif


enum class FILE_ACCESS : file_flag_type {
#ifdef MSTL_PLATFORM_WINDOWS__
    READ = GENERIC_READ,
    WRITE = GENERIC_WRITE,
    READ_WRITE = GENERIC_READ | GENERIC_WRITE,
    APPEND = FILE_APPEND_DATA
#elif defined(MSTL_PLATFORM_LINUX__)
    READ = O_RDONLY,
    WRITE = O_WRONLY,
    READ_WRITE = O_RDWR,
    APPEND = O_WRONLY | O_APPEND
#endif
};

inline FILE_ACCESS operator |(FILE_ACCESS a, FILE_ACCESS b) {
    return static_cast<FILE_ACCESS>(static_cast<int>(a) | static_cast<int>(b));
}
inline FILE_ACCESS operator &(FILE_ACCESS a, FILE_ACCESS b) {
    return static_cast<FILE_ACCESS>(static_cast<int>(a) & static_cast<int>(b));
}


enum class FILE_SHARED : file_flag_type {
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

inline FILE_SHARED operator |(FILE_SHARED a, FILE_SHARED b) {
    return static_cast<FILE_SHARED>(static_cast<int>(a) | static_cast<int>(b));
}
inline FILE_SHARED operator &(FILE_SHARED a, FILE_SHARED b) {
    return static_cast<FILE_SHARED>(static_cast<int>(a) & static_cast<int>(b));
}


enum class FILE_CREATION : file_flag_type {
#ifdef MSTL_PLATFORM_WINDOWS__
    CREATE_FORCE = CREATE_ALWAYS,
    CREATE_NO_EXIST = CREATE_NEW,
    OPEN_FORCE = OPEN_ALWAYS,
    OPEN_EXIST = OPEN_EXISTING,
    TRUNCATE_EXIST = TRUNCATE_EXISTING
#elif defined(MSTL_PLATFORM_LINUX__)
    CREATE_FORCE = O_CREAT | O_TRUNC | O_WRONLY,
    CREATE_NO_EXIST = O_CREAT | O_EXCL,
    OPEN_FORCE = O_CREAT,
    OPEN_EXIST = 0,
    TRUNCATE_EXIST = O_TRUNC
#endif
};

inline FILE_CREATION operator |(FILE_CREATION a, FILE_CREATION b) {
    return static_cast<FILE_CREATION>(static_cast<int>(a) | static_cast<int>(b));
}
inline FILE_CREATION operator &(FILE_CREATION a, FILE_CREATION b) {
    return static_cast<FILE_CREATION>(static_cast<int>(a) & static_cast<int>(b));
}


enum class FILE_ATTRI : file_flag_type {
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
    OTHERS = 0
#elif defined(MSTL_PLATFORM_LINUX__)
    NORMAL = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH,
    READONLY = S_IRUSR | S_IRGRP | S_IROTH,
    DIRECTORY = S_IFDIR,
    DEVICE = S_IFBLK | S_IFCHR,
    REPARSE_POINT = S_IFLNK,
    OTHERS = 0
#endif
};

inline FILE_ATTRI operator |(FILE_ATTRI a, FILE_ATTRI b) {
    return static_cast<FILE_ATTRI>(
        static_cast<file_flag_type>(a) | static_cast<file_flag_type>(b));
}
inline FILE_ATTRI operator &(FILE_ATTRI a, FILE_ATTRI b) {
    return static_cast<FILE_ATTRI>(
        static_cast<file_flag_type>(a) & static_cast<file_flag_type>(b));
}


enum class FILE_POINTER : file_flag_type {
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

enum class FILE_LOCK : file_flag_type {
#ifdef MSTL_PLATFORM_WINDOWS__
    SHARED = 0,
    EXCLUSIVE = LOCKFILE_EXCLUSIVE_LOCK,
    FAIL_IMMEDIATELY = LOCKFILE_FAIL_IMMEDIATELY
#elif defined(MSTL_PLATFORM_LINUX__)
    SHARED = LOCK_SH,
    EXCLUSIVE = LOCK_EX,
    FAIL_IMMEDIATELY = LOCK_NB
#endif
};

inline FILE_LOCK operator |(FILE_LOCK a, FILE_LOCK b) {
    return static_cast<FILE_LOCK>(static_cast<int>(a) | static_cast<int>(b));
}
inline FILE_LOCK operator &(FILE_LOCK a, FILE_LOCK b) {
    return static_cast<FILE_LOCK>(static_cast<int>(a) & static_cast<int>(b));
}


class MSTL_API file {
public:
#ifdef MSTL_PLATFORM_WINDOWS__
    using size_type = DWORD;
    using difference_type = LONG;
    using file_handle = HANDLE;
    using time_type = FILETIME;
#elif defined(MSTL_PLATFORM_LINUX__)
    using size_type = size_t;
    using difference_type = ::off_t;
    using file_handle = int;
    using time_type = ::time_t;
#endif

private:
    static file_handle INVALID_HANDLE() noexcept;

    file_handle handle_ = INVALID_HANDLE();
    string path_{};
    bool opened_ = false;
    bool append_mode_ = false;

    static constexpr size_type BUFFER_SIZE = 8192; // 8KB Buffer
    mutable vector<char> read_buffer_{};
    mutable size_type read_buffer_pos_ = 0;
    mutable size_type read_buffer_size_ = 0;
    mutable vector<char> write_buffer_{};
    mutable size_type write_buffer_pos_ = 0;

    static constexpr auto FILE_SPLITER =
#ifdef MSTL_PLATFORM_WINDOWS__
        "/\\";
#elif defined(MSTL_PLATFORM_LINUX__)
        "/";
#endif

private:
    bool flush_write_buffer() const noexcept;
    bool fill_read_buffer() const noexcept;

    static datetime filetime_to_datetime(const time_type& ft) noexcept;
    static time_type datetime_to_filetime(const datetime& dt) noexcept;

    static string get_last_error_msg();

#ifdef MSTL_PLATFORM_LINUX__
    static mode_t convert_attributes(const FILE_ATTRI attr);
#endif

public:
    file() = default;

    explicit file(string path,
        FILE_ACCESS access = FILE_ACCESS::READ_WRITE,
        FILE_SHARED share_mode = FILE_SHARED::SHARE_READ,
        FILE_CREATION creation = FILE_CREATION::OPEN_EXIST,
        FILE_ATTRI attributes = FILE_ATTRI::NORMAL,
        bool append = false);

    file(const file&) = delete;
    file& operator =(const file&) = delete;

    file(file&& other) noexcept;
    file& operator =(file&& other) noexcept;

    ~file();


    bool open(
        const string& path,
        FILE_ACCESS access = FILE_ACCESS::READ_WRITE,
        FILE_SHARED share_mode = FILE_SHARED::SHARE_READ,
        FILE_CREATION creation = FILE_CREATION::OPEN_EXIST,
        FILE_ATTRI attributes = FILE_ATTRI::NORMAL,
        bool append = false);

    bool open(
        FILE_ACCESS access = FILE_ACCESS::READ_WRITE,
        FILE_SHARED share_mode = FILE_SHARED::SHARE_READ,
        FILE_CREATION creation = FILE_CREATION::OPEN_EXIST,
        FILE_ATTRI attributes = FILE_ATTRI::NORMAL,
        bool append = false);

    void close() noexcept;
    bool flush() const noexcept;

    size_type write(const string& data, size_type size) const;
    size_type write(const string& data) const;

    size_type read(string& str, size_type size) const;
    size_type read(string& str) const;
    string read() const;

    size_type read_binary(string& str, size_type size) const;
    size_type read_binary(string& str) const;
    string read_binary() const;

    bool read_line(string& line) const;
    string read_line() const;
    vector<string> read_lines() const;

    size_type size() const noexcept;
    static size_type size(const string& path);

    bool seek(difference_type distance, FILE_POINTER method = FILE_POINTER::END) const noexcept;
    difference_type tell() const noexcept;

    bool prefetch(size_type hint_size = 0) const noexcept;
    bool truncate(difference_type size) const noexcept;

    bool lock(difference_type offset, difference_type length, FILE_LOCK mode = FILE_LOCK::EXCLUSIVE) const noexcept;
    bool unlock(difference_type offset, difference_type length) const noexcept;

    FILE_ATTRI attributes() const noexcept;
    bool set_attributes(FILE_ATTRI attr) const noexcept;

#ifdef MSTL_PLATFORM_WINDOWS__
    datetime creation_time() const noexcept;
#endif
    datetime last_access_time() const noexcept;
    datetime last_write_time() const noexcept;

#ifdef MSTL_PLATFORM_WINDOWS__
    bool set_all_times(const datetime& create, const datetime& access, const datetime& write) const noexcept;
#elif defined(MSTL_PLATFORM_LINUX__)
    bool set_all_times(const datetime& access, const datetime& write) const noexcept;
#endif

#ifdef MSTL_PLATFORM_WINDOWS__
    bool set_creation_time(const datetime& dt) const noexcept;
#endif
    bool set_last_access_time(const datetime& dt) const noexcept;
    bool set_last_write_time(const datetime& dt) const noexcept;

    const string& path() const noexcept;
    bool opened() const noexcept;
    bool is_append() const noexcept;

    bool exists() const noexcept;
    static bool exists(const string& path) noexcept;

    bool is_directory() const noexcept;
    static bool is_directory(const string& path) noexcept;

    bool is_file() const noexcept;
    static bool is_file(const string& path) noexcept;

    string extension() const noexcept;
    static string extension(const string& path) noexcept;

    static constexpr string_view extension(const string_view path) noexcept {
        const size_t last_sep = path.find_last_of(FILE_SPLITER);
        const string_view filename = last_sep == string::npos ? path : path.substr(last_sep + 1);

        const size_t last_dot = filename.find_last_of('.');
        if (last_dot == string::npos || last_dot == 0 || last_dot == filename.size() - 1) {
            return {};
        }
        return filename.substr(last_dot + 1);
    }

    bool create_directories() const;
    static bool create_directories(const string& path);

    static bool create_and_write(const string& path, const string& content, bool append = false);

    bool remove() const noexcept;
    static bool remove(const string& path) noexcept;

    bool remove_directory() const noexcept;
    static bool remove_directory(const string& path) noexcept;

    static bool read(const string& path, string& content,
        FILE_CREATION creation = FILE_CREATION::OPEN_EXIST,
        FILE_ATTRI attributes = FILE_ATTRI::NORMAL);

    static string read(const string& path,
        FILE_CREATION creation = FILE_CREATION::OPEN_EXIST,
        FILE_ATTRI attributes = FILE_ATTRI::NORMAL);

    static bool read_binary(const string& path, string& content,
        FILE_CREATION creation = FILE_CREATION::OPEN_EXIST,
        FILE_ATTRI attributes = FILE_ATTRI::NORMAL);

    static string read_binary(const string& path,
        FILE_CREATION creation = FILE_CREATION::OPEN_EXIST,
        FILE_ATTRI attributes = FILE_ATTRI::NORMAL);

    static bool copy(const string& from, const string& to, bool overwrite = true);
    static bool move(const string& from, const string& to, bool overwrite = true) noexcept;
    static bool rename(const string& old_name, const string& new_name);
};

MSTL_END_NAMESPACE__
#endif // MSTL_FILE_HPP__