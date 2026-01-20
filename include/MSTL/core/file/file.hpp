#ifndef MSTL_CORE_FILE_FILE_HPP__
#define MSTL_CORE_FILE_FILE_HPP__
#include "MSTL/core/container/vector.hpp"
#include "MSTL/core/container/unordered_map.hpp"
#include "MSTL/core/time/datetime.hpp"
#include "MSTL/core/iterator/file_line_iterator.hpp"
#include "MSTL/core/async/mutex.hpp"
#include "path.hpp"
#ifdef MSTL_PLATFORM_WINDOWS__
#include <Windows.h>
#include "MSTL/core/config/undef_cmacro.hpp"
#endif
#ifdef MSTL_PLATFORM_LINUX__
#include <sys/stat.h>
#include <aio.h>
#endif
MSTL_BEGIN_NAMESPACE__

class MSTL_API file {
public:
#ifdef MSTL_PLATFORM_WINDOWS__
    using size_type = ::DWORD;
    using difference_type = ::LONGLONG;
    using file_handle = ::HANDLE;
    using time_type = ::FILETIME;
    using aiocb_type = ::OVERLAPPED;
#elif defined(MSTL_PLATFORM_LINUX__)
    using size_type = size_t;
    using difference_type = ::off_t;
    using file_handle = int;
    using time_type = ::time_t;
    using aiocb_type = ::aiocb;
#endif

    struct chunk_info {
        difference_type offset;
        size_type size;
        size_type chunk_index;
    };

    struct map_info {
        void* address = nullptr;
        size_type size = 0;
        size_type offset = 0;
        FILE_ACCESS access = FILE_ACCESS::READ;
        bool is_mapped = false;
    };

    struct binary_diff_entry {
        difference_type offset = 0;
        byte_t byte1 = 0;
        byte_t byte2 = 0;
        int64_t size_diff = 0;
        bool is_size_diff = false;
    };

    struct async_context {
        string data{};
        string* buffer = nullptr;
        aiocb_type* cb = nullptr;
        bool is_write;

        explicit async_context(string &&d);
        explicit async_context(string* buf);
        ~async_context();
    };

    struct async_result {
        bool completed = false;
        size_t bytes_transferred = 0;
        int error_code = 0;
        aiocb_type* cb = nullptr;
        async_context* user_context = nullptr;
    };

private:
    MSTL_ALWAYS_INLINE static const file_handle& INVALID_HANDLE() noexcept {
        static const auto INVALID_HANDLE =
#ifdef MSTL_PLATFORM_WINDOWS__
            INVALID_HANDLE_VALUE;
#elif defined(MSTL_PLATFORM_LINUX__)
            -1;
#endif
        return INVALID_HANDLE;
    }

    file_handle handle_ = INVALID_HANDLE();
    _MSTL path path_{};
    bool opened_ = false;
    bool append_mode_ = false;

    mutable vector<byte_t> read_buffer_{};
    mutable size_type read_buffer_pos_ = 0;
    mutable size_type read_buffer_size_ = 0;
    mutable vector<byte_t> write_buffer_{};
    mutable size_type write_buffer_pos_ = 0;

    mutable mutex map_mutex_;

    void* mapped_ptr_ = nullptr;
    size_type mapped_size_ = 0;
    size_type mapped_offset_ = 0;
    FILE_ACCESS mapped_access_ = FILE_ACCESS::READ;
    size_type is_mapped_ = false;

#ifdef MSTL_PLATFORM_WINDOWS__
    ::HANDLE mapping_handle_ = INVALID_HANDLE_VALUE;
#endif

    mutable mutex async_mutex_;
    mutable vector<aiocb_type*> async_operations_;
    mutable unordered_map<aiocb_type*, async_context*> async_contexts_;

    mutable string last_error_msg_;
    mutable int last_error_code_ = 0;

    size_type buffer_size_ = FILE_BUFFER_SIZE;

private:
    bool complete_async_result(async_result& result, size_type bytes_transferred);
    bool check_async_completion(async_result& result);

    bool flush_write_buffer() const noexcept;
    bool fill_read_buffer() const noexcept;

    static datetime filetime_to_datetime(const time_type& ft) noexcept;
    static time_type datetime_to_filetime(const datetime& dt) noexcept;
    static string get_last_error_msg();

    void set_last_error() const;

    void adjust_buffer_size();

public:
    file() = default;

    explicit file(
        _MSTL path p,
        FILE_ACCESS access = FILE_ACCESS::READ_WRITE,
        FILE_SHARED share_mode = FILE_SHARED::SHARE_READ,
        FILE_CREATION creation = FILE_CREATION::OPEN_EXIST,
        FILE_ATTRI attributes = FILE_ATTRI::NORMAL,
        bool append = false) : path_(_MSTL move(p)) {
        this->open(path_, append, access, share_mode, creation, attributes);
    }

    file(const file&) = delete;
    file& operator =(const file&) = delete;

    file(file&& other) noexcept;
    file& operator =(file&& other) noexcept;

    ~file();

    bool open(_MSTL path p, bool append = false,
        FILE_ACCESS access = FILE_ACCESS::READ_WRITE,
        FILE_SHARED share_mode = FILE_SHARED::SHARE_READ_WRITE,
        FILE_CREATION creation = FILE_CREATION::OPEN_EXIST,
        FILE_ATTRI attributes = FILE_ATTRI::NORMAL);

    bool open(bool append = false,
        FILE_ACCESS access = FILE_ACCESS::READ_WRITE,
        FILE_SHARED share_mode = FILE_SHARED::SHARE_READ_WRITE,
        FILE_CREATION creation = FILE_CREATION::OPEN_EXIST,
        FILE_ATTRI attributes = FILE_ATTRI::NORMAL);

    void close() noexcept;
    bool flush() noexcept;

    MSTL_NODISCARD file_handle native_handle() const noexcept { return handle_; }

    size_type write(const string& data, size_type size);
    size_type write(const string& data);
    size_type write(const void* data, size_type size);

    size_type read(void* buffer, size_type size) const;
    size_type read(string& str, size_type size) const;
    size_type read(string& str) const;
    MSTL_NODISCARD string read() const;

    size_type read_binary(void* buffer, size_type size) const;
    size_type read_binary(string& str, size_type size) const;
    size_type read_binary(string& str) const;
    MSTL_NODISCARD string read_binary() const;

    bool read_line(string& line) const;
    MSTL_NODISCARD string read_line() const;
    MSTL_NODISCARD vector<string> read_lines() const;

    vector<string> read_chunks(size_type chunk_size = FILE_BUFFER_SIZE * 16) const;
    bool write_chunks(const vector<string>& chunks);
    MSTL_NODISCARD vector<chunk_info> chunks_info(size_type chunk_size) const;

    async_result async_read(string& buffer, size_type size, difference_type offset = -1) const;
    async_result async_write(string data, size_type size, difference_type offset = -1);
    bool wait_async(async_result& result, uint32_t timeout_ms = numeric_traits<uint32_t>::max());
    void cancel_async(async_result& result);

    MSTL_NODISCARD size_type size() const;
    bool size(size_type& out_size) const;
    MSTL_NODISCARD uint64_t size64() const;

    MSTL_NODISCARD const _MSTL path& path() const noexcept { return path_; }

    MSTL_NODISCARD bool is_opened() const noexcept { return opened_; }
    MSTL_NODISCARD bool is_append() const noexcept { return append_mode_; }

    MSTL_NODISCARD string last_error() const noexcept { return last_error_msg_; }
    MSTL_NODISCARD int last_error_code() const noexcept { return last_error_code_; }

    void clear_error() noexcept;

    MSTL_NODISCARD file_line_iterator begin_lines() const { return file_line_iterator(this); }
    MSTL_NODISCARD file_line_iterator end_lines() const { return {}; }

    MSTL_NODISCARD static bool compare(const _MSTL path& file1, const _MSTL path& file2, bool binary = true);
    MSTL_NODISCARD static bool compare_binary(const _MSTL path& file1, const _MSTL path& file2);

    MSTL_NODISCARD static bool compare_text(
        const _MSTL path& file1, const _MSTL path& file2,
        bool ignore_case = false, bool ignore_whitespace = false);

    MSTL_NODISCARD static vector<binary_diff_entry> binary_diff(
        const _MSTL path& file1,
        const _MSTL path& file2,
        size_type max_diffs = 100);

    bool seek(difference_type distance, FILE_POINTER method = FILE_POINTER::END) const noexcept;
    difference_type tell() const noexcept;
    difference_type system_tell() const noexcept;

    bool prefetch(size_type hint_size = 0) const noexcept;
    bool truncate(difference_type size) const noexcept;

    bool lock(difference_type offset, difference_type length, FILE_LOCK mode = FILE_LOCK::EXCLUSIVE) const noexcept;
    bool unlock(difference_type offset, difference_type length) const noexcept;

    bool try_lock(difference_type offset, difference_type length, FILE_LOCK mode) const noexcept;
    
    MSTL_NODISCARD bool is_locked(
        difference_type offset,
        difference_type length,
        FILE_LOCK* out_type) const noexcept;

    bool lock_whole(FILE_LOCK mode) const noexcept;
    bool unlock_whole() const noexcept;

    bool map(size_type offset = 0, size_type size = 0,
        FILE_ACCESS access = FILE_ACCESS::READ,
        FILE_MAP_HINT hint = FILE_MAP_HINT::SEQUENTIAL);
    void unmap() noexcept;
    bool remap(size_type new_offset, size_type new_size);
    bool flush_mapped(bool async = false);

    bool lock_mapped_pages(bool lock_in_memory) const noexcept;
    MSTL_NODISCARD map_info map_infos() const noexcept;

    MSTL_NODISCARD void* mapped_data() const noexcept { return mapped_ptr_; }
    MSTL_NODISCARD size_type mapped_size() const noexcept { return mapped_size_; }
    MSTL_NODISCARD size_type mapped_offset() const noexcept { return mapped_offset_; }
    MSTL_NODISCARD FILE_ACCESS mapped_access() const noexcept { return mapped_access_; }
    MSTL_NODISCARD bool is_mapped() const noexcept { return mapped_ptr_ != nullptr; }

    MSTL_NODISCARD FILE_ATTRI attributes() const noexcept;
    bool set_attributes(FILE_ATTRI attr) noexcept;

#ifdef MSTL_PLATFORM_WINDOWS__
    MSTL_NODISCARD datetime creation_time() const noexcept;
#endif
    MSTL_NODISCARD datetime last_access_time() const noexcept;
    MSTL_NODISCARD datetime last_write_time() const noexcept;

#ifdef MSTL_PLATFORM_WINDOWS__
    bool set_all_times(const datetime& create, const datetime& access, const datetime& write) noexcept;
#elif defined(MSTL_PLATFORM_LINUX__)
    bool set_all_times(const datetime& access, const datetime& write) noexcept;
#endif

#ifdef MSTL_PLATFORM_WINDOWS__
    bool set_creation_time(const datetime& dt) noexcept;
#endif
    bool set_last_access_time(const datetime& dt) noexcept;
    bool set_last_write_time(const datetime& dt) noexcept;

    MSTL_NODISCARD static size_type size(const _MSTL path& p);
    static bool size(const _MSTL path& p, size_type& size);

    static bool create_and_write(const _MSTL path& p, const string& content, bool append = false);

    static bool read(const _MSTL path& p, string& content,
        FILE_CREATION creation = FILE_CREATION::OPEN_EXIST,
        FILE_ATTRI attributes = FILE_ATTRI::NORMAL);

    MSTL_NODISCARD static string read(const _MSTL path& p,
        FILE_CREATION creation = FILE_CREATION::OPEN_EXIST,
        FILE_ATTRI attributes = FILE_ATTRI::NORMAL);

    static bool read_binary(const _MSTL path& p, string& content,
        FILE_CREATION creation = FILE_CREATION::OPEN_EXIST,
        FILE_ATTRI attributes = FILE_ATTRI::NORMAL);

    MSTL_NODISCARD static string read_binary(const _MSTL path& p,
        FILE_CREATION creation = FILE_CREATION::OPEN_EXIST,
        FILE_ATTRI attributes = FILE_ATTRI::NORMAL);
};


class MSTL_API file_lock_guard {
public:
    using difference_type = file::difference_type;

private:
    file& file_;
    difference_type offset_;
    difference_type length_;
    bool locked_;

public:
    file_lock_guard(file& f, difference_type offset, difference_type length, FILE_LOCK mode);
    ~file_lock_guard();

    file_lock_guard(const file_lock_guard&) = delete;
    file_lock_guard& operator =(const file_lock_guard&) = delete;

    MSTL_NODISCARD bool is_locked() const { return locked_; }

    bool unlock();
};

MSTL_END_NAMESPACE__
#endif // MSTL_CORE_FILE_FILE_HPP__
