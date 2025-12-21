#ifndef MSTL_CORE_FILE_PATH_HPP__
#define MSTL_CORE_FILE_PATH_HPP__
#include "MSTL/core/interface/istringify.hpp"
#include "MSTL/core/iterator/path_iterator.hpp"
MSTL_BEGIN_NAMESPACE__

class MSTL_API path : public icommon<path>, public istringify<path> {
private:
    string path_;

public:
    path() = default;
    explicit path(string p) : path_(_MSTL move(p)) {}
    explicit path(const string_view p) : path_(p) {}
    explicit path(const char* p) : path_(p) {}

    path(const path&) = default;
    path(path&&) noexcept = default;
    path& operator =(const path&) = default;
    path& operator =(path&&) noexcept = default;

    MSTL_NODISCARD const string& str() const noexcept { return path_; }
    MSTL_NODISCARD string_view view() const noexcept { return path_.view(); }
    MSTL_NODISCARD const char* c_str() const noexcept { return path_.c_str(); }
    MSTL_NODISCARD bool empty() const noexcept { return path_.empty(); }

    MSTL_NODISCARD path_iterator begin() const noexcept { return path_iterator(&path_, 0); }
    MSTL_NODISCARD path_iterator end() const noexcept { return path_iterator(); }

    MSTL_NODISCARD path parent_path() const;
    MSTL_NODISCARD string_view filename() const noexcept;
    MSTL_NODISCARD string_view stem() const noexcept;
    MSTL_NODISCARD string_view extension() const noexcept;

    MSTL_NODISCARD path lexically_normal() const noexcept;
    MSTL_NODISCARD path absolute(const path& base = current_path()) const;
    MSTL_NODISCARD path relative(const path& base) const;

    static path current_path();
    static path temp_directory_path();

    path& operator /=(const path& p);
    path& operator /=(string_view p);
    path operator /(const path& p) const;
    path operator /(string_view p) const;

    MSTL_NODISCARD bool exists() const noexcept;
    MSTL_NODISCARD bool is_directory() const noexcept;
    MSTL_NODISCARD bool is_file() const noexcept;

    bool create_directories() const;
    bool remove() const noexcept;
    bool remove_directory() const noexcept;
    bool remove_all_in_directory(bool recursive = true) const noexcept;
    bool remove_all() const noexcept;

    bool copy(const path& to, bool overwrite = true) const;
    bool copy_directory(const path& destination, bool overwrite = true) const;
    bool move(const path& to, bool overwrite = true) const noexcept;
    bool rename(const path& new_name) const;

    MSTL_NODISCARD static bool exists(const string& path) noexcept;
    MSTL_NODISCARD static bool is_directory(const string& path) noexcept;
    MSTL_NODISCARD static bool is_file(const string& path) noexcept;
    MSTL_NODISCARD static string_view extension(string_view path) noexcept;

    static bool create_directories(const string& path);
    static bool remove(const string& path) noexcept;
    static bool remove_directory(const string& path) noexcept;
    static bool remove_all_in_directory(const string& directory_path, bool recursive = true) noexcept;
    static bool remove_all(const string& path) noexcept;

    static bool copy(const path& from, const path& to, bool overwrite = true);
    static bool copy_directory(const path& source, const path& destination, bool overwrite = true);
    static bool move(const path& from, const path& to, bool overwrite = true) noexcept;
    static bool rename(const path& old_name, const path& new_name);

    operator string_view() const noexcept { return path_.view(); }

    MSTL_NODISCARD bool operator ==(const path& rhs) const noexcept;
    MSTL_NODISCARD bool operator <(const path& rhs) const noexcept;

    MSTL_NODISCARD size_t to_hash() const;
    MSTL_NODISCARD string to_string() const { return lexically_normal().str(); }

    void swap(path& rhs) noexcept { path_.swap(rhs.path_); }
};

MSTL_END_NAMESPACE__
#endif // MSTL_CORE_FILE_PATH_HPP__
