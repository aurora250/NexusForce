#ifndef MSTL_CORE_FILE_FILE_HPP__
#define MSTL_CORE_FILE_FILE_HPP__
#include "MSTL/core/container/vector.hpp"
#include "MSTL/core/time/datetime.hpp"
#include "MSTL/core/iterator/file_line_iterator.hpp"
#include "path.hpp"
#ifdef MSTL_PLATFORM_WINDOWS__
#include <Windows.h>
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
#elif defined(MSTL_PLATFORM_LINUX__)
    using size_type = size_t;
    using difference_type = ::off_t;
    using file_handle = int;
    using time_type = ::time_t;
#endif

    struct chunk_info {
        difference_type offset;
        size_type size;
        size_type chunk_index;
    };

    struct binary_diff_entry {
        difference_type offset;
        unsigned char byte1;
        unsigned char byte2;
    };

    struct async_result {
        bool completed = false;
        size_t bytes_transferred = 0;
        int error_code = 0;
#ifdef MSTL_PLATFORM_WINDOWS__
        ::OVERLAPPED* overlapped = nullptr;
#elif defined(MSTL_PLATFORM_LINUX__)
        ::aiocb* cb = nullptr;
#endif
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
    path path_{};
    bool opened_ = false;
    bool append_mode_ = false;

    mutable vector<char> read_buffer_{};
    mutable size_type read_buffer_pos_ = 0;
    mutable size_type read_buffer_size_ = 0;
    mutable vector<char> write_buffer_{};
    mutable size_type write_buffer_pos_ = 0;

    void* mapped_ptr_ = nullptr;
    size_type mapped_size_ = 0;
#ifdef MSTL_PLATFORM_WINDOWS__
    ::HANDLE mapping_handle_ = INVALID_HANDLE_VALUE;
#endif

#ifdef MSTL_PLATFORM_WINDOWS__
    mutable vector<::OVERLAPPED*> async_operations_;
#elif defined(MSTL_PLATFORM_LINUX__)
    mutable vector<::aiocb*> async_operations_;
#endif

    mutable string last_error_msg_;
    mutable int last_error_code_ = 0;

    size_type buffer_size_ = FILE_BUFFER_SIZE;

private:
    bool flush_write_buffer() const noexcept;
    bool fill_read_buffer() const noexcept;

    static datetime filetime_to_datetime(const time_type& ft) noexcept;
    static time_type datetime_to_filetime(const datetime& dt) noexcept;
    static string get_last_error_msg();

    void set_last_error() const;

    void adjust_buffer_size();

#ifdef MSTL_PLATFORM_LINUX__
    static mode_t convert_attributes(FILE_ATTRI attr);
#endif

public:
    file() = default;

    explicit file(
        path p,
        FILE_ACCESS access = FILE_ACCESS::READ_WRITE,
        FILE_SHARED share_mode = FILE_SHARED::SHARE_READ,
        FILE_CREATION creation = FILE_CREATION::OPEN_EXIST,
        FILE_ATTRI attributes = FILE_ATTRI::NORMAL,
        bool append = false) : path_(_MSTL move(p)) {
        this->open(path_, append, access, share_mode, creation, attributes);
    }

    file(const file&) = delete;
    file& operator=(const file&) = delete;

    file(file&& other) noexcept;
    file& operator=(file&& other) noexcept;

    ~file();


    bool open(path p, bool append = false,
        FILE_ACCESS access = FILE_ACCESS::READ_WRITE,
        FILE_SHARED share_mode = FILE_SHARED::SHARE_READ_WRITE,
        FILE_CREATION creation = FILE_CREATION::OPEN_EXIST,
        FILE_ATTRI attributes = FILE_ATTRI::NORMAL);

    bool open(bool append = false,
        FILE_ACCESS access = FILE_ACCESS::READ_WRITE,
        FILE_SHARED share_mode = FILE_SHARED::SHARE_READ_WRITE,
        FILE_CREATION creation = FILE_CREATION::OPEN_EXIST,
        FILE_ATTRI attributes = FILE_ATTRI::NORMAL) {
        return this->open(path_, append, access, share_mode, creation, attributes);
    }

    void close() noexcept;
    bool flush() const noexcept;


    size_type write(const string& data, size_type size) const;
    size_type write(const string& data) const { return this->write(data, data.size()); }

    size_type read(string& str, size_type size) const;
    size_type read(string& str) const { return this->read(str, str.size()); }
    string read() const;


    vector<string> read_chunks(size_type chunk_size = FILE_BUFFER_SIZE * 16) const;
    bool write_chunks(const vector<string>& chunks);
    vector<chunk_info> get_chunk_info(size_type chunk_size) const;


    size_type read_binary(string& str, size_type size) const;

    size_type read_binary(string& str) const {
        const size_type s = size();
        str.resize(s);
        return this->read_binary(str, s);
    }

    string read_binary() const;


    bool read_line(string& line) const;

    string read_line() const {
        string line;
        if (!read_line(line)) return {};
        return line;
    }

    vector<string> read_lines() const;


    async_result async_read(string& buffer, size_type size, difference_type offset = -1);
    async_result async_write(const string& data, size_type size, difference_type offset = -1);
    bool wait_async(async_result& result, uint32_t timeout_ms = 0xFFFFFFFF);
    void cancel_async(async_result& result);


    MSTL_NODISCARD size_type size() const noexcept;
    MSTL_NODISCARD const path& get_path() const noexcept { return path_; }
    MSTL_NODISCARD bool opened() const noexcept { return opened_; }
    MSTL_NODISCARD bool is_append() const noexcept { return append_mode_; }


    string last_error() const noexcept { return last_error_msg_; }
    int last_error_code() const noexcept { return last_error_code_; }

    void clear_error() noexcept {
        last_error_msg_.clear();
        last_error_code_ = 0;
    }


    file_line_iterator begin_lines() const { return file_line_iterator(this); }
    file_line_iterator end_lines() const { return file_line_iterator(); }


    static bool compare(const path& file1, const path& file2, bool binary = true);
    static bool compare_binary(const path& file1, const path& file2);
    static bool compare_text(const path& file1, const path& file2);

    static vector<binary_diff_entry> binary_diff(
        const path& file1, const path& file2, size_type max_diffs = 100);


    bool seek(difference_type distance, FILE_POINTER method = FILE_POINTER::END) const noexcept;
    difference_type tell() const noexcept;

    bool prefetch(size_type hint_size = 0) const noexcept;
    bool truncate(difference_type size) const noexcept;


    bool lock(difference_type offset, difference_type length,
        FILE_LOCK mode = FILE_LOCK::EXCLUSIVE) const noexcept;
    bool unlock(difference_type offset, difference_type length) const noexcept;


    bool map(size_type offset = 0, size_type size = 0,
        FILE_ACCESS access = FILE_ACCESS::READ,
        FILE_MAP_HINT hint = FILE_MAP_HINT::SEQUENTIAL);

    void unmap() noexcept;

    void* mapped_data() const noexcept { return mapped_ptr_; }
    size_type mapped_size() const noexcept { return mapped_size_; }
    bool is_mapped() const noexcept { return mapped_ptr_ != nullptr; }


    MSTL_NODISCARD FILE_ATTRI attributes() const noexcept;
    bool set_attributes(FILE_ATTRI attr) const noexcept;


#ifdef MSTL_PLATFORM_WINDOWS__
    datetime creation_time() const noexcept;
#endif
    datetime last_access_time() const noexcept;
    datetime last_write_time() const noexcept;

#ifdef MSTL_PLATFORM_WINDOWS__
    bool set_all_times(const datetime& create, const datetime& access,
        const datetime& write) const noexcept;
#elif defined(MSTL_PLATFORM_LINUX__)
    bool set_all_times(const datetime& access, const datetime& write) const noexcept;
#endif

#ifdef MSTL_PLATFORM_WINDOWS__
    bool set_creation_time(const datetime& dt) const noexcept;
#endif
    bool set_last_access_time(const datetime& dt) const noexcept;
    bool set_last_write_time(const datetime& dt) const noexcept;


    static size_type size(const path& p);
    static bool size(const path& p, size_type& size);

    static bool create_and_write(const path& p, const string& content, bool append = false);

    static bool read(const path& p, string& content,
        FILE_CREATION creation = FILE_CREATION::OPEN_EXIST,
        FILE_ATTRI attributes = FILE_ATTRI::NORMAL);

    static string read(const path& p,
            FILE_CREATION creation = FILE_CREATION::OPEN_EXIST,
            FILE_ATTRI attributes = FILE_ATTRI::NORMAL) {
        string content;
        file::read(p, content, creation, attributes);
        return content;
    }

    static bool read_binary(const path& p, string& content,
        FILE_CREATION creation = FILE_CREATION::OPEN_EXIST,
        FILE_ATTRI attributes = FILE_ATTRI::NORMAL);

    static string read_binary(const path& p,
        FILE_CREATION creation = FILE_CREATION::OPEN_EXIST,
        FILE_ATTRI attributes = FILE_ATTRI::NORMAL) {
        string content;
        file::read_binary(p, content, creation, attributes);
        return content;
    }
};

MSTL_END_NAMESPACE__
#endif // MSTL_CORE_FILE_FILE_HPP__
