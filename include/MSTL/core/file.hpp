#ifndef MSTL_FILE_HPP__
#define MSTL_FILE_HPP__
#include "vector.hpp"
#include "datetime.hpp"
#ifdef MSTL_PLATFORM_WINDOWS__
#include <Windows.h>
#endif
#ifdef MSTL_PLATFORM_LINUX__
#include <fcntl.h>
#include <sys/stat.h>
#endif
MSTL_BEGIN_NAMESPACE__

#ifdef MSTL_PLATFORM_WINDOWS__
using file_underlying_type_t = ::DWORD;
#elif defined(MSTL_PLATFORM_LINUX__)
using file_underlying_type_t = int;
#endif

enum class FILE_ACCESS : file_underlying_type_t {
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
    return static_cast<FILE_ACCESS>(
        static_cast<file_underlying_type_t>(a) |
        static_cast<file_underlying_type_t>(b)
        );
}
constexpr FILE_ACCESS operator &(FILE_ACCESS a, FILE_ACCESS b) {
    return static_cast<FILE_ACCESS>(
        static_cast<file_underlying_type_t>(a) &
        static_cast<file_underlying_type_t>(b)
        );
}


enum class FILE_SHARED : file_underlying_type_t {
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
    return static_cast<FILE_SHARED>(
        static_cast<file_underlying_type_t>(a) |
        static_cast<file_underlying_type_t>(b)
        );
}
constexpr FILE_SHARED operator &(FILE_SHARED a, FILE_SHARED b) {
    return static_cast<FILE_SHARED>(
        static_cast<file_underlying_type_t>(a) &
        static_cast<file_underlying_type_t>(b)
        );
}


enum class FILE_CREATION : file_underlying_type_t {
#ifdef MSTL_PLATFORM_WINDOWS__
    CREATE_FORCE = CREATE_ALWAYS,
    CREATE_NO_EXIST = CREATE_NEW,
    OPEN_FORCE = OPEN_ALWAYS,
    OPEN_EXIST = OPEN_EXISTING,
    TRUNCATE_EXIST = TRUNCATE_EXISTING
#elif defined(MSTL_PLATFORM_LINUX__)
    CREATE_FORCE = O_CREAT | O_TRUNC,
    CREATE_NO_EXIST = O_CREAT | O_EXCL | O_WRONLY,
    OPEN_FORCE = O_CREAT,
    OPEN_EXIST = 0,
    TRUNCATE_EXIST = O_TRUNC
#endif
};

constexpr FILE_CREATION operator |(FILE_CREATION a, FILE_CREATION b) {
    return static_cast<FILE_CREATION>(
        static_cast<file_underlying_type_t>(a) |
        static_cast<file_underlying_type_t>(b)
        );
}
constexpr FILE_CREATION operator &(FILE_CREATION a, FILE_CREATION b) {
    return static_cast<FILE_CREATION>(
        static_cast<file_underlying_type_t>(a) &
        static_cast<file_underlying_type_t>(b)
        );
}


enum class FILE_ATTRI : file_underlying_type_t {
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

constexpr FILE_ATTRI operator |(FILE_ATTRI a, FILE_ATTRI b) {
    return static_cast<FILE_ATTRI>(
        static_cast<file_underlying_type_t>(a) |
        static_cast<file_underlying_type_t>(b)
        );
}
constexpr FILE_ATTRI operator &(FILE_ATTRI a, FILE_ATTRI b) {
    return static_cast<FILE_ATTRI>(
        static_cast<file_underlying_type_t>(a) &
        static_cast<file_underlying_type_t>(b)
        );
}


enum class FILE_POINTER : file_underlying_type_t {
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

enum class FILE_LOCK : file_underlying_type_t {
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

constexpr FILE_LOCK operator |(FILE_LOCK a, FILE_LOCK b) {
    return static_cast<FILE_LOCK>(
        static_cast<file_underlying_type_t>(a) |
        static_cast<file_underlying_type_t>(b)
        );
}
constexpr FILE_LOCK operator &(FILE_LOCK a, FILE_LOCK b) {
    return static_cast<FILE_LOCK>(
        static_cast<file_underlying_type_t>(a) &
        static_cast<file_underlying_type_t>(b)
        );
}

MSTL_INLINE17 constexpr size_t FILE_BUFFER_SIZE = 8192; // 8KB Buffer


class MSTL_API file {
public:
#ifdef MSTL_PLATFORM_WINDOWS__
    using size_type = ::DWORD;
    using difference_type = ::LONGLONG;
    using file_handle = ::HANDLE;
    using time_type = ::FILETIME;
#elif defined(MSTL_PLATFORM_LINUX__)
    using size_type = size_t;
    using difference_type = ::off_t;
    using file_handle = int;
    using time_type = ::time_t;
#endif

private:
    MSTL_ALWAYS_INLINE static const file_handle& INVALID_HANDLE() noexcept {
        static file_handle INVALID_HANDLE =
    #ifdef MSTL_PLATFORM_WINDOWS__
            INVALID_HANDLE_VALUE;
#elif defined(MSTL_PLATFORM_LINUX__)
            -1;
#endif
        return INVALID_HANDLE;
    }

    file_handle handle_ = INVALID_HANDLE();
    string path_{};
    bool opened_ = false;
    bool append_mode_ = false;

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

    explicit file(
        string path,
        const FILE_ACCESS access = FILE_ACCESS::READ_WRITE,
        const FILE_SHARED share_mode = FILE_SHARED::SHARE_READ,
        const FILE_CREATION creation = FILE_CREATION::OPEN_EXIST,
        const FILE_ATTRI attributes = FILE_ATTRI::NORMAL,
        const bool append = false) : path_(_MSTL move(path)) {
        this->open(path_, append, access, share_mode, creation, attributes);
    }

    file(const file&) = delete;
    file& operator =(const file&) = delete;

    file(file&& other) noexcept;
    file& operator =(file&& other) noexcept;

    ~file();


    bool open(string path, bool append = false,
        FILE_ACCESS access = FILE_ACCESS::READ_WRITE,
        FILE_SHARED share_mode = FILE_SHARED::SHARE_READ,
        FILE_CREATION creation = FILE_CREATION::OPEN_EXIST,
        FILE_ATTRI attributes = FILE_ATTRI::NORMAL);

    bool open(bool append = false,
        FILE_ACCESS access = FILE_ACCESS::READ_WRITE,
        FILE_SHARED share_mode = FILE_SHARED::SHARE_READ,
        FILE_CREATION creation = FILE_CREATION::OPEN_EXIST,
        FILE_ATTRI attributes = FILE_ATTRI::NORMAL) {
        return this->open(_MSTL move(path_), append, access, share_mode, creation, attributes);
    }

    void close() noexcept;
    bool flush() const noexcept;

    size_type write(const string& data, size_type size) const;
    size_type write(const string& data) const { return this->write(data, data.size()); }

    size_type read(string& str, size_type size) const;
    size_type read(string& str) const;
    string read() const;

    size_type read_binary(string& str, size_type size) const;
    size_type read_binary(string& str) const {
        const size_type s = size(); str.resize(s);
        return this->read_binary(str, s);
    }
    string read_binary() const { return this->read_binary(path_); }

    bool read_line(string& line) const;
    string read_line() const {
        string line; if (!read_line(line)) return {};
        return line;
    }
    vector<string> read_lines() const;

    MSTL_NODISCARD size_type size() const noexcept;
    static size_type size(const string& path);
    static bool size(const string& path, size_type& size);

    bool seek(difference_type distance, FILE_POINTER method = FILE_POINTER::END) const noexcept;
    difference_type tell() const noexcept;

    bool prefetch(size_type hint_size = 0) const noexcept;
    bool truncate(difference_type size) const noexcept;

    bool lock(difference_type offset, difference_type length, FILE_LOCK mode = FILE_LOCK::EXCLUSIVE) const noexcept;
    bool unlock(difference_type offset, difference_type length) const noexcept;

    MSTL_NODISCARD FILE_ATTRI attributes() const noexcept;
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

    MSTL_NODISCARD string_view path() const noexcept { return path_.view(); }
    MSTL_NODISCARD bool opened() const noexcept { return opened_; }
    MSTL_NODISCARD bool is_append() const noexcept { return append_mode_; }

    MSTL_NODISCARD bool exists() const noexcept { return file::exists(path_); }
    MSTL_NODISCARD static bool exists(string_view path) noexcept;
    MSTL_NODISCARD static bool exists(const string& path) noexcept { return file::exists(path.view()); }
    MSTL_NODISCARD static bool exists(const char* path) noexcept { return file::exists(string_view{path}); }

    MSTL_NODISCARD bool is_directory() const noexcept { return file::is_directory(path_); }
    MSTL_NODISCARD static bool is_directory(string_view path) noexcept;
    MSTL_NODISCARD static bool is_directory(const string& path) noexcept { return file::is_directory(path.view()); }
    MSTL_NODISCARD static bool is_directory(const char* path) noexcept { return file::is_directory(string_view{path}); }

    MSTL_NODISCARD bool is_file() const noexcept { return file::is_file(path_); }
    MSTL_NODISCARD static bool is_file(string_view path) noexcept;
    MSTL_NODISCARD static bool is_file(const string& path) noexcept { return file::is_file(path.view()); }
    MSTL_NODISCARD static bool is_file(const char* path) noexcept { return file::is_file(string_view{path}); }

    MSTL_NODISCARD string_view extension() const noexcept { return file::extension(path_.view()); }
    MSTL_NODISCARD static string_view extension(string_view path) noexcept;
    MSTL_NODISCARD static string_view extension(const string& path) noexcept { return file::extension(path.view()); }
    MSTL_NODISCARD static string_view extension(const char* path) noexcept { return file::extension(string_view{path}); }

    bool create_directories() const { return file::create_directories(path_); }
    static bool create_directories(const string& path);

    static bool create_and_write(const string& path, const string& content, bool append = false);

    bool remove() const noexcept { return file::remove(path_); }
    static bool remove(string_view path) noexcept;
    static bool remove(const string& path) noexcept { return file::remove(path.view()); }
    static bool remove(const char* path) noexcept { return file::remove(string_view{path}); }

    bool remove_directory() const noexcept { return file::remove_directory(path_); }
    static bool remove_directory(string_view path) noexcept;
    static bool remove_directory(const string& path) noexcept { return remove_directory(path.view()); }
    static bool remove_directory(const char* path) noexcept { return remove_directory(string_view{path}); }

    bool remove_all_in_directory(const bool recursive = true) const noexcept { return file::remove_all_in_directory(path_, recursive); }
    static bool remove_all_in_directory(const string& directory_path, bool recursive = true) noexcept;

    static bool read(const string& path, string& content,
        FILE_CREATION creation = FILE_CREATION::OPEN_EXIST,
        FILE_ATTRI attributes = FILE_ATTRI::NORMAL);

    static string read(const string& path,
        FILE_CREATION creation = FILE_CREATION::OPEN_EXIST,
        FILE_ATTRI attributes = FILE_ATTRI::NORMAL) {
        string content; file::read(path, content, creation, attributes);
        return content;
    }

    static bool read_binary(const string& path, string& content,
        FILE_CREATION creation = FILE_CREATION::OPEN_EXIST,
        FILE_ATTRI attributes = FILE_ATTRI::NORMAL);

    static string read_binary(const string& path,
        FILE_CREATION creation = FILE_CREATION::OPEN_EXIST,
        FILE_ATTRI attributes = FILE_ATTRI::NORMAL) {
        string content; file::read_binary(path, content, creation, attributes);
        return content;
    }

    static bool copy(const string& from, const string& to, bool overwrite = true);
    static bool copy_directory(const string& source, const string& destination, bool overwrite = true);

    static bool move(const string& from, const string& to, bool overwrite = true) noexcept;

    static bool rename(const string& old_name, const string& new_name) {
        return file::move(old_name, new_name, true);
    }
};

MSTL_END_NAMESPACE__
#endif // MSTL_FILE_HPP__
