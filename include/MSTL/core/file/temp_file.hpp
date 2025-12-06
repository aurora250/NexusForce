#ifndef MSTL_CORE_FILE_TEMP_FILE_HPP__
#define MSTL_CORE_FILE_TEMP_FILE_HPP__
#include "file.hpp"
MSTL_BEGIN_NAMESPACE__

class MSTL_API temp_file {
public:
    explicit temp_file(const string& prefix = "tmp", const string& suffix = ".tmp");
    ~temp_file();

    temp_file(const temp_file&) = delete;
    temp_file& operator=(const temp_file&) = delete;

    temp_file(temp_file&& other) noexcept;
    temp_file& operator=(temp_file&& other) noexcept;

    const path& get_path() const noexcept { return temp_path_; }
    file& get_file() noexcept { return file_; }
    const file& get_file() const noexcept { return file_; }

    void keep() noexcept { auto_delete_ = false; }
    void cleanup();

private:
    path temp_path_;
    file file_;
    bool auto_delete_ = true;

    static path generate_temp_path(const string& prefix, const string& suffix);
};

MSTL_END_NAMESPACE__
#endif // MSTL_CORE_FILE_TEMP_FILE_HPP__
