#ifndef MSTL_PLUGIN_DYNAMIC_LIBRARY_HPP__
#define MSTL_PLUGIN_DYNAMIC_LIBRARY_HPP__
#include "MSTL/core/string/string.hpp"
MSTL_BEGIN_NAMESPACE__

MSTL_ERROR_BUILD_FINAL_CLASS(dl_exception, system_exception, "Dynamic Library Operation Failed")


class MSTL_API dynamic_library {
private:
    void* handle_;
    string path_;

private:
    void open();
    void close();

public:
    explicit dynamic_library(const string& path);
    ~dynamic_library();

    dynamic_library(const dynamic_library&) = delete;
    dynamic_library& operator =(const dynamic_library&) = delete;
    dynamic_library(dynamic_library&& other) noexcept;
    dynamic_library& operator =(dynamic_library&& other) noexcept;

    template <typename T>
    T get_symbol(const string& name) const {
        return reinterpret_cast<T>(get_symbol_row(name));
    }

    void* get_symbol_row(const string& name) const;
    bool has_symbol(const string& name) const noexcept;

    bool is_open() const noexcept { return handle_ != nullptr; }
    void unload() { close(); }

    void* native_handle() const noexcept { return handle_; }
    const string& path() const noexcept { return path_; }
};


MSTL_END_NAMESPACE__
#endif // MSTL_PLUGIN_DYNAMIC_LIBRARY_HPP__
