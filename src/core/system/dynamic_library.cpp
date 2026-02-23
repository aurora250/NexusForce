#include <MSTL/core/system/dynamic_library.hpp>
#ifdef MSTL_PLATFORM_WINDOWS__
#include <MSTL/core/config/windef.hpp>
#include <libloaderapi.h>
#else
#include <dlfcn.h>
#endif
MSTL_BEGIN_NAMESPACE__

void dynamic_library::open() {
    if (handle_) return;

#ifdef MSTL_PLATFORM_WINDOWS__
    handle_ = ::LoadLibraryA(path_.data());
    if (!handle_) {
        throw_exception(dynamic_library_exception("dynamic library load failed."));
    }
#else
    handle_ = ::dlopen(path_.data(), RTLD_LAZY | RTLD_LOCAL);
    if (!handle_) {
        throw_exception(dynamic_library_exception(::dlerror()));
    }
#endif
}

void dynamic_library::close() {
    if (handle_) {
#ifdef MSTL_PLATFORM_WINDOWS__
        ::FreeLibrary(static_cast<::HMODULE>(handle_));
#else
        ::dlclose(handle_);
#endif
        handle_ = nullptr;
    }
}

dynamic_library::dynamic_library(const string& pth)
: handle_(nullptr), path_(pth) {
    open();
}

dynamic_library::dynamic_library(dynamic_library&& other) noexcept
: handle_(other.handle_), path_(move(other.path_)) {
    other.handle_ = nullptr;
}

dynamic_library& dynamic_library::operator =(dynamic_library&& other) noexcept {
    if (this != &other) {
        close();
        handle_ = other.handle_;
        path_ = move(other.path_);
        other.handle_ = nullptr;
    }
    return *this;
}

dynamic_library::~dynamic_library() {
    close();
}

void* dynamic_library::symbol(const string& name) const {
    if (!is_open()) {
        throw_exception(dynamic_library_exception("Library not loaded"));
    }

#ifdef MSTL_PLATFORM_WINDOWS__
    const ::FARPROC proc = ::GetProcAddress(static_cast<::HMODULE>(handle_), name.data());
    if (!proc) {
        throw_exception(dynamic_library_exception("GetProcAddress failed"));
    }
    return reinterpret_cast<void*>(proc);
#else
    ::dlerror();
    void* sym = ::dlsym(handle_, name.data());
    const char* error = ::dlerror();
    if (error) {
        throw_exception(dynamic_library_exception(error));
    }
    return sym;
#endif
}

bool dynamic_library::has_symbol(const string& name) const noexcept {
    if (!is_open()) return false;

#ifdef MSTL_PLATFORM_WINDOWS__
    return ::GetProcAddress(static_cast<::HMODULE>(handle_), name.data()) != nullptr;
#else
    ::dlerror();
    ::dlsym(handle_, name.data());
    return ::dlerror() == nullptr;
#endif
}

MSTL_END_NAMESPACE__
